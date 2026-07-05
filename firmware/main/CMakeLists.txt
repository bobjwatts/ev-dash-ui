idf_component_register(
    SRCS "main.c" "dashboard_buttons.c"
    INCLUDE_DIRS "."
    REQUIRES ev_dash_ui esp32_p4_function_ev_board esp_timer esp_driver_gpio lvgl__lvgl)
