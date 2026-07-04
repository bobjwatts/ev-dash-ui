/**
 * @file top_bar_gen.h
 * @brief Top status bar: clock, P/R/N/D gear selector, READY/sys_state indicator.
 */

#ifndef TOP_BAR_GEN_H
#define TOP_BAR_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ev_dash_gen.h"

/**
 * Create the top bar and attach it to @p parent.
 * Reads globals: hour, minute, gear, sys_state.
 */
lv_obj_t * top_bar_create(lv_obj_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* TOP_BAR_GEN_H */
