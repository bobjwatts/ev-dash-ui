/**
 * @file blinker.c
 * @brief Turn-signal blinker flash driver.
 *
 * A single 500 ms lv_timer toggles the indicator colour:
 *   active  → alternates between COLOR_WARNING (amber) and transparent
 *   inactive → stays dim (COLOR_GREY_DARK)
 */

/*********************
 *      INCLUDES
 *********************/

#include "blinker.h"

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_obj_t * s_left  = NULL;
static lv_obj_t * s_right = NULL;

/**********************
 *  STATIC FUNCTIONS
 **********************/

static void blinker_timer_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    static bool flash_on = false;
    flash_on = !flash_on;

    bool l_active = lv_subject_get_int(&blinker_left)  != 0;
    bool r_active = lv_subject_get_int(&blinker_right) != 0;

    lv_obj_set_style_text_color(s_left,
        (l_active && flash_on) ? COLOR_WARNING : COLOR_GREY_DARK, 0);

    lv_obj_set_style_text_color(s_right,
        (r_active && flash_on) ? COLOR_WARNING : COLOR_GREY_DARK, 0);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void blinker_init(lv_obj_t * left, lv_obj_t * right)
{
    s_left  = left;
    s_right = right;
    lv_timer_create(blinker_timer_cb, 500, NULL);
}
