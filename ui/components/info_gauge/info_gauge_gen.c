/**
 * @file info_gauge_gen.c
 * @brief Right-hand info gauge — four switchable panels (button / charging auto-switch).
 *
 * Panels:
 *   0 Pack voltage   — blue arc fill ∝ pack V
 *   1 Cell balance   — green arc fill ∝ cell spread (lower mV = fuller)
 *   2 Efficiency     — teal arc fill ∝ Wh/km
 *   3 Charging       — green arc fill ∝ SOC%
 */

/*********************
 *      INCLUDES
 *********************/

#include "info_gauge_gen.h"
#include "../../ev_dash.h"

/*********************
 *      DEFINES
 *********************/

#define INFO_GAUGE_W        264
#define INFO_GAUGE_H        264

#define INFO_ARC_SIZE       210
#define INFO_ARC_WIDTH      25
#define INFO_ARC_START_DEG  160
#define INFO_ARC_END_DEG    20

#define PACK_V_MIN          280
#define PACK_V_MAX          420
#define CELL_BALANCE_MAX_MV 50
#define EFFICIENCY_MAX_WH_KM 300

#define INFO_COLOR_TEAL     lv_color_hex(0x00C8B4)

/* Layout offsets from dial centre (px) */
#define MAIN_VALUE_Y        (-50)
#define MAIN_TITLE_Y        (-25)
#define SUB_LEFT_X          (0)
#define SUB_RIGHT_X         50
#define SUB_ROW_Y           40
#define SUB_BOTTOM_Y        88
#define SUB_RING_SIZE       56

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_obj_t * cont;
    lv_obj_t * theme_arc;
    lv_obj_t * main_value;
    lv_obj_t * main_title;
    lv_obj_t * sub_left_ring;
    lv_obj_t * sub_left_val;
    lv_obj_t * sub_left_cap;
    lv_obj_t * sub_right_ring;
    lv_obj_t * sub_right_val;
    lv_obj_t * sub_right_cap;
    lv_obj_t * sub_bottom_ring;
    lv_obj_t * sub_bottom_val;
    lv_obj_t * sub_bottom_cap;
} info_gauge_ui_t;

/***********************
 *  STATIC VARIABLES
 **********************/

static info_gauge_ui_t s_ui;

/***********************
 *  STATIC PROTOTYPES
 **********************/

static lv_obj_t * sub_gauge_create(lv_obj_t * parent, int32_t x_ofs, int32_t y_ofs,
                                   lv_obj_t ** ring_out, lv_obj_t ** val_out, lv_obj_t ** cap_out);
static void info_gauge_set_arc_fill(lv_color_t color, int value_pct);
static int info_gauge_arc_pct_for_panel(int panel);
static void info_gauge_refresh(void);
static void info_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
static void info_gauge_bind_observers(lv_obj_t * cont);

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_obj_t * sub_gauge_create(lv_obj_t * parent, int32_t x_ofs, int32_t y_ofs,
                                   lv_obj_t ** ring_out, lv_obj_t ** val_out, lv_obj_t ** cap_out)
{
    lv_obj_t * ring = lv_obj_create(parent);
    lv_obj_set_size(ring, SUB_RING_SIZE, SUB_RING_SIZE);
    lv_obj_align(ring, LV_ALIGN_CENTER, x_ofs, y_ofs);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(ring, COLOR_GREY_DARK, 0);
    lv_obj_set_style_border_width(ring, 2, 0);
    lv_obj_set_style_pad_all(ring, 0, 0);
    lv_obj_set_flag(ring, LV_OBJ_FLAG_SCROLLABLE, false);

    lv_obj_t * val = lv_label_create(ring);
    lv_label_set_text(val, "--");
    lv_obj_set_style_text_font(val, font_body, 0);
    lv_obj_set_style_text_color(val, COLOR_TEXT_HI, 0);
    lv_obj_align(val, LV_ALIGN_CENTER, 0, -6);

    lv_obj_t * cap = lv_label_create(ring);
    lv_label_set_text(cap, "");
    lv_obj_set_style_text_font(cap, font_small, 0);
    lv_obj_set_style_text_color(cap, COLOR_TEXT_LO, 0);
    lv_obj_align(cap, LV_ALIGN_CENTER, 0, 10);

    *ring_out = ring;
    *val_out  = val;
    *cap_out  = cap;
    return ring;
}

static void info_gauge_set_arc_fill(lv_color_t color, int value_pct)
{
    lv_obj_set_style_arc_color(s_ui.theme_arc, color, LV_PART_INDICATOR);
    if(value_pct < 0) value_pct = 0;
    if(value_pct > 100) value_pct = 100;
    lv_arc_set_value(s_ui.theme_arc, value_pct);
}

static int info_gauge_arc_pct_for_panel(int panel)
{
    switch(panel) {
        case INFO_PANEL_PACK_VOLTAGE: {
            int v = (int)lv_subject_get_float(&battery_voltage_v);
            return LV_CLAMP(0, lv_map(v, PACK_V_MIN, PACK_V_MAX, 0, 100), 100);
        }
        case INFO_PANEL_CELL_BALANCE: {
            /* Lower spread = healthier = fuller arc */
            int mv = lv_subject_get_int(&cell_balance_mv);
            return LV_CLAMP(0, 100 - lv_map(mv, 0, CELL_BALANCE_MAX_MV, 0, 100), 100);
        }
        case INFO_PANEL_EFFICIENCY: {
            int wh = lv_subject_get_int(&efficiency_wh_per_km);
            return LV_CLAMP(0, lv_map(wh, 0, EFFICIENCY_MAX_WH_KM, 0, 100), 100);
        }
        case INFO_PANEL_CHARGING:
            return LV_CLAMP(0, lv_subject_get_int(&state_of_charge_pct), 100);
        default:
            return 0;
    }
}

static lv_color_t info_gauge_arc_color_for_panel(int panel)
{
    switch(panel) {
        case INFO_PANEL_PACK_VOLTAGE:  return COLOR_BLUE;
        case INFO_PANEL_CELL_BALANCE:  return COLOR_GREEN;
        case INFO_PANEL_EFFICIENCY:    return INFO_COLOR_TEAL;
        case INFO_PANEL_CHARGING:      return COLOR_GREEN;
        default:                       return COLOR_TEXT_MID;
    }
}

static void info_gauge_format_energy_kwh(char * buf, size_t buf_sz)
{
    /* energy_kwh_remaining stored as tenths of kWh (32 → 3.2 kWh) */
    int tenths = lv_subject_get_int(&energy_kwh_remaining);
    lv_snprintf(buf, buf_sz, "%d.%d kWh", tenths / 10, tenths % 10);
}

