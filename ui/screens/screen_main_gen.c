/**
 * @file screen_main_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_main_gen.h"
#include "../ev_dash.h"
#include "../user_code/speedometer_assets.h"

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
#if !SPEEDOMETER_DEBUG_SIZE_RINGS
    lv_obj_set_flex_flow(lv_obj_0, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_main_place(lv_obj_0, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(lv_obj_0, LV_FLEX_ALIGN_CENTER, 0);
#endif
    lv_obj_set_style_pad_all(lv_obj_0, 0, 0);
    lv_obj_set_style_bg_color(lv_obj_0, COLOR_BG, 0);
    lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

    lv_obj_t * speedometer_0 = speedometer_create(lv_obj_0, &speed_kmh, &speed_needle_angle);
#if SPEEDOMETER_DEBUG_SIMPLE_CENTER
    lv_obj_set_align(speedometer_0, LV_ALIGN_CENTER);
#elif SPEEDOMETER_DEBUG_SIZE_RINGS
    speedometer_debug_layout_screen(lv_obj_0, speedometer_0);
#else
    lv_obj_set_style_align(speedometer_0, LV_ALIGN_CENTER, 0);
#endif

    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

