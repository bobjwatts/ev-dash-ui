#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "esp_log.h"
#include "ev_dash.h"
#include "ev_dash_gen.h"
#include "images/ev_dash_assets.h"
#include "screens/screen_main_gen.h"
#include "user_code/speedometer_assets.h"

extern const lv_image_dsc_t dial_speed_dial_data;
extern const lv_image_dsc_t dial_speed_needle_data;

static const char *TAG = "ev_dash";



static void demo_speed_timer_cb(lv_timer_t *timer)

{

    LV_UNUSED(timer);

    static int speed = 0;

    static int dir = 1;



    speed += dir * 2;

    if (speed >= 200) {

        speed = 200;

        dir = -1;

    } else if (speed <= 0) {

        speed = 0;

        dir = 1;

    }



    lv_subject_set_int(&speed_kmh, speed);

}



void app_main(void)

{

    ESP_LOGI(TAG, "Starting EV dash on ESP32-P4");



    bsp_display_cfg_t disp_cfg = {

        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),

        .buffer_size = BSP_LCD_H_RES * 120,

        .double_buffer = true,

        .hw_cfg = {

            .hdmi_resolution = BSP_HDMI_RES_NONE,

            .dsi_bus = {

                .phy_clk_src = 0,

                .lane_bit_rate_mbps = BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS,

            },

        },

        .flags = {

            .buff_dma = true,

            .buff_spiram = true,

            .sw_rotate = false,

        },

    };



    lv_display_t *disp = bsp_display_start_with_config(&disp_cfg);

    if (disp == NULL) {

        ESP_LOGE(TAG, "Display init failed");

        return;

    }



    ESP_LOGI(TAG, "LVGL display resolution: %dx%d",

             lv_display_get_horizontal_resolution(disp),

             lv_display_get_vertical_resolution(disp));

    ESP_LOGI(TAG, "BSP panel target: %dx%d",

             BSP_LCD_H_RES, BSP_LCD_V_RES);



    bsp_display_backlight_on();



    bsp_display_lock(0);

    ev_dash_init(NULL);

    ESP_LOGI(TAG, "Embedded assets id=%s dial=%dx%d needle=%dx%d (data_size dial=%u needle=%u)",
             EV_DASH_ASSETS_ID,
             dial_speed_dial_data.header.w, dial_speed_dial_data.header.h,
             dial_speed_needle_data.header.w, dial_speed_needle_data.header.h,
             (unsigned)dial_speed_dial_data.data_size,
             (unsigned)dial_speed_needle_data.data_size);
#if defined(EV_DASH_DIAL_CF)
    ESP_LOGI(TAG, "Dial color format: %s", EV_DASH_DIAL_CF);
#endif
    if (dial_speed_dial_data.header.w != EV_DASH_DIAL_EMBED_W ||
        dial_speed_dial_data.header.h != EV_DASH_DIAL_EMBED_H) {
        ESP_LOGW(TAG, "Stale dial embed in firmware — run gen_image_data.ps1 then idf.py fullclean build flash");
    }
    ESP_LOGI(TAG, "Dial embed=%dx%d display target=%dx%d",
             dial_speed_dial_data.header.w, dial_speed_dial_data.header.h,
             EV_DASH_DIAL_DISPLAY_W, EV_DASH_DIAL_DISPLAY_H);
#if SPEEDOMETER_DEBUG_SIZE_RINGS
    ESP_LOGI(TAG, "Size debug rings ON: magenta=%d green=%d orange=%d px (vector 1:1)",
             SPEEDOMETER_DIAL_DISPLAY_PX, SPEEDOMETER_ARC_TRACK_PX, SPEEDOMETER_FACE_DISPLAY_PX);
#if defined(EV_DASH_DEBUG_DRAW_TEST) && EV_DASH_DEBUG_DRAW_TEST
    extern const lv_image_dsc_t debug_img_439_rgb565_data;
    ESP_LOGI(TAG, "Draw-test solids: dbg100=100x100 dbg439_a8=439x439 dbg439_rgb565=%u B",
             (unsigned)debug_img_439_rgb565_data.data_size);
#endif
#if defined(SPEEDOMETER_DEBUG_DIAL_SOLID) && SPEEDOMETER_DEBUG_DIAL_SOLID
    ESP_LOGI(TAG, "Dial source: SOLID RGB565 439x439 cyan (not dial art)");
#endif
#if SPEEDOMETER_DEBUG_SIMPLE_CENTER
    ESP_LOGI(TAG, "Layout: centred speedometer only (no side panel)");
#endif
#endif

    lv_screen_load(screen_main_create());

    bsp_display_unlock();



    lv_timer_create(demo_speed_timer_cb, 50, NULL);



    ESP_LOGI(TAG, "UI loaded");

}


