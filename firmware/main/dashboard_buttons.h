/**
 * @file dashboard_buttons.h
 * @brief Panel-cycle and trip-reset GPIO buttons for the dashboard.
 */

#ifndef DASHBOARD_BUTTONS_H
#define DASHBOARD_BUTTONS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init GPIO inputs and start the debounce poll task.
 * Call after LVGL / ev_dash_init.
 */
void dashboard_buttons_init(void);

/**
 * Reset trip distance and energy integrator (trip-reset button action).
 * Safe to call from any task — acquires the display lock.
 */
void dashboard_trip_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* DASHBOARD_BUTTONS_H */
