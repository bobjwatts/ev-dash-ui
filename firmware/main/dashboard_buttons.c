/**
 * @file dashboard_buttons.c
 * @brief Panel-cycle and trip-reset buttons with debounce.
 *
 * Panel button: cycles info_panel (3 panels driving, 4 when charging).
 * Trip reset:   clears trip_km, trip_wh_used, efficiency_wh_per_km.
 *
 * Charging UX:
 *   - On charge start → auto-switch to INFO_PANEL_CHARGING once
 *   - On charge stop  → if on charging panel, fall back to pack voltage
 */

#include "dashboard_buttons.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ev_dash_gen.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char * TAG = "dash_btn";

/* Change these if your wiring differs (input, internal pull-up, active LOW). */
#define DASH_BTN_PANEL_GPIO       GPIO_NUM_2
#define DASH_BTN_TRIP_RESET_GPIO  GPIO_NUM_3

#define DEBOUNCE_MS        50
#define POLL_MS            20

typedef struct {
    gpio_num_t gpio;
    bool       last_level;
    int64_t    last_change_us;
    bool       pressed;
} btn_state_t;

static btn_state_t s_panel_btn;
static btn_state_t s_trip_btn;
static int         s_prev_sys_state = -1;

static bool btn_poll(btn_state_t * btn)
{
    bool level = gpio_get_level(btn->gpio);
    int64_t now = esp_timer_get_time();

    if(level != btn->last_level) {
        btn->last_level = level;
        btn->last_change_us = now;
    }

    if((now - btn->last_change_us) < (DEBOUNCE_MS * 1000)) {
        return false;
    }

    bool raw_pressed = (level == 0);
    if(raw_pressed && !btn->pressed) {
        btn->pressed = true;
        return true;
    }
    if(!raw_pressed) {
        btn->pressed = false;
    }
    return false;
}

static int info_panel_count(void)
{
    return (lv_subject_get_int(&sys_state) == SYSSTATE_SYS_CHARGE) ? 4 : 3;
}

static void advance_info_panel(void)
{
    int panel = lv_subject_get_int(&info_panel);
    int count = info_panel_count();
    panel = (panel + 1) % count;
    lv_subject_set_int(&info_panel, panel);
    ESP_LOGI(TAG, "info panel → %d (%d available)", panel, count);
}

static void check_charging_transitions(void)
{
    int st = lv_subject_get_int(&sys_state);

    if(st == SYSSTATE_SYS_CHARGE && s_prev_sys_state != SYSSTATE_SYS_CHARGE) {
        lv_subject_set_int(&info_panel, INFO_PANEL_CHARGING);
        ESP_LOGI(TAG, "charging started → panel %d", INFO_PANEL_CHARGING);
    }
    else if(st != SYSSTATE_SYS_CHARGE && s_prev_sys_state == SYSSTATE_SYS_CHARGE) {
        if(lv_subject_get_int(&info_panel) == INFO_PANEL_CHARGING) {
            lv_subject_set_int(&info_panel, INFO_PANEL_PACK_VOLTAGE);
            ESP_LOGI(TAG, "charging stopped → panel %d", INFO_PANEL_PACK_VOLTAGE);
        }
    }

    /* Clamp panel index if charging ended while on slot 3 */
    if(st != SYSSTATE_SYS_CHARGE &&
       lv_subject_get_int(&info_panel) >= info_panel_count()) {
        lv_subject_set_int(&info_panel, INFO_PANEL_PACK_VOLTAGE);
    }

    s_prev_sys_state = st;
}

static void btn_task(void * arg)
{
    LV_UNUSED(arg);

    while(true) {
        bsp_display_lock(0);

        check_charging_transitions();

        if(btn_poll(&s_panel_btn)) {
            advance_info_panel();
        }
        if(btn_poll(&s_trip_btn)) {
            lv_subject_set_int(&trip_km, 0);
            lv_subject_set_int(&trip_wh_used, 0);
            lv_subject_set_int(&efficiency_wh_per_km, 0);
            ESP_LOGI(TAG, "trip reset");
        }

        bsp_display_unlock();

        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}

void dashboard_trip_reset(void)
{
    bsp_display_lock(0);
    lv_subject_set_int(&trip_km, 0);
    lv_subject_set_int(&trip_wh_used, 0);
    lv_subject_set_int(&efficiency_wh_per_km, 0);
    bsp_display_unlock();
    ESP_LOGI(TAG, "trip reset");
}

void dashboard_buttons_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << DASH_BTN_PANEL_GPIO) | (1ULL << DASH_BTN_TRIP_RESET_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));

    s_panel_btn.gpio = DASH_BTN_PANEL_GPIO;
    s_panel_btn.last_level = gpio_get_level(DASH_BTN_PANEL_GPIO);
    s_panel_btn.last_change_us = esp_timer_get_time();
    s_panel_btn.pressed = false;

    s_trip_btn.gpio = DASH_BTN_TRIP_RESET_GPIO;
    s_trip_btn.last_level = gpio_get_level(DASH_BTN_TRIP_RESET_GPIO);
    s_trip_btn.last_change_us = esp_timer_get_time();
    s_trip_btn.pressed = false;

    s_prev_sys_state = lv_subject_get_int(&sys_state);

    xTaskCreate(btn_task, "dash_btn", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "panel=GPIO%d trip_reset=GPIO%d", DASH_BTN_PANEL_GPIO, DASH_BTN_TRIP_RESET_GPIO);
}
