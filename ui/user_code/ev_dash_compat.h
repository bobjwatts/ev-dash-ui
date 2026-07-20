/**
 * @file ev_dash_compat.h
 * @brief Stable names for hand-written firmware/UI code after lved-cli regen.
 *
 * lved-cli prefixes enum values (e.g. INFOPANEL_INFO_PANEL_PACK_VOLTAGE).
 * Hand-written components and firmware keep the short names from globals.xml.
 */
#ifndef EV_DASH_COMPAT_H
#define EV_DASH_COMPAT_H

#include "../ev_dash_gen.h"

/* InfoPanel — short names used in info_gauge, dashboard_buttons, main.c */
#ifndef INFO_PANEL_PACK_VOLTAGE
#define INFO_PANEL_PACK_VOLTAGE  INFOPANEL_INFO_PANEL_PACK_VOLTAGE
#endif
#ifndef INFO_PANEL_CELL_BALANCE
#define INFO_PANEL_CELL_BALANCE  INFOPANEL_INFO_PANEL_CELL_BALANCE
#endif
#ifndef INFO_PANEL_EFFICIENCY
#define INFO_PANEL_EFFICIENCY    INFOPANEL_INFO_PANEL_EFFICIENCY
#endif
#ifndef INFO_PANEL_CHARGING
#define INFO_PANEL_CHARGING      INFOPANEL_INFO_PANEL_CHARGING
#endif
#ifndef INFO_PANEL_SOH
#define INFO_PANEL_SOH           INFOPANEL_INFO_PANEL_SOH
#endif

#endif /* EV_DASH_COMPAT_H */
