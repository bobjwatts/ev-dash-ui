# EV Dash — LVGL UI

LVGL 9.5 UI project for an ESP32-P4 based electric-vehicle instrument cluster.

**Display:** 1024 × 600 px · EK79007AD · MIPI-DSI  
**Target:** ESP32-P4 · ESP-IDF v6.0.1  

---

## Workflow

```
Edit XML in Cursor  →  git push  →  GitHub Actions builds WASM  →  GitHub Pages
                                                                         ↑
                                         Load runtime URL in LVGL Pro Editor
```

1. **Edit** — build screens and components in `ui/` using Cursor (AI-assisted XML editing).
2. **Push** — pushing to `main` triggers the build workflow automatically.
3. **Preview** — the workflow compiles a WASM preview runtime and deploys it to GitHub Pages.
4. **Style** — open [pro.lvgl.io](https://pro.lvgl.io), load your XML, then point  
   **Preview → Custom runtime URL** at your GitHub Pages URL to see the running UI live in the browser.
5. **Export** — save any XML changes made in the online editor back to `ui/` and commit.

### GitHub Pages URL (after first deployment)

```
https://<your-github-username>.github.io/<repo-name>/lved-runtime.js
```

Enable GitHub Pages in **Settings → Pages → Source: GitHub Actions** before the first push.

---

## Project structure

```
lvgl_editor/
├── ui/                         ← working LVGL UI project
│   ├── project.xml             ← display size (1024×600), LVGL version
│   ├── globals.xml             ← fonts, colours, data subjects (live data)
│   ├── screens/                ← one .xml file per screen
│   ├── components/             ← reusable component .xml files
│   ├── fonts/                  ← font source files (TTF) — generated .c goes here
│   ├── images/                 ← image assets
│   ├── widgets/                ← custom widget folders
│   ├── CMakeLists.txt          ← generated — links ui as lib-ui for ESP-IDF
│   └── user_config.cmake       ← add hand-written .c sources here
└── .github/workflows/
    └── preview.yml             ← build & deploy workflow
```

---

## Data model (`globals.xml` subjects)

These subjects are bound to UI widgets and updated at runtime from your ESP-IDF firmware via `lv_subject_set_*()`.

| Subject | Type | Description |
|---|---|---|
| `speed_kmh` | int | Vehicle speed |
| `state_of_charge_pct` | int | Battery SoC 0–100 |
| `power_kw` | float | Motor power (+ discharge / − regen) |
| `battery_voltage_v` | float | Pack voltage |
| `battery_current_a` | float | Pack current |
| `batt_temp_c` | int | Battery temperature |
| `motor_temp_c` | int | Motor temperature |
| `inverter_temp_c` | int | Inverter temperature |
| `motor_rpm` | int | Motor RPM |
| `gear` | int | GearPosition enum (PARK/REVERSE/NEUTRAL/DRIVE/BRAKE) |
| `regen_level` | int | Regen paddle setting (0–3) |
| `odometer_km` | int | Total odometer |
| `trip_km` | int | Trip meter |
| `range_est_km` | int | Estimated range |
| `energy_kwh_remaining` | int | Usable energy remaining |
| `sys_state` | int | SysState enum (FAULT/READY/ACTIVE/CHARGE) |
| `fault_code` | int | Active fault code (0 = none) |

---

## ESP-IDF integration

The generated `lib-ui` CMake library is consumed by the ESP-IDF component system:

```cmake
# In your top-level CMakeLists.txt or idf_component_register()
add_subdirectory(ui)
target_link_libraries(${COMPONENT_LIB} PRIVATE lib-ui)
```

Initialise the UI after `lv_init()` and display setup:

```c
#include "ui_ev_dash.h"

void app_main(void)
{
    /* ... display driver init, lv_init(), lv_display_create() ... */

    ui_ev_dash_init(NULL);          /* NULL = assets embedded as C arrays */

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

Update live data from any task via the subject API:

```c
#include "ui_ev_dash_gen.h"

/* Example: CAN RX task updating speed and SoC */
lv_subject_set_int(&subject_speed_kmh, can_data.speed);
lv_subject_set_int(&subject_state_of_charge_pct, can_data.soc);
lv_subject_set_float(&subject_power_kw, can_data.power_kw);
```

---

## Local C code generation (optional)

If you want to regenerate the `.c`/`.h` files locally without pushing:

```bash
# Download the CLI once
curl -L https://github.com/lvgl/lvgl_editor/releases/download/v1.2.1/LVGL_Pro_CLI-1.2.1-linux-mac.zip -o lvgl-cli.zip
unzip lvgl-cli.zip -d lvgl-cli

# Generate C from XML
node ./lvgl-cli/lved-cli.js generate ui -ss

# Compile WASM preview runtime (optional)
node ./lvgl-cli/lved-cli.js compile ui
```
