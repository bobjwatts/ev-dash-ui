/*
 * Bench CAN simulator — pretends to be a ZombieVerter on the dash bus.
 *
 * Default: packed CanMap frames (0x500..0x509, two spot32 fields each).
 * Set SIM_TX_MODE to 1 for legacy per-param spot IDs, 0 for extended spot IDs.
 *
 * BMS extras for SOH panel (param id > 2047 → std remap 1803/1804/1809 in packed mode):
 *   BMS_Vmin, BMS_Vmax — cell snapshot + balance estimate
 *   BMS_SOH            — pack SOH % (Phase 1)
 *   opmode Run on 0x502 — triggers once-per-drive cell dot snapshot
 *
 * Target: any ESP32 (not P4). Wire a second SN65HVD230 to the dash bus:
 *   GPIO 4 = CTX, GPIO 5 = CRX, 3.3 V, GND common with dash transceiver.
 *   CANH <-> CANH, CANL <-> CANL with the dash SN65HVD230.
 *   120 ohm between CANH and CANL on at least one board (needed even with one node).
 *
 * Build (from this directory):
 *   idf.py set-target esp32
 *   idf.py build flash monitor
 */

#include <math.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "zv_can_pack.h"
#include "zv_can_params.h"

static const char *TAG = "zv_sim";

#define SIM_TX_GPIO      GPIO_NUM_4
#define SIM_RX_GPIO      GPIO_NUM_5
#define SIM_BITRATE_HZ   500000
#define SIM_PERIOD_MS    100

/* Motor temp (tmpm → power gauge arc): 0→150→0 °C over TEMP_SWEEP_PERIOD_S. */
#define TEMP_SWEEP_MAX_C       150.0f
#define TEMP_SWEEP_PERIOD_S    120.0f   /* full up+down cycle */

/* 1 = transmit without needing another node to ACK (sim runs alone on the bench). */
#define SIM_SELF_TEST    1

/* 2 = packed 0x500..0x509 (default), 1 = std spot IDs, 0 = extended spot IDs */
#define SIM_TX_MODE      2

#define ZV_CAN_BROADCAST_BASE  0x02A0000U

static twai_node_handle_t s_node;
static uint8_t            s_tx_buf[8];
static bool               s_bus_diag_logged;

static bool sim_bus_is_active(const twai_node_status_t *st)
{
    return st->state == TWAI_ERROR_ACTIVE ||
           st->state == TWAI_ERROR_WARNING ||
           st->state == TWAI_ERROR_PASSIVE;
}

static void sim_log_bus_diag(const twai_node_status_t *st, const twai_node_record_t *rec)
{
    if(s_bus_diag_logged) {
        return;
    }
    s_bus_diag_logged = true;
    ESP_LOGW(TAG,
             "TWAI bus-off (TEC=%u REC=%u bus_errs=%lu) — hardware checklist:",
             st->tx_error_count, st->rx_error_count, (unsigned long)rec->bus_err_num);
    ESP_LOGW(TAG, "  HVD230: 3.3 V, GND, RS pin -> GND (normal slope)");
    ESP_LOGW(TAG, "  ESP: GPIO4->CTX, GPIO5->CRX (not swapped)");
    ESP_LOGW(TAG, "  Bus: CANH-CANL + 120 ohm terminator (required even for one node)");
    ESP_LOGW(TAG, "  Dash: powered, same GND, CANH/CANL connected");
}

static bool sim_ensure_bus_active(void)
{
    twai_node_status_t st;
    twai_node_record_t rec;

    twai_node_get_info(s_node, &st, &rec);
    if(sim_bus_is_active(&st)) {
        return true;
    }
    if(st.state != TWAI_ERROR_BUS_OFF) {
        return false;
    }

    sim_log_bus_diag(&st, &rec);
    esp_err_t err = twai_node_recover(s_node);
    if(err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return false;
    }

    for(int i = 0; i < 100; i++) {
        vTaskDelay(pdMS_TO_TICKS(10));
        twai_node_get_info(s_node, &st, NULL);
        if(sim_bus_is_active(&st)) {
            return true;
        }
    }
    return false;
}

