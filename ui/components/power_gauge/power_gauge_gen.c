/**
 * @file power_gauge_gen.c
 * @brief Power gauge — 264×264 px dial to the left of the speedometer.
 *
 * Layers (bottom → top):
 *   1. small_dial_speed_face.png  (264×263 background image)
 *   2. White static arc           (bg_angles 120°→60°, 300° sweep – inactive track)
 *   3. Yellow active arc          (bg_angles 270°→60°, fills CW when power_kw > 0)
 *   4. Green active arc           (bg_angles 120°→270°, REVERSE, fills CCW when power_kw < 0)
 *   5. small_dial_speed_arc_mask.png  (214×214 tick-gap overlay)
 *   6. Scale labels               (−160, −80, 0, +80, +160 kW)
 *   7. Needle                     (10×68 px, green/yellow, pivot at image bottom = dial centre)
 *   8. Centre readout             (power kW large, odometer km small)
 *   9. Temperature arc            (motor_temp_c, bottom gap, colour-coded)
 *
 * Arc geometry (LVGL: 0° = 3 o'clock, increasing CW):
 *   Arc range: 120° (−160 kW, lower-left) → 270° (0 kW, top) → 60° (+160 kW, lower-right)
 *   Total sweep: 300° over ±160 kW = 0.9375°/kW
 *
 * Needle geometry:
 *   ARM = 68 px (full image height, no transparent whitespace below pivot)
 *   Container centre = 132 px → needle top-y = 64 px, pivot = (5, 68)
 */

/*********************
 *      INCLUDES
 *********************/

#include "power_gauge_gen.h"
#include "../../ev_dash.h"
#include <math.h>

/*********************
 *      DEFINES
 *********************/

/* Container */
#define POWER_GAUGE_W             264
#define POWER_GAUGE_H             264
#define POWER_GAUGE_CENTER        132   /* POWER_GAUGE_W / 2 */

/* Arc */
#define POWER_ARC_SIZE            210   /* slightly inside the 214 px mask */
#define POWER_ARC_WIDTH           15
#define POWER_ARC_BG_START        120   /* degrees at kW_min (−160 kW) */
#define POWER_ARC_BG_END          60    /* degrees at kW_max (+160 kW)  */

/* Needle */
#define POWER_GAUGE_NEEDLE_W      10
#define POWER_GAUGE_NEEDLE_ARM_PX 68    /* image height = tip-to-pivot = no whitespace below */

/* Scale labels — radius from container centre */
#define POWER_LABEL_RADIUS        88
#define POWER_LABEL_COUNT         5

/* kW range */
#define POWER_KW_MIN              (-160)
#define POWER_KW_MAX              160

