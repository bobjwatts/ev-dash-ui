# Regenerate Big Shoulders role fonts for LVGL.
# ASCII + degree (U+00B0) + blinker triangles (U+25C4 / U+25BA) per Figma dash design.
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Font = Join-Path $Root "ui\fonts\BigShoulders-Bold.ttf"
$OutDir = Join-Path $Root "ui\fonts"
# ASCII + degree (U+00B0) + blinker chevrons (U+2039/U+203A; Big Shoulders has no ◄/►)
$Range = "0x20-0x7F,0xB0,0x2039,0x203A"

$Sizes = @(
    @{ Name = "font_small";   Size = 12 },
    @{ Name = "font_body";    Size = 16 },
    @{ Name = "font_subhead"; Size = 18 },
    @{ Name = "font_heading"; Size = 25 },
    @{ Name = "font_display"; Size = 54 }
)

foreach ($f in $Sizes) {
    $out = Join-Path $OutDir "$($f.Name)_data.c"
    Write-Host "Generating $($f.Name) ($($f.Size) px) -> $out"
    npx --yes lv_font_conv `
        --font $Font `
        -o $out `
        --size $f.Size `
        --bpp 4 `
        --format lvgl `
        --no-compress `
        --range $Range
}

Write-Host "Done."
