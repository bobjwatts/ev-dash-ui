/**
 * @file power_gauge_gen.h
 * @brief Power gauge dial (264×264): motor power kW + regen display.
 */

#ifndef POWER_GAUGE_GEN_H
#define POWER_GAUGE_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

lv_obj_t * power_gauge_create(lv_obj_t * parent,
                               lv_subject_t * power_kw,
                               lv_subject_t * needle_angle,
                               lv_subject_t * odometer_km,
                               lv_subject_t * motor_temp_c);

#ifdef __cplusplus
}
#endif

#endif /* POWER_GAUGE_GEN_H */
