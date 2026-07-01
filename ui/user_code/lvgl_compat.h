/**
 * @file lvgl_compat.h
 * @brief LVGL 9.5 API shims for building against LVGL 9.4.
 *
 * The LVGL Pro Editor code-generator targets the 9.5 API. These thin wrappers
 * implement the missing symbols using the 9.4 observer / style API so the
 * generated .c files compile unmodified.
 */

#ifndef LVGL_COMPAT_H
#define LVGL_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#  include "lvgl.h"
#else
#  include "lvgl/lvgl.h"
#endif

#if LV_USE_OBSERVER

/**
 * Bind an integer Subject to a local style property of a Widget.
 * Mirrors LVGL 9.5's lv_obj_bind_style_prop().
 *
 * Each time the Subject's value changes, the Widget's local style property
 * is updated with that integer value and the Widget is invalidated.
 * The binding is automatically removed when the Widget is deleted.
 *
 * @param obj       Widget to bind to
 * @param prop      Style property (e.g. LV_STYLE_TRANSFORM_ROTATION)
 * @param selector  Style selector (0 for default state)
 * @param subject   Integer Subject to observe
 * @return          Pointer to the created Observer (managed by the Widget)
 */
lv_observer_t * lv_obj_bind_style_prop(lv_obj_t * obj,
                                        lv_style_prop_t prop,
                                        lv_style_selector_t selector,
                                        lv_subject_t * subject);

#endif /* LV_USE_OBSERVER */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LVGL_COMPAT_H */
