/* Riverdi RVT121HVDFWN00: two-lane DSI -> SN65DSI83 -> internal LVDS panel.
 * Register definitions/sequence: TI SLLSEC1I, sections 7.4.2 and 7.6.
 * Module timings: Riverdi DS_RVT121HVDFWN00_Rev.1.0, page 12.
 */
#include "riverdi_display.h"
#include "sdkconfig.h"

#if CONFIG_EV_DASH_DISPLAY_RIVERDI

#include <string.h>
#include "bsp/esp-bsp.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_clk_tree.h"
#include "esp_idf_version.h"
#include "esp_private/esp_clk_tree_common.h"
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 1, 0)
#include "clk_ctrl_os.h"
#endif
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/mipi_dsi_host_ll.h"

#define RIVERDI_PWR_DN_GPIO GPIO_NUM_27
#define RIVERDI_PWM_GPIO    GPIO_NUM_26
#define SN65DSI83_ADDR      0x2D
#define SN65DSI83_ALT_ADDR  0x2C
#define I2C_TIMEOUT_MS     100

/* The default 240 MHz DPI source has an INTEGER divider: requesting 72.4 MHz
 * would actually produce 80 MHz, outside the panel's 66.3..78.9 MHz range.
 * Use APLL at 72 MHz / 1, with an exact 864 Mbps DSI rate (432 MHz clock).
 * Bridge divider 6 then produces 72 MHz LVDS, ~59.67 Hz at 1440 x 838 total.
 * RGB565 stays in PSRAM; the P4 DSI bridge expands it to RGB888 on the wire.
 */
#define RIVERDI_PIXEL_CLOCK_HZ 72000000U
#define RIVERDI_LANE_MBPS      864
#define RIVERDI_HRES           1280
#define RIVERDI_VRES           800
#define RIVERDI_HSYNC          32
#define RIVERDI_HBP            (88 - RIVERDI_HSYNC)
#define RIVERDI_HFP            72
#define RIVERDI_VSYNC          4
#define RIVERDI_VBP            (23 - RIVERDI_VSYNC)
#define RIVERDI_VFP            15

static const char *TAG = "riverdi";

static esp_err_t bridge_read(i2c_master_dev_handle_t dev, uint8_t reg, void *data, size_t size)
{
    return i2c_master_transmit_receive(dev, &reg, 1, data, size, I2C_TIMEOUT_MS);
}

static esp_err_t bridge_write(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t value)
{
    const uint8_t data[] = {reg, value};
    return i2c_master_transmit(dev, data, sizeof(data), I2C_TIMEOUT_MS);
}

/* Preserve reserved bits and verify every configuration write. */
static esp_err_t bridge_update(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t current;
    ESP_RETURN_ON_ERROR(bridge_read(dev, reg, &current, 1), TAG, "read CSR 0x%02x", reg);
    current = (current & ~mask) | (value & mask);
    ESP_RETURN_ON_ERROR(bridge_write(dev, reg, current), TAG, "write CSR 0x%02x", reg);
    ESP_RETURN_ON_ERROR(bridge_read(dev, reg, &current, 1), TAG, "verify CSR 0x%02x", reg);
    ESP_RETURN_ON_FALSE((current & mask) == (value & mask), ESP_ERR_INVALID_RESPONSE,
                       TAG, "CSR 0x%02x readback 0x%02x, expected masked 0x%02x", reg, current, value);
    return ESP_OK;
}

static esp_err_t bridge_configure(i2c_master_dev_handle_t dev, uint8_t address)
{
    /* ID bytes are specified in ascending address order here (TI prints 08..00).
     * These IDs are shared across the DSI8x family; log the revision separately.
     */
    static const uint8_t expected_id[] = {0x35, 0x38, 0x49, 0x53, 0x44, 0x20, 0x20, 0x20};
    uint8_t id[9];
    ESP_RETURN_ON_ERROR(bridge_read(dev, 0x00, id, sizeof(id)), TAG, "SN65DSI83 ID read failed");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, id, sizeof(id), ESP_LOG_INFO);
    ESP_RETURN_ON_FALSE(memcmp(id, expected_id, sizeof(expected_id)) == 0, ESP_ERR_INVALID_RESPONSE,
                       TAG, "Unexpected bridge ID at 0x%02x", address);
    ESP_LOGI(TAG, "SN65DSI83 detected at 0x%02x, revision 0x%02x", address, id[8]);

    ESP_RETURN_ON_ERROR(bridge_update(dev, 0x0D, 0x01, 0), TAG, "Disable PLL");
    ESP_RETURN_ON_ERROR(bridge_update(dev, 0x0A, 0x0F, 0x05), TAG, "LVDS range / DSI clock source");
    ESP_RETURN_ON_ERROR(bridge_update(dev, 0x0B, 0xFB, 0x28), TAG, "DSI clock divider 6");
    ESP_RETURN_ON_ERROR(bridge_update(dev, 0x10, 0x18, 0x10), TAG, "Two DSI lanes");
    ESP_RETURN_ON_ERROR(bridge_update(dev, 0x12, 0xFF, (RIVERDI_LANE_MBPS / 2) / 5), TAG, "DSI clock range");

    /* DE positive, HS/VS negative; preserve reserved bits. Format 2 is the
     * initial LVDS mapping assumption, selectable without changing the driver.
     * Leave LVDS voltage swing, termination and pin order at TI reset defaults.
     */
    uint8_t lvds_format = 0x68;
