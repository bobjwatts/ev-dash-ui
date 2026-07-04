/**
 * @file ev_dash.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "ev_dash.h"
#include "user_code/speedometer_needle.h"
#include "user_code/power_gauge_needle.h"
#include <string.h>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

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

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Copy an lv_image_dsc_t's pixel data from flash into a PSRAM heap buffer so
 * LVGL's software renderer accesses it via the D-cache (sequential-read
 * friendly at 200 MHz) instead of the flash XIP cache (limited size, thrashes
 * when multiple large images are composited simultaneously).
 *
 * Falls back to the original flash pointer on allocation failure.
 */
static const lv_image_dsc_t * cache_img_to_psram(const lv_image_dsc_t * src)
{
    if(!src) return src;
#ifdef ESP_PLATFORM
    lv_image_dsc_t * dsc = heap_caps_malloc(sizeof(*dsc), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(!dsc) return src;
    *dsc = *src;
    void * data = heap_caps_malloc(src->data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(!data) { heap_caps_free(dsc); return src; }
    memcpy(data, src->data, src->data_size);
    dsc->data = (const uint8_t *)data;
    return dsc;
#else
    return src;   /* simulator — flash == RAM, no benefit */
#endif
}

void ev_dash_init(const char * asset_path)
{
    ev_dash_init_gen(asset_path);

    /* Cache all large image pixel data into PSRAM heap.
     * These externs are the raw flash C-arrays; the global const void* pointers
     * (dial_speed_dial etc.) are updated here so every widget that uses them
     * automatically reads from PSRAM. */
    extern const lv_image_dsc_t dial_speed_dial_data;
    extern const lv_image_dsc_t dial_speed_arc_mask_data;
    extern const lv_image_dsc_t dial_speed_needle_data;
    extern const lv_image_dsc_t small_dial_face_data;
    extern const lv_image_dsc_t small_dial_arc_mask_data;
    extern const lv_image_dsc_t small_dial_needle_green_data;
    extern const lv_image_dsc_t small_dial_needle_yellow_data;

    dial_speed_dial      = cache_img_to_psram(&dial_speed_dial_data);
    dial_speed_arc_mask  = cache_img_to_psram(&dial_speed_arc_mask_data);
    dial_speed_needle    = cache_img_to_psram(&dial_speed_needle_data);
    small_dial_face      = cache_img_to_psram(&small_dial_face_data);
    small_dial_arc_mask  = cache_img_to_psram(&small_dial_arc_mask_data);
    small_dial_needle_green   = cache_img_to_psram(&small_dial_needle_green_data);
    small_dial_needle_yellow  = cache_img_to_psram(&small_dial_needle_yellow_data);

    speedometer_needle_init();
    power_gauge_needle_init();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/