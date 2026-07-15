/*
 * Bench CAN simulator — pretends to be a ZombieVerter broadcasting spot values.
 *
 * Target: any ESP32 (not P4). Wire a second SN65HVD230 to the dash bus:
 *   GPIO 4 = CTX, GPIO 5 = CRX, 3.3 V, GND common with dash transceiver.
 *   CANH ↔ CANH, CANL ↔ CANL with the dash SN65HVD230.
 *   120 Ω between CANH and CANL on at least one board (needed even with one node).
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

static const char *TAG = "zv_sim";

#define SIM_TX_GPIO      GPIO_NUM_4
#define SIM_RX_GPIO      GPIO_NUM_5
#define SIM_BITRATE_HZ   500000
#define SIM_PERIOD_MS    100

/* 1 = transmit without needing another node to ACK (sim runs alone on the bench).
 * Frames still go out on CANH/CANL — the dash can receive when wired up.        */
#define SIM_SELF_TEST    1

#define ZV_CAN_BROADCAST_BASE  0x02A0000U

#define ZV_PARAM_OPMODE      2002
#define ZV_PARAM_UDC           2006
#define ZV_PARAM_UDC2          2007
#define ZV_PARAM_POWER         2011
#define ZV_PARAM_IDC           2012
#define ZV_PARAM_KWH           2013
#define ZV_PARAM_SOC           2015
#define ZV_PARAM_SPEED_RPM     2016
#define ZV_PARAM_VEH_SPEED     2017
#define ZV_PARAM_DIR           2024
#define ZV_PARAM_TMPHS         2028
#define ZV_PARAM_TMPM          2029
#define ZV_PARAM_HOUR          2065
#define ZV_PARAM_MIN           2066
#define ZV_PARAM_U12V          2070
#define ZV_PARAM_BMS_VMIN      2084
#define ZV_PARAM_BMS_VMAX      2085
#define ZV_PARAM_BMS_TMAX      2087

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

    /* Recovery needs 128 x 11 recessive bits — poll until active or timeout */
    for(int i = 0; i < 100; i++) {
        vTaskDelay(pdMS_TO_TICKS(10));
        twai_node_get_info(s_node, &st, NULL);
        if(sim_bus_is_active(&st)) {
            return true;
        }
    }
    return false;
}

static int32_t float_to_spot(float value)
{
    return (int32_t)(value * 32.0f);
}

static void pack_spot_be(uint8_t *buf, int32_t raw)
{
    buf[0] = (uint8_t)((raw >> 24) & 0xFF);
    buf[1] = (uint8_t)((raw >> 16) & 0xFF);
    buf[2] = (uint8_t)((raw >> 8) & 0xFF);
    buf[3] = (uint8_t)(raw & 0xFF);
}

static esp_err_t send_spot_i32(uint16_t param_id, int32_t raw)
{
    if(!sim_ensure_bus_active()) {
        return ESP_ERR_INVALID_STATE;
    }

    pack_spot_be(s_tx_buf, raw);

    twai_frame_t frame = {
        .header = {
            .id  = ZV_CAN_BROADCAST_BASE + param_id,
            .ide = 1,
            .dlc = twaifd_len2dlc(4),
        },
        .buffer     = s_tx_buf,
        .buffer_len = 4,
    };

    esp_err_t err = twai_node_transmit(s_node, &frame, 50);
    if(err == ESP_OK) {
        /* TX is async — wait before reusing s_tx_buf */
        twai_node_transmit_wait_all_done(s_node, 200);
    }
    return err;
}

static esp_err_t send_spot_float(uint16_t param_id, float value)
{
    return send_spot_i32(param_id, float_to_spot(value));
}

static void sim_task(void *arg)
{
    (void)arg;
    int tick = 0;

    ESP_LOGI(TAG, "Broadcasting ZV spot frames every %d ms", SIM_PERIOD_MS);

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
        /* ZombieVerter DIRS: 2=P, -1=R, 0=N, 1=D — cycle every ~5 s */
        static const int8_t GEAR_CYCLE[] = { 2, -1, 0, 1 };
        int dir = GEAR_CYCLE[(tick / 50) % 4];

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
        send_spot_float(ZV_PARAM_TMPM, 55.0f);
        send_spot_float(ZV_PARAM_BMS_TMAX, 28.0f);
        send_spot_float(ZV_PARAM_BMS_VMIN, 3.95f);
        send_spot_float(ZV_PARAM_BMS_VMAX, 4.03f);
        send_spot_float(ZV_PARAM_U12V, 13.8f);
        send_spot_i32(ZV_PARAM_OPMODE, 1); /* Run */
        send_spot_i32(ZV_PARAM_HOUR, 14);
        send_spot_i32(ZV_PARAM_MIN, (tick / 10) % 60);

        if((tick % 50) == 0) {
            ESP_LOGI(TAG, "tick=%d speed=%.0f kph power=%.1f kW dir=%d", tick, speed, power, dir);
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

    ESP_LOGI(TAG, "ZV bench sim on TWAI %d bps, TX=GPIO%d RX=GPIO%d (self_test=%d)",
             SIM_BITRATE_HZ, (int)SIM_TX_GPIO, (int)SIM_RX_GPIO, SIM_SELF_TEST);
    ESP_LOGI(TAG, "Connect CANH/CANL/GND to dash SN65HVD230 (common ground required)");

    xTaskCreate(sim_task, "zv_sim", 4096, NULL, 5, NULL);
}