#if CONFIG_EV_DASH_RIVERDI_LVDS_FORMAT1
    lvds_format |= 0x02;
#endif
    ESP_RETURN_ON_ERROR(bridge_update(dev, 0x18, 0xEA, lvds_format), TAG, "LVDS 24-bit format / polarity");

    static const struct { uint8_t reg, mask, value; } timing[] = {
        {0x20, 0xFF, RIVERDI_HRES & 0xFF}, {0x21, 0x0F, RIVERDI_HRES >> 8},
        {0x24, 0xFF, RIVERDI_VRES & 0xFF}, {0x25, 0x0F, RIVERDI_VRES >> 8},
        {0x28, 0xFF, 33},                 {0x29, 0x0F, 0}, /* >=32 pixel sync delay */
        {0x2C, 0xFF, RIVERDI_HSYNC},      {0x2D, 0x03, 0},
        {0x30, 0xFF, RIVERDI_VSYNC},      {0x31, 0x03, 0},
        {0x34, 0xFF, RIVERDI_HBP},        {0x36, 0xFF, RIVERDI_VBP},
        {0x38, 0xFF, RIVERDI_HFP},        {0x3A, 0xFF, RIVERDI_VFP},
        {0x3C, 0x10, 0}, /* Real DSI video, not the bridge's internal test pattern */
    };
    for (size_t i = 0; i < sizeof(timing) / sizeof(timing[0]); ++i) {
        ESP_RETURN_ON_ERROR(bridge_update(dev, timing[i].reg, timing[i].mask, timing[i].value),
                            TAG, "Bridge timing configuration failed");
    }
#if CONFIG_EV_DASH_RIVERDI_BRIDGE_PATTERN
    ESP_RETURN_ON_ERROR(bridge_update(dev, 0x3C, 0x10, 0x10), TAG, "Enable bridge internal test pattern");
    ESP_LOGW(TAG, "SN65DSI83 INTERNAL PATTERN enabled; LVDS pixels bypass P4/LVGL video data");
#endif

    ESP_RETURN_ON_ERROR(bridge_update(dev, 0x0D, 0x01, 1), TAG, "Enable bridge PLL");
    vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t pll;
    ESP_RETURN_ON_ERROR(bridge_read(dev, 0x0A, &pll, 1), TAG, "Read PLL status");
    ESP_RETURN_ON_FALSE(pll & 0x80, ESP_ERR_TIMEOUT, TAG,
                       "Bridge PLL not locked (0x0A=0x%02x); check continuous DSI clock", pll);
    ESP_RETURN_ON_ERROR(bridge_write(dev, 0x09, 1), TAG, "Bridge soft reset");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "Bridge configuration verified; PLL locked, 2 lanes, RGB888, LVDS Format %d",
             (lvds_format & 2) ? 1 : 2);
    return ESP_OK;
}