static uint32_t sim_spot_can_id(uint16_t param_id)
{
#if SIM_TX_MODE == 1
    switch(param_id) {
        case ZV_PARAM_HOUR:      return ZV_STD_CAN_HOUR;
        case ZV_PARAM_MIN:       return ZV_STD_CAN_MIN;
        case ZV_PARAM_U12V:      return ZV_STD_CAN_U12V;
        case ZV_PARAM_BMS_VMIN:  return ZV_STD_CAN_BMS_VMIN;
        case ZV_PARAM_BMS_VMAX:  return ZV_STD_CAN_BMS_VMAX;
        case ZV_PARAM_BMS_TMAX:  return ZV_STD_CAN_BMS_TMAX;
        case ZV_PARAM_BMS_SOH:   return ZV_STD_CAN_BMS_SOH;
        default:                 return param_id;
    }
#else
    return ZV_CAN_BROADCAST_BASE + param_id;
#endif
}

static esp_err_t sim_transmit_frame(uint32_t can_id, bool extended, size_t len)
{
    if(!sim_ensure_bus_active()) {
        return ESP_ERR_INVALID_STATE;
    }

    twai_frame_t frame = {
        .header = {
            .id  = can_id,
            .ide = extended ? 1 : 0,
            .dlc = twaifd_len2dlc(len),
        },
        .buffer     = s_tx_buf,
        .buffer_len = len,
    };

    esp_err_t err = twai_node_transmit(s_node, &frame, 50);
    if(err == ESP_OK) {
        twai_node_transmit_wait_all_done(s_node, 200);
    }
    return err;
}

