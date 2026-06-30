/**
 * @file speed_scale_ring_fine_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "speed_scale_ring_fine_gen.h"
#include "../../ev_dash.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * speed_scale_ring_fine_create(lv_obj_t * parent, lv_subject_t * value)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t scale_ticks;
    static lv_style_t scale_ticks_dark;
    static lv_style_t scale_section_ticks;
    static lv_style_t scale_main;
    static lv_style_t scale_main_dark;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&scale_ticks);
        lv_style_set_line_color(&scale_ticks, COLOR_WHITE);
        lv_style_set_line_width(&scale_ticks, 2);
        lv_style_set_length(&scale_ticks, 11);

        lv_style_init(&scale_ticks_dark);
        lv_style_set_line_color(&scale_ticks_dark, COLOR_GREEN);
        lv_style_set_line_width(&scale_ticks_dark, 2);
        lv_style_set_length(&scale_ticks_dark, 11);

        lv_style_init(&scale_section_ticks);
        lv_style_set_line_color(&scale_section_ticks, COLOR_GAUGE_ACTIVE);
        lv_style_set_line_width(&scale_section_ticks, 2);
        lv_style_set_length(&scale_section_ticks, 7);

        lv_style_init(&scale_main);
        lv_style_set_width(&scale_main, 141);
        lv_style_set_height(&scale_main, 141);
        lv_style_set_arc_width(&scale_main, 0);
        lv_style_set_text_font(&scale_main, font_small);
        lv_style_set_text_color(&scale_main, COLOR_GREEN);

        lv_style_init(&scale_main_dark);
        lv_style_set_text_color(&scale_main_dark, COLOR_GREEN_DARK);
        lv_style_set_text_font(&scale_main_dark, font_small);

        style_inited = true;
    }

    lv_obj_t * lv_scale_0 = lv_scale_create(parent);
    lv_obj_set_name_static(lv_scale_0, "speed_scale_ring_fine_#");
    lv_scale_set_major_tick_every(lv_scale_0, 0);
    lv_scale_set_mode(lv_scale_0, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_angle_range(lv_scale_0, 220);
    lv_scale_set_min_value(lv_scale_0, 0);
    lv_scale_set_max_value(lv_scale_0, 200);
    lv_scale_set_rotation(lv_scale_0, -200);
    lv_obj_set_width(lv_scale_0, 380);
    lv_obj_set_height(lv_scale_0, 380);
    lv_obj_set_style_arc_width(lv_scale_0, 1, 0);
    lv_obj_set_style_radius(lv_scale_0, 42, 0);
    lv_obj_set_style_align(lv_scale_0, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_radial_offset(lv_scale_0, 32, 0);
    lv_obj_set_style_text_color(lv_scale_0, COLOR_GAUGE_ACTIVE, 0);
    lv_obj_set_style_text_align(lv_scale_0, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_line_dash_width(lv_scale_0, 200, 0);
    lv_scale_set_draw_ticks_on_top(lv_scale_0, true);
    lv_scale_set_total_tick_count(lv_scale_0, 120);
    lv_obj_set_flag(lv_scale_0, LV_OBJ_FLAG_SCROLLABLE, false);

    lv_obj_add_style(lv_scale_0, &scale_main, 0);
    lv_obj_bind_style(lv_scale_0, &scale_main_dark, 0, &dark_theme, 1);
    lv_obj_add_style(lv_scale_0, &scale_ticks, LV_PART_ITEMS);
    lv_obj_add_style(lv_scale_0, &scale_ticks, LV_PART_INDICATOR);
    lv_obj_bind_style(lv_scale_0, &scale_ticks_dark, LV_PART_ITEMS, &dark_theme, 1);
    lv_obj_bind_style(lv_scale_0, &scale_ticks_dark, LV_PART_INDICATOR, &dark_theme, 1);
    lv_scale_section_t * lv_scale_section_0 = lv_scale_add_section(lv_scale_0);
    lv_scale_set_section_min_value(lv_scale_0, lv_scale_section_0, 0);
    lv_scale_bind_section_max_value(lv_scale_0, lv_scale_section_0, value);
    lv_scale_set_section_style_items(lv_scale_0, lv_scale_section_0, &scale_section_ticks);
    lv_scale_set_section_style_indicator(lv_scale_0, lv_scale_section_0, &scale_section_ticks);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_scale_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

