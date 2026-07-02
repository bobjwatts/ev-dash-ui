# Convert dial/needle/arc-mask PNGs to LVGL C sources using LVGLImage.py.
# Single-PNG workflow: sc_dial_speed_face.png is the complete finished dial art.
#
# Usage (from repo root):
#   .\tools\gen_image_data_lvgl.ps1
#   .\tools\gen_image_data_lvgl.ps1 -DialCf RGB565   # opaque dial, bakes in background colour
#
param(
    [string]$UiRoot = (Join-Path $PSScriptRoot "..\ui"),
    [ValidateSet("RGB565", "RGB565A8")]
    [string]$DialCf = "RGB565A8"
)

$ErrorActionPreference = "Stop"

# ── Python ───────────────────────────────────────────────────────────────────
$PythonCandidates = @(
    "C:\Espressif\tools\python\v6.0.1\venv\Scripts\python.exe",
    "python3",
    "python"
)
$Python = $null
foreach ($candidate in $PythonCandidates) {
    if ($candidate -match '^python') {
        $cmd = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($cmd) { $Python = $cmd.Source; break }
    } elseif (Test-Path $candidate) {
        $Python = $candidate; break
    }
}
if (-not $Python) {
    throw "Python not found. Install ESP-IDF Python or add python to PATH."
}

$LvglImagePy = Join-Path $PSScriptRoot "..\firmware\managed_components\lvgl__lvgl\scripts\LVGLImage.py"
if (-not (Test-Path $LvglImagePy)) {
    throw "LVGLImage.py not found at $LvglImagePy - run 'idf.py build' once to fetch the lvgl component."
}

try { & $Python -m pip install pypng lz4 -q --disable-pip-version-check *>$null } catch {}

# ── Source PNGs ───────────────────────────────────────────────────────────────
$dialPng   = Join-Path $UiRoot "images\sc_dial_speed_face.png"
$needlePng = Join-Path $UiRoot "images\sc_dial_speed_needle.png"
$maskPng   = Join-Path $UiRoot "images\sc_dial_speed_arc_mask.png"
$bgPng     = Join-Path $UiRoot "images\background.png"

foreach ($f in @($dialPng, $needlePng, $maskPng)) {
    if (-not (Test-Path $f)) { throw "Missing source PNG: $f" }
}

# ── Temp output dir ───────────────────────────────────────────────────────────
$outDir = Join-Path $UiRoot "images\_lvgl_conv_out"
if (Test-Path $outDir) { Remove-Item $outDir -Recurse -Force }
New-Item -ItemType Directory -Path $outDir | Out-Null

# ── Convert dial ──────────────────────────────────────────────────────────────
if ($DialCf -eq "RGB565") {
    Write-Host "Step 1: LVGLImage.py dial (RGB565, dithered, background 0x1F1F24 = COLOR_BG) ..."
    & $Python $LvglImagePy `
        --ofmt=C --cf=RGB565 --compress=NONE `
        --rgb565dither --background=0x1F1F24 `
        -o $outDir --name=dial_speed_dial_data `
        $dialPng -v
} else {
    Write-Host "Step 1: LVGLImage.py dial (RGB565A8, dithered) ..."
    & $Python $LvglImagePy `
        --ofmt=C --cf=RGB565A8 --compress=NONE `
        --rgb565dither `
        -o $outDir --name=dial_speed_dial_data `
        $dialPng -v
}

# ── Convert needle ────────────────────────────────────────────────────────────
Write-Host "Step 2: LVGLImage.py needle (RGB565A8) ..."
& $Python $LvglImagePy `
    --ofmt=C --cf=RGB565A8 --compress=NONE `
    -o $outDir --name=dial_speed_needle_data `
    $needlePng -v

# ── Convert arc mask ──────────────────────────────────────────────────────────
Write-Host "Step 3: LVGLImage.py arc mask (RGB565A8) ..."
& $Python $LvglImagePy `
    --ofmt=C --cf=RGB565A8 --compress=NONE `
    -o $outDir --name=dial_speed_arc_mask_data `
    $maskPng -v

