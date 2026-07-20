/*
 * ZombieVerter CAN telemetry receiver.
 *
 * Hardware: SN65HVD230 3.3 V transceiver on TWAI (500 kbps).
 *   TX = GPIO 4, RX = GPIO 5  (ESP32-P4 Function EV Board J1 pins 18 / 16)
 *
 * Decodes OpenInverter telemetry:
 *   Packed CanMap (preferred): CAN ID 0x500..0x509, two spot32 BE fields per frame
 *   Spot broadcasts: extended 0x02A0000+param_id or standard param/remap ids
 * Updates LVGL subjects from an lv_timer so observer callbacks run on the LVGL thread.
 */

#include "zv_can.h"
#include "zv_can_pack.h"
#include "zv_can_params.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "ev_dash_gen.h"
#include "bms_cell_health.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

#include <string.h>

static const char *TAG = "zv_can";

static int s_last_opmode = -1;

#define ZV_CAN_TX_GPIO   GPIO_NUM_4
#define ZV_CAN_RX_GPIO   GPIO_NUM_5

#define ZV_CAN_BITRATE_HZ  500000
#define ZV_CAN_RX_DEPTH    32
#define ZV_CAN_RX_STACK    4096
#define ZV_CAN_RX_PRIO     5
#define ZV_CAN_APPLY_MS    50

#define ZV_CAN_BROADCAST_PREFIX  0x02A0000U
#define ZV_CAN_BROADCAST_MASK    0x00FF0000U

typedef struct {
    twai_frame_t frame;
    uint8_t      data[TWAI_FRAME_MAX_LEN];
} zv_rx_slot_t;

typedef struct {
    bool     have_opmode;
    bool     have_lasterr;
    bool     have_udc;
    bool     have_udc2;
    bool     have_power;
    bool     have_idc;
    bool     have_kwh;
    bool     have_soc;
    bool     have_speed_rpm;
    bool     have_veh_speed;
    bool     have_dir;
    bool     have_tmphs;
    bool     have_tmpm;
    bool     have_hour;
    bool     have_min;
    bool     have_u12v;
    bool     have_uaux;
    bool     have_tmpaux;
    bool     have_deltav;
    bool     have_bms_vmin;
    bool     have_bms_vmax;
    bool     have_bms_tmin;
    bool     have_bms_tmax;
    bool     have_bms_tavg;
    bool     have_bms_charge_lim;
    bool     have_bms_soh;
    bool     have_chg_temp;
    bool     have_ccs_i;

    int32_t  opmode;
    int32_t  lasterr;
    float    udc;
    float    udc2;
    float    power_kw;
    float    idc;
    float    kwh;
    int32_t  soc;
    int32_t  speed_rpm;
    float    veh_speed_kmh;
    int32_t  dir;
    float    tmphs;
    float    tmpm;
    int32_t  hour;
    int32_t  minute;
    float    u12v;
    float    uaux;
    float    tmpaux;
    float    deltav;
    float    bms_vmin;
    float    bms_vmax;
    float    bms_tmin;
    float    bms_tmax;
    float    bms_tavg;
    float    bms_charge_lim;
    int32_t  bms_soh;
    float    chg_temp;
    float    ccs_i;

    uint32_t rx_frames;
    uint32_t rx_spot_frames;
    uint32_t rx_pack_frames;
    int64_t  last_rx_us;
} zv_snapshot_t;

typedef struct {
    twai_node_handle_t node;
    zv_rx_slot_t       pool[ZV_CAN_RX_DEPTH];
    SemaphoreHandle_t  free_sem;
    SemaphoreHandle_t  ready_sem;
    int                write_idx;
    int                read_idx;
    SemaphoreHandle_t  snap_mtx;
    zv_snapshot_t      snap;
    TaskHandle_t       rx_task;
} zv_ctx_t;

static zv_ctx_t s_ctx;

/* Trip integrator (reset via zv_can_trip_reset / trip-reset button). */
static float s_trip_m;
static float s_trip_wh;

#define ZV_EFF_MIN_SPEED_KMH  5.0f
#define ZV_EFF_MIN_POWER_KW   0.05f
#define ZV_EFF_MAX_WH_KM      300

