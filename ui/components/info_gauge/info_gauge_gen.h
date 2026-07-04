/**
 * @file info_gauge_gen.h
 * @brief Right-hand info gauge shell (264×264) — placeholder for warning lights / status cluster.
 */

#ifndef INFO_GAUGE_GEN_H
#define INFO_GAUGE_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * Create the empty info gauge and attach it to @p parent.
 * Uses the same small-dial face asset as the power gauge; content TBD.
 */
lv_obj_t * info_gauge_create(lv_obj_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* INFO_GAUGE_GEN_H */
