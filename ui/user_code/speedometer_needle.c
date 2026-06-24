/**
 * speedometer_needle.c
 *
 * Rotates the speedometer needle image in response to changes in the
 * speed_kmh subject.  Call speedometer_needle_init() once after
 * ui_ev_dash_init() in your firmware's app_main / lv_task.
 *
 * Angle convention (LVGL, tenths of degrees):
 *   0° = 12 o'clock (top), clockwise positive.
 *   Needle image points straight UP at 0° (as exported from Figma).
 *
 * TODO: Measure SPEED_ANGLE_MIN / SPEED_ANGLE_MAX from your Figma file.
 *   Open the design, select the needle at 0 km/h and read its rotation,
 *   then repeat at 200 km/h.  Enter the values below in whole degrees.
 *   Example values assume a standard 270° gauge sweep:
 *     0 km/h  → needle at 225° (7:30 position)
 *     200 km/h → needle at 315° → wraps as -45° → stored as 3150 + offset
 *
 *   LVGL stores rotation in tenths of degrees.  225° = 2250, 315° = 3150.
 */

#include "lvgl/lvgl.h"
#include "ui_ev_dash_gen.h"   /* exposes subject_speed_kmh */

/* ── Tune these two values to match your Figma needle positions ─────────── */
#define SPEED_ANGLE_MIN_DECIDEG   2250   /* needle angle at   0 km/h (225.0°) */
#define SPEED_ANGLE_MAX_DECIDEG   3150   /* needle angle at 200 km/h (315.0°) */
#define SPEED_KMH_MAX             200
/* ────────────────────────────────────────────────────────────────────────── */

/* Needle image dimensions at source resolution (before scale_x/scale_y) */
#define NEEDLE_SRC_WIDTH   33
#define NEEDLE_SRC_HEIGHT  274

static void needle_observer_cb(lv_subject_t * subject, lv_observer_t * observer)
{
    lv_obj_t * needle = (lv_obj_t *)lv_observer_get_target(observer);

    int32_t speed_kmh = lv_subject_get_int(subject);
    speed_kmh = LV_CLAMP(0, speed_kmh, SPEED_KMH_MAX);

    int32_t angle = lv_map(speed_kmh,
                           0, SPEED_KMH_MAX,
                           SPEED_ANGLE_MIN_DECIDEG,
                           SPEED_ANGLE_MAX_DECIDEG);

    /* Pivot at the bottom-centre of the needle image (the axle point). */
    lv_image_set_pivot(needle, NEEDLE_SRC_WIDTH / 2, NEEDLE_SRC_HEIGHT);
    lv_image_set_rotation(needle, (int16_t)angle);
}

/**
 * speedometer_needle_init
 *
 * Call once, after ui_ev_dash_init(), passing the lv_obj_t * of the
 * needle image.  The easiest way to obtain that pointer is via the
 * LVGL XML-generated accessor, e.g.:
 *
 *   speedometer_needle_init(ui_get_speedometer_needle());
 *
 * If the generated code does not expose an accessor yet, traverse the
 * object tree from the screen root:
 *
 *   lv_obj_t * needle = lv_obj_get_child(speedometer_container, NEEDLE_INDEX);
 *   speedometer_needle_init(needle);
 */
void speedometer_needle_init(lv_obj_t * needle_img)
{
    LV_ASSERT_OBJ(needle_img, &lv_image_class);

    /* Set initial position immediately */
    lv_image_set_pivot(needle_img, NEEDLE_SRC_WIDTH / 2, NEEDLE_SRC_HEIGHT);
    lv_image_set_rotation(needle_img, SPEED_ANGLE_MIN_DECIDEG);

    /* Subscribe to speed changes */
    lv_subject_add_observer_obj(&subject_speed_kmh,
                                needle_observer_cb,
                                needle_img,
                                NULL);
}