/* Temperature arc — same gap geometry as SOC on the speedometer */
#define TEMP_ARC_SIZE             210
#define TEMP_ARC_WIDTH            8
#define TEMP_ARC_START_DEG        61
#define TEMP_ARC_END_DEG          119
#define TEMP_RANGE_MAX            150   /* °C */
#define TEMP_WARM_C               80
#define TEMP_HOT_C                100
#define TEMP_COLD_C               40

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void power_arc_cb(lv_observer_t * observer, lv_subject_t * subject);
static void power_label_cb(lv_observer_t * observer, lv_subject_t * subject);
static void power_needle_color_cb(lv_observer_t * observer, lv_subject_t * subject);
static void temp_arc_cb(lv_observer_t * observer, lv_subject_t * subject);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * power_gauge_create(lv_obj_t * parent,
                               lv_subject_t * power_kw,
                               lv_subject_t * needle_angle,
                               lv_subject_t * odometer_km,
                               lv_subject_t * motor_temp_c)
{
    LV_TRACE_OBJ_CREATE("begin");

    /* ── Container ───────────────────────────────────────────────── */
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_set_name_static(cont, "power_gauge_#");
    lv_obj_set_size(cont, POWER_GAUGE_W, POWER_GAUGE_H);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_flag(cont, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    /* ── 1. Dial face image ──────────────────────────────────────── */
    lv_obj_t * face_img = lv_image_create(cont);
    lv_image_set_src(face_img, small_dial_face);
    lv_image_set_scale(face_img, LV_SCALE_NONE);
    lv_obj_set_align(face_img, LV_ALIGN_TOP_LEFT);

    /* ── 2. White static background arc (full 300° track) ────────── */
    lv_obj_t * bg_arc = lv_arc_create(cont);
    lv_obj_set_size(bg_arc, POWER_ARC_SIZE, POWER_ARC_SIZE);
    lv_obj_align(bg_arc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(bg_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(bg_arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_arc_width(bg_arc, POWER_ARC_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_arc_color(bg_arc, COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(bg_arc, false, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(bg_arc, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bg_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_outline_width(bg_arc, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(bg_arc, 0, LV_PART_KNOB);
    lv_arc_set_bg_angles(bg_arc, POWER_ARC_BG_START, POWER_ARC_BG_END);
    lv_arc_set_range(bg_arc, 0, 1);
    lv_arc_set_value(bg_arc, 0);

    /* ── 3. Yellow active arc (right/power half: 270°→60°) ─────── */
    lv_obj_t * yellow_arc = lv_arc_create(cont);
    lv_obj_set_size(yellow_arc, POWER_ARC_SIZE, POWER_ARC_SIZE);
    lv_obj_align(yellow_arc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(yellow_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(yellow_arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_arc_width(yellow_arc, POWER_ARC_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(yellow_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(yellow_arc, false, LV_PART_MAIN);
    lv_obj_set_style_arc_width(yellow_arc, POWER_ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(yellow_arc, COLOR_GAUGE_ACTIVE, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(yellow_arc, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(yellow_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_outline_width(yellow_arc, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(yellow_arc, 0, LV_PART_KNOB);
    lv_arc_set_bg_angles(yellow_arc, 270, 60);       /* right half: top → lower-right */
    lv_arc_set_range(yellow_arc, 0, POWER_KW_MAX);
    lv_arc_set_value(yellow_arc, 0);
    lv_obj_set_user_data(yellow_arc, (void *)(uintptr_t)0);   /* 0 = power side */
    lv_subject_add_observer_obj(power_kw, power_arc_cb, yellow_arc, NULL);

    /* ── 4. Green active arc (left/regen half: 120°→270°, reverse) ─ */
    lv_obj_t * green_arc = lv_arc_create(cont);
    lv_obj_set_size(green_arc, POWER_ARC_SIZE, POWER_ARC_SIZE);
    lv_obj_align(green_arc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(green_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(green_arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_arc_width(green_arc, POWER_ARC_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(green_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(green_arc, false, LV_PART_MAIN);
    lv_obj_set_style_arc_width(green_arc, POWER_ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(green_arc, COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(green_arc, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(green_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_outline_width(green_arc, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(green_arc, 0, LV_PART_KNOB);
    lv_arc_set_bg_angles(green_arc, 120, 270);       /* left half: lower-left → top */
    lv_arc_set_mode(green_arc, LV_ARC_MODE_REVERSE); /* fills from top toward lower-left */
    lv_arc_set_range(green_arc, 0, -POWER_KW_MIN);   /* 0..160 */
    lv_arc_set_value(green_arc, 0);
    lv_obj_set_user_data(green_arc, (void *)(uintptr_t)1);    /* 1 = regen side */
    lv_subject_add_observer_obj(power_kw, power_arc_cb, green_arc, NULL);

    /* ── 5. Tick-gap mask image ──────────────────────────────────── */
    extern const lv_image_dsc_t small_dial_arc_mask_data;
    lv_obj_t * mask_img = lv_image_create(cont);
    lv_image_set_src(mask_img, &small_dial_arc_mask_data);
    lv_image_set_scale(mask_img, LV_SCALE_NONE);
    lv_obj_align(mask_img, LV_ALIGN_CENTER, 0, 0);

    /* ── 6. Scale labels (−160, −80, 0, +80, +160 kW) ───────────── */
    static const int power_label_vals[POWER_LABEL_COUNT] = { -160, -80, 0, 80, 160 };
    static const char * power_label_strs[POWER_LABEL_COUNT] = { "-160", "-80", "0", "+80", "+160" };

    for(int i = 0; i < POWER_LABEL_COUNT; i++) {
        int v = power_label_vals[i];
        /* angle = POWER_ARC_BG_START + (v - KW_MIN) * (300 / (KW_MAX - KW_MIN)) */
        float angle_deg = (float)POWER_ARC_BG_START +
                          ((float)(v - POWER_KW_MIN) / (float)(POWER_KW_MAX - POWER_KW_MIN)) * 300.0f;
        float angle_rad = angle_deg * (3.14159265f / 180.0f);
        int32_t x_ofs = (int32_t)(cosf(angle_rad) * (float)POWER_LABEL_RADIUS);
        int32_t y_ofs = (int32_t)(sinf(angle_rad) * (float)POWER_LABEL_RADIUS);

        lv_obj_t * lbl = lv_label_create(cont);
        lv_label_set_text(lbl, power_label_strs[i]);
        lv_obj_set_style_text_font(lbl, font_small, 0);
        lv_obj_set_style_text_color(lbl, COLOR_WHITE, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, x_ofs, y_ofs);
        lv_obj_set_user_data(lbl, (void *)(intptr_t)v);
        lv_subject_add_observer_obj(power_kw, power_label_cb, lbl, NULL);
    }

    /* ── 7. Needle ───────────────────────────────────────────────── */
    extern const lv_image_dsc_t small_dial_needle_yellow_data;
    extern const lv_image_dsc_t small_dial_needle_green_data;
    lv_obj_t * needle_img = lv_image_create(cont);
    lv_image_set_src(needle_img, &small_dial_needle_yellow_data);
    /* Match speedometer_image_set_1to1: explicit size + inner-align + 1:1 scale */
    lv_image_set_inner_align(needle_img, LV_IMAGE_ALIGN_CENTER);
    lv_obj_set_size(needle_img, POWER_GAUGE_NEEDLE_W, POWER_GAUGE_NEEDLE_ARM_PX);
    lv_image_set_scale(needle_img, LV_SCALE_NONE);
    lv_obj_set_align(needle_img, LV_ALIGN_TOP_MID);
    lv_obj_set_y(needle_img, POWER_GAUGE_CENTER - POWER_GAUGE_NEEDLE_ARM_PX);
    /* Style transform pivot — same pattern as speedometer_needle_set_pivot.
     * LV_STYLE_TRANSFORM_ROTATION uses these style props, NOT lv_image_set_pivot. */
    lv_obj_set_style_transform_pivot_x(needle_img, POWER_GAUGE_NEEDLE_W / 2, 0);
    lv_obj_set_style_transform_pivot_y(needle_img, POWER_GAUGE_NEEDLE_ARM_PX, 0);
    lv_obj_set_style_bg_opa(needle_img, LV_OPA_TRANSP, 0);
    lv_obj_bind_style_prop(needle_img, LV_STYLE_TRANSFORM_ROTATION, 0, needle_angle);
    lv_subject_add_observer_obj(power_kw, power_needle_color_cb, needle_img, NULL);

    /* ── 8. Centre readout ───────────────────────────────────────── */
    lv_obj_t * lbl_power = lv_label_create(cont);
    lv_label_bind_text(lbl_power, power_kw, "%+.0f");
    lv_obj_set_style_text_font(lbl_power, font_heading, 0);
    lv_obj_set_style_text_color(lbl_power, COLOR_TEXT_HI, 0);
    lv_obj_align(lbl_power, LV_ALIGN_CENTER, 0, -14);

    lv_obj_t * lbl_kw_unit = lv_label_create(cont);
    lv_label_set_text(lbl_kw_unit, "kW");
    lv_obj_set_style_text_font(lbl_kw_unit, font_small, 0);
    lv_obj_set_style_text_color(lbl_kw_unit, COLOR_TEXT_MID, 0);
    lv_obj_align(lbl_kw_unit, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t * lbl_odo = lv_label_create(cont);
    lv_label_bind_text(lbl_odo, odometer_km, "%d km");
    lv_obj_set_style_text_font(lbl_odo, font_small, 0);
    lv_obj_set_style_text_color(lbl_odo, COLOR_TEXT_LO, 0);
    lv_obj_align(lbl_odo, LV_ALIGN_CENTER, 0, 30);

    /* ── 9. Temperature arc (bottom gap, same geometry as SOC arc) ── */
    lv_obj_t * temp_arc = lv_arc_create(cont);
    lv_obj_set_size(temp_arc, TEMP_ARC_SIZE, TEMP_ARC_SIZE);
    lv_obj_align(temp_arc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(temp_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(temp_arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_arc_width(temp_arc, TEMP_ARC_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_arc_color(temp_arc, COLOR_GAUGE_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(temp_arc, false, LV_PART_MAIN);
    lv_obj_set_style_arc_width(temp_arc, TEMP_ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(temp_arc, COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(temp_arc, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(temp_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_outline_width(temp_arc, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(temp_arc, 0, LV_PART_KNOB);
    lv_arc_set_bg_angles(temp_arc, TEMP_ARC_START_DEG, TEMP_ARC_END_DEG);
    lv_arc_set_range(temp_arc, 0, TEMP_RANGE_MAX);
    lv_arc_set_value(temp_arc, 0);
    lv_subject_add_observer_obj(motor_temp_c, temp_arc_cb, temp_arc, NULL);

    /* Temperature label below arc */
    lv_obj_t * lbl_temp = lv_label_create(cont);
    lv_label_bind_text(lbl_temp, motor_temp_c, "%d°C");
    lv_obj_set_style_text_font(lbl_temp, font_small, 0);
    lv_obj_set_style_text_color(lbl_temp, COLOR_TEXT_MID, 0);
    lv_obj_align(lbl_temp, LV_ALIGN_CENTER, 0, 105);

    LV_TRACE_OBJ_CREATE("finished");
    return cont;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void power_arc_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * arc    = (lv_obj_t *)lv_observer_get_target_obj(observer);
    int arc_side      = (int)(uintptr_t)lv_obj_get_user_data(arc);
    float kw          = lv_subject_get_float(subject);
    int val;
    if(arc_side == 0) {
        /* yellow — power side */
        val = (kw > 0.0f) ? (int)kw : 0;
        if(val > POWER_KW_MAX) val = POWER_KW_MAX;
    } else {
        /* green — regen side */
        val = (kw < 0.0f) ? (int)(-kw) : 0;
        if(val > -POWER_KW_MIN) val = -POWER_KW_MIN;
    }
    lv_arc_set_value(arc, val);
}

static void power_label_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * lbl    = (lv_obj_t *)lv_observer_get_target_obj(observer);
    int threshold     = (int)(intptr_t)lv_obj_get_user_data(lbl);
    float kw          = lv_subject_get_float(subject);
    lv_color_t col;

    if(threshold > 0) {
        col = (kw >= (float)threshold) ? COLOR_GAUGE_ACTIVE : COLOR_WHITE;
    } else if(threshold < 0) {
        col = (kw <= (float)threshold) ? COLOR_GREEN : COLOR_WHITE;
    } else {
        /* 0 kW label — yellow when motoring, green when regen, white at rest */
        if(kw > 0.5f)       col = COLOR_GAUGE_ACTIVE;
        else if(kw < -0.5f) col = COLOR_GREEN;
        else                 col = COLOR_WHITE;
    }
    lv_obj_set_style_text_color(lbl, col, 0);
}

static void power_needle_color_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * needle = (lv_obj_t *)lv_observer_get_target_obj(observer);
    extern const lv_image_dsc_t small_dial_needle_green_data;
    extern const lv_image_dsc_t small_dial_needle_yellow_data;
    float kw = lv_subject_get_float(subject);
    const lv_image_dsc_t * new_src = (kw < 0.0f) ? &small_dial_needle_green_data
                                                   : &small_dial_needle_yellow_data;
    if(lv_image_get_src(needle) == new_src) return;   /* no change */
    lv_image_set_src(needle, new_src);
}

static void temp_arc_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * arc = (lv_obj_t *)lv_observer_get_target_obj(observer);
    int temp = lv_subject_get_int(subject);
    lv_arc_set_value(arc, temp < 0 ? 0 : (temp > TEMP_RANGE_MAX ? TEMP_RANGE_MAX : temp));

    lv_color_t col;
    if(temp >= TEMP_HOT_C)       col = COLOR_DANGER;
    else if(temp >= TEMP_WARM_C) col = COLOR_WARNING;
    else if(temp >= TEMP_COLD_C) col = COLOR_GREEN;
    else                          col = lv_color_hex(0x2244CC);  /* blue — cold motor */
    lv_obj_set_style_arc_color(arc, col, LV_PART_INDICATOR);
}
