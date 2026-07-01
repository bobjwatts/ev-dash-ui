# EV Dash — ESP32-P4 firmware

Flashes the LVGL UI from `../ui` onto the **ESP32-P4-Function-EV-Board** with the 7″ **1024×600** EK79007 panel.

> **Full repo guide:** see the [root README](../README.md) for architecture, asset pipeline, known issues, and UI workflow.

## Quick start

```powershell
# From repo root — generate dial embeds (required after clone)
.\tools\gen_image_data_lvgl.ps1

# ESP-IDF environment (adjust paths)
& C:\esp\v6.0.1\esp-idf\export.ps1

cd firmware
idf.py set-target esp32p4   # first time only
idf.py build
idf.py -p COMx -b 115200 flash monitor
```

## Prerequisites

- ESP-IDF **6.0+** (tested with 6.0.1)
- Board wired per the [user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html)

## Environment (PowerShell)

```powershell
$env:IDF_TOOLS_PATH = "C:\Espressif"
$env:IDF_PYTHON_ENV_PATH = "C:\Espressif\tools\python\v6.0.1\venv"
$env:PATH = "C:\Espressif\tools\python\v6.0.1\venv\Scripts;" + $env:PATH
& C:\esp\v6.0.1\esp-idf\export.ps1
```

## Layout

```
firmware/
├── main/main.c                    # BSP display init + ev_dash_init()
├── main/idf_component.yml         # BSP, LVGL, usb (IDF 6)
├── components/ev_dash_ui/         # Explicit list of ../ui sources
├── components/espressif__esp_lvgl_port/  # Vendored (local patches)
└── sdkconfig.defaults             # P4, PSRAM, RGB565, tear avoidance
```

## Updating the UI

1. Change XML in `../ui/` and regenerate `*_gen.c` (LVGL CLI or CI).
2. If PNGs changed: `.\tools\gen_image_data_lvgl.ps1` from repo root.
3. If new `ui/*.c` files were added, append them to `components/ev_dash_ui/CMakeLists.txt`.
4. `idf.py build flash`

## Clean rebuild

```powershell
idf.py fullclean
idf.py build
```

Required when switching ESP-IDF versions or if serial warns about stale dial embed dimensions.
