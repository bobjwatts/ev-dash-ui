/*
 * This file is part of the tumanako_vc project.
 *
 * Copyright (C) 2018 Johannes Huebner <dev@johanneshuebner.com>
 *               2024 Daniel Öster <info@dalasevrepair.fi>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "leafbms.h"
#include "my_fp.h"
#include "my_math.h"

#define ZE0_BATTERY 0  // 2011-2013 ZE0
#define AZE0_BATTERY 1 // 2013-2017 AZE0
#define ZE1_BATTERY 2  // 2018+ ZE1
static uint8_t LEAF_battery_Type = ZE0_BATTERY;
static int temperature = 0;

static int leaf_temp_c_from_5c0_byte2(uint8_t b2)
{
  return ((b2 >> 1) - 40);
}

static float leaf_cell_v_from_5c0(const uint8_t *bytes)
{
  /* ZE0/AZE0/ZE1: LB_HistData_Cell_Voltage @ bit 42, 6 bits, ×40 + 1900 mV */
  uint8_t raw = (bytes[5] >> 2) & 0x3F;
  return (raw * 40.0f + 1900.0f) / 1000.0f;
}

static void leaf_decode_5c0_legacy(const uint8_t *bytes)
{
  /* ZE0 + AZE0: mux in byte0 bits 6..7 (EV-can_ZE0.dbc) — 1=MAX, 2=AVG, 3=MIN */
  uint8_t mux = bytes[0] >> 6;
  int temp_c = leaf_temp_c_from_5c0_byte2(bytes[2]);

  switch (mux) {
  case 1:
    Param::SetInt(Param::BMS_Tmax, temp_c);
    Param::SetFloat(Param::BMS_Vmax, leaf_cell_v_from_5c0(bytes));
    break;
  case 2:
    Param::SetInt(Param::BMS_Tavg, temp_c);
    break;
  case 3:
    Param::SetInt(Param::BMS_Tmin, temp_c);
    Param::SetFloat(Param::BMS_Vmin, leaf_cell_v_from_5c0(bytes));
    break;
  default:
    break;
  }
}

static void leaf_decode_5c0_ze1(const uint8_t *bytes)
{
  /* ZE1: mux in byte0 bits 0..2 (EV-can_ZE1.dbc) */
  uint8_t mux = bytes[0] & 0x07;
  int temp_c = leaf_temp_c_from_5c0_byte2(bytes[2]);

  switch (mux) {
  case 1:
    Param::SetInt(Param::BMS_Tmax, temp_c);
    Param::SetFloat(Param::BMS_Vmax, leaf_cell_v_from_5c0(bytes));
    break;
  case 2:
    Param::SetInt(Param::BMS_Tavg, temp_c);
    break;
  case 3:
    Param::SetInt(Param::BMS_Tmin, temp_c);
    Param::SetFloat(Param::BMS_Vmin, leaf_cell_v_from_5c0(bytes));
    break;
  default:
    break;
  }
}

void LeafBMS::Task100Ms() {
  /* Leaf CAN updates BMS_Vmin/Vmax/temps in DecodeCAN. Base BMS::Task100Ms zeros
   * them every 100 ms — do not call it or spot values stay at 0. */
}

void LeafBMS::SetCanInterface(CanHardware *can) {
  can->RegisterUserMessage(0x1DB); // Leaf BMS message 10ms
  can->RegisterUserMessage(0x1DC); // Leaf BMS message 10ms
  can->RegisterUserMessage(0x55B); // Leaf BMS message 100ms
  can->RegisterUserMessage(0x5BC); // Leaf BMS message 100ms (500ms on ZE0)
  can->RegisterUserMessage(0x5C0); // Leaf BMS historical mux (500ms) — cell min/max + temps
  can->RegisterUserMessage(0x59E); // Leaf BMS 500ms (AZE0/ZE1 battery detect)
  can->RegisterUserMessage(0x1C2); // Leaf BMS message 10ms (ZE1)
  can->RegisterUserMessage(0x1ED); // Leaf BMS message 10ms (ZE1, only on 62kWh)
}