# ── Convert background (optional — skip if not present) ───────────────────────
if (Test-Path $bgPng) {
    Write-Host "Step 4: LVGLImage.py background (RGB565, dithered) ..."
    & $Python $LvglImagePy `
        --ofmt=C --cf=RGB565 --compress=NONE --rgb565dither `
        --background=0x1F1F24 `
        -o $outDir --name=background_data `
        $bgPng -v
}

# ── Install outputs ───────────────────────────────────────────────────────────
$imgDir = Join-Path $UiRoot "images"
Copy-Item (Join-Path $outDir "dial_speed_dial_data.c")     (Join-Path $imgDir "dial_speed_dial_data.c")     -Force
Copy-Item (Join-Path $outDir "dial_speed_needle_data.c")   (Join-Path $imgDir "dial_speed_needle_data.c")   -Force
Copy-Item (Join-Path $outDir "dial_speed_arc_mask_data.c") (Join-Path $imgDir "dial_speed_arc_mask_data.c") -Force
if (Test-Path $bgPng) {
    Copy-Item (Join-Path $outDir "background_data.c") (Join-Path $imgDir "background_data.c") -Force
}
Remove-Item $outDir -Recurse -Force

$installed = "dial_speed_dial_data.c, dial_speed_needle_data.c, dial_speed_arc_mask_data.c"
if (Test-Path $bgPng) { $installed += ", background_data.c" }
Write-Host "Installed $installed"

# ── Read dimensions for assets header ─────────────────────────────────────────
Add-Type -AssemblyName System.Drawing
$dialBmp   = [System.Drawing.Bitmap]::FromFile($dialPng)
$needleBmp = [System.Drawing.Bitmap]::FromFile($needlePng)
try {
    $dialW   = $dialBmp.Width;   $dialH   = $dialBmp.Height
    $needleW = $needleBmp.Width; $needleH = $needleBmp.Height
} finally {
    $dialBmp.Dispose(); $needleBmp.Dispose()
}

$cfTag  = $DialCf.ToLower()
$assetId = "lvgl-${dialW}-${cfTag}"

$assetsHeader = Join-Path $imgDir "ev_dash_assets.h"
$hw = [IO.StreamWriter]::new($assetsHeader, $false, [Text.UTF8Encoding]::new($false))
try {
    $hw.WriteLine('/* Generated by tools/gen_image_data_lvgl.ps1 — do not edit */')
    $hw.WriteLine('#ifndef EV_DASH_ASSETS_H')
    $hw.WriteLine('#define EV_DASH_ASSETS_H')
    $hw.WriteLine('')
    $hw.WriteLine("#define EV_DASH_DIAL_DISPLAY_W  $dialW")
    $hw.WriteLine("#define EV_DASH_DIAL_DISPLAY_H  $dialH")
    $hw.WriteLine("#define EV_DASH_DIAL_EMBED_W    $dialW")
    $hw.WriteLine("#define EV_DASH_DIAL_EMBED_H    $dialH")
    $hw.WriteLine("#define EV_DASH_DIAL_W          $dialW")
    $hw.WriteLine("#define EV_DASH_DIAL_H          $dialH")
    $hw.WriteLine("#define EV_DASH_DIAL_CF         `"$DialCf`"")
    $hw.WriteLine("#define EV_DASH_NEEDLE_W        $needleW")
    $hw.WriteLine("#define EV_DASH_NEEDLE_H        $needleH")
    $hw.WriteLine("#define EV_DASH_ASSETS_ID       `"$assetId`"")
    $hw.WriteLine('')
    $hw.WriteLine('#endif')
    $hw.WriteLine('')
} finally {
    $hw.Close()
}

Write-Host "Generated ev_dash_assets.h (id=$assetId dial=${dialW}x${dialH} needle=${needleW}x${needleH})"
Write-Host ""
Write-Host "Next: cd firmware; idf.py build flash monitor"
