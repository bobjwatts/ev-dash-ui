# EV Dash — ESP32-P4 firmware

Runs the display pipeline on the **ESP32-P4-Function-EV-Board** with the
**1280x800 Riverdi RVT121HVDFWN00**, retaining the original 1024x600 EK79007
panel as a selectable fallback.

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

## Riverdi RVT121HVDFWN00 bring-up

The default application display is now the **1280 x 800 Riverdi**, with its
**SN65DSI83 DSI-to-LVDS bridge**. Startup probes the two possible 7-bit I2C
addresses, 0x2D then 0x2C, and verifies the ID before writing bridge registers.
The connected Riverdi module was detected at **0x2C**. It is not initialized with
the stock EK79007 DCS command sequence. The existing stock BSP configuration
remains available under `idf.py menuconfig` -> **EV dashboard display** ->
**Connected display** -> **Original Function EV Board LCD**. Leave the BSP's
original 1024x600 selection in place for that fallback; its other 1280x800 option
is an unrelated portrait ILI9881C panel, not this Riverdi.
The dependency manifest retains LVGL 9.4.0 and explicitly selects the existing
patched local LVGL port so a new build preserves this framebuffer pipeline.

For an existing build, run `idf.py menuconfig` once and verify the new selection;
`sdkconfig.defaults` does not overwrite values already saved in `sdkconfig`.
The default Riverdi build loads the existing **1024x600 dashboard** on the
1280x800 display using the existing LVGL direct-mode double framebuffers.
The UI keeps its original size and is centred on a full-screen black background:
128-pixel side margins and 100-pixel top/bottom margins on the Riverdi.
To repeat the verified P4 video
test, enable **Show a solid blue screen instead of the dashboard** in the same
menu. Riverdi startup does not probe the dev-board touchscreen; this module
has no touch controller.

The optional **Use the SN65DSI83 internal pattern for diagnosis** setting
(`CONFIG_EV_DASH_RIVERDI_BRIDGE_PATTERN`, default off) makes the bridge generate
its own LVDS test pattern. It ignores P4 video data while retaining the DSI clock,
timings and LVDS configuration. Startup logs `INTERNAL PATTERN enabled` in this
mode; the blue LVGL framebuffer is then not the displayed image.
Disable the setting to return to the P4 solid-blue test.

Measure the module's **5 V supply at the Riverdi connector with the backlight
running**, rather than relying on the supply's display. During bring-up, both
P4 video and the bridge's internal pattern flashed and went black with 4.8 V
measured at the module despite a 5.3 V supply indication. Correcting the measured
module voltage restored a stable internal pattern. The temporary monitoring
task and 20-second backlight delay used for diagnosis have been removed.
With the internal pattern disabled, the P4/LVGL solid-blue test was also
confirmed to stay visible with the corrected supply voltage.

The dashboard has also been displayed on the Riverdi and visually confirmed.
There is a separate, unresolved **early startup fault** with the current
ESP-IDF 6.1 / P4 revision 3.2 dashboard build: without startup diagnostics it
repeatedly raises a store-access fault while FreeRTOS fills the main task stack,
before `app_main()` and display initialization. The solid-blue build starts
normally. A diagnostic dashboard build reaches `UI loaded`; its heap checks
pass. Even a single normal log before scheduler startup changes the outcome,
while a delay alone does not, so this is not evidence of a repaired heap or
a proven memory-timing cause.

The current hardware test build explicitly enables **Check heap before starting
the scheduler (diagnostic build)** (`CONFIG_EV_DASH_STARTUP_HEAP_DIAGNOSTIC`).
This option defaults off and is not a root-cause fix. It keeps the observed
working diagnostic path reproducible without leaving unconditional probes in
normal startup. Logs identify the diagnostic build. Investigate this startup
fault before treating the dashboard firmware as ready for deployment.
The diagnostic build was flashed and checked for 65 seconds after reset: heap
integrity passed, the bridge reported `0xE5=0x00`, and `UI loaded` appeared with
no panic or further reset. This checks the diagnostic path only.

Logical connections (do not reverse DSI lane polarity in software):

| P4 signal | Riverdi J1 |
|---|---|
| D0+ / D0- | 2 / 3 |
| D1+ / D1- | 5 / 6 |
| CLK+ / CLK- | 8 / 9 |
| GPIO8 I2C SCL / GPIO7 I2C SDA (BSP bus) | 20 / 21 |
| GPIO27 PWR_DN, active low | 17 |
| GPIO26 backlight, high = full brightness | 18 |
| 3.3 V VREF | 24 |
| 5 V module supply | 26-29 |
| Common ground | 30 and connector grounds |

The bring-up uses **two DSI lanes at 864 Mbps/lane**, a continuous **432 MHz
DSI clock**, and the bridge's divide-by-6 clock to produce **72 MHz LVDS**.
APLL supplies the matching P4 pixel clock (approximately 59.67 Hz refresh).
72 MHz is within the specified 66.3-78.9 MHz range and close to the 72.4 MHz
typical value. The default P4 240 MHz DPI source has an integer divider and
would round a 72.4 MHz request to 80 MHz, outside that range.

Timings are H: active 1280, sync 32, back porch 56, front porch 72 (total 1440);
V: active 800, sync 4, back porch 19, front porch 15 (total 838). The datasheet's
88/23 back-porch figures **include sync**, so they must not be used directly as
the IDF porch fields. The chosen sync widths are within the module's limits.
Framebuffers remain RGB565; P4 hardware expands output to RGB888 because the
SN65DSI83 does not accept RGB565 DSI video.

GPIO27 is held low with the DSI clock running and data lanes idle before release.
Bridge writes preserve reserved bits and are read back; PLL enable/settling and
soft reset precede video start. The driver clears startup flags and checks CSR
0xE5 again after several frames. A nonzero status stops startup with an error
log and leaves PWR_DN low and backlight off. GPIO26 is driven high for full
brightness; the stock BSP's 5 kHz backlight PWM exceeds this module's specified
200-1000 Hz dimming range.
The Riverdi path also disables IDF's default per-frame BTA acknowledgment request
using the P4 HAL, since this bridge's DSI interface is receive-only.

The module datasheet does not specify the internal LVDS color mapping or sync
polarity. Initial settings use **VESA / Format 2, positive DE, negative HS/VS**.
If an image has correct geometry but incorrect colors, try **Use JEIDA / LVDS
Format 1** in the display menu. A stable solid-blue image has been confirmed on
hardware with these settings; full dashboard color accuracy still needs visual
verification. The boot logs report the selected format.

Expected boot log stages:

```text
GPIO27 PWR_DN=0 (asserted LOW); GPIO26 backlight OFF
MIPI DSI init: 2 lanes, 864 Mbps/lane, continuous HS clock
GPIO27 PWR_DN=1 (released HIGH)
I2C init: BSP bus, SCL=GPIO8 SDA=GPIO7, 100 kHz
SN65DSI83 detected at 0x2c, revision ...
Bridge configuration verified; PLL locked, 2 lanes, RGB888, LVDS Format 2
DPI init: 1280x800, pixel clock ..., totals 1440x838, RGB565 -> RGB888
DSI video started; two RGB565 framebuffers (2048000 bytes each)
SN65DSI83 post-video status 0xE5=0x00
Framebuffer/LVGL display started; GPIO26 PWM=HIGH (100% backlight)
UI loaded (CAN: GPIO4=TX GPIO5=RX; panel=GPIO2 trip_reset=GPIO3)
```

If neither 0x2D nor 0x2C acknowledges, check common ground, VREF, PWR_DN and
I2C wiring. The failure log includes P4-side SCL/SDA and PWR_DN levels; these
do not establish continuity to the Riverdi connector. PWR_DN is pulled low
again after failure.
For PLL failure, check the DSI clock pair and continuous-HS setup. For
0xE5 errors, retain the complete log (decoded sync/CRC/ECC/LLP/SoT/PLL flags).
Do not treat successful I2C configuration alone as proof of working video.

Sources: [Riverdi module datasheet](https://download.riverdi.com/RVT121HVDFWN00/DS_RVT121HVDFWN00_Rev.1.0.pdf),
[TI SN65DSI83 datasheet, sections 7.4.2 and 7.6](https://www.ti.com/lit/ds/symlink/sn65dsi83.pdf).

## Prerequisites (toolchain)

- ESP-IDF **6.0+** (stock setup used 6.0.1; Riverdi firmware build verified with 6.1.0)
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