void LeafBMS::DecodeCAN(int id, uint8_t *data) {
  uint8_t *bytes = (uint8_t *)data;

  switch (id) {
  case 0x1DB: {
    if (isMessageCorrupt(bytes)) {
      // Message content malformed, abort reading data from it! Raise flag!
      break;
    }
    float cur = uint16_t(bytes[0] << 3) + uint16_t(bytes[1] >> 5);
    if (cur > 1023) {
      cur -= 2047; // check if negative
    }
    uint16_t udc = uint16_t(bytes[2] << 2) + uint16_t(bytes[3] >> 6);
    // bool interlock = (bytes[3] & (1 << 3)) >> 3;
    // bool full = (bytes[3] & (1 << 4)) >> 4;
    
    if (Param::GetInt(Param::ShuntType) == 0) { 
      // Only populate if no shunt is used
      float BattCur = cur / 2;
      float BattVoltage = udc / 2;
      Param::SetFloat(Param::idc, BattCur);
      if (BattVoltage < 450) {
        Param::SetFloat(Param::udc2, BattVoltage);
      }
      if (BattVoltage > 200) {
        Param::SetFloat(Param::udcsw, BattVoltage - 20);
        // Set for precharging based on actual voltage
      }
      float kw = (BattVoltage * BattCur) / 1000;
      // calculate power and post to parameter database
      Param::SetFloat(Param::power, kw);
    }
    break;
  }
  case 0x1DC: {
    if (isMessageCorrupt(bytes)) {
      // Message content malformed, abort reading data from it! Raise flag!
      break;
    }
    float dislimit = uint16_t(bytes[0] << 2) + uint16_t(bytes[1] >> 6);
    dislimit = dislimit * 0.25; // Kw discharge limit
    float chglimit = uint16_t((bytes[1] & 0x3F) << 4) + uint16_t(bytes[2] >> 4);
    chglimit = chglimit * 0.25; // Kw charge limit
    float chargelimit =
        uint16_t((bytes[2] & 0x0F) << 6) + uint16_t(bytes[3] >> 2);
    chargelimit = chargelimit * 0.1; // Kw charger limit

    chargelimit = chargelimit * 1000 /
                  Param::GetFloat(Param::udc2); // Transform into Amps
    // Param::SetFixed(Param::dislim, dislimit / 4);

    Param::SetFloat(Param::BMS_ChargeLim, chargelimit);
    Param::SetInt(Param::BMS_MaxInput, chglimit);
    Param::SetInt(Param::BMS_MaxOutput, dislimit);
    break;
  }
  case 0x55B: {
    if (isMessageCorrupt(bytes)) {
      // Message content malformed, abort reading data from it! Raise flag!
      break;
    }
    float soc = uint16_t(bytes[0] << 2) + uint16_t(bytes[1] >> 6);
    if (Param::GetInt(Param::ShuntType) ==
        0) // Only populate if no shunt is used
    {
      soc = soc * 0.1;
      Param::SetFloat(Param::SOC, soc);
    }

    uint16_t IsoTemp = uint16_t(bytes[4] << 2) + uint16_t(bytes[5] >> 6);

    Param::SetInt(Param::BMS_IsoMeas, IsoTemp);
    break;
  }
  case 0x5BC: {
    /* LB_Capacity_Deterioration_Rate @ byte4 bits 1..7 — pack SOH 0–100% */
    int soh = bytes[4] >> 1;
    if (soh < 0)
      soh = 0;
    if (soh > 100)
      soh = 100;
    Param::SetInt(Param::BMS_SOH, soh);

    /* 0x5BC avg battery temp only on ZE0; ZE1 uses 0x5C0 mux */
    if (LEAF_battery_Type == ZE0_BATTERY) {
      temperature = (bytes[3] - 40);
      Param::SetInt(Param::BMS_Tavg, temperature);
    }
    break;
  }
  case 0x5C0: {
    if (LEAF_battery_Type == ZE1_BATTERY)
      leaf_decode_5c0_ze1(bytes);
    else
      leaf_decode_5c0_legacy(bytes); /* ZE0 default + AZE0 */
    break;
  }
  case 0x59E: {
    // AZE0 2013-2017 or ZE1 2018-2023 battery detected
    // Only detect as AZE0 if not already set as ZE1
    if (LEAF_battery_Type != ZE1_BATTERY) {
      LEAF_battery_Type = AZE0_BATTERY;
    }
    break;
  }
  case 0x1C2: {
    // ZE1 2018-2023 battery detected!
    LEAF_battery_Type = ZE1_BATTERY;
    break;
  }
  case 0x1ED: {
    // ZE1 62kWh battery detected!
    LEAF_battery_Type = ZE1_BATTERY;
    break;
  }
  default:
    break;
  }
}

bool LeafBMS::isMessageCorrupt(uint8_t *data) {
  uint8_t crc = 0;
  uint8_t polynomial = 0x85;

  for (int b = 0; b < 8; b++) {
    uint8_t byte =
        (b == 7) ? 0 : data[b]; // Treat 8th byte as 0 during calculation.
    for (int i = 7; i >= 0; i--) {
      uint8_t bit = ((byte & (1 << i)) > 0) ? 1 : 0;
      if (crc >= 0x80)
        crc = (uint8_t)(((crc << 1) + bit) ^ polynomial);
      else
        crc = (uint8_t)((crc << 1) + bit);
    }
  }
  return crc != data[7];
}
