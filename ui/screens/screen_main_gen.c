/**
 * @file screen_main_gen.c
 * @brief Main dashboard screen.
 *
 * Layout (flex-column):
 *   ┌──────────────────────────────────────────────────────────┐  55 px
 *   │  top_bar: [clock]         [P][R][N][D]  [● READY]        │
 *   ├──────────────────────────────────────────────────────────┤
 *   │  dial_row (flex-row, vertically centred):                │ 545 px
 *   │  [power_gauge] [◄] [speedometer] [►] [info_gauge]           │
 *   └──────────────────────────────────────────────────────────┘
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_main_gen.h"
#include "../ev_dash.h"
#include "../user_code/ev_dash_typography.h"
#include "../components/top_bar/top_bar_gen.h"
#include "../components/info_gauge/info_gauge_gen.h"
#include "../components/power_gauge/power_gauge_gen.h"
#include "../user_code/blinker.h"

/*********************
 *      DEFINES
 *********************/

#define TOP_BAR_H     55    /* px */
#define BLINKER_W     28    /* px — width reserved for each arrow indicator */

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * screen_main_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static bool style_inited = false;
    if(!style_inited) {
        style_inited = true;
    }

    /* ── Root: full-screen flex-column ─────────────────────────────── */
    lv_obj_t * root = lv_obj_create(NULL);
    lv_obj_set_name_static(root, "screen_main_#");
    lv_obj_set_size(root, 1024, 600);
    lv_obj_set_style_bg_color(root, COLOR_BG, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_pad_row(root, 0, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_main_place(root, LV_FLEX_ALIGN_START, 0);
    lv_obj_set_style_flex_cross_place(root, LV_FLEX_ALIGN_START, 0);
    lv_obj_set_flag(root, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_set_style_border_width(root, 0, 0);

    /* ── Top bar ────────────────────────────────────────────────────── */
    top_bar_create(root);

    /* ── Dial row ───────────────────────────────────────────────────── */
    lv_obj_t * dial_row = lv_obj_create(root);
    lv_obj_set_name_static(dial_row, "dial_row_#");
    lv_obj_set_width(dial_row, 1024);
    lv_obj_set_flex_grow(dial_row, 1);          /* fills remaining 545 px */
    lv_obj_set_style_bg_opa(dial_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dial_row, 0, 0);
    lv_obj_set_style_pad_all(dial_row, 0, 0);
    lv_obj_set_flex_flow(dial_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_main_place(dial_row, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_set_style_flex_cross_place(dial_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_flag(dial_row, LV_OBJ_FLAG_SCROLLABLE, false);

    /* Power gauge */
    power_gauge_create(dial_row, &power_kw, &power_needle_angle,
                       &odometer_km, &motor_temp_c);

    /* Left blinker arrow */
    lv_obj_t * left_blinker = lv_label_create(dial_row);
    lv_label_set_text(left_blinker, "\xe2\x80\xb9");   /* ‹ — chevron; Figma uses vector ◄ */
    lv_obj_set_style_text_font(left_blinker, font_heading, 0);
    lv_obj_set_style_text_color(left_blinker, COLOR_GREY_DARK, 0);
    lv_obj_set_width(left_blinker, BLINKER_W);
    lv_obj_set_style_text_align(left_blinker, LV_TEXT_ALIGN_CENTER, 0);

    /* Speedometer */
    speedometer_create(dial_row, &speed_kmh, &speed_needle_angle);

    /* Right blinker arrow */
    lv_obj_t * right_blinker = lv_label_create(dial_row);
    lv_label_set_text(right_blinker, "\xe2\x80\xba");   /* › — chevron; Figma uses vector ► */
    lv_obj_set_style_text_font(right_blinker, font_heading, 0);
    lv_obj_set_style_text_color(right_blinker, COLOR_GREY_DARK, 0);
    lv_obj_set_width(right_blinker, BLINKER_W);
    lv_obj_set_style_text_align(right_blinker, LV_TEXT_ALIGN_CENTER, 0);

    /* Right info gauge — empty shell for warning lights / status cluster */
    info_gauge_create(dial_row);

    /* Blinker flash timer — 500 ms, toggles amber when subject is set */
    blinker_init(left_blinker, right_blinker);

    ev_dash_apply_letter_spacing(root);

    LV_TRACE_OBJ_CREATE("finished");

    return root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
