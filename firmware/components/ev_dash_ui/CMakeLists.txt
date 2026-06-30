set(UI_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../ui)

set(EV_DASH_UI_SOURCES
    ${UI_DIR}/components/gauge_center_readout/gauge_center_readout_gen.c
    ${UI_DIR}/components/speed_scale_ring/speed_scale_ring_fine_gen.c
    ${UI_DIR}/components/speed_scale_ring/speed_scale_ring_gen.c
    ${UI_DIR}/components/speedometer/speedometer_gen.c
    ${UI_DIR}/ev_dash_gen.c
    ${UI_DIR}/ev_dash.c
    ${UI_DIR}/fonts/font_body_data.c
    ${UI_DIR}/fonts/font_display_data.c
    ${UI_DIR}/fonts/font_heading_data.c
    ${UI_DIR}/fonts/font_small_data.c
    ${UI_DIR}/images/dial_speed_dial_data.c
    ${UI_DIR}/images/dial_speed_needle_data.c
    ${UI_DIR}/images/debug_img_100_data.c
    ${UI_DIR}/images/debug_img_439_data.c
    ${UI_DIR}/images/debug_img_439_rgb565_data.c
    ${UI_DIR}/screens/screen_main_gen.c
    ${UI_DIR}/user_code/speedometer_assets.c
    ${UI_DIR}/user_code/speedometer_needle.c)

idf_component_register(
    SRCS ${EV_DASH_UI_SOURCES}
    INCLUDE_DIRS ${UI_DIR} ${UI_DIR}/user_code
    REQUIRES lvgl__lvgl)