#define ZV_REGEN_DEADBAND_KW  0.5f
#define ZV_REGEN_LEVEL1_KW    8.0f
#define ZV_REGEN_LEVEL2_KW    18.0f

static int zv_power_to_regen_level(float power_kw)
{
    if(power_kw >= -ZV_REGEN_DEADBAND_KW) {
        return 0;
    }

    float regen_kw = -power_kw;
    if(regen_kw < ZV_REGEN_LEVEL1_KW) {
        return 1;
    }
    if(regen_kw < ZV_REGEN_LEVEL2_KW) {
        return 2;
    }
    return 3;
}

static const char *zv_gear_to_str(int gear_pos)
{
    switch(gear_pos) {
        case GEARPOSITION_GEAR_PARK:    return "P";
        case GEARPOSITION_GEAR_REVERSE:  return "R";
        case GEARPOSITION_GEAR_NEUTRAL: return "N";
        case GEARPOSITION_GEAR_DRIVE:   return "D";
        case GEARPOSITION_GEAR_BRAKE:   return "B";
        default:                        return "?";
    }
}

/* VCU leaves BMS temps at 0 when no BMS is configured — treat 0 as unset. */
static bool zv_temp_usable(float temp_c, bool have)
{
    return have && temp_c > -50.0f && temp_c < 200.0f && temp_c != 0.0f;
}

static bool zv_pick_batt_temp_c(const zv_snapshot_t *snap, int *temp_c_out)
{
    if(zv_temp_usable(snap->bms_tmax, snap->have_bms_tmax)) {
        *temp_c_out = (int)(snap->bms_tmax + 0.5f);
        return true;
    }
    if(zv_temp_usable(snap->bms_tavg, snap->have_bms_tavg)) {
        *temp_c_out = (int)(snap->bms_tavg + 0.5f);
        return true;
    }
    if(zv_temp_usable(snap->bms_tmin, snap->have_bms_tmin)) {
        *temp_c_out = (int)(snap->bms_tmin + 0.5f);
        return true;
    }
    if(zv_temp_usable(snap->tmpaux, snap->have_tmpaux)) {
        *temp_c_out = (int)(snap->tmpaux + 0.5f);
        return true;
    }
    if(zv_temp_usable(snap->chg_temp, snap->have_chg_temp)) {
        *temp_c_out = (int)(snap->chg_temp + 0.5f);
        return true;
    }
    return false;
}

static int zv_opmode_to_sys_state(int32_t opmode)
{
    /* ZV opmode: 0=Off, 1=Run, 2=Precharge, 3=PchFail, 4=Charge, 5=Preheat */
    switch(opmode) {
        case 1: return SYSSTATE_SYS_READY;
        case 2: return SYSSTATE_SYS_ACTIVE;
        case 4: return SYSSTATE_SYS_CHARGE;
        case 3: return SYSSTATE_SYS_FAULT;
        case 0: return SYSSTATE_SYS_OFF;
        default: return SYSSTATE_SYS_OFF;
    }
}

static int zv_dir_to_gear(int32_t dir)
{
    /* ZombieVerter DIRS: -1=R, 0=N, 1=D, 2=P */
    switch(dir) {
        case -1: return GEARPOSITION_GEAR_REVERSE;
        case 0:  return GEARPOSITION_GEAR_NEUTRAL;
        case 1:  return GEARPOSITION_GEAR_DRIVE;
        case 2:  return GEARPOSITION_GEAR_PARK;
        default: return GEARPOSITION_GEAR_NEUTRAL;
    }
}

static void zv_integrate_trip(float speed_kmh, float power_kw)
{
    const float dt_h = (float)ZV_CAN_APPLY_MS / 3600000.0f;

    if(speed_kmh > 0.0f) {
        s_trip_m += speed_kmh * 1000.0f * dt_h;
    }
    s_trip_wh += power_kw * 1000.0f * dt_h;

    lv_subject_set_int(&trip_km, (int)(s_trip_m / 1000.0f));
    lv_subject_set_int(&trip_wh_used, (int)(s_trip_wh + 0.5f));
}