lv_display_t *riverdi_display_start(void)
{
    esp_err_t ret = ESP_OK;
    esp_lcd_dsi_bus_handle_t bus = NULL;
    esp_lcd_panel_handle_t panel = NULL;
    esp_ldo_channel_handle_t phy_power = NULL;
    i2c_master_dev_handle_t bridge = NULL;
    bool apll_acquired = false;
    bool dma2d_enabled = false;
    bool lvgl_started = false;
    uint32_t pixel_clock_hz = 0;
    uint8_t bridge_address = SN65DSI83_ADDR;

    const gpio_config_t gpio_cfg = {
        .pin_bit_mask = (1ULL << RIVERDI_PWR_DN_GPIO) | (1ULL << RIVERDI_PWM_GPIO),
        .mode = GPIO_MODE_INPUT_OUTPUT,
    };
    ESP_GOTO_ON_ERROR(gpio_config(&gpio_cfg), fail, TAG, "Display GPIO init failed");
    ESP_GOTO_ON_ERROR(gpio_set_level(RIVERDI_PWM_GPIO, 0), fail, TAG, "Backlight off failed");
    ESP_GOTO_ON_ERROR(gpio_set_level(RIVERDI_PWR_DN_GPIO, 0), fail, TAG, "PWR_DN assert failed");
    ESP_LOGI(TAG, "GPIO27 PWR_DN=%d (asserted LOW); GPIO26 backlight OFF", gpio_get_level(RIVERDI_PWR_DN_GPIO));

    const esp_ldo_channel_config_t ldo_cfg = {.chan_id = 3, .voltage_mv = 2500};
    ESP_GOTO_ON_ERROR(esp_ldo_acquire_channel(&ldo_cfg, &phy_power), fail, TAG, "DSI PHY power failed");
    const esp_lcd_dsi_bus_config_t bus_cfg = {
        .bus_id = 0,
        .num_data_lanes = 2,
        .lane_bit_rate_mbps = RIVERDI_LANE_MBPS,
        .flags.clock_lane_force_hs = true,
    };
    ESP_LOGI(TAG, "MIPI DSI init: 2 lanes, %d Mbps/lane, continuous HS clock", RIVERDI_LANE_MBPS);
    ESP_GOTO_ON_ERROR(esp_lcd_new_dsi_bus(&bus_cfg, &bus), fail, TAG, "DSI bus init failed");

    /* TI sequence: clock HS / data LP11 BEFORE releasing power-down.
     * No DPI panel or DCS IO is started yet, so data lanes remain idle LP11.
     */
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_GOTO_ON_ERROR(gpio_set_level(RIVERDI_PWR_DN_GPIO, 1), fail, TAG, "PWR_DN release failed");
    ESP_LOGI(TAG, "GPIO27 PWR_DN=%d (released HIGH)", gpio_get_level(RIVERDI_PWR_DN_GPIO));
    vTaskDelay(pdMS_TO_TICKS(20));

    ESP_LOGI(TAG, "I2C init: BSP bus, SCL=GPIO%d SDA=GPIO%d, 100 kHz", BSP_I2C_SCL, BSP_I2C_SDA);
    ESP_GOTO_ON_ERROR(bsp_i2c_init(), fail, TAG, "I2C init failed");
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle();
    /* TI ADDR selects 7-bit address 0x2C or 0x2D. This Riverdi module was
     * measured at 0x2C; support both and verify its ID before any CSR writes.
     */
    ret = i2c_master_probe(i2c_bus, bridge_address, I2C_TIMEOUT_MS);
    if (ret == ESP_ERR_NOT_FOUND) {
        bridge_address = SN65DSI83_ALT_ADDR;
        ret = i2c_master_probe(i2c_bus, bridge_address, I2C_TIMEOUT_MS);
    }
    ESP_GOTO_ON_ERROR(ret, fail, TAG,
                      "Bridge probe failed (0x2D/0x2C); P4 levels SCL=%d SDA=%d PWR_DN=%d",
                      gpio_get_level(BSP_I2C_SCL), gpio_get_level(BSP_I2C_SDA),
                      gpio_get_level(RIVERDI_PWR_DN_GPIO));
    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = bridge_address,
        .scl_speed_hz = 100000,
    };
    ESP_GOTO_ON_ERROR(i2c_master_bus_add_device(i2c_bus, &dev_cfg, &bridge), fail, TAG, "I2C bridge attach failed");
    ESP_GOTO_ON_ERROR(bridge_configure(bridge, bridge_address), fail, TAG, "SN65DSI83 configuration failed");

    ESP_GOTO_ON_ERROR(esp_clk_tree_enable_src(SOC_MOD_CLK_APLL, true), fail, TAG, "APLL enable failed");
    apll_acquired = true;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 0)
    ret = esp_clk_tree_src_set_freq_hz(SOC_MOD_CLK_APLL, RIVERDI_PIXEL_CLOCK_HZ, &pixel_clock_hz);
#else
    ret = periph_rtc_apll_freq_set(RIVERDI_PIXEL_CLOCK_HZ, &pixel_clock_hz);
