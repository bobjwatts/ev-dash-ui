/**
 * @file lvgl_compat.c
 * @brief LVGL 9.5 API shims for building against LVGL 9.4.
 */

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#  include "lvgl.h"
#else
#  include "lvgl/lvgl.h"
#endif

#include "lvgl_compat.h"

#if LV_USE_OBSERVER

/* Context stored as observer user_data. Heap-allocated once per binding;
 * for this firmware objects live for the lifetime of the screen so the
 * allocation is effectively static. */
typedef struct {
    lv_style_prop_t     prop;
    lv_style_selector_t selector;
} style_prop_ctx_t;

static void style_prop_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(!obj) return;

    style_prop_ctx_t * ctx = lv_observer_get_user_data(observer);

    lv_style_value_t val;
    val.num = lv_subject_get_int(subject);

    lv_obj_set_local_style_prop(obj, ctx->prop, val, ctx->selector);
    lv_obj_invalidate(obj);
}

lv_observer_t * lv_obj_bind_style_prop(lv_obj_t * obj,
                                        lv_style_prop_t prop,
                                        lv_style_selector_t selector,
                                        lv_subject_t * subject)
{
    style_prop_ctx_t * ctx = lv_malloc(sizeof(style_prop_ctx_t));
    LV_ASSERT_MALLOC(ctx);
    if(!ctx) return NULL;

    ctx->prop     = prop;
    ctx->selector = selector;

    /* Apply initial value immediately so the widget starts in the right state. */
    lv_style_value_t val;
    val.num = lv_subject_get_int(subject);
    lv_obj_set_local_style_prop(obj, prop, val, selector);

    return lv_subject_add_observer_obj(subject, style_prop_observer_cb, obj, ctx);
}

#endif /* LV_USE_OBSERVER */
