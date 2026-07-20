set(UI_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../ui)

set(EV_DASH_UI_SOURCES
    ${UI_DIR}/components/gauge_center_readout/gauge_center_readout_gen.c
    ${UI_DIR}/components/power_gauge/power_gauge_gen.c
    ${UI_DIR}/components/soc_gauge/soc_gauge_gen.c
    ${UI_DIR}/components/speed_scale_ring/speed_scale_ring_fine_gen.c
    ${UI_DIR}/components/speed_scale_ring/speed_scale_ring_gen.c
    ${UI_DIR}/components/speedometer/speedometer_gen.c
    ${UI_DIR}/ev_dash_gen.c
    ${UI_DIR}/ev_dash.c
    ${UI_DIR}/fonts/font_body_data.c
    ${UI_DIR}/fonts/font_display_data.c
    ${UI_DIR}/fonts/font_heading_data.c
    ${UI_DIR}/fonts/font_subhead_data.c
    ${UI_DIR}/fonts/font_small_data.c
    ${UI_DIR}/images/background_data.c
    ${UI_DIR}/images/dial_speed_arc_mask_data.c
    ${UI_DIR}/images/dial_speed_dial_data.c
    ${UI_DIR}/images/dial_speed_needle_data.c
    ${UI_DIR}/images/small_dial_face_data.c
    ${UI_DIR}/images/small_dial_arc_mask_data.c
    ${UI_DIR}/images/small_dial_needle_green_data.c
    ${UI_DIR}/images/small_dial_needle_yellow_data.c
    ${UI_DIR}/screens/screen_main_gen.c
    ${UI_DIR}/components/info_gauge/info_gauge_gen.c
    ${UI_DIR}/components/top_bar/top_bar_gen.c
    ${UI_DIR}/user_code/ev_dash_typography.c
    ${UI_DIR}/user_code/bms_cell_health.c
    ${UI_DIR}/user_code/blinker.c
    ${UI_DIR}/user_code/lvgl_compat.c
    ${UI_DIR}/user_code/power_gauge_needle.c
    ${UI_DIR}/user_code/speedometer_assets.c
    ${UI_DIR}/user_code/speedometer_needle.c)

idf_component_register(
    SRCS ${EV_DASH_UI_SOURCES}
    INCLUDE_DIRS ${UI_DIR} ${UI_DIR}/user_code
    REQUIRES lvgl__lvgl)
