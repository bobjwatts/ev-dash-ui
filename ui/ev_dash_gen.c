/**
 * @file ev_dash_gen.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "ev_dash_gen.h"

#if LV_USE_XML
#endif /* LV_USE_XML */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/*----------------
 * Translations
 *----------------*/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/*--------------------
 *  Permanent screens
 *-------------------*/

/*----------------
 * Fonts
 *----------------*/

lv_font_t * font_display;
extern lv_font_t font_display_data;
lv_font_t * font_heading;
extern lv_font_t font_heading_data;
lv_font_t * font_body;
extern lv_font_t font_body_data;
lv_font_t * font_small;
extern lv_font_t font_small_data;

/*----------------
 * Images
 *----------------*/

const void * dial_speed_dial;
extern const lv_image_dsc_t dial_speed_dial_data;
const void * dial_speed_needle;
extern const lv_image_dsc_t dial_speed_needle_data;

/*----------------
 * Global styles
 *----------------*/

/*----------------
 * Subjects
 *----------------*/

lv_subject_t speed_kmh;
lv_subject_t gear;
lv_subject_t gear_str;
lv_subject_t regen_level;
lv_subject_t power_kw;
lv_subject_t energy_kwh_remaining;
lv_subject_t state_of_charge_pct;
lv_subject_t battery_voltage_v;
lv_subject_t battery_current_a;
lv_subject_t batt_temp_c;
lv_subject_t motor_temp_c;
lv_subject_t inverter_temp_c;
lv_subject_t motor_rpm;
lv_subject_t odometer_km;
lv_subject_t trip_km;
lv_subject_t range_est_km;
lv_subject_t sys_state;
lv_subject_t fault_code;
lv_subject_t dark_theme;
lv_subject_t speed_needle_angle;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ev_dash_init_gen(const char * asset_path)
{
    char buf[256];


    /*----------------
     * Fonts
     *----------------*/

    /* get font 'font_display' from a C array */
    font_display = &font_display_data;
    /* get font 'font_heading' from a C array */
    font_heading = &font_heading_data;
    /* get font 'font_body' from a C array */
    font_body = &font_body_data;
    /* get font 'font_small' from a C array */
    font_small = &font_small_data;


    /*----------------
     * Images
     *----------------*/
    dial_speed_dial = &dial_speed_dial_data;
    dial_speed_needle = &dial_speed_needle_data;

    /*----------------
     * Global styles
     *----------------*/

    /*----------------
     * Subjects
     *----------------*/
    lv_subject_init_int(&speed_kmh, 0);
    lv_subject_init_int(&gear, 0);
    static char gear_str_buf[UI_SUBJECT_STRING_LENGTH];
    static char gear_str_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&gear_str,
                           gear_str_buf,
                           gear_str_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "P"
                          );
    lv_subject_init_int(&regen_level, 0);
    lv_subject_init_float(&power_kw, 0.0);
    lv_subject_init_int(&energy_kwh_remaining, 0);
    lv_subject_init_int(&state_of_charge_pct, 100);
    lv_subject_init_float(&battery_voltage_v, 0.0);
    lv_subject_init_float(&battery_current_a, 0.0);
    lv_subject_init_int(&batt_temp_c, 0);
    lv_subject_init_int(&motor_temp_c, 0);
    lv_subject_init_int(&inverter_temp_c, 0);
    lv_subject_init_int(&motor_rpm, 0);
    lv_subject_init_int(&odometer_km, 0);
    lv_subject_init_int(&trip_km, 0);
    lv_subject_init_int(&range_est_km, 0);
    lv_subject_init_int(&sys_state, 1);
    lv_subject_init_int(&fault_code, 0);
    lv_subject_init_int(&dark_theme, 0);
    lv_subject_init_int(&speed_needle_angle, -1090);

    /*----------------
     * Translations
     *----------------*/

#if LV_USE_XML
    /* Register widgets */

    /* Register fonts */
    lv_xml_register_font(NULL, "font_display", font_display);
    lv_xml_register_font(NULL, "font_heading", font_heading);
    lv_xml_register_font(NULL, "font_body", font_body);
    lv_xml_register_font(NULL, "font_small", font_small);

    /* Register subjects */
    lv_xml_register_subject(NULL, "speed_kmh", &speed_kmh);
    lv_xml_register_subject(NULL, "gear", &gear);
    lv_xml_register_subject(NULL, "gear_str", &gear_str);
    lv_xml_register_subject(NULL, "regen_level", &regen_level);
    lv_xml_register_subject(NULL, "power_kw", &power_kw);
    lv_xml_register_subject(NULL, "energy_kwh_remaining", &energy_kwh_remaining);
    lv_xml_register_subject(NULL, "state_of_charge_pct", &state_of_charge_pct);
    lv_xml_register_subject(NULL, "battery_voltage_v", &battery_voltage_v);
    lv_xml_register_subject(NULL, "battery_current_a", &battery_current_a);
    lv_xml_register_subject(NULL, "batt_temp_c", &batt_temp_c);
    lv_xml_register_subject(NULL, "motor_temp_c", &motor_temp_c);
    lv_xml_register_subject(NULL, "inverter_temp_c", &inverter_temp_c);
    lv_xml_register_subject(NULL, "motor_rpm", &motor_rpm);
    lv_xml_register_subject(NULL, "odometer_km", &odometer_km);
    lv_xml_register_subject(NULL, "trip_km", &trip_km);
    lv_xml_register_subject(NULL, "range_est_km", &range_est_km);
    lv_xml_register_subject(NULL, "sys_state", &sys_state);
    lv_xml_register_subject(NULL, "fault_code", &fault_code);
    lv_xml_register_subject(NULL, "dark_theme", &dark_theme);
    lv_xml_register_subject(NULL, "speed_needle_angle", &speed_needle_angle);

    /* Register callbacks */
#endif

    /* Register all the global assets so that they won't be created again when globals.xml is parsed.
     * While running in the editor skip this step to update the preview when the XML changes */
#if LV_USE_XML && !defined(LV_EDITOR_PREVIEW)
    /* Register images */
    lv_xml_register_image(NULL, "dial_speed_dial", dial_speed_dial);
    lv_xml_register_image(NULL, "dial_speed_needle", dial_speed_needle);
#endif

#if LV_USE_XML == 0
    /*--------------------
     *  Permanent screens
     *-------------------*/
    /* If XML is enabled it's assumed that the permanent screens are created
     * manaully from XML using lv_xml_create() */
#endif
}

/* Callbacks */

/**********************
 *   STATIC FUNCTIONS
 **********************/