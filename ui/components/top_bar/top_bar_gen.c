/**
 * @file top_bar_gen.c
 * @brief Top status bar: clock (ZombieVerter Hour/Min), P/R/N/D gear selector,
 *        READY / precharge / fault / charge indicator driven by sys_state.
 */

/*********************
 *      INCLUDES
 *********************/

#include "top_bar_gen.h"

/*********************
 *      DEFINES
 *********************/

#define PILL_ITEM_SIZE   34   /* px — diameter of each gear circle inside the pill */
#define PILL_PAD          4   /* px — padding inside pill border */
#define PILL_COL_GAP      3   /* px — gap between gear items */
#define READY_DOT_SIZE   20   /* px — diameter of the status dot */

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void clock_cb(lv_observer_t * observer, lv_subject_t * subject);
static void gear_pill_cb(lv_observer_t * observer, lv_subject_t * subject);
static void ready_indicator_cb(lv_observer_t * observer, lv_subject_t * subject);

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* ── Clock ───────────────────────────────────────────────────────────────── */

static void clock_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    LV_UNUSED(subject);
    lv_obj_t * lbl = (lv_obj_t *)lv_observer_get_target_obj(observer);
    char buf[6];
    lv_snprintf(buf, sizeof(buf), "%02d:%02d",
                lv_subject_get_int(&hour),
                lv_subject_get_int(&minute));
    lv_label_set_text(lbl, buf);
}

/* ── Gear selector pill ──────────────────────────────────────────────────── */

/* Each child of the pill stores its GearPosition value in user_data.
 * The label colour inverts when the item becomes active. */
static void gear_pill_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * pill = (lv_obj_t *)lv_observer_get_target_obj(observer);
    int g = lv_subject_get_int(subject);
    uint32_t n = lv_obj_get_child_count(pill);
    for(uint32_t i = 0; i < n; i++) {
        lv_obj_t * item = lv_obj_get_child(pill, i);
        bool active = ((int)(intptr_t)lv_obj_get_user_data(item) == g);
        /* Active: white filled circle; inactive: transparent */
        lv_obj_set_style_bg_opa(item, active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        /* Active label: dark text on white; inactive: light grey text */
        lv_obj_t * lbl = lv_obj_get_child(item, 0);
        lv_obj_set_style_text_color(lbl, active ? COLOR_BG : COLOR_TEXT_MID, 0);
    }
}

