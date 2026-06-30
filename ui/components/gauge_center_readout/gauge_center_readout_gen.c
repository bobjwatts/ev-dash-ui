/**
 * @file gauge_center_readout_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "gauge_center_readout_gen.h"
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

lv_obj_t * gauge_center_readout_create(lv_obj_t * parent, lv_subject_t * value, const char * unit, const char * fmt, int32_t value_y, int32_t unit_y)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t value_text;
    static lv_style_t unit_text;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&value_text);
        lv_style_set_text_font(&value_text, font_display);
        lv_style_set_text_color(&value_text, COLOR_TEXT_HI);

        lv_style_init(&unit_text);
        lv_style_set_text_font(&unit_text, font_body);
        lv_style_set_text_color(&unit_text, COLOR_TEXT_MID);

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(parent);
    lv_obj_set_name_static(lv_obj_0, "gauge_center_readout_#");
    lv_obj_set_width(lv_obj_0, 200);
    lv_obj_set_height(lv_obj_0, 100);
    lv_obj_set_style_bg_opa(lv_obj_0, 0, 0);
    lv_obj_set_style_border_width(lv_obj_0, 0, 0);
    lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

    lv_obj_t * lv_label_0 = lv_label_create(lv_obj_0);
    lv_label_bind_text(lv_label_0, value, fmt);
    lv_obj_set_align(lv_label_0, LV_ALIGN_CENTER);
    lv_obj_set_y(lv_label_0, value_y);
    lv_obj_add_style(lv_label_0, &value_text, 0);

    lv_obj_t * lv_label_1 = lv_label_create(lv_obj_0);
    lv_label_set_text(lv_label_1, unit);
    lv_obj_set_align(lv_label_1, LV_ALIGN_CENTER);
    lv_obj_set_y(lv_label_1, unit_y);
    lv_obj_add_style(lv_label_1, &unit_text, 0);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

