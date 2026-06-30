/**
 * @file gauge_center_readout_gen.h
 */

#ifndef GAUGE_CENTER_READOUT_H
#define GAUGE_CENTER_READOUT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
    #include "lvgl_private.h"
#else
    #include "lvgl/lvgl.h"
    #include "lvgl/lvgl_private.h"
#endif

#ifdef LV_USE_XML
    #include "lv_xml/lv_xml.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t * gauge_center_readout_create(lv_obj_t * parent, lv_subject_t * value, const char * unit, const char * fmt, int32_t value_y, int32_t unit_y);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*GAUGE_CENTER_READOUT_H*/