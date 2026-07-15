#include "dashboard_buttons.h"
#include "zv_can.h"
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

void app_main(void)
{
    ESP_LOGI(TAG, "Starting EV dash on ESP32-P4");

    bsp_display_cfg_t disp_cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        /* buffer_size / buff_spiram ignored in avoid_tearing mode —
         * the port uses DPI hardware framebuffers directly.          */
        .buffer_size  = BSP_LCD_H_RES * BSP_LCD_V_RES,
        .double_buffer = true,
        .hw_cfg = {
            .hdmi_resolution = BSP_HDMI_RES_NONE,
            .dsi_bus = {
                .phy_clk_src       = 0,
                .lane_bit_rate_mbps = BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS,
            },
        },
        .flags = {
            .buff_dma    = true,
            .buff_spiram = true,
            .sw_rotate   = false,
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
    ESP_LOGI(TAG, "BSP panel target: %dx%d", BSP_LCD_H_RES, BSP_LCD_V_RES);

    bsp_display_backlight_on();

    bsp_display_lock(0);

    ev_dash_init(NULL);

    lv_subject_set_int(&info_panel, INFO_PANEL_PACK_VOLTAGE);

    ESP_LOGI(TAG, "Embedded assets id=%s dial=%dx%d needle=%dx%d (dial=%u B needle=%u B)",
             EV_DASH_ASSETS_ID,
             dial_speed_dial_data.header.w, dial_speed_dial_data.header.h,
             dial_speed_needle_data.header.w, dial_speed_needle_data.header.h,
             (unsigned)dial_speed_dial_data.data_size,
             (unsigned)dial_speed_needle_data.data_size);
#if defined(EV_DASH_DIAL_CF)
    ESP_LOGI(TAG, "Dial colour format: %s", EV_DASH_DIAL_CF);
#endif
    if (dial_speed_dial_data.header.w != EV_DASH_DIAL_EMBED_W ||
        dial_speed_dial_data.header.h != EV_DASH_DIAL_EMBED_H) {
        ESP_LOGW(TAG, "Stale dial embed — run gen_image_data_lvgl.ps1 then idf.py fullclean build flash");
    }
#if SPEEDOMETER_DEBUG_SIZE_RINGS
    ESP_LOGI(TAG, "Size debug rings ON: magenta=%d green=%d orange=%d px",
             SPEEDOMETER_DIAL_DISPLAY_PX, SPEEDOMETER_ARC_TRACK_PX, SPEEDOMETER_FACE_DISPLAY_PX);
#endif

    lv_screen_load(screen_main_create());
    bsp_display_unlock();

    dashboard_buttons_init();

    if(zv_can_init() != ESP_OK) {
        ESP_LOGW(TAG, "ZombieVerter CAN init failed — gauges stay at defaults until TWAI is wired");
    }

    ESP_LOGI(TAG, "UI loaded (CAN: GPIO4=TX GPIO5=RX; panel=GPIO2 trip_reset=GPIO3)");
}
