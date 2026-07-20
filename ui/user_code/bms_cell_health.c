/**
 * @file bms_cell_health.c
 */

#include "bms_cell_health.h"

#include <string.h>

static uint8_t s_health[BMS_CELL_COUNT];
static bool    s_fetched;

static int voltage_to_health_pct(float v)
{
    /* Rough Li-ion map: 3.0 V ≈ weak, 4.2 V ≈ full */
    int h = (int)(((v - 3.0f) / 1.2f) * 100.0f + 0.5f);
    if(h < 0)   h = 0;
    if(h > 100) h = 100;
    return h;
}

void bms_cell_health_reset_drive(void)
{
    s_fetched = false;
    memset(s_health, 0, sizeof(s_health));
}

bool bms_cell_health_snapshot_done(void)
{
    return s_fetched;
}

bool bms_cell_health_try_snapshot(float vmin_v, float vmax_v, int balance_mv)
{
    if(s_fetched) {
        return false;
    }
    if(vmin_v <= 0.5f || vmax_v <= 0.5f || vmax_v <= vmin_v) {
        return false;
    }

    int spread_mv = (int)((vmax_v - vmin_v) * 1000.0f + 0.5f);
    if(balance_mv < 0) {
        balance_mv = spread_mv;
    }

    /* Synthetic ladder across cells using observed min/max until 0x7BB Group 2. */
    for(int i = 0; i < BMS_CELL_COUNT; i++) {
        float t = (float)i / (float)(BMS_CELL_COUNT - 1);
        float v = vmin_v + t * (vmax_v - vmin_v);
        s_health[i] = (uint8_t)voltage_to_health_pct(v);
    }

    /* Pull down a few cells to reflect measured imbalance. */
    int weak = balance_mv / 4;
    if(weak < 1 && spread_mv > 15) {
        weak = 1;
    }
    if(weak > 12) {
        weak = 12;
    }
    for(int w = 0; w < weak; w++) {
        int idx = (w * BMS_CELL_COUNT) / weak;
        if(s_health[idx] > 25) {
            s_health[idx] = (uint8_t)(s_health[idx] - 18);
        }
    }

    s_fetched = true;
    return true;
}

uint8_t bms_cell_health_get(int cell_index)
{
    if(cell_index < 0 || cell_index >= BMS_CELL_COUNT || !s_fetched) {
        return 0;
    }
    return s_health[cell_index];
}
