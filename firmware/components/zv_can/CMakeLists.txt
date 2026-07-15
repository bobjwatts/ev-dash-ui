idf_component_register(
    SRCS "zv_can.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_twai ev_dash_ui esp_timer freertos lvgl__lvgl)
