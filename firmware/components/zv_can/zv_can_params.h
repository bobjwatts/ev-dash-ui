/*
 * ZombieVerter / OpenInverter spot-value parameter IDs (VALUE_ENTRY ids).
 * Broadcast CAN ID = ZV_CAN_BROADCAST_BASE + param_id
 */
#pragma once

#define ZV_CAN_BROADCAST_BASE  0x02A0000U

/* Spot values used by the dashboard */
#define ZV_PARAM_OPMODE        2002
#define ZV_PARAM_LASTERR       2004
#define ZV_PARAM_UDC           2006
#define ZV_PARAM_UDC2          2007
#define ZV_PARAM_DELTAV        2009
#define ZV_PARAM_UAUX          2031
#define ZV_PARAM_POWER         2011
#define ZV_PARAM_IDC           2012
#define ZV_PARAM_KWH           2013
#define ZV_PARAM_SOC           2015
#define ZV_PARAM_SPEED_RPM     2016
#define ZV_PARAM_VEH_SPEED     2017
#define ZV_PARAM_DIR           2024
#define ZV_PARAM_TMPHS         2028
#define ZV_PARAM_TMPM          2029
#define ZV_PARAM_TMPAUX        2030
#define ZV_PARAM_HOUR          2065
#define ZV_PARAM_MIN           2066
#define ZV_PARAM_CHG_TEMP      2078
#define ZV_PARAM_U12V          2070
#define ZV_PARAM_BMS_VMIN      2084
#define ZV_PARAM_BMS_VMAX      2085
#define ZV_PARAM_BMS_TMIN      2086
#define ZV_PARAM_BMS_TMAX      2087
#define ZV_PARAM_BMS_CHARGE_LIM 2088
#define ZV_PARAM_CCS_I         2054
#define ZV_PARAM_BMS_TAVG      2103
#define ZV_PARAM_BMS_SOH       2124

/* Remapped 11-bit CAN IDs for spot params whose id > 2047 (stock VCU CanMap limit). */
#define ZV_STD_CAN_HOUR       1800
#define ZV_STD_CAN_MIN        1801
#define ZV_STD_CAN_U12V       1802
#define ZV_STD_CAN_BMS_VMIN   1803
#define ZV_STD_CAN_BMS_VMAX   1804
#define ZV_STD_CAN_BMS_TMAX   1805
#define ZV_STD_CAN_CCS_I      1806
#define ZV_STD_CAN_BMS_TMIN   1807
#define ZV_STD_CAN_BMS_TAVG   1808
#define ZV_STD_CAN_BMS_SOH    1809