static esp_err_t send_spot_i32(uint16_t param_id, int32_t raw)
{
    zv_pack_spot_be(s_tx_buf, raw);
#if SIM_TX_MODE == 1
    return sim_transmit_frame(sim_spot_can_id(param_id), false, 4);
#elif SIM_TX_MODE == 0
    return sim_transmit_frame(sim_spot_can_id(param_id), true, 4);
#else
    (void)param_id;
    (void)raw;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t send_spot_float(uint16_t param_id, float value)
{
    return send_spot_i32(param_id, zv_spot32_from_float(value));
}

static esp_err_t send_spot_int(uint16_t param_id, int32_t value)
{
    return send_spot_i32(param_id, zv_spot32_from_int(value));
}

static bool sim_std_spot_can_id(uint16_t param_id, uint32_t *can_id_out)
{
    switch(param_id) {
        case ZV_PARAM_HOUR:         *can_id_out = ZV_STD_CAN_HOUR;     return true;
        case ZV_PARAM_MIN:          *can_id_out = ZV_STD_CAN_MIN;      return true;
        case ZV_PARAM_U12V:         *can_id_out = ZV_STD_CAN_U12V;     return true;
        case ZV_PARAM_BMS_VMIN:     *can_id_out = ZV_STD_CAN_BMS_VMIN; return true;
        case ZV_PARAM_BMS_VMAX:     *can_id_out = ZV_STD_CAN_BMS_VMAX; return true;
        case ZV_PARAM_BMS_TMAX:     *can_id_out = ZV_STD_CAN_BMS_TMAX; return true;
        case ZV_PARAM_BMS_TMIN:     *can_id_out = ZV_STD_CAN_BMS_TMIN; return true;
        case ZV_PARAM_BMS_TAVG:     *can_id_out = ZV_STD_CAN_BMS_TAVG; return true;
        case ZV_PARAM_BMS_SOH:      *can_id_out = ZV_STD_CAN_BMS_SOH;  return true;
        default:                    return false;
    }
}

/** Std 11-bit spot (dash remap for params with id > 2047). */
static esp_err_t send_spot_std_i32(uint16_t param_id, int32_t raw)
{
    uint32_t can_id;

    if(!sim_std_spot_can_id(param_id, &can_id)) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    zv_pack_spot_be(s_tx_buf, raw);
    return sim_transmit_frame(can_id, false, 4);
}

static esp_err_t send_spot_std_float(uint16_t param_id, float value)
{
    return send_spot_std_i32(param_id, zv_spot32_from_float(value));
}

static esp_err_t send_spot_std_int(uint16_t param_id, int32_t value)
{
    return send_spot_std_i32(param_id, zv_spot32_from_int(value));
}

static void sim_send_bms_spots(float vmin, float vmax, int soh_pct)
{
#if SIM_TX_MODE == 2
    send_spot_std_float(ZV_PARAM_BMS_VMIN, vmin);
    send_spot_std_float(ZV_PARAM_BMS_VMAX, vmax);
    send_spot_std_int(ZV_PARAM_BMS_SOH, soh_pct);
#else
    send_spot_float(ZV_PARAM_BMS_VMIN, vmin);
    send_spot_float(ZV_PARAM_BMS_VMAX, vmax);
    send_spot_int(ZV_PARAM_BMS_SOH, soh_pct);
#endif
}

static esp_err_t send_packed_frame(uint16_t can_id, int32_t raw_a, int32_t raw_b)
{
    zv_pack_spot_be(s_tx_buf, raw_a);
    zv_pack_spot_be(s_tx_buf + 4, raw_b);
    return sim_transmit_frame(can_id, false, 8);
}

static esp_err_t send_packed_float(uint16_t can_id, float val_a, float val_b)
{
    return send_packed_frame(can_id, zv_spot32_from_float(val_a), zv_spot32_from_float(val_b));
}

static esp_err_t send_packed_int_pair(uint16_t can_id, int32_t val_a, int32_t val_b)
{
    return send_packed_frame(can_id, zv_spot32_from_int(val_a), zv_spot32_from_int(val_b));
}

/** Slow SOH drift 81–93% for bench visibility. */
static int sim_pack_soh_pct(int tick)
{
    float t = (float)tick * 0.1f;
    int soh = 87 + (int)(6.0f * sinf(t * 0.12f) + 0.5f);
    if(soh < 55) {
        soh = 55;
    }
    if(soh > 100) {
        soh = 100;
    }
    return soh;
}

static void sim_pack_cell_voltages(int tick, float *vmin_out, float *vmax_out)
{
    float t = (float)tick * 0.1f;
    float vmin = 3.90f + 0.03f * sinf(t * 0.2f);
    float vmax = vmin + 0.08f + 0.04f * sinf(t * 0.13f);

    *vmin_out = vmin;
    *vmax_out = vmax;
}

#if SIM_TX_MODE == 2
static void sim_send_packed_cycle(float speed, float power, float volts, int dir, int tick,
                                  float motor_temp_c, float vmin, float vmax, int soh_pct)
{
    send_packed_float(0x500, speed, power);
    send_packed_frame(0x501, zv_spot32_from_float(72.0f), zv_spot32_from_int(dir));
    send_packed_int_pair(0x502, 1, 0);   /* opmode Run → cell health snapshot */
    send_packed_float(0x503, volts, power * 1000.0f / volts);
    send_packed_float(0x504, 42.0f, motor_temp_c);   /* tmphs, tmpm (motor) */
    send_packed_float(0x505, speed * 50.0f, 18.5f);
    send_packed_int_pair(0x506, 14, (tick / 10) % 60);
    send_packed_float(0x507, 13.8f, 0.0f);   /* uaux, CCS_I */
    send_packed_float(0x508, 28.0f, 72.0f);  /* BMS_Tavg, BMS_ChargeLim (Leaf-like) */
    send_packed_float(0x509, 28.0f, vmax - vmin);  /* tmpaux, deltaV */
    sim_send_bms_spots(vmin, vmax, soh_pct);
}
#endif

#if SIM_TX_MODE != 2
static void sim_send_spot_cycle(float speed, float power, float volts, int dir, int tick,
                                float motor_temp_c, float vmin, float vmax, int soh_pct)
{
    send_spot_float(ZV_PARAM_VEH_SPEED, speed);
    send_spot_float(ZV_PARAM_POWER, power);
    send_spot_float(ZV_PARAM_SOC, 72.0f);
    send_spot_float(ZV_PARAM_UDC2, volts);
    send_spot_float(ZV_PARAM_UDC, volts + 2.0f);
    send_spot_float(ZV_PARAM_IDC, power * 1000.0f / volts);
    send_spot_float(ZV_PARAM_KWH, 18.5f);
    send_spot_float(ZV_PARAM_SPEED_RPM, speed * 50.0f);
    send_spot_i32(ZV_PARAM_DIR, dir);
    send_spot_float(ZV_PARAM_TMPHS, 42.0f);
    send_spot_float(ZV_PARAM_TMPM, motor_temp_c);
    send_spot_float(ZV_PARAM_BMS_TMAX, 28.0f);
    sim_send_bms_spots(vmin, vmax, soh_pct);
    send_spot_float(ZV_PARAM_UAUX, 13.8f);
    send_spot_float(ZV_PARAM_DELTAV, vmax - vmin);
    send_spot_i32(ZV_PARAM_OPMODE, 1);
    send_spot_i32(ZV_PARAM_HOUR, 14);
    send_spot_i32(ZV_PARAM_MIN, (tick / 10) % 60);
}
#endif

/** Triangle 0 → TEMP_SWEEP_MAX_C → 0 for bench testing of temp arc colour bands. */
static float sim_motor_temp_c(int tick)
{
    const int half_ticks = (int)(TEMP_SWEEP_PERIOD_S * 0.5f * 1000.0f / (float)SIM_PERIOD_MS);
    if(half_ticks <= 0) {
        return 0.0f;
    }

    int phase = tick % (half_ticks * 2);
    if(phase < half_ticks) {
        return (float)phase / (float)half_ticks * TEMP_SWEEP_MAX_C;
    }
    return (float)(half_ticks * 2 - phase) / (float)half_ticks * TEMP_SWEEP_MAX_C;
}

static void sim_task(void *arg)
{
    (void)arg;
    int tick = 0;

    ESP_LOGI(TAG, "Broadcasting ZV frames every %d ms (mode=%d)", SIM_PERIOD_MS, SIM_TX_MODE);

    while(1) {
        if(!sim_ensure_bus_active()) {
            if((tick % 50) == 0) {
                ESP_LOGW(TAG, "tick=%d — waiting for bus (see checklist above)", tick);
            }
            tick++;
            vTaskDelay(pdMS_TO_TICKS(SIM_PERIOD_MS));
            continue;
        }

        float t = (float)tick * 0.1f;
        float speed = 40.0f + 30.0f * sinf(t * 0.7f);
        float power = 25.0f * sinf(t * 1.1f);
        float volts = 355.0f + 5.0f * sinf(t * 0.3f);
        float motor_temp = sim_motor_temp_c(tick);
        float vmin = 0.0f;
        float vmax = 0.0f;
        int soh_pct = sim_pack_soh_pct(tick);
        sim_pack_cell_voltages(tick, &vmin, &vmax);
        static const int8_t GEAR_CYCLE[] = { 2, -1, 0, 1 };
        int dir = GEAR_CYCLE[(tick / 50) % 4];

#if SIM_TX_MODE == 2
        sim_send_packed_cycle(speed, power, volts, dir, tick, motor_temp, vmin, vmax, soh_pct);
#else
        sim_send_spot_cycle(speed, power, volts, dir, tick, motor_temp, vmin, vmax, soh_pct);
#endif

        if((tick % 50) == 0) {
            ESP_LOGI(TAG,
                     "tick=%d speed=%.0f kph power=%.1f kW motor=%.0f C SOH=%d%% Vmin=%.3f Vmax=%.3f dir=%d",
                     tick, speed, power, motor_temp, soh_pct, (double)vmin, (double)vmax, dir);
        }

        tick++;
        vTaskDelay(pdMS_TO_TICKS(SIM_PERIOD_MS));
    }
}

void app_main(void)
{
    twai_onchip_node_config_t cfg = {
        .io_cfg = {
            .tx = SIM_TX_GPIO,
            .rx = SIM_RX_GPIO,
            .quanta_clk_out = GPIO_NUM_NC,
            .bus_off_indicator = GPIO_NUM_NC,
        },
        .bit_timing.bitrate = SIM_BITRATE_HZ,
        .tx_queue_depth = 16,
        .fail_retry_cnt = -1,
#if SIM_SELF_TEST
        .flags.enable_self_test = 1,
#endif
    };

    ESP_ERROR_CHECK(twai_new_node_onchip(&cfg, &s_node));
    ESP_ERROR_CHECK(twai_node_enable(s_node));

    twai_node_status_t st;
    twai_node_get_info(s_node, &st, NULL);
    ESP_LOGI(TAG, "TWAI started: state=%d TEC=%u REC=%u", (int)st.state, st.tx_error_count, st.rx_error_count);

    ESP_LOGI(TAG, "ZV bench sim on TWAI %d bps, TX=GPIO%d RX=GPIO%d (self_test=%d mode=%d)",
             SIM_BITRATE_HZ, (int)SIM_TX_GPIO, (int)SIM_RX_GPIO, SIM_SELF_TEST, SIM_TX_MODE);
    ESP_LOGI(TAG, "Motor temp (tmpm): 0→%.0f→0 °C over %.0f s — blue / orange @ %d / red @ %d",
             TEMP_SWEEP_MAX_C, TEMP_SWEEP_PERIOD_S, 70, 85);
    ESP_LOGI(TAG, "SOH panel: BMS_SOH ~81-93%% on std 1809, Vmin/Vmax on 1803/1804, opmode Run");
    ESP_LOGI(TAG, "Connect CANH/CANL/GND to dash SN65HVD230 (common ground required)");

    xTaskCreate(sim_task, "zv_sim", 4096, NULL, 5, NULL);
}
