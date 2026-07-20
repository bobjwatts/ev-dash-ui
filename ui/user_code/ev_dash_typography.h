/**
 * @file ev_dash_typography.h
 * @brief Letter spacing helpers for Big Shoulders text styles.
 */
#ifndef EV_DASH_TYPOGRAPHY_H
#define EV_DASH_TYPOGRAPHY_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Letter spacing (px) for a registered dash font — 10% of nominal size. */
int32_t ev_dash_letter_space_for_font(const lv_font_t * font);

/** Apply 10% letter spacing to all labels under @p root (recursive). */
void ev_dash_apply_letter_spacing(lv_obj_t * root);

#ifdef __cplusplus
}
#endif

#endif /* EV_DASH_TYPOGRAPHY_H */
