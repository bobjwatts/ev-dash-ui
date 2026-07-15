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
#define ZV_PARAM_POWER         2011
#define ZV_PARAM_IDC           2012
#define ZV_PARAM_KWH           2013
#define ZV_PARAM_SOC           2015
#define ZV_PARAM_SPEED_RPM     2016
#define ZV_PARAM_VEH_SPEED     2017
#define ZV_PARAM_DIR           2024
#define ZV_PARAM_TMPHS         2028
#define ZV_PARAM_TMPM          2029
#define ZV_PARAM_HOUR          2065
#define ZV_PARAM_MIN           2066
#define ZV_PARAM_U12V          2070
#define ZV_PARAM_BMS_VMIN      2084
#define ZV_PARAM_BMS_VMAX      2085
#define ZV_PARAM_BMS_TMAX      2087
#define ZV_PARAM_CCS_I         2054