static void zv_update_efficiency(float speed_kmh, float power_kw)
{
    if(speed_kmh >= ZV_EFF_MIN_SPEED_KMH && power_kw >= ZV_EFF_MIN_POWER_KW) {
        int wh_km = (int)(power_kw * 1000.0f / speed_kmh + 0.5f);
        if(wh_km > ZV_EFF_MAX_WH_KM) {
            wh_km = ZV_EFF_MAX_WH_KM;
        }
        lv_subject_set_int(&efficiency_wh_per_km, wh_km);
    } else {
        lv_subject_set_int(&efficiency_wh_per_km, 0);
    }
}

static void snapshot_set_float(float *dst, bool *have, float val)
{
    *dst  = val;
    *have = true;
}

static void snapshot_set_i32(int32_t *dst, bool *have, int32_t val)
{
    *dst  = val;
    *have = true;
}

static void snapshot_apply_param(zv_snapshot_t *snap, uint16_t param_id, float val)
{
    switch(param_id) {
        case ZV_PARAM_OPMODE:
            snapshot_set_i32(&snap->opmode, &snap->have_opmode, (int32_t)val);
            break;
        case ZV_PARAM_LASTERR:
            snapshot_set_i32(&snap->lasterr, &snap->have_lasterr, (int32_t)val);
            break;
        case ZV_PARAM_UDC:
            snapshot_set_float(&snap->udc, &snap->have_udc, val);
            break;
        case ZV_PARAM_UDC2:
            snapshot_set_float(&snap->udc2, &snap->have_udc2, val);
            break;
        case ZV_PARAM_POWER:
            snapshot_set_float(&snap->power_kw, &snap->have_power, val);
            break;
        case ZV_PARAM_IDC:
            snapshot_set_float(&snap->idc, &snap->have_idc, val);
            break;
        case ZV_PARAM_KWH:
            snapshot_set_float(&snap->kwh, &snap->have_kwh, val);
            break;
        case ZV_PARAM_SOC:
            snapshot_set_i32(&snap->soc, &snap->have_soc, (int32_t)val);
            break;
        case ZV_PARAM_SPEED_RPM:
            snapshot_set_i32(&snap->speed_rpm, &snap->have_speed_rpm, (int32_t)val);
            break;
        case ZV_PARAM_VEH_SPEED:
            snapshot_set_float(&snap->veh_speed_kmh, &snap->have_veh_speed, val);
            break;
        case ZV_PARAM_DIR:
            snapshot_set_i32(&snap->dir, &snap->have_dir, (int32_t)val);
            break;
        case ZV_PARAM_TMPHS:
            snapshot_set_float(&snap->tmphs, &snap->have_tmphs, val);
            break;
        case ZV_PARAM_TMPM:
            snapshot_set_float(&snap->tmpm, &snap->have_tmpm, val);
            break;
        case ZV_PARAM_HOUR:
            snapshot_set_i32(&snap->hour, &snap->have_hour, (int32_t)val);
            break;
        case ZV_PARAM_MIN:
            snapshot_set_i32(&snap->minute, &snap->have_min, (int32_t)val);
            break;
        case ZV_PARAM_U12V:
            snapshot_set_float(&snap->u12v, &snap->have_u12v, val);
            break;
        case ZV_PARAM_UAUX:
            snapshot_set_float(&snap->uaux, &snap->have_uaux, val);
            break;
        case ZV_PARAM_TMPAUX:
            snapshot_set_float(&snap->tmpaux, &snap->have_tmpaux, val);
            break;
        case ZV_PARAM_DELTAV:
            snapshot_set_float(&snap->deltav, &snap->have_deltav, val);
            break;
        case ZV_PARAM_CHG_TEMP:
            snapshot_set_float(&snap->chg_temp, &snap->have_chg_temp, val);
            break;
        case ZV_PARAM_BMS_VMIN:
            snapshot_set_float(&snap->bms_vmin, &snap->have_bms_vmin, val);
            break;
        case ZV_PARAM_BMS_VMAX:
            snapshot_set_float(&snap->bms_vmax, &snap->have_bms_vmax, val);
            break;
        case ZV_PARAM_BMS_TMIN:
            snapshot_set_float(&snap->bms_tmin, &snap->have_bms_tmin, val);
            break;
        case ZV_PARAM_BMS_TMAX:
            snapshot_set_float(&snap->bms_tmax, &snap->have_bms_tmax, val);
            break;
        case ZV_PARAM_BMS_TAVG:
            snapshot_set_float(&snap->bms_tavg, &snap->have_bms_tavg, val);
            break;
        case ZV_PARAM_BMS_CHARGE_LIM:
            snapshot_set_float(&snap->bms_charge_lim, &snap->have_bms_charge_lim, val);
            break;
        case ZV_PARAM_BMS_SOH: {
            int soh = (int)(val + 0.5f);
            if(soh < 0) {
                soh = 0;
            }
            if(soh > 100) {
                soh = 100;
            }
            snapshot_set_i32(&snap->bms_soh, &snap->have_bms_soh, soh);
            break;
        }
        case ZV_PARAM_CCS_I:
            snapshot_set_float(&snap->ccs_i, &snap->have_ccs_i, val);
            break;
        default:
            break;
    }
}

