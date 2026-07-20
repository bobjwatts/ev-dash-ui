/**
 * @file ev_dash_gen.h
 */

#ifndef EV_DASH_GEN_H
#define EV_DASH_GEN_H

#ifndef UI_SUBJECT_STRING_LENGTH
#define UI_SUBJECT_STRING_LENGTH 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
    #include "lvgl_private.h"
#else
    #include "lvgl/lvgl.h"
    #include "lvgl/lvgl_private.h"
#endif

#ifdef LV_USE_XML
    #include "lv_xml/lv_xml.h"
#endif



/*********************
 *      DEFINES
 *********************/

#define COLOR_BLACK lv_color_hex(0x000000)

#define COLOR_GRAY00 lv_color_hex(0x101010)

#define COLOR_GRAY0 lv_color_hex(0x212121)

#define COLOR_GRAY1 lv_color_hex(0x404040)

#define COLOR_GRAY2 lv_color_hex(0x9A9A9A)

#define COLOR_GRAY3 lv_color_hex(0xC9C9C9)

#define COLOR_BLUE lv_color_hex(0x67A2F1)

#define COLOR_RED lv_color_hex(0x920027)

#define COLOR_GREEN lv_color_hex(0x399A65)

#define COLOR_YELLOW_DARK lv_color_hex(0xC3A205)

#define COLOR_YELLOW lv_color_hex(0xF3E176)

#define COLOR_WHITE lv_color_hex(0xFFFFFF)

#define COLOR_BG lv_color_hex(0x101010)

#define COLOR_SURFACE lv_color_hex(0x212121)

#define COLOR_BORDER lv_color_hex(0x404040)

#define COLOR_GREY_DARK lv_color_hex(0x404040)

#define COLOR_GREY_LIGHT lv_color_hex(0xC9C9C9)

#define COLOR_ACCENT lv_color_hex(0x67A2F1)

#define COLOR_ACCENT_DIM lv_color_hex(0x212121)

#define COLOR_OK lv_color_hex(0x399A65)

#define COLOR_WARNING lv_color_hex(0xC3A205)

#define COLOR_DANGER lv_color_hex(0x920027)

#define COLOR_TEXT_HI lv_color_hex(0xC9C9C9)

#define COLOR_TEXT_MID lv_color_hex(0x9A9A9A)

#define COLOR_TEXT_LO lv_color_hex(0x404040)

#define COLOR_GAUGE_ACTIVE lv_color_hex(0xF3E176)

#define COLOR_GAUGE_TRACK lv_color_hex(0x212121)

#define COLOR_GREEN_MID lv_color_hex(0x399A65)

#define COLOR_GREEN_DARK lv_color_hex(0x404040)

#define FONT_SIZE_DISPLAY 54

#define FONT_SIZE_HEADING 25

#define FONT_SIZE_SUBHEAD 18

#define FONT_SIZE_BODY 16

#define FONT_SIZE_SMALL 12

#define FONT_LETTER_SPACE_DISPLAY 5

#define FONT_LETTER_SPACE_HEADING 3

#define FONT_LETTER_SPACE_SUBHEAD 2

#define FONT_LETTER_SPACE_BODY 2

#define FONT_LETTER_SPACE_SMALL 1

#define SOC_WARN_PCT 20

#define SOC_CRIT_PCT 10

#define TEMP_WARN_C 70

#define TEMP_CRIT_C 85

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    GEARPOSITION_GEAR_PARK = 0,
    GEARPOSITION_GEAR_REVERSE = 1,
    GEARPOSITION_GEAR_NEUTRAL = 2,
    GEARPOSITION_GEAR_DRIVE = 3,
    GEARPOSITION_GEAR_BRAKE = 4
}GearPosition_t;

typedef enum {
    SYSSTATE_SYS_FAULT = 0,
    SYSSTATE_SYS_READY = 1,
    SYSSTATE_SYS_ACTIVE = 2,
    SYSSTATE_SYS_CHARGE = 3,
    SYSSTATE_SYS_OFF = 4
}SysState_t;

typedef enum {
    INFOPANEL_INFO_PANEL_PACK_VOLTAGE = 0,
    INFOPANEL_INFO_PANEL_CELL_BALANCE = 1,
    INFOPANEL_INFO_PANEL_EFFICIENCY = 2,
    INFOPANEL_INFO_PANEL_CHARGING = 3,
    INFOPANEL_INFO_PANEL_SOH = 4
}InfoPanel_t;

/**********************
 * GLOBAL VARIABLES
 **********************/

/*-------------------
 * Permanent screens
 *------------------*/

/*----------------
 * Global styles
 *----------------*/

/*----------------
 * Fonts
 *----------------*/

extern lv_font_t * font_display;

extern lv_font_t * font_heading;

extern lv_font_t * font_subhead;

extern lv_font_t * font_body;

extern lv_font_t * font_small;

/*----------------
 * Images
 *----------------*/

extern const void * dial_speed_needle;
extern const void * dial_speed_dial;
extern const void * dial_speed_arc_mask;
extern const void * small_dial_face;
extern const void * small_dial_arc_mask;
extern const void * small_dial_needle_green;
extern const void * small_dial_needle_yellow;

/*----------------
 * Subjects
 *----------------*/

extern lv_subject_t speed_kmh;
extern lv_subject_t gear;
extern lv_subject_t gear_str;
extern lv_subject_t regen_level;
extern lv_subject_t power_kw;
extern lv_subject_t energy_kwh_remaining;
extern lv_subject_t state_of_charge_pct;
extern lv_subject_t battery_voltage_v;
extern lv_subject_t battery_current_a;
extern lv_subject_t batt_temp_c;
extern lv_subject_t motor_temp_c;
extern lv_subject_t inverter_temp_c;
extern lv_subject_t motor_rpm;
extern lv_subject_t odometer_km;
extern lv_subject_t trip_km;
extern lv_subject_t range_est_km;
extern lv_subject_t hour;
extern lv_subject_t minute;
extern lv_subject_t blinker_left;
extern lv_subject_t blinker_right;
extern lv_subject_t info_panel;
extern lv_subject_t cell_balance_mv;
extern lv_subject_t state_of_health_pct;
extern lv_subject_t cell_health_ready;
extern lv_subject_t efficiency_wh_per_km;
extern lv_subject_t trip_wh_used;
extern lv_subject_t aux_voltage_v;
extern lv_subject_t charge_amps_a;
extern lv_subject_t sys_state;
extern lv_subject_t fault_code;
extern lv_subject_t dark_theme;
extern lv_subject_t speed_needle_angle;
extern lv_subject_t power_needle_angle;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*----------------
 * Event Callbacks
 *----------------*/

/**
 * Initialize the component library
 */

void ev_dash_init_gen(const char * asset_path);

/**********************
 *      MACROS
 **********************/

/**********************
 *   POST INCLUDES
 **********************/

/*Include all the widgets, components and screens of this library*/
#include "components/gauge_center_readout/gauge_center_readout_gen.h"
#include "components/speed_scale_ring/speed_scale_ring_fine_gen.h"
#include "components/speed_scale_ring/speed_scale_ring_gen.h"
#include "components/speedometer/speedometer_gen.h"
#include "screens/screen_main_gen.h"

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*EV_DASH_GEN_H*/