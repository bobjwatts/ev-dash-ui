/**
 * @file soc_gauge_gen.c
 * @brief State-of-charge arc + range readout rendered inside the speedometer dial.
 *
 * Layout (all coordinates relative to the 439×439 speedometer centre):
 *
 *         [408 km]          ← SOC_LBL_RANGE_Y
 *          [range]          ← SOC_LBL_UNIT_Y
 *   E ─────────────── F     ← SOC arc, hugging dial border in bottom gap
 *           [SOC]           ← SOC_LBL_SOC_Y
 *
 * Arc geometry (LVGL: 0° = 3 o'clock, increasing clockwise):
 *   bg_angles(SOC_ARC_START_DEG=61°, SOC_ARC_END_DEG=119°)
 *   → 58° arc centred on 90° (bottom), 42° gap each side from speed arc.
 *   LV_ARC_MODE_REVERSE → indicator fills from E toward F as SOC rises.
 *   Colour: green → COLOR_WARNING (≤20%) → COLOR_DANGER (≤10%).
 */

/*********************
 *      INCLUDES
 *********************/

#include "soc_gauge_gen.h"
#include "../../ev_dash_gen.h"
#include <math.h>

/*********************
 *      DEFINES
 *********************/

/* Arc widget — same radius as the speed scale ring (379 px) so it
 * hugs the dial border in the bottom gap left by the speed arc.    */
#define SOC_ARC_SIZE        379
#define SOC_ARC_WIDTH       10

/* Angular endpoints (LVGL degrees, clockwise from 3 o'clock).
 * Speed arc ends at 19° and 161°.  A 42° gap each side gives a clean
 * visual separation between the speed ring and the SOC bar.          */
#define SOC_ARC_START_DEG   61    /* F side — lower right */
#define SOC_ARC_END_DEG     119   /* E side — lower left  */

/* Labels sit well inside the arc ring, clear of the speed scale labels
 * which live at 150 px radius.                                        */
#define SOC_LABEL_RADIUS    160

#define SOC_WARN_PCT        20    /* below this → orange warning */
#define SOC_CRIT_PCT        10    /* below this → red danger     */

/* ── Text positions (x,y offset from speedometer centre, px) ───────
 * Tweak these and reflash to dial in placement without touching code. */
#define SOC_LBL_E_F_RADIUS  SOC_LABEL_RADIUS   /* E/F computed from angle */
#define SOC_LBL_SOC_Y       155
#define SOC_LBL_RANGE_Y     100
#define SOC_LBL_UNIT_Y      118

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void soc_arc_cb(lv_observer_t * observer, lv_subject_t * subject);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * soc_gauge_create(lv_obj_t * parent,
                             lv_subject_t * soc_pct,
                             lv_subject_t * range_km)
{
    LV_TRACE_OBJ_CREATE("begin");

    /* ── SOC arc ──────────────────────────────────────────────────── */
    lv_obj_t * arc = lv_arc_create(parent);
    lv_obj_set_size(arc, SOC_ARC_SIZE, SOC_ARC_SIZE);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);

    /* Background (inactive) track — square ends */
    lv_obj_set_style_arc_width(arc, SOC_ARC_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, COLOR_GAUGE_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);

    /* Active indicator — square ends */
    lv_obj_set_style_arc_width(arc, SOC_ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);

    /* Hide knob */
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_outline_width(arc, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 0, LV_PART_KNOB);

    /* Arc shape: clockwise from F(61°) through bottom(90°) to E(119°). */
    lv_arc_set_bg_angles(arc, SOC_ARC_START_DEG, SOC_ARC_END_DEG);
    lv_arc_set_mode(arc, LV_ARC_MODE_REVERSE);   /* fills from E toward F as SOC rises */
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 100);                   /* show full until first subject update */
    lv_subject_add_observer_obj(soc_pct, soc_arc_cb, arc, NULL);

    /* ── E / F endpoint labels ────────────────────────────────────── */
    float e_rad = (float)SOC_ARC_END_DEG   * (3.14159265f / 180.0f);
    float f_rad = (float)SOC_ARC_START_DEG * (3.14159265f / 180.0f);
    int32_t e_x = (int32_t)(cosf(e_rad) * (float)SOC_LBL_E_F_RADIUS);
    int32_t e_y = (int32_t)(sinf(e_rad) * (float)SOC_LBL_E_F_RADIUS);
    int32_t f_x = (int32_t)(cosf(f_rad) * (float)SOC_LBL_E_F_RADIUS);
    int32_t f_y = (int32_t)(sinf(f_rad) * (float)SOC_LBL_E_F_RADIUS);

    lv_obj_t * lbl_e = lv_label_create(parent);
    lv_label_set_text(lbl_e, "E");
    lv_obj_set_style_text_font(lbl_e, font_small, 0);
    lv_obj_set_style_text_color(lbl_e, COLOR_TEXT_MID, 0);
    lv_obj_align(lbl_e, LV_ALIGN_CENTER, e_x, e_y);

    lv_obj_t * lbl_f = lv_label_create(parent);
    lv_label_set_text(lbl_f, "F");
    lv_obj_set_style_text_font(lbl_f, font_small, 0);
    lv_obj_set_style_text_color(lbl_f, COLOR_TEXT_MID, 0);
    lv_obj_align(lbl_f, LV_ALIGN_CENTER, f_x, f_y);

    /* ── "SOC" label (below arc centre) ──────────────────────────── */
    lv_obj_t * lbl_soc = lv_label_create(parent);
    lv_label_set_text(lbl_soc, "SOC");
    lv_obj_set_style_text_font(lbl_soc, font_small, 0);
    lv_obj_set_style_text_color(lbl_soc, COLOR_TEXT_MID, 0);
    lv_obj_align(lbl_soc, LV_ALIGN_CENTER, 0, SOC_LBL_SOC_Y);

    /* ── Range readout — inside the dial above the SOC arc ────────── */
    lv_obj_t * lbl_range_val = lv_label_create(parent);
    lv_label_bind_text(lbl_range_val, range_km, "%d km");
    lv_obj_set_style_text_font(lbl_range_val, font_body, 0);
    lv_obj_set_style_text_color(lbl_range_val, COLOR_GREEN, 0);
    lv_obj_align(lbl_range_val, LV_ALIGN_CENTER, 0, SOC_LBL_RANGE_Y);

    lv_obj_t * lbl_range_unit = lv_label_create(parent);
    lv_label_set_text(lbl_range_unit, "range");
    lv_obj_set_style_text_font(lbl_range_unit, font_small, 0);
    lv_obj_set_style_text_color(lbl_range_unit, COLOR_GREEN_MID, 0);
    lv_obj_align(lbl_range_unit, LV_ALIGN_CENTER, 0, SOC_LBL_UNIT_Y);

    LV_TRACE_OBJ_CREATE("finished");
    return arc;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void soc_arc_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * arc = (lv_obj_t *)lv_observer_get_target_obj(observer);
    int soc = lv_subject_get_int(subject);
    lv_arc_set_value(arc, soc);

    /* Colour shifts from green → warning → danger as charge drops. */
    lv_color_t col;
    if(soc <= SOC_CRIT_PCT)       col = COLOR_DANGER;
    else if(soc <= SOC_WARN_PCT)  col = COLOR_WARNING;
    else                           col = COLOR_GREEN;
    lv_obj_set_style_arc_color(arc, col, LV_PART_INDICATOR);
}
