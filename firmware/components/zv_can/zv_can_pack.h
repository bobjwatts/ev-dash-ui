/*
 * Packed CanMap layout — two OpenInverter spot32 values per 8-byte CAN frame.
 *
 * Matches VCU libopeninv CanMap encoding (big-endian spot32 x32):
 *   field A: offset 31, length -32, gain 32  -> bytes 0..3
 *   field B: offset 63, length -32, gain 32  -> bytes 4..7
 *
 * VCU terminal setup (10 message slots, 20 params) — run then `save`:
 *   can t Veh_Speed 1280 31 -32 32
 *   can t power     1280 63 -32 32
 *   can t SOC       1281 31 -32 32
 *   can t dir       1281 63 -32 32
 *   can t opmode    1282 31 -32 32
 *   can t lasterr   1282 63 -32 32
 *   can t udc2      1283 31 -32 32
 *   can t idc       1283 63 -32 32
 *   can t tmphs     1284 31 -32 32
 *   can t tmpm      1284 63 -32 32
 *   can t speed     1285 31 -32 32
 *   can t KWh       1285 63 -32 32
 *   can t Hour      1286 31 -32 32
 *   can t Min       1286 63 -32 32
 *   can t uaux      1287 31 -32 32
 *   can t CCS_I     1287 63 -32 32
 *   can t BMS_Tavg  1288 31 -32 32   (Leaf BMS — no cell min/max in ZV)
 *   can t BMS_ChargeLim 1288 63 -32 32
 *   can t tmpaux    1289 31 -32 32
 *   can t deltaV    1289 63 -32 32
 *
 * SimpBMS / Kangoo (cell min/max instead of Tavg on 1288):
 *   can t BMS_Vmin  1288 31 -32 32
 *   can t BMS_Vmax  1288 63 -32 32
 *
 * ZE1 Leaf with 0x5C0 decode in leafbms.cpp (cell min/max on 1289):
 *   can t BMS_Vmin  1289 31 -32 32
 *   can t BMS_Vmax  1289 63 -32 32
 *
 * Pack SOH from Leaf 0x5BC (leafbms.cpp publishes BMS_SOH param 2124):
 *   can b BMS_SOH
 *   (extended 0x02A084C, or std remap 1809 on dash receive path)
 *
 * CAN IDs are 0x500..0x509 (decimal 1280..1289), within 11-bit limit.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "zv_can_params.h"

#define ZV_PACK_CAN_BASE     0x500U
#define ZV_PACK_FRAME_COUNT  10U

#define ZV_PACK_PARAM_NONE   0xFFFFU

typedef enum {
    ZV_PACK_VAL_FLOAT = 0,
    ZV_PACK_VAL_INT   = 1,
} zv_pack_val_kind_t;

typedef struct {
    uint16_t           can_id;
    uint16_t           param_a;
    uint16_t           param_b;
    zv_pack_val_kind_t kind_a;
    zv_pack_val_kind_t kind_b;
} zv_pack_frame_def_t;

static const zv_pack_frame_def_t zv_pack_frames[ZV_PACK_FRAME_COUNT] = {
    { 0x500, ZV_PARAM_VEH_SPEED, ZV_PARAM_POWER,     ZV_PACK_VAL_FLOAT, ZV_PACK_VAL_FLOAT },
    { 0x501, ZV_PARAM_SOC,       ZV_PARAM_DIR,       ZV_PACK_VAL_FLOAT, ZV_PACK_VAL_INT   },
    { 0x502, ZV_PARAM_OPMODE,    ZV_PARAM_LASTERR,   ZV_PACK_VAL_INT,   ZV_PACK_VAL_INT   },
    { 0x503, ZV_PARAM_UDC2,      ZV_PARAM_IDC,       ZV_PACK_VAL_FLOAT, ZV_PACK_VAL_FLOAT },
    { 0x504, ZV_PARAM_TMPHS,     ZV_PARAM_TMPM,      ZV_PACK_VAL_FLOAT, ZV_PACK_VAL_FLOAT },
    { 0x505, ZV_PARAM_SPEED_RPM, ZV_PARAM_KWH,       ZV_PACK_VAL_FLOAT, ZV_PACK_VAL_FLOAT },
    { 0x506, ZV_PARAM_HOUR,      ZV_PARAM_MIN,       ZV_PACK_VAL_INT,   ZV_PACK_VAL_INT   },
    { 0x507, ZV_PARAM_UAUX,      ZV_PARAM_CCS_I,     ZV_PACK_VAL_FLOAT, ZV_PACK_VAL_FLOAT },
    { 0x508, ZV_PARAM_BMS_TAVG,      ZV_PARAM_BMS_CHARGE_LIM, ZV_PACK_VAL_FLOAT, ZV_PACK_VAL_FLOAT },
    { 0x509, ZV_PARAM_TMPAUX,     ZV_PARAM_DELTAV,    ZV_PACK_VAL_FLOAT, ZV_PACK_VAL_FLOAT },
};

static inline int32_t zv_spot32_from_float(float value)
{
    return (int32_t)(value * 32.0f);
}

static inline int32_t zv_spot32_from_int(int32_t value)
{
    return value * 32;
}

static inline void zv_pack_spot_be(uint8_t *buf, int32_t raw)
{
    buf[0] = (uint8_t)((raw >> 24) & 0xFF);
    buf[1] = (uint8_t)((raw >> 16) & 0xFF);
    buf[2] = (uint8_t)((raw >> 8) & 0xFF);
    buf[3] = (uint8_t)(raw & 0xFF);
}

static inline float zv_unpack_spot_float(const uint8_t *data)
{
    int32_t raw = (int32_t)(((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                            ((uint32_t)data[2] << 8) | (uint32_t)data[3]);
    return (float)raw / 32.0f;
}

static inline int32_t zv_unpack_spot_int(const uint8_t *data)
{
    int32_t raw = (int32_t)(((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                            ((uint32_t)data[2] << 8) | (uint32_t)data[3]);
    return raw / 32;
}

static inline bool zv_pack_can_id_valid(uint32_t can_id)
{
    return can_id >= ZV_PACK_CAN_BASE &&
           can_id < (ZV_PACK_CAN_BASE + ZV_PACK_FRAME_COUNT);
}

static inline const zv_pack_frame_def_t *zv_pack_frame_for_id(uint32_t can_id)
{
    if(!zv_pack_can_id_valid(can_id)) {
        return NULL;
    }
    return &zv_pack_frames[can_id - ZV_PACK_CAN_BASE];
}