static bool zv_resolve_spot_param(uint32_t can_id, uint16_t *param_id_out)
{
    if((can_id & ZV_CAN_BROADCAST_MASK) == ZV_CAN_BROADCAST_PREFIX) {
        *param_id_out = (uint16_t)(can_id & 0xFFFFU);
        return true;
    }

    if(can_id > 0x7FFU) {
        return false;
    }

    switch((uint16_t)can_id) {
        case ZV_STD_CAN_HOUR:     *param_id_out = ZV_PARAM_HOUR;     return true;
        case ZV_STD_CAN_MIN:      *param_id_out = ZV_PARAM_MIN;      return true;
        case ZV_STD_CAN_U12V:     *param_id_out = ZV_PARAM_U12V;     return true;
        case ZV_STD_CAN_BMS_VMIN: *param_id_out = ZV_PARAM_BMS_VMIN; return true;
        case ZV_STD_CAN_BMS_VMAX: *param_id_out = ZV_PARAM_BMS_VMAX; return true;
        case ZV_STD_CAN_BMS_TMAX: *param_id_out = ZV_PARAM_BMS_TMAX; return true;
        case ZV_STD_CAN_CCS_I:    *param_id_out = ZV_PARAM_CCS_I;    return true;
        case ZV_STD_CAN_BMS_TMIN: *param_id_out = ZV_PARAM_BMS_TMIN; return true;
        case ZV_STD_CAN_BMS_TAVG: *param_id_out = ZV_PARAM_BMS_TAVG; return true;
        case ZV_STD_CAN_BMS_SOH:  *param_id_out = ZV_PARAM_BMS_SOH;  return true;
        default:
            break;
    }

    /* VCU CanMap: spot param id as 11-bit standard CAN id (2000–2047 range). */
    if(can_id >= 2000U && can_id <= 2047U) {
        *param_id_out = (uint16_t)can_id;
        return true;
    }

    return false;
}

static void snapshot_apply_spot_field(zv_snapshot_t *snap, uint16_t param_id, const uint8_t *data,
                                    zv_pack_val_kind_t kind)
{
    if(param_id == ZV_PACK_PARAM_NONE) {
        return;
    }

    if(kind == ZV_PACK_VAL_INT || param_id == ZV_PARAM_DIR) {
        snapshot_apply_param(snap, param_id, (float)zv_unpack_spot_int(data));
    } else {
        snapshot_apply_param(snap, param_id, zv_unpack_spot_float(data));
    }
}

static void handle_spot_frame(zv_snapshot_t *snap, uint16_t param_id, const uint8_t *data, uint8_t dlc)
{
    if(dlc < 4) {
        return;
    }

    snap->rx_spot_frames++;
    snap->last_rx_us = esp_timer_get_time();
    snapshot_apply_spot_field(snap, param_id, data,
                              (param_id == ZV_PARAM_DIR) ? ZV_PACK_VAL_INT : ZV_PACK_VAL_FLOAT);
}

static void handle_packed_frame(zv_snapshot_t *snap, uint32_t can_id, const uint8_t *data, uint8_t dlc)
{
    const zv_pack_frame_def_t *def;

    if(dlc < 8) {
        return;
    }

    def = zv_pack_frame_for_id(can_id);
    if(def == NULL) {
        return;
    }

    snap->rx_pack_frames++;
    snap->last_rx_us = esp_timer_get_time();
    snapshot_apply_spot_field(snap, def->param_a, data, def->kind_a);
    snapshot_apply_spot_field(snap, def->param_b, data + 4, def->kind_b);
}

static void handle_spot_can_id(zv_snapshot_t *snap, uint32_t can_id, const uint8_t *data, uint8_t dlc)
{
    uint16_t param_id;

    if(!zv_resolve_spot_param(can_id, &param_id)) {
        return;
    }
    handle_spot_frame(snap, param_id, data, dlc);
}

static bool IRAM_ATTR zv_on_rx_done(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata,
                                    void *user_ctx)
{
    BaseType_t woken = pdFALSE;
    zv_ctx_t *ctx = (zv_ctx_t *)user_ctx;

    if(xSemaphoreTakeFromISR(ctx->free_sem, &woken) != pdTRUE) {
        return woken == pdTRUE;
    }

    zv_rx_slot_t *slot = &ctx->pool[ctx->write_idx];
    if(twai_node_receive_from_isr(handle, &slot->frame) == ESP_OK) {
        ctx->write_idx = (ctx->write_idx + 1) % ZV_CAN_RX_DEPTH;
        xSemaphoreGiveFromISR(ctx->ready_sem, &woken);
    } else {
        xSemaphoreGiveFromISR(ctx->free_sem, &woken);
    }

    return woken == pdTRUE;
}

static bool IRAM_ATTR zv_on_error(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx)
{
    (void)handle;
    (void)user_ctx;
    ESP_EARLY_LOGW(TAG, "TWAI err 0x%lx", (unsigned long)edata->err_flags.val);
    return false;
}

static void zv_try_cell_health_snapshot(const zv_snapshot_t * local, int balance_mv)
{
    if(!local->have_opmode || local->opmode != 1) {
        return;
    }
    if(bms_cell_health_snapshot_done()) {
        return;
    }

    float vmin = local->have_bms_vmin ? local->bms_vmin : 0.0f;
    float vmax = local->have_bms_vmax ? local->bms_vmax : 0.0f;
    if(!bms_cell_health_try_snapshot(vmin, vmax, balance_mv)) {
        return;
    }

    lv_subject_set_int(&cell_health_ready, 1);
    ESP_LOGI(TAG, "cell health snapshot vmin=%.3f vmax=%.3f balance=%d mV",
             vmin, vmax, balance_mv);
}

