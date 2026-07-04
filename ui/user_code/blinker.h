/**
 * @file blinker.h
 * @brief Turn-signal blinker flash driver.
 *
 * Call blinker_init() once after creating the two indicator labels.
 * The internal 500 ms timer then flashes whichever blinker has its
 * subject set to non-zero by the firmware.
 */

#ifndef BLINKER_H
#define BLINKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../ev_dash_gen.h"

/**
 * Attach a 500 ms repeating timer that flashes @p left and @p right.
 * @param left   lv_obj_t label showing the left-turn arrow
 * @param right  lv_obj_t label showing the right-turn arrow
 */
void blinker_init(lv_obj_t * left, lv_obj_t * right);

#ifdef __cplusplus
}
#endif

#endif /* BLINKER_H */
