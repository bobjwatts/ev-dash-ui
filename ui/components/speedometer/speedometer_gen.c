/**
 * @file speedometer_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "speedometer_gen.h"
#include "../../ev_dash.h"
#include "../../user_code/speedometer_assets.h"

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

lv_obj_t * speedometer_create(lv_obj_t * parent, lv_subject_t * speed, lv_subject_t * needle_angle)
{
    LV_TRACE_OBJ_CREATE("begin");

    lv_obj_t * lv_obj_0 = lv_obj_create(parent);
    lv_obj_set_name_static(lv_obj_0, "speedometer_#");
    lv_obj_set_width(lv_obj_0, 439);
    lv_obj_set_height(lv_obj_0, 439);
    lv_obj_set_style_bg_opa(lv_obj_0, 0, 0);
    lv_obj_set_style_border_width(lv_obj_0, 0, 0);
    lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_add_flag(lv_obj_0, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    lv_obj_t * lv_image_0 = lv_image_create(lv_obj_0);
#if defined(SPEEDOMETER_DEBUG_DIAL_SOLID) && SPEEDOMETER_DEBUG_DIAL_SOLID && defined(EV_DASH_DEBUG_DRAW_TEST) && EV_DASH_DEBUG_DRAW_TEST
    extern const lv_image_dsc_t debug_img_439_rgb565_data;
    lv_image_set_src(lv_image_0, &debug_img_439_rgb565_data);
    speedometer_dial_image_set(lv_image_0, &debug_img_439_rgb565_data);
#else
    lv_image_set_src(lv_image_0, dial_speed_dial);
    speedometer_dial_image_set(lv_image_0, (const lv_image_dsc_t *)dial_speed_dial);
#endif
    lv_obj_set_align(lv_image_0, LV_ALIGN_TOP_LEFT);

    lv_obj_t * speed_scale_ring_0 = speed_scale_ring_create(lv_obj_0, speed);
    lv_obj_set_align(speed_scale_ring_0, LV_ALIGN_CENTER);

    lv_obj_t * speed_scale_ring_fine_0 = speed_scale_ring_fine_create(lv_obj_0, speed);
    lv_obj_set_align(speed_scale_ring_fine_0, LV_ALIGN_CENTER);

    lv_obj_t * lv_image_1 = lv_image_create(lv_obj_0);
    lv_image_set_src(lv_image_1, dial_speed_needle);
    speedometer_image_set_1to1(lv_image_1, (const lv_image_dsc_t *)dial_speed_needle, "needle");
    lv_obj_set_align(lv_image_1, LV_ALIGN_TOP_MID);
    lv_obj_set_y(lv_image_1, 35);
    speedometer_needle_set_pivot(lv_image_1, (const lv_image_dsc_t *)dial_speed_needle);
    lv_obj_set_style_bg_opa(lv_image_1, 0, 0);
    lv_obj_bind_style_prop(lv_image_1, LV_STYLE_TRANSFORM_ROTATION, 0, needle_angle);

#ifndef SPEEDOMETER_DEBUG_HIDE_READOUT
    lv_obj_t * gauge_center_readout_0 = gauge_center_readout_create(lv_obj_0, speed, "km/h", "%d", -11, 30);
    lv_obj_set_align(gauge_center_readout_0, LV_ALIGN_CENTER);
#endif

#if SPEEDOMETER_DEBUG_SIZE_RINGS
    speedometer_debug_overlay_rings(lv_obj_0);
#endif

    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