static void zv_can_apply_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    zv_snapshot_t local;
    memset(&local, 0, sizeof(local));

    if(xSemaphoreTake(s_ctx.snap_mtx, pdMS_TO_TICKS(5)) != pdTRUE) {
        return;
    }
    memcpy(&local, &s_ctx.snap, sizeof(local));
    xSemaphoreGive(s_ctx.snap_mtx);

    if(local.have_veh_speed) {
        int kmh = (int)(local.veh_speed_kmh + 0.5f);
        if(kmh < 0) {
            kmh = 0;
        }
        lv_subject_set_int(&speed_kmh, kmh);
    }

    if(local.have_power) {
        lv_subject_set_float(&power_kw, local.power_kw);
        lv_subject_set_int(&regen_level, zv_power_to_regen_level(local.power_kw));
    }

    if(local.have_soc) {
        int soc = (int)local.soc;
        if(soc < 0) {
            soc = 0;
        }
        if(soc > 100) {
            soc = 100;
        }
        lv_subject_set_int(&state_of_charge_pct, soc);
    }

    if(local.have_udc2) {
        lv_subject_set_float(&battery_voltage_v, local.udc2);
    } else if(local.have_udc) {
        lv_subject_set_float(&battery_voltage_v, local.udc);
    }

    if(local.have_idc) {
        lv_subject_set_float(&battery_current_a, local.idc);
    }

    {
        int batt_c = 0;
        if(zv_pick_batt_temp_c(&local, &batt_c)) {
            lv_subject_set_int(&batt_temp_c, batt_c);
        }
    }

    if(local.have_tmphs) {
        lv_subject_set_int(&inverter_temp_c, (int)(local.tmphs + 0.5f));
    }

    if(local.have_tmpm) {
        lv_subject_set_int(&motor_temp_c, (int)(local.tmpm + 0.5f));
    }

    if(local.have_uaux) {
        lv_subject_set_float(&aux_voltage_v, local.uaux);
    } else if(local.have_u12v) {
        lv_subject_set_float(&aux_voltage_v, local.u12v);
    }

    {
        int balance_mv = -1;

        if(local.have_bms_vmin && local.have_bms_vmax &&
           local.bms_vmin > 0.5f && local.bms_vmax > 0.5f) {
            balance_mv = (int)((local.bms_vmax - local.bms_vmin) * 1000.0f + 0.5f);
        } else if(local.have_deltav) {
            float spread_v = local.deltav >= 0.0f ? local.deltav : -local.deltav;
            balance_mv = (int)(spread_v * 1000.0f + 0.5f);
        }

        if(balance_mv >= 0) {
            lv_subject_set_int(&cell_balance_mv, balance_mv);
        }

        zv_try_cell_health_snapshot(&local, balance_mv);
    }

    if(local.have_bms_soh) {
        lv_subject_set_int(&state_of_health_pct, (int)local.bms_soh);
    }

    if(local.have_kwh) {
        int tenths = (int)(local.kwh * 10.0f + 0.5f);
        if(tenths < 0) {
            tenths = 0;
        }
        lv_subject_set_int(&energy_kwh_remaining, tenths);
    }

    if(local.have_speed_rpm) {
        lv_subject_set_int(&motor_rpm, (int)local.speed_rpm);
    }

    if(local.have_hour) {
        int h = (int)local.hour;
        if(h < 0) {
            h = 0;
        }
        if(h > 23) {
            h = h % 24;
        }
        lv_subject_set_int(&hour, h);
    }

    if(local.have_min) {
        int m = (int)local.minute;
        if(m < 0) {
            m = 0;
        }
        if(m > 59) {
            m = m % 60;
        }
        lv_subject_set_int(&minute, m);
    }

    if(local.have_dir) {
        int gear_pos = zv_dir_to_gear(local.dir);
        lv_subject_set_int(&gear, gear_pos);
        lv_subject_copy_string(&gear_str, zv_gear_to_str(gear_pos));
    }

    if(local.have_veh_speed || local.have_power) {
        float speed = local.have_veh_speed ? local.veh_speed_kmh : 0.0f;
        float power = local.have_power ? local.power_kw : 0.0f;
        zv_integrate_trip(speed, power);
        zv_update_efficiency(speed, power);
    }

    if(local.have_opmode) {
        int op = (int)local.opmode;
        if(op == 0 && s_last_opmode != 0) {
            bms_cell_health_reset_drive();
            lv_subject_set_int(&cell_health_ready, 0);
        }
        s_last_opmode = op;
        lv_subject_set_int(&sys_state, zv_opmode_to_sys_state(local.opmode));
    }

    if(local.have_lasterr) {
        lv_subject_set_int(&fault_code, (int)local.lasterr);
    }

    if(local.have_ccs_i) {
        lv_subject_set_float(&charge_amps_a, local.ccs_i);
    } else if(local.have_bms_charge_lim && local.bms_charge_lim > 0.0f) {
        lv_subject_set_float(&charge_amps_a, local.bms_charge_lim);
    } else if(local.have_idc && local.have_opmode && local.opmode == 4) {
        lv_subject_set_float(&charge_amps_a, local.idc > 0.0f ? local.idc : -local.idc);
    }
}

