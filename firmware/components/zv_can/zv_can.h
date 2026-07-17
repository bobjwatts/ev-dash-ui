/*
 * ZombieVerter CAN telemetry — TWAI receiver for OpenInverter spot-value broadcasts.
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start TWAI (GPIO 4 TX / GPIO 5 RX, 500 kbps) and an LVGL timer that maps
 * received packed (0x500..0x509) and spot values into dashboard subjects.
 * Call after the UI is loaded.
 */
esp_err_t zv_can_init(void);

/** Reset trip distance / energy integrators (matches trip-reset button). */
void zv_can_trip_reset(void);

#ifdef __cplusplus
}
#endif
