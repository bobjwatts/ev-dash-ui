/**
 * @file screen_main_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_main_gen.h"
#include "../ev_dash.h"

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

lv_obj_t * screen_main_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    static bool style_inited = false;

    if (!style_inited) {

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
    lv_obj_set_name_static(lv_obj_0, "screen_main_#");
    lv_obj_set_width(lv_obj_0, 1024);
    lv_obj_set_height(lv_obj_0, 600);
    lv_obj_set_flex_flow(lv_obj_0, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_main_place(lv_obj_0, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(lv_obj_0, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_all(lv_obj_0, 0, 0);
    lv_obj_set_style_bg_color(lv_obj_0, COLOR_BG, 0);
    lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

    /* Full-screen background image — rendered first so it sits below all widgets. */
    lv_obj_t * bg_img = lv_image_create(lv_obj_0);
    lv_image_set_src(bg_img, background);
    lv_obj_set_size(bg_img, 1024, 600);
    lv_obj_set_pos(bg_img, 0, 0);
    lv_obj_remove_flag(bg_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(bg_img, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_t * power_gauge_0 = power_gauge_create(lv_obj_0, &power_kw, &power_needle_angle,
                                                    &odometer_km, &motor_temp_c);
    lv_obj_set_style_align(power_gauge_0, LV_ALIGN_CENTER, 0);

    lv_obj_t * speedometer_0 = speedometer_create(lv_obj_0, &speed_kmh, &speed_needle_angle);
    lv_obj_set_style_align(speedometer_0, LV_ALIGN_CENTER, 0);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

