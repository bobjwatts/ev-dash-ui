#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "ev_dash_gen.h"
#include "power_gauge_needle.h"

/* Arc: 120° (−160 kW) → 270° (0 kW) → 60° (+160 kW), 300° total sweep.
 * Needle rotation 0 = 12 o'clock. At 0 kW → arc angle 270° → rotation = 0°.
 * At ±160 kW → ±150° = ±1500 decideg.                                       */
#define POWER_ANGLE_MIN_DECIDEG   (-1100)
#define POWER_ANGLE_MAX_DECIDEG   1100
#define POWER_KW_MIN              (-160.0f)
#define POWER_KW_MAX              160.0f

static void sync_needle_angle_from_power(lv_subject_t * subject)
{
    float kw = lv_subject_get_float(subject);
    if(kw < POWER_KW_MIN) kw = POWER_KW_MIN;
    if(kw > POWER_KW_MAX) kw = POWER_KW_MAX;

    int32_t angle = (int32_t)(((kw - POWER_KW_MIN) / (POWER_KW_MAX - POWER_KW_MIN)) *
                               (float)(POWER_ANGLE_MAX_DECIDEG - POWER_ANGLE_MIN_DECIDEG) +
                               (float)POWER_ANGLE_MIN_DECIDEG);

    lv_subject_set_int(&power_needle_angle, angle);
}

static void power_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    LV_UNUSED(observer);
    sync_needle_angle_from_power(subject);
}

void power_gauge_needle_init(void)
{
    static bool registered;

    if(registered) return;
    registered = true;

    sync_needle_angle_from_power(&power_kw);
    lv_subject_add_observer(&power_kw, power_observer_cb, NULL);
}