#endif
    ESP_GOTO_ON_ERROR(ret,
                      fail, TAG, "APLL pixel clock configuration failed");
    ESP_GOTO_ON_FALSE(pixel_clock_hz > RIVERDI_PIXEL_CLOCK_HZ - 1000 && pixel_clock_hz < RIVERDI_PIXEL_CLOCK_HZ + 1000,
                      ESP_ERR_INVALID_STATE, fail, TAG, "Unexpected APLL clock: %lu Hz", (unsigned long)pixel_clock_hz);
    const esp_lcd_dpi_panel_config_t dpi_cfg = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_APLL,
        .dpi_clock_freq_mhz = (float)pixel_clock_hz / 1000000.0f,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB888,
        .num_fbs = 2,
        .video_timing = {
            .h_size = RIVERDI_HRES, .v_size = RIVERDI_VRES,
            .hsync_pulse_width = RIVERDI_HSYNC, .hsync_back_porch = RIVERDI_HBP, .hsync_front_porch = RIVERDI_HFP,
            .vsync_pulse_width = RIVERDI_VSYNC, .vsync_back_porch = RIVERDI_VBP, .vsync_front_porch = RIVERDI_VFP,
        },
    };
    ESP_LOGI(TAG, "DPI init: 1280x800, pixel clock %lu Hz, totals 1440x838, RGB565 -> RGB888",
             (unsigned long)pixel_clock_hz);
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_dpi(bus, &dpi_cfg, &panel), fail, TAG, "DPI framebuffer allocation failed");
    /* IDF enables frame BTA ACK by default. The SN65DSI83 has receive-only DSI
     * lanes; do not request a return packet at the end of each frame. There is
     * no public DPI configuration field for this in IDF 6.0/6.1.
     */
    mipi_dsi_host_ll_dpi_enable_frame_ack(MIPI_DSI_LL_GET_HOST(0), false);
    /* The panel driver now holds its own APLL reference. */
    esp_clk_tree_enable_src(SOC_MOD_CLK_APLL, false);
    apll_acquired = false;
    ESP_GOTO_ON_ERROR(esp_lcd_dpi_panel_enable_dma2d(panel), fail, TAG, "DPI DMA2D enable failed");
    dma2d_enabled = true;
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(panel), fail, TAG, "DSI video start failed");
    ESP_LOGI(TAG, "DSI video started; two RGB565 framebuffers (%u bytes each)", RIVERDI_HRES * RIVERDI_VRES * 2);

    /* TI optional post-stream check: discard startup flags, then check again.
     * Allow multiple frames before clearing so link startup has settled.
     */
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_GOTO_ON_ERROR(bridge_write(bridge, 0xE5, 0xFF), fail, TAG, "Clear bridge error flags failed");
    vTaskDelay(pdMS_TO_TICKS(50));
    uint8_t errors;
    ESP_GOTO_ON_ERROR(bridge_read(bridge, 0xE5, &errors, 1), fail, TAG, "Read bridge error flags failed");
    ESP_LOGI(TAG, "SN65DSI83 post-video status 0xE5=0x%02x", errors);
    ESP_GOTO_ON_FALSE(errors == 0, ESP_ERR_INVALID_RESPONSE, fail, TAG,
                      "Bridge video errors: sync=%d CRC=%d ECC=%d/%d LLP=%d SoT=%d PLL=%d",
                      !!(errors & 0x80), !!(errors & 0x40), !!(errors & 0x20), !!(errors & 0x10),
                      !!(errors & 0x08), !!(errors & 0x04), !!(errors & 0x01));

    const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_GOTO_ON_ERROR(lvgl_port_init(&port_cfg), fail, TAG, "LVGL port init failed");
    lvgl_started = true;
    const lvgl_port_display_cfg_t display_cfg = {
        .panel_handle = panel,
        .buffer_size = RIVERDI_HRES * RIVERDI_VRES,
        .double_buffer = true,
        .hres = RIVERDI_HRES, .vres = RIVERDI_VRES,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {.buff_dma = true, .buff_spiram = true, .direct_mode = true},
    };
    const lvgl_port_display_dsi_cfg_t dsi_cfg = {.flags.avoid_tearing = true};
    lv_display_t *display = lvgl_port_add_disp_dsi(&display_cfg, &dsi_cfg);
    ESP_GOTO_ON_FALSE(display, ESP_ERR_NO_MEM, fail, TAG, "LVGL framebuffer/display start failed");
    i2c_master_bus_rm_device(bridge);
    /* Full-on DC is a valid 100% PWM level and avoids the stock BSP's 5 kHz
     * PWM, which exceeds Riverdi's specified 200..1000 Hz dimming range.
     */
    gpio_set_level(RIVERDI_PWM_GPIO, 1);
    ESP_LOGI(TAG, "Framebuffer/LVGL display started; GPIO26 PWM=HIGH (100%% backlight)");
    return display;

fail:
    ESP_LOGE(TAG, "Riverdi display start failed: %s; backlight OFF, PWR_DN LOW", esp_err_to_name(ret));
    gpio_set_level(RIVERDI_PWM_GPIO, 0);
    gpio_set_level(RIVERDI_PWR_DN_GPIO, 0);
    if (lvgl_started) lvgl_port_deinit();
    if (panel) {
        if (dma2d_enabled) esp_lcd_dpi_panel_disable_dma2d(panel);
        esp_lcd_panel_del(panel);
    }
    if (apll_acquired) esp_clk_tree_enable_src(SOC_MOD_CLK_APLL, false);
    if (bus) esp_lcd_del_dsi_bus(bus);
    if (phy_power) esp_ldo_release_channel(phy_power);
    if (bridge) i2c_master_bus_rm_device(bridge);
    return NULL;
}

#endif /* CONFIG_EV_DASH_DISPLAY_RIVERDI */