static void info_gauge_refresh(void)
{
    int panel = lv_subject_get_int(&info_panel);
    char buf[24];

    info_gauge_set_arc_fill(info_gauge_arc_color_for_panel(panel),
                            info_gauge_arc_pct_for_panel(panel));

    switch(panel) {
        case INFO_PANEL_PACK_VOLTAGE:
            lv_snprintf(buf, sizeof(buf), "%.0fv", (double)lv_subject_get_float(&battery_voltage_v));
            lv_label_set_text(s_ui.main_value, buf);
            lv_label_set_text(s_ui.main_title, "Pack Voltage");

            lv_snprintf(buf, sizeof(buf), "%d°", lv_subject_get_int(&batt_temp_c));
            lv_label_set_text(s_ui.sub_left_val, buf);
            lv_label_set_text(s_ui.sub_left_cap, "BAT TEMP");

            lv_snprintf(buf, sizeof(buf), "%d°", lv_subject_get_int(&inverter_temp_c));
            lv_label_set_text(s_ui.sub_right_val, buf);
            lv_label_set_text(s_ui.sub_right_cap, "INV TEMP");

            lv_snprintf(buf, sizeof(buf), "%.1fv", (double)lv_subject_get_float(&aux_voltage_v));
            lv_label_set_text(s_ui.sub_bottom_val, buf);
            lv_label_set_text(s_ui.sub_bottom_cap, "AUX");
            lv_obj_set_style_border_color(s_ui.sub_bottom_ring, COLOR_DANGER, 0);
            break;

        case INFO_PANEL_CELL_BALANCE:
            lv_snprintf(buf, sizeof(buf), "%dmV", lv_subject_get_int(&cell_balance_mv));
            lv_label_set_text(s_ui.main_value, buf);
            lv_label_set_text(s_ui.main_title, "Cell Balance");

            lv_snprintf(buf, sizeof(buf), "%d°", lv_subject_get_int(&batt_temp_c));
            lv_label_set_text(s_ui.sub_left_val, buf);
            lv_label_set_text(s_ui.sub_left_cap, "BAT TEMP");

            lv_snprintf(buf, sizeof(buf), "%d°", lv_subject_get_int(&inverter_temp_c));
            lv_label_set_text(s_ui.sub_right_val, buf);
            lv_label_set_text(s_ui.sub_right_cap, "INV TEMP");

            lv_snprintf(buf, sizeof(buf), "%d%%", lv_subject_get_int(&state_of_charge_pct));
            lv_label_set_text(s_ui.sub_bottom_val, buf);
            lv_label_set_text(s_ui.sub_bottom_cap, "SOC");
            lv_obj_set_style_border_color(s_ui.sub_bottom_ring, COLOR_WARNING, 0);
            break;

        case INFO_PANEL_EFFICIENCY:
            lv_snprintf(buf, sizeof(buf), "%dWh/km", lv_subject_get_int(&efficiency_wh_per_km));
            lv_label_set_text(s_ui.main_value, buf);
            lv_label_set_text(s_ui.main_title, "Efficiency");

            lv_snprintf(buf, sizeof(buf), "%d°", lv_subject_get_int(&batt_temp_c));
            lv_label_set_text(s_ui.sub_left_val, buf);
            lv_label_set_text(s_ui.sub_left_cap, "BAT TEMP");

            lv_snprintf(buf, sizeof(buf), "%d°", lv_subject_get_int(&inverter_temp_c));
            lv_label_set_text(s_ui.sub_right_val, buf);
            lv_label_set_text(s_ui.sub_right_cap, "INV");

            info_gauge_format_energy_kwh(buf, sizeof(buf));
            lv_label_set_text(s_ui.sub_bottom_val, buf);
            lv_label_set_text(s_ui.sub_bottom_cap, "");
            lv_obj_set_style_border_color(s_ui.sub_bottom_ring, INFO_COLOR_TEAL, 0);
            break;

        case INFO_PANEL_CHARGING:
            lv_snprintf(buf, sizeof(buf), "%d%%", lv_subject_get_int(&state_of_charge_pct));
            lv_label_set_text(s_ui.main_value, buf);
            lv_label_set_text(s_ui.main_title, "Charging");

            lv_snprintf(buf, sizeof(buf), "%d°", lv_subject_get_int(&batt_temp_c));
            lv_label_set_text(s_ui.sub_left_val, buf);
            lv_label_set_text(s_ui.sub_left_cap, "BAT TEMP");

            lv_snprintf(buf, sizeof(buf), "%.0fA", (double)lv_subject_get_float(&charge_amps_a));
            lv_label_set_text(s_ui.sub_right_val, buf);
            lv_label_set_text(s_ui.sub_right_cap, "AMPS");

            info_gauge_format_energy_kwh(buf, sizeof(buf));
            lv_label_set_text(s_ui.sub_bottom_val, buf);
            lv_label_set_text(s_ui.sub_bottom_cap, "");
            lv_obj_set_style_border_color(s_ui.sub_bottom_ring, COLOR_GREEN, 0);
            break;

        default:
            break;
    }

    lv_obj_invalidate(s_ui.cont);
}

static void info_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    LV_UNUSED(observer);
    LV_UNUSED(subject);
    info_gauge_refresh();
}