static lv_obj_t * gear_pill_create(lv_obj_t * parent)
{
    /* Gear positions — displayed left to right */
    static const struct { const char * label; int val; } GEARS[] = {
        { "P", GEARPOSITION_GEAR_PARK    },
        { "R", GEARPOSITION_GEAR_REVERSE },
        { "N", GEARPOSITION_GEAR_NEUTRAL },
        { "D", GEARPOSITION_GEAR_DRIVE   },
    };
    static const int N_GEARS = (int)(sizeof(GEARS) / sizeof(GEARS[0]));

    /* Outer pill */
    lv_obj_t * pill = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(pill, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(pill, COLOR_GREY_DARK, 0);
    lv_obj_set_style_border_width(pill, 2, 0);
    lv_obj_set_style_border_opa(pill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(pill, PILL_PAD, 0);
    lv_obj_set_style_pad_column(pill, PILL_COL_GAP, 0);
    lv_obj_set_flex_flow(pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_cross_place(pill, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_height(pill, PILL_ITEM_SIZE + PILL_PAD * 2);
    lv_obj_set_width(pill, LV_SIZE_CONTENT);
    lv_obj_set_flag(pill, LV_OBJ_FLAG_SCROLLABLE, false);

    for(int i = 0; i < N_GEARS; i++) {
        /* Circle container for each gear letter */
        lv_obj_t * item = lv_obj_create(pill);
        lv_obj_set_size(item, PILL_ITEM_SIZE, PILL_ITEM_SIZE);
        lv_obj_set_style_radius(item, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(item, COLOR_WHITE, 0);
        lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(item, 0, 0);
        lv_obj_set_style_pad_all(item, 0, 0);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_flex_main_place(item, LV_FLEX_ALIGN_CENTER, 0);
        lv_obj_set_style_flex_cross_place(item, LV_FLEX_ALIGN_CENTER, 0);
        lv_obj_set_flag(item, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_user_data(item, (void *)(intptr_t)GEARS[i].val);

        lv_obj_t * lbl = lv_label_create(item);
        lv_label_set_text(lbl, GEARS[i].label);
        lv_obj_set_style_text_font(lbl, font_heading, 0);
        lv_obj_set_style_text_color(lbl, COLOR_TEXT_MID, 0);
    }

    lv_subject_add_observer_obj(&gear, gear_pill_cb, pill, NULL);
    return pill;
}

/* ── READY / sys_state indicator ─────────────────────────────────────────── */

static void ready_indicator_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * cont = (lv_obj_t *)lv_observer_get_target_obj(observer);
    lv_obj_t * dot  = lv_obj_get_child(cont, 0);
    lv_obj_t * lbl  = lv_obj_get_child(cont, 1);

    lv_color_t color;
    const char * text;
    switch(lv_subject_get_int(subject)) {
        case SYSSTATE_SYS_READY:  color = COLOR_OK;      text = "READY";  break;
        case SYSSTATE_SYS_ACTIVE: color = COLOR_WARNING;  text = "PRECHG"; break;
        case SYSSTATE_SYS_CHARGE: color = COLOR_ACCENT;   text = "CHARGE"; break;
        default:                   color = COLOR_DANGER;   text = "FAULT";  break;
    }
    lv_obj_set_style_bg_color(dot, color, 0);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
}

static lv_obj_t * ready_indicator_create(lv_obj_t * parent)
{
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_column(cont, 8, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_cross_place(cont, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_flag(cont, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    /* Status dot */
    lv_obj_t * dot = lv_obj_create(cont);
    lv_obj_set_size(dot, READY_DOT_SIZE, READY_DOT_SIZE);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, COLOR_OK, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_pad_all(dot, 0, 0);

    /* Status text */
    lv_obj_t * lbl = lv_label_create(cont);
    lv_label_set_text(lbl, "READY");
    lv_obj_set_style_text_font(lbl, font_heading, 0);
    lv_obj_set_style_text_color(lbl, COLOR_OK, 0);

    lv_subject_add_observer_obj(&sys_state, ready_indicator_cb, cont, NULL);
    return cont;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * top_bar_create(lv_obj_t * parent)
{
    lv_obj_t * bar = lv_obj_create(parent);
    lv_obj_set_name_static(bar, "top_bar_#");
    lv_obj_set_width(bar, 1024);
    lv_obj_set_height(bar, 55);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_left(bar, 20, 0);
    lv_obj_set_style_pad_right(bar, 20, 0);
    lv_obj_set_style_pad_top(bar, 0, 0);
    lv_obj_set_style_pad_bottom(bar, 0, 0);
    lv_obj_set_style_pad_column(bar, 16, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_cross_place(bar, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_main_place(bar, LV_FLEX_ALIGN_START, 0);
    lv_obj_set_flag(bar, LV_OBJ_FLAG_SCROLLABLE, false);

    /* ── Clock (left) ── */
    lv_obj_t * clock_lbl = lv_label_create(bar);
    lv_label_set_text(clock_lbl, "00:00");
    lv_obj_set_style_text_font(clock_lbl, font_heading, 0);
    lv_obj_set_style_text_color(clock_lbl, COLOR_TEXT_HI, 0);
    lv_subject_add_observer_obj(&hour,   clock_cb, clock_lbl, NULL);
    lv_subject_add_observer_obj(&minute, clock_cb, clock_lbl, NULL);

    /* ── Spacer (pushes gear pill + READY to the right) ── */
    lv_obj_t * spacer = lv_obj_create(bar);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_style_pad_all(spacer, 0, 0);
    lv_obj_set_size(spacer, 0, 1);
    lv_obj_set_flex_grow(spacer, 1);

    /* ── P/R/N/D gear pill ── */
    gear_pill_create(bar);

    /* ── READY / sys_state indicator ── */
    ready_indicator_create(bar);

    return bar;
}
