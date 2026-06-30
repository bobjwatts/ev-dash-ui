# EV Dash — ESP32-P4 firmware

Flashes the LVGL UI from `../ui` onto the **ESP32-P4-Function-EV-Board** with the 7″ **1024×600** EK79007 panel.

## Prerequisites

- ESP-IDF **6.0+** (tested with 6.0.1 at `C:\esp\v6.0.1\esp-idf`)
- Board wired per the [user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html) (PWM → GPIO26, LCD_RST → GPIO27)

## Environment

From a normal PowerShell session:

```powershell
$env:IDF_TOOLS_PATH = "C:\Espressif"
$env:IDF_PYTHON_ENV_PATH = "C:\Espressif\tools\python\v6.0.1\venv"
$env:PATH = "C:\Espressif\tools\python\v6.0.1\venv\Scripts;" + $env:PATH
& C:\esp\v6.0.1\esp-idf\export.ps1
```

Or use the **ESP-IDF PowerShell** shortcut if it points at your IDF 6 install.

## Build

```powershell
cd firmware
idf.py set-target esp32p4   # first time only
idf.py build
```

### Switching ESP-IDF versions

If you previously built with another IDF version (e.g. 5.3), **delete the build cache** before building with IDF 6. A stale `build/` directory can leave wrong compiler flags (e.g. `xesppie` in `-march`) or broken link state:

```powershell
idf.py fullclean
# or: Remove-Item -Recurse -Force build
idf.py build
```

### Managed components (IDF 6)

`main/idf_component.yml` pulls in BSP 5.2.x, LVGL 9.5, and the `espressif/usb` component required by the P4 Function EV Board on IDF 6.

## Flash & monitor

Replace `COMx` with your USB-UART port. Use a lower baud if flash fails (`Serial data stream stopped`):

```powershell
idf.py -p COMx -b 115200 flash monitor
```

The demo firmware sweeps speed 0→200→0 km/h so you can verify the needle and yellow tick labels.

## Layout

```
firmware/
├── main/main.c              # BSP display init + ev_dash_init()
├── main/idf_component.yml   # BSP, LVGL, usb (IDF 6)
├── components/ev_dash_ui/   # wraps ../ui sources
└── sdkconfig.defaults       # P4, PSRAM, 1024×600 BSP, LVGL 9.5 options
```

## Updating the UI

After changing XML and regenerating `ui/` sources, rebuild from `firmware/`:

```powershell
idf.py build flash
```

If you add new `ui/*_data.c` files, append them to `components/ev_dash_ui/CMakeLists.txt`.
