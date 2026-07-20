/**
 * @file bms_cell_health.h
 * @brief Per-cell health snapshot — fetched once per drive from BMS pack data.
 *
 * Per-cell voltages are not on the CanMap yet; the snapshot derives a 96-cell
 * health grid from pack Vmin/Vmax and balance spread until the VCU exposes
 * a cell array (0x7BB Group 2).
 */
#ifndef BMS_CELL_HEALTH_H
#define BMS_CELL_HEALTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/** Leaf-style 96s pack (compile-time until BMS reports cell count). */
#define BMS_CELL_COUNT  96

/** Clear snapshot so the next drive can fetch again (VCU opmode → off). */
void bms_cell_health_reset_drive(void);

/** True after a successful snapshot this drive. */
bool bms_cell_health_snapshot_done(void);

/**
 * Try to capture cell health once per drive from pack-level BMS readings.
 * @return true if snapshot was stored this call
 */
bool bms_cell_health_try_snapshot(float vmin_v, float vmax_v, int balance_mv);

/** Per-cell health % (0–100); 0 if no snapshot yet. */
uint8_t bms_cell_health_get(int cell_index);

#ifdef __cplusplus
}
#endif

#endif /* BMS_CELL_HEALTH_H */
