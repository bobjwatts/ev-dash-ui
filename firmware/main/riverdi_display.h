#pragma once

#include "lvgl.h"

/* Starts the Riverdi video path using the existing esp_lvgl_port. No touch probe.
 * On failure, returns NULL with backlight off and PWR_DN asserted.
 * Successful hardware resources remain owned for the application's lifetime. */
lv_display_t *riverdi_display_start(void);
