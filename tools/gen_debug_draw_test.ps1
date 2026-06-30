# Solid test images via official LVGLImage.py (draw-path isolation).
param(
    [string]$UiRoot = (Join-Path $PSScriptRoot "..\ui")
)

$ErrorActionPreference = "Stop"

$Python = "C:\Espressif\tools\python\v6.0.1\venv\Scripts\python.exe"
if (-not (Test-Path $Python)) {
    $cmd = Get-Command python -ErrorAction SilentlyContinue
    if ($cmd) { $Python = $cmd.Source } else { throw "Python not found" }
}

$LvglImagePy = Join-Path $PSScriptRoot "..\firmware\managed_components\lvgl__lvgl\scripts\LVGLImage.py"
if (-not (Test-Path $LvglImagePy)) {
    throw "LVGLImage.py not found - run idf.py build once"
}

& $Python -m pip install pypng lz4 pillow -q 2>$null

$imgDir = Join-Path $UiRoot "images"
$outDir = Join-Path $imgDir "_lvgl_conv_out"
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

$png100 = Join-Path $imgDir "debug_img_100.png"
$png439 = Join-Path $imgDir "debug_img_439.png"
$png439rgb = Join-Path $imgDir "debug_img_439_rgb565.png"

$pySnippet = @"
from PIL import Image
Image.new('RGBA', (100, 100), (255, 0, 255, 255)).save(r'$png100')
Image.new('RGBA', (439, 439), (0, 255, 0, 255)).save(r'$png439')
Image.new('RGBA', (439, 439), (0, 255, 255, 255)).save(r'$png439rgb')
print('wrote debug PNGs')
"@
& $Python -c $pySnippet

$specs = @(
    @{ Png = $png100; Name = "debug_img_100_data"; Cf = "RGB565A8" },
    @{ Png = $png439; Name = "debug_img_439_data"; Cf = "RGB565A8" },
    @{ Png = $png439rgb; Name = "debug_img_439_rgb565_data"; Cf = "RGB565" }
)
foreach ($spec in $specs) {
    & $Python $LvglImagePy `
        --ofmt=C --cf=$($spec.Cf) --compress=NONE `
        -o $outDir --name=$($spec.Name) $spec.Png -v
    Copy-Item (Join-Path $outDir "$($spec.Name).c") (Join-Path $imgDir "$($spec.Name).c") -Force
    Write-Host "Installed $($spec.Name).c ($($spec.Cf))"
}

$assetsHeader = Join-Path $imgDir "ev_dash_assets.h"
$lines = if (Test-Path $assetsHeader) { [System.Collections.Generic.List[string]](Get-Content $assetsHeader) } else { [System.Collections.Generic.List[string]]@() }
if ($lines.Count -eq 0) {
    $lines.Add('/* Generated - do not edit */')
    $lines.Add('#ifndef EV_DASH_ASSETS_H')
    $lines.Add('#define EV_DASH_ASSETS_H')
    $lines.Add('')
}
$filtered = $lines | Where-Object { $_ -notmatch 'EV_DASH_DEBUG_' }
$filtered = $filtered | Where-Object { $_ -ne '#endif' }
$filtered += ''
$filtered += '/* Draw-path test solids (tools/gen_debug_draw_test.ps1) */'
$filtered += '#define EV_DASH_DEBUG_DRAW_TEST 1'
$filtered += '#define EV_DASH_DEBUG_IMG_100_W 100'
$filtered += '#define EV_DASH_DEBUG_IMG_100_H 100'
$filtered += '#define EV_DASH_DEBUG_IMG_439_W 439'
$filtered += '#define EV_DASH_DEBUG_IMG_439_H 439'
$filtered += ''
$filtered += '#endif'
$filtered += ''
[IO.File]::WriteAllLines($assetsHeader, $filtered)
Write-Host "Updated $assetsHeader"
Write-Host "Rebuild firmware. Solid dial test uses cyan RGB565 439x439."
