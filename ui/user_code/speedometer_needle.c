#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "ev_dash_gen.h"
#include "speedometer_needle.h"

#define SPEED_ANGLE_MIN_DECIDEG   (-1090)
#define SPEED_ANGLE_MAX_DECIDEG   1090
#define SPEED_KMH_MAX             200

static void sync_needle_angle_from_speed(lv_subject_t * subject)
{
    int32_t speed_kmh = lv_subject_get_int(subject);
    speed_kmh = LV_CLAMP(0, speed_kmh, SPEED_KMH_MAX);

    int32_t angle = lv_map(speed_kmh,
                           0, SPEED_KMH_MAX,
                           SPEED_ANGLE_MIN_DECIDEG,
                           SPEED_ANGLE_MAX_DECIDEG);

    lv_subject_set_int(&speed_needle_angle, angle);
}

static void speed_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    LV_UNUSED(observer);
    sync_needle_angle_from_speed(subject);
}

void speedometer_needle_init(void)
{
    static bool registered;

    if(registered) return;
    registered = true;

    sync_needle_angle_from_speed(&speed_kmh);
    lv_subject_add_observer(&speed_kmh, speed_observer_cb, NULL);
}
