/* Forwarding header: LVGL Pro Editor codegen targets LVGL 9.5 which exposes
   lv_xml at <lv_xml/lv_xml.h>, but LVGL 9.4 keeps it at src/others/xml/.
   This shim resolves the mismatch without touching managed_components. */
#include "others/xml/lv_xml.h"