static void info_gauge_bind_observers(lv_obj_t * cont)
{
    lv_subject_add_observer_obj(&info_panel, info_observer_cb, cont, NULL);
    lv_subject_add_observer_obj(&sys_state, info_observer_cb, cont, NULL);
    lv_subject_add_observer_obj(&battery_voltage_v, info_observer_cb, cont, NULL);
    lv_subject_add_observer_obj(&cell_balance_mv, info_observer_cb, cont, NULL);
    lv_subject_add_observer_obj(&efficiency_wh_per_km, info_observer_cb, cont, NULL);
    lv_subject_add_observer_obj(&energy_kwh_remaining, info_observer_cb, cont, NULL);
    lv_subject_add_observer_obj(&batt_temp_c, info_observer_cb, cont, NULL);
    lv_subject_add_observer_obj(&inverter_temp_c, info_observer_cb, cont, NULL);
    lv_subject_add_observer_obj(&aux_voltage_v, info_observer_cb, cont, NULL);
    lv_subject_add_observer_obj(&charge_amps_a, info_observer_cb, cont, NULL);
    lv_subject_add_observer_obj(&state_of_charge_pct, info_observer_cb, cont, NULL);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * info_gauge_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_set_name_static(cont, "info_gauge_#");
    lv_obj_set_size(cont, INFO_GAUGE_W, INFO_GAUGE_H);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_flag(cont, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    s_ui.cont = cont;

    lv_obj_t * face_img = lv_image_create(cont);
    lv_image_set_src(face_img, small_dial_face);
    lv_image_set_scale(face_img, LV_SCALE_NONE);
    lv_obj_set_align(face_img, LV_ALIGN_TOP_LEFT);

    /* Theme arc — value fill only (no needle); colour + amount per panel */
    s_ui.theme_arc = lv_arc_create(cont);
    lv_obj_set_size(s_ui.theme_arc, INFO_ARC_SIZE, INFO_ARC_SIZE);
    lv_obj_align(s_ui.theme_arc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(s_ui.theme_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(s_ui.theme_arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_arc_width(s_ui.theme_arc, INFO_ARC_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ui.theme_arc, COLOR_TEXT_MID, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(s_ui.theme_arc, false, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ui.theme_arc, INFO_ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ui.theme_arc, COLOR_BLUE, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(s_ui.theme_arc, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_ui.theme_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_outline_width(s_ui.theme_arc, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_ui.theme_arc, 0, LV_PART_KNOB);
    lv_arc_set_bg_angles(s_ui.theme_arc, INFO_ARC_START_DEG, INFO_ARC_END_DEG);
    lv_arc_set_range(s_ui.theme_arc, 0, 100);
    lv_arc_set_value(s_ui.theme_arc, 0);

    lv_obj_t * mask_img = lv_image_create(cont);
    lv_image_set_src(mask_img, small_dial_arc_mask);
    lv_image_set_scale(mask_img, LV_SCALE_NONE);
    lv_obj_align(mask_img, LV_ALIGN_CENTER, 0, 0);

    s_ui.main_value = lv_label_create(cont);
    lv_label_set_text(s_ui.main_value, "350v");
    lv_obj_set_style_text_font(s_ui.main_value, font_heading, 0);
    lv_obj_set_style_text_color(s_ui.main_value, COLOR_TEXT_HI, 0);
    lv_obj_align(s_ui.main_value, LV_ALIGN_CENTER, 0, MAIN_VALUE_Y);

    s_ui.main_title = lv_label_create(cont);
    lv_label_set_text(s_ui.main_title, "Pack Voltage");
    lv_obj_set_style_text_font(s_ui.main_title, font_small, 0);
    lv_obj_set_style_text_color(s_ui.main_title, COLOR_TEXT_MID, 0);
    lv_obj_align(s_ui.main_title, LV_ALIGN_CENTER, 0, MAIN_TITLE_Y);

    sub_gauge_create(cont, SUB_LEFT_X, SUB_ROW_Y,
                     &s_ui.sub_left_ring, &s_ui.sub_left_val, &s_ui.sub_left_cap);
    sub_gauge_create(cont, SUB_RIGHT_X, SUB_ROW_Y,
                     &s_ui.sub_right_ring, &s_ui.sub_right_val, &s_ui.sub_right_cap);
    sub_gauge_create(cont, 0, SUB_BOTTOM_Y,
                     &s_ui.sub_bottom_ring, &s_ui.sub_bottom_val, &s_ui.sub_bottom_cap);

    info_gauge_bind_observers(cont);
    info_gauge_refresh();

    LV_TRACE_OBJ_CREATE("finished");
    return cont;
}
