/**
 * @file info_gauge_gen.c
 * @brief Right-hand info gauge shell — dial face only, ready for warning lights / temps.
 */

/*********************
 *      INCLUDES
 *********************/

#include "info_gauge_gen.h"
#include "../../ev_dash.h"

/*********************
 *      DEFINES
 *********************/

#define INFO_GAUGE_W  264
#define INFO_GAUGE_H  264

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * info_gauge_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_set_name_static(cont, "info_gauge_#");
    lv_obj_set_size(cont, INFO_GAUGE_W, INFO_GAUGE_H);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_flag(cont, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    /* Dial face — same asset as power gauge; pixel data in PSRAM at ev_dash_init. */
    lv_obj_t * face_img = lv_image_create(cont);
    lv_image_set_src(face_img, small_dial_face);
    lv_image_set_scale(face_img, LV_SCALE_NONE);
    lv_obj_set_align(face_img, LV_ALIGN_TOP_LEFT);

    LV_TRACE_OBJ_CREATE("finished");

    return cont;
}
