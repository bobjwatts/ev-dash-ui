/**
 * @file speedometer_gen.c
 * @brief Speedometer widget — dial image, active arc, tick-mask overlay,
 *        manual speed labels, needle, centre readout, and SOC gauge.
 */

/*********************
 *      INCLUDES
 *********************/

#include "speedometer_gen.h"
#include "../../ev_dash.h"
#include "../../user_code/speedometer_assets.h"
#include "../soc_gauge/soc_gauge_gen.h"
#include <math.h>

/*********************
 *      DEFINES
 *********************/

/* Speed scale labels — placed manually at each 20 km/h mark.
 * Radius is measured from the centre of the 439×439 speedometer container.
 * 150px sits inside the active-arc ring (outer edge ≈ 189px).       */
#define SPEED_LABEL_RADIUS     140
#define SPEED_LABEL_ARC_START  161   /* degrees at speed=0  (LVGL: 0°=3 o'clock, CW) */
#define SPEED_LABEL_ARC_SWEEP  218   /* total sweep 161°→379°(=19°) over 0–200 km/h  */
#define SPEED_LABEL_COUNT      11

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

static void active_arc_speed_cb(lv_observer_t * observer, lv_subject_t * subject);
static void speed_label_cb(lv_observer_t * observer, lv_subject_t * subject);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * speedometer_create(lv_obj_t * parent, lv_subject_t * speed, lv_subject_t * needle_angle)
{
    LV_TRACE_OBJ_CREATE("begin");

    lv_obj_t * lv_obj_0 = lv_obj_create(parent);
    lv_obj_set_name_static(lv_obj_0, "speedometer_#");
    lv_obj_set_width(lv_obj_0, 439);
    lv_obj_set_height(lv_obj_0, 439);
    lv_obj_set_style_bg_opa(lv_obj_0, 0, 0);
    lv_obj_set_style_border_width(lv_obj_0, 0, 0);
    lv_obj_set_style_pad_all(lv_obj_0, 0, 0);
    lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_add_flag(lv_obj_0, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    lv_obj_t * lv_image_0 = lv_image_create(lv_obj_0);
    lv_image_set_src(lv_image_0, dial_speed_dial);
    speedometer_dial_image_set(lv_image_0, (const lv_image_dsc_t *)dial_speed_dial);
    lv_obj_set_align(lv_image_0, LV_ALIGN_TOP_LEFT);

    /* Scale rings disabled: lv_scale redraws all 120+ ticks on every speed change
     * = entire ring dirty area each frame = 15 fps. Static ticks baked into dial PNG instead.
     * Active zone replaced with a cheap lv_arc that only updates its end-angle. */
    // lv_obj_t * speed_scale_ring_0 = speed_scale_ring_create(lv_obj_0, speed);
    // lv_obj_set_align(speed_scale_ring_0, LV_ALIGN_CENTER);
    // lv_obj_t * speed_scale_ring_fine_0 = speed_scale_ring_fine_create(lv_obj_0, speed);
    // lv_obj_set_align(speed_scale_ring_fine_0, LV_ALIGN_CENTER);

    /* Active-zone arc — grows from speed=0 (161°) to speed=200 (19°) clockwise,
     * matching the needle sweep of ±109° from top (SPEED_ANGLE_MIN/MAX_DECIDEG).
     * Size 379×379; 30 px width fills the ring band. */
    lv_obj_t * active_arc = lv_arc_create(lv_obj_0);
    lv_obj_set_size(active_arc, 379, 379);
    lv_obj_align(active_arc, LV_ALIGN_CENTER, 1, 1);
    lv_obj_remove_flag(active_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(active_arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_arc_width(active_arc, 25, LV_PART_MAIN);
    lv_obj_set_style_arc_color(active_arc, COLOR_GREY_DARK, LV_PART_MAIN);
    lv_obj_set_style_arc_width(active_arc, 25, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(active_arc, COLOR_GAUGE_ACTIVE, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(active_arc, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(active_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_outline_width(active_arc, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(active_arc, 0, LV_PART_KNOB);
    lv_arc_set_bg_angles(active_arc, 157, 23);
    lv_arc_set_range(active_arc, 0, 200);
    lv_arc_set_value(active_arc, 0);
    lv_subject_add_observer_obj(speed, active_arc_speed_cb, active_arc, NULL);

    /* Tick overlay mask — pixel data served from PSRAM (cached at ev_dash_init). */
    lv_obj_t * arc_mask_img = lv_image_create(lv_obj_0);
    lv_image_set_src(arc_mask_img, dial_speed_arc_mask);
    speedometer_image_set_1to1(arc_mask_img, (const lv_image_dsc_t *)dial_speed_arc_mask, "arc_mask");
    lv_obj_align(arc_mask_img, LV_ALIGN_CENTER, 1, 1);

    /* Speed scale labels — manually positioned at each 20 km/h mark.
     * Colour is updated by speed_label_cb; only the label crossing the threshold
     * is redrawn each frame (~20×14 px dirty region, vs 380×380 for lv_scale). */
    static const int speed_label_vals[SPEED_LABEL_COUNT] = {
        0, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200
    };
    for (int i = 0; i < SPEED_LABEL_COUNT; i++) {
        int v = speed_label_vals[i];
        float angle_rad = ((float)(SPEED_LABEL_ARC_START) +
                           ((float)v / 200.0f) * (float)SPEED_LABEL_ARC_SWEEP)
                          * (3.14159265f / 180.0f);
        int32_t x_ofs = (int32_t)(cosf(angle_rad) * (float)SPEED_LABEL_RADIUS);
        int32_t y_ofs = (int32_t)(sinf(angle_rad) * (float)SPEED_LABEL_RADIUS);

        lv_obj_t * lbl = lv_label_create(lv_obj_0);
        char buf[5];
        lv_snprintf(buf, sizeof(buf), "%d", v);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_font(lbl, font_heading, 0);
        lv_obj_set_style_text_color(lbl, COLOR_GREY_DARK, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, x_ofs, y_ofs);
        /* user_data: bits[N:1] = threshold speed, bit[0] = current active state */
        lv_obj_set_user_data(lbl, (void *)(uintptr_t)(v << 1 | 0));
        lv_subject_add_observer_obj(speed, speed_label_cb, lbl, NULL);
    }

    lv_obj_t * lv_image_1 = lv_image_create(lv_obj_0);
    lv_image_set_src(lv_image_1, dial_speed_needle);
    speedometer_image_set_1to1(lv_image_1, (const lv_image_dsc_t *)dial_speed_needle, "needle");
    lv_obj_set_align(lv_image_1, LV_ALIGN_TOP_MID);
    /* Place needle so the rotation pivot (SPEEDOMETER_NEEDLE_ARM_PX below the image top)
     * lands on the container centre (439/2 = 219 px).
     * The pivot may lie outside the image when bottom whitespace has been trimmed. */
    lv_obj_set_y(lv_image_1, 439 / 2 - SPEEDOMETER_NEEDLE_ARM_PX);
    speedometer_needle_set_pivot(lv_image_1, (const lv_image_dsc_t *)dial_speed_needle);
    lv_obj_set_style_bg_opa(lv_image_1, 0, 0);
    lv_obj_bind_style_prop(lv_image_1, LV_STYLE_TRANSFORM_ROTATION, 0, needle_angle);

#if !SPEEDOMETER_DEBUG_HIDE_READOUT
    lv_obj_t * gauge_center_readout_0 = gauge_center_readout_create(lv_obj_0, speed, "km/h", "%d", -11, 30);
    lv_obj_set_align(gauge_center_readout_0, LV_ALIGN_CENTER);
#endif

    soc_gauge_create(lv_obj_0, &state_of_charge_pct, &range_est_km);

#if SPEEDOMETER_DEBUG_SIZE_RINGS
    speedometer_debug_overlay_rings(lv_obj_0);
#endif

    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void active_arc_speed_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * arc = (lv_obj_t *)lv_observer_get_target_obj(observer);
    lv_arc_set_value(arc, lv_subject_get_int(subject));
}

static void speed_label_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * lbl    = (lv_obj_t *)lv_observer_get_target_obj(observer);
    uintptr_t data    = (uintptr_t)lv_obj_get_user_data(lbl);
    int  threshold    = (int)(data >> 1);
    bool was_active   = (data & 1u) != 0;
    bool is_active    = lv_subject_get_int(subject) >= threshold;
    if(was_active == is_active) return;   /* state unchanged — skip redraw entirely */
    lv_obj_set_user_data(lbl, (void *)((uintptr_t)(threshold << 1) | (uintptr_t)is_active));
    lv_obj_set_style_text_color(lbl, is_active ? COLOR_GAUGE_ACTIVE : COLOR_GREY_DARK, 0);
}