static void zv_can_rx_task(void *arg)
{
    (void)arg;
    int64_t last_log_us = esp_timer_get_time();

    ESP_LOGI(TAG, "RX task running");

    while(1) {
        if(xSemaphoreTake(s_ctx.ready_sem, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        zv_rx_slot_t *slot = &s_ctx.pool[s_ctx.read_idx];
        twai_frame_t *frame = &slot->frame;
        int dlc = (int)twaifd_dlc2len(frame->header.dlc);

        if(xSemaphoreTake(s_ctx.snap_mtx, pdMS_TO_TICKS(10)) == pdTRUE) {
            s_ctx.snap.rx_frames++;
            if(zv_pack_can_id_valid(frame->header.id)) {
                handle_packed_frame(&s_ctx.snap, frame->header.id, frame->buffer, (uint8_t)dlc);
            } else {
                handle_spot_can_id(&s_ctx.snap, frame->header.id, frame->buffer, (uint8_t)dlc);
            }
            xSemaphoreGive(s_ctx.snap_mtx);
        }

        s_ctx.read_idx = (s_ctx.read_idx + 1) % ZV_CAN_RX_DEPTH;
        xSemaphoreGive(s_ctx.free_sem);

        int64_t now = esp_timer_get_time();
        if((now - last_log_us) > 10 * 1000000LL) {
            last_log_us = now;
            if(xSemaphoreTake(s_ctx.snap_mtx, pdMS_TO_TICKS(10)) == pdTRUE) {
                ESP_LOGI(TAG, "rx=%lu pack=%lu spot=%lu last=%lld ms ago",
                         (unsigned long)s_ctx.snap.rx_frames,
                         (unsigned long)s_ctx.snap.rx_pack_frames,
                         (unsigned long)s_ctx.snap.rx_spot_frames,
                         (long long)((now - s_ctx.snap.last_rx_us) / 1000));
                xSemaphoreGive(s_ctx.snap_mtx);
            }
        }
    }
}

void zv_can_trip_reset(void)
{
    s_trip_m  = 0.0f;
    s_trip_wh = 0.0f;
    lv_subject_set_int(&trip_km, 0);
    lv_subject_set_int(&trip_wh_used, 0);
    lv_subject_set_int(&efficiency_wh_per_km, 0);
}

esp_err_t zv_can_init(void)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    zv_can_trip_reset();

    for(int i = 0; i < ZV_CAN_RX_DEPTH; i++) {
        s_ctx.pool[i].frame.buffer = s_ctx.pool[i].data;
        s_ctx.pool[i].frame.buffer_len = sizeof(s_ctx.pool[i].data);
    }

    s_ctx.free_sem = xSemaphoreCreateCounting(ZV_CAN_RX_DEPTH, ZV_CAN_RX_DEPTH);
    s_ctx.ready_sem = xSemaphoreCreateCounting(ZV_CAN_RX_DEPTH, 0);
    s_ctx.snap_mtx = xSemaphoreCreateMutex();
    if(s_ctx.free_sem == NULL || s_ctx.ready_sem == NULL || s_ctx.snap_mtx == NULL) {
        return ESP_ERR_NO_MEM;
    }

    twai_onchip_node_config_t node_config = {
        .io_cfg = {
            .tx = ZV_CAN_TX_GPIO,
            .rx = ZV_CAN_RX_GPIO,
            .quanta_clk_out = GPIO_NUM_NC,
            .bus_off_indicator = GPIO_NUM_NC,
        },
        .bit_timing.bitrate = ZV_CAN_BITRATE_HZ,
        .tx_queue_depth = 5,
        .fail_retry_cnt = 3,
    };

    esp_err_t err = twai_new_node_onchip(&node_config, &s_ctx.node);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "twai_new_node_onchip: %s", esp_err_to_name(err));
        return err;
    }

    twai_event_callbacks_t cbs = {
        .on_rx_done = zv_on_rx_done,
        .on_error = zv_on_error,
    };
    err = twai_node_register_event_callbacks(s_ctx.node, &cbs, &s_ctx);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "twai_node_register_event_callbacks: %s", esp_err_to_name(err));
        twai_node_delete(s_ctx.node);
        return err;
    }

    err = twai_node_enable(s_ctx.node);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "twai_node_enable: %s", esp_err_to_name(err));
        twai_node_delete(s_ctx.node);
        return err;
    }

    BaseType_t ok = xTaskCreate(zv_can_rx_task, "zv_can_rx", ZV_CAN_RX_STACK, NULL, ZV_CAN_RX_PRIO,
                                &s_ctx.rx_task);
    if(ok != pdPASS) {
        twai_node_disable(s_ctx.node);
        twai_node_delete(s_ctx.node);
        return ESP_ERR_NO_MEM;
    }

    lv_timer_create(zv_can_apply_timer_cb, ZV_CAN_APPLY_MS, NULL);

    ESP_LOGI(TAG, "TWAI %d bps TX=GPIO%d RX=GPIO%d — packed 0x500..0x509 + spot IDs",
             ZV_CAN_BITRATE_HZ, (int)ZV_CAN_TX_GPIO, (int)ZV_CAN_RX_GPIO);
    return ESP_OK;
}
