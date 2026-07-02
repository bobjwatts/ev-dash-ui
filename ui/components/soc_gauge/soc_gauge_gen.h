/**
 * @file soc_gauge_gen.h
 * @brief State-of-charge gauge: bottom arc + range readout inside the speedometer dial.
 */

#ifndef SOC_GAUGE_GEN_H
#define SOC_GAUGE_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

lv_obj_t * soc_gauge_create(lv_obj_t * parent,
                             lv_subject_t * soc_pct,
                             lv_subject_t * range_km);

#ifdef __cplusplus
}
#endif

#endif /* SOC_GAUGE_GEN_H */
