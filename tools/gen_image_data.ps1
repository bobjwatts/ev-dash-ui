# Generate LVGL 9 image C sources from sc_* PNGs at native resolution (no scaling).
#   sc_dial_speed_face.png         409x409  -> composited into dial_speed_dial
#   sc_dial_speed_face_border.png  439x439  -> composited into dial_speed_dial (RGB565 opaque)
#   sc_dial_speed_needle.png       native   -> dial_speed_needle (RGB565A8)
param(
    [string]$UiRoot = (Join-Path $PSScriptRoot "..\ui"),
    # Bright red flatten background shows exact bitmap bounds on the LCD.
    [switch]$DialDebugRedBg = $true,
    # Embed at half native; P4 LVGL draws ~2x so 220px embed ≈ 440px on screen in a 439 widget.
    [int]$DialEmbedPx = 220
)

Add-Type -AssemblyName System.Drawing

function Format-HexBytes {
    param([byte[]]$Bytes, [int]$Offset, [int]$Count)
    $sb = [System.Text.StringBuilder]::new($Count * 5)
    for ($i = 0; $i -lt $Count; $i++) {
        if ($i -gt 0) { [void]$sb.Append(',') }
        [void]$sb.AppendFormat('0x{0:x2}', $Bytes[$Offset + $i])
    }
    return $sb.ToString()
}

function Read-BitmapArgb {
    param([System.Drawing.Bitmap]$Bmp)

    $w = $Bmp.Width
    $h = $Bmp.Height
    $rect = [System.Drawing.Rectangle]::new(0, 0, $w, $h)
    $data = $Bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $argbBytes = New-Object byte[] ($w * $h * 4)
    $scan0 = $data.Scan0.ToInt64()
    for ($y = 0; $y -lt $h; $y++) {
        $rowPtr = [IntPtr]($scan0 + ($y * $data.Stride))
        [Runtime.InteropServices.Marshal]::Copy($rowPtr, $argbBytes, ($y * $w * 4), ($w * 4))
    }
    $Bmp.UnlockBits($data)
    return @{ Width = $w; Height = $h; Argb = $argbBytes }
}

function Write-Rgb565RoundtripPng {
    param(
        [byte[]]$Rgb565Bytes,
        [int]$Width,
        [int]$Height,
        [int]$Stride,
        [string]$OutPath
    )

    $bmp = New-Object System.Drawing.Bitmap $Width, $Height, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    for ($y = 0; $y -lt $Height; $y++) {
        for ($x = 0; $x -lt $Width; $x++) {
            $co = ($y * $Stride) + ($x * 2)
            $lo = $Rgb565Bytes[$co]
            $hi = $Rgb565Bytes[$co + 1]
            $rgb565 = $lo -bor ($hi -shl 8)
            $r = (($rgb565 -shr 11) -band 0x1F) * 255 / 31
            $g = (($rgb565 -shr 5) -band 0x3F) * 255 / 63
            $b = ($rgb565 -band 0x1F) * 255 / 31
            $bmp.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $r, $g, $b))
        }
    }
    $bmp.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

function Sanitize-UnpremultipliedArgb {
    param([byte[]]$ArgbBytes, [int]$Width, [int]$Height)

    $out = [byte[]]::new($ArgbBytes.Length)
    [Array]::Copy($ArgbBytes, $out, $ArgbBytes.Length)
    for ($y = 0; $y -lt $Height; $y++) {
        for ($x = 0; $x -lt $Width; $x++) {
            $i = ($y * $Width + $x) * 4
            if ($out[$i + 3] -eq 0) {
                $out[$i] = 0
                $out[$i + 1] = 0
                $out[$i + 2] = 0
            }
        }
    }
    return $out
}

function Convert-ArgbToRgb565A8 {
    param([byte[]]$Argb, [int]$Width, [int]$Height)

    $stride = $Width * 2
    $colorBytes = $stride * $Height
    $alphaBytes = $Width * $Height
    $out = New-Object byte[] ($colorBytes + $alphaBytes)

    for ($y = 0; $y -lt $Height; $y++) {
        for ($x = 0; $x -lt $Width; $x++) {
            $i = ($y * $Width + $x) * 4
            $b = $Argb[$i]
            $g = $Argb[$i + 1]
            $r = $Argb[$i + 2]
            $a = $Argb[$i + 3]
            $r5 = ($r -shr 3) -band 0x1F
            $g6 = ($g -shr 2) -band 0x3F
            $b5 = ($b -shr 3) -band 0x1F
            $rgb565 = ($r5 -shl 11) -bor ($g6 -shl 5) -bor $b5
            $co = ($y * $stride) + ($x * 2)
            $out[$co] = $rgb565 -band 0xFF
            $out[$co + 1] = ($rgb565 -shr 8) -band 0xFF
            $ao = $colorBytes + ($y * $Width) + $x
            $out[$ao] = $a
        }
    }

    return @{ Bytes = $out; Stride = $stride; Width = $Width; Height = $Height }
}

function Convert-ArgbToRgb565Opaque {
    param(
        [byte[]]$Argb,
        [int]$Width,
        [int]$Height,
        [int]$BgR = 0x1F,
        [int]$BgG = 0x1F,
        [int]$BgB = 0x24
    )

    $stride = $Width * 2
    $out = New-Object byte[] ($stride * $Height)

    for ($y = 0; $y -lt $Height; $y++) {
        for ($x = 0; $x -lt $Width; $x++) {
            $i = ($y * $Width + $x) * 4
            $b = $Argb[$i]
            $g = $Argb[$i + 1]
            $r = $Argb[$i + 2]
            $a = $Argb[$i + 3]
            if ($a -lt 255) {
                $na = $a / 255.0
                $r = [int]($r * $na + $BgR * (1.0 - $na))
                $g = [int]($g * $na + $BgG * (1.0 - $na))
                $b = [int]($b * $na + $BgB * (1.0 - $na))
            }
            $r5 = ($r -shr 3) -band 0x1F
            $g6 = ($g -shr 2) -band 0x3F
            $b5 = ($b -shr 3) -band 0x1F
            $rgb565 = ($r5 -shl 11) -bor ($g6 -shl 5) -bor $b5
            $co = ($y * $stride) + ($x * 2)
            $out[$co] = $rgb565 -band 0xFF
            $out[$co + 1] = ($rgb565 -shr 8) -band 0xFF
        }
    }

    return @{ Bytes = $out; Stride = $stride; Width = $Width; Height = $Height }
}

function Write-LvglImageMapC {
    param(
        [string]$OutPath,
        [string]$Symbol,
        [string]$Comment,
        [byte[]]$Bytes,
        [int]$Width,
        [int]$Height,
        [int]$Stride,
        [string]$ColorFormat
    )

    $guard = $Symbol.ToUpper()
    $writer = [IO.StreamWriter]::new($OutPath, $false, [Text.UTF8Encoding]::new($false))
    try {
        $writer.WriteLine("/* $Comment */")
        $writer.WriteLine('')
        $writer.WriteLine('#ifdef LV_LVGL_H_INCLUDE_SIMPLE')
        $writer.WriteLine('#include "lvgl.h"')
        $writer.WriteLine('#else')
        $writer.WriteLine('#include "lvgl/lvgl.h"')
        $writer.WriteLine('#endif')
        $writer.WriteLine('')
        $writer.WriteLine("#ifndef $guard")
        $writer.WriteLine("#define $guard 1")
        $writer.WriteLine('#endif')
        $writer.WriteLine('')
        $writer.WriteLine("#if $guard")
        $writer.WriteLine('')
        $writer.WriteLine("const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t ${Symbol}_map[] = {")

        $rowSize = 64
        for ($offset = 0; $offset -lt $Bytes.Length; $offset += $rowSize) {
            $count = [Math]::Min($rowSize, $Bytes.Length - $offset)
            $row = Format-HexBytes -Bytes $Bytes -Offset $offset -Count $count
            $writer.WriteLine("    $row,")
        }

        $writer.WriteLine('};')
        $writer.WriteLine('')
        $writer.WriteLine("const lv_image_dsc_t $Symbol = {")
        $writer.WriteLine('    .header = {')
        $writer.WriteLine('        .magic = LV_IMAGE_HEADER_MAGIC,')
        $writer.WriteLine("        .cf = $ColorFormat,")
        $writer.WriteLine("        .w = $Width,")
        $writer.WriteLine("        .h = $Height,")
        $writer.WriteLine("        .stride = $Stride,")
        $writer.WriteLine('    },')
        $writer.WriteLine("    .data_size = sizeof(${Symbol}_map),")
        $writer.WriteLine("    .data = ${Symbol}_map,")
        $writer.WriteLine('};')
        $writer.WriteLine('')
        $writer.WriteLine('#endif')
        $writer.WriteLine('')
    }
    finally {
        $writer.Close()
    }
}

function Write-LvglImageC {
    param(
        [string]$PngPath,
        [string]$OutPath,
        [string]$Symbol
    )

    $srcBmp = [System.Drawing.Bitmap]::FromFile($PngPath)
    try {
        $frame = Read-BitmapArgb -Bmp $srcBmp
        $w = $frame.Width
        $h = $frame.Height
        $rgb565a8 = Convert-ArgbToRgb565A8 -Argb (Sanitize-UnpremultipliedArgb -ArgbBytes $frame.Argb -Width $w -Height $h) -Width $w -Height $h
    }
    finally {
        $srcBmp.Dispose()
    }

    Write-LvglImageMapC `
        -OutPath $OutPath `
        -Symbol $Symbol `
        -Comment "Generated from $([IO.Path]::GetFileName($PngPath)) at native resolution" `
        -Bytes $rgb565a8.Bytes `
        -Width $w `
        -Height $h `
        -Stride $rgb565a8.Stride `
        -ColorFormat "LV_COLOR_FORMAT_RGB565A8"

    Write-Host "Generated $OutPath ($w x $h, rgb565a8)"
}

function Write-CompositeDialC {
    param(
        [string]$FacePng,
        [string]$BorderPng,
        [string]$OutPath,
        [string]$Symbol,
        [string]$DebugDir,
        [int]$EmbedPx,
        [int]$FlattenBgR = 0x1F,
        [int]$FlattenBgG = 0x1F,
        [int]$FlattenBgB = 0x24,
        [string]$FlattenBgLabel = "COLOR_BG"
    )

    $faceSrc = [System.Drawing.Bitmap]::FromFile($FacePng)
    $borderSrc = [System.Drawing.Bitmap]::FromFile($BorderPng)
    $canvas = $null
    try {
        $w = [Math]::Max($faceSrc.Width, $borderSrc.Width)
        $h = [Math]::Max($faceSrc.Height, $borderSrc.Height)

        $canvas = New-Object System.Drawing.Bitmap $w, $h, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $g = [System.Drawing.Graphics]::FromImage($canvas)
        try {
            $g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceOver
            $g.Clear([System.Drawing.Color]::FromArgb(255, $FlattenBgR, $FlattenBgG, $FlattenBgB))
            $g.DrawImage($faceSrc, ($w - $faceSrc.Width) / 2, ($h - $faceSrc.Height) / 2)
            $g.DrawImage($borderSrc, ($w - $borderSrc.Width) / 2, ($h - $borderSrc.Height) / 2)
        }
        finally {
            $g.Dispose()
        }

        $displayW = $w
        $displayH = $h

        if ($DebugDir) {
            [IO.Directory]::CreateDirectory($DebugDir) | Out-Null
            $canvas.Save((Join-Path $DebugDir "_debug_dial_full.png"), [System.Drawing.Imaging.ImageFormat]::Png)
        }

        if ($EmbedPx -gt 0 -and ($EmbedPx -ne $w -or $EmbedPx -ne $h)) {
            $scaled = New-Object System.Drawing.Bitmap $EmbedPx, $EmbedPx, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $sg = [System.Drawing.Graphics]::FromImage($scaled)
            try {
                $sg.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $sg.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $sg.DrawImage($canvas, 0, 0, $EmbedPx, $EmbedPx)
            }
            finally {
                $sg.Dispose()
            }
            $canvas.Dispose()
            $canvas = $scaled
            $w = $EmbedPx
            $h = $EmbedPx
            if ($DebugDir) {
                $canvas.Save((Join-Path $DebugDir "_debug_dial_embed.png"), [System.Drawing.Imaging.ImageFormat]::Png)
            }
        }

        $frame = Read-BitmapArgb -Bmp $canvas
        $argb = Sanitize-UnpremultipliedArgb -ArgbBytes $frame.Argb -Width $w -Height $h
        $rgb565a8 = Convert-ArgbToRgb565A8 -Argb $argb -Width $w -Height $h

        if ($DebugDir) {
            Write-Rgb565RoundtripPng -Rgb565Bytes $rgb565a8.Bytes[0..($rgb565a8.Stride * $h - 1)] -Width $w -Height $h -Stride $rgb565a8.Stride `
                -OutPath (Join-Path $DebugDir "_debug_dial_roundtrip.png")
        }
    }
    finally {
        if ($canvas) { $canvas.Dispose() }
        $faceSrc.Dispose()
        $borderSrc.Dispose()
    }

    Write-LvglImageMapC `
        -OutPath $OutPath `
        -Symbol $Symbol `
        -Comment "Generated composite dial (face + border), RGB565A8 on $FlattenBgLabel, embed ${w}x${h} for ${displayW}px display" `
        -Bytes $rgb565a8.Bytes `
        -Width $w `
        -Height $h `
        -Stride $rgb565a8.Stride `
        -ColorFormat "LV_COLOR_FORMAT_RGB565A8"

    Write-Host "Generated $OutPath (embed $w x $h rgb565a8, on-screen target ${displayW}x${displayH})"
    return @{ DisplayW = $displayW; DisplayH = $displayH; EmbedW = $w; EmbedH = $h }
}

$facePng = Join-Path $UiRoot "images\sc_dial_speed_face.png"
$borderPng = Join-Path $UiRoot "images\sc_dial_speed_face_border.png"
$needlePng = Join-Path $UiRoot "images\sc_dial_speed_needle.png"

$dialFlattenBgR = 0x1F
$dialFlattenBgG = 0x1F
$dialFlattenBgB = 0x24
$dialFlattenBgLabel = "COLOR_BG"
$dialAssetsSuffix = "${DialEmbedPx}embed-439disp-rgb565a8"
if ($DialDebugRedBg) {
    $dialFlattenBgR = 0xFF
    $dialFlattenBgG = 0x00
    $dialFlattenBgB = 0x00
    $dialFlattenBgLabel = "bright red (bounds debug)"
    $dialAssetsSuffix = "${DialEmbedPx}embed-439disp-rgb565a8-redbg-debug"
    Write-Host "Dial flatten background: bright red (bounds debug)"
}
Write-Host "Dial embed: ${DialEmbedPx}px -> on-screen 439px (P4 draw workaround)"

$dialSizes = Write-CompositeDialC `
    -FacePng $facePng `
    -BorderPng $borderPng `
    -OutPath (Join-Path $UiRoot "images\dial_speed_dial_data.c") `
    -Symbol "dial_speed_dial_data" `
    -DebugDir (Join-Path $UiRoot "images") `
    -EmbedPx $DialEmbedPx `
    -FlattenBgR $dialFlattenBgR `
    -FlattenBgG $dialFlattenBgG `
    -FlattenBgB $dialFlattenBgB `
    -FlattenBgLabel $dialFlattenBgLabel

Write-LvglImageC `
    -PngPath $needlePng `
    -OutPath (Join-Path $UiRoot "images\dial_speed_needle_data.c") `
    -Symbol "dial_speed_needle_data"

$assetsHeader = Join-Path $UiRoot "images\ev_dash_assets.h"
$faceBmp = [System.Drawing.Bitmap]::FromFile($facePng)
$borderBmp = [System.Drawing.Bitmap]::FromFile($borderPng)
$needleBmp = [System.Drawing.Bitmap]::FromFile($needlePng)
try {
    $dialDisplayW = $dialSizes.DisplayW
    $dialDisplayH = $dialSizes.DisplayH
    $dialEmbedW = $dialSizes.EmbedW
    $dialEmbedH = $dialSizes.EmbedH
    $needleW = $needleBmp.Width
    $needleH = $needleBmp.Height

    $headerWriter = [IO.StreamWriter]::new($assetsHeader, $false, [Text.UTF8Encoding]::new($false))
    try {
        $headerWriter.WriteLine('/* Generated by tools/gen_image_data.ps1 — do not edit */')
        $headerWriter.WriteLine('#ifndef EV_DASH_ASSETS_H')
        $headerWriter.WriteLine('#define EV_DASH_ASSETS_H')
        $headerWriter.WriteLine('')
        $headerWriter.WriteLine("#define EV_DASH_DIAL_DISPLAY_W $dialDisplayW")
        $headerWriter.WriteLine("#define EV_DASH_DIAL_DISPLAY_H $dialDisplayH")
        $headerWriter.WriteLine("#define EV_DASH_DIAL_EMBED_W $dialEmbedW")
        $headerWriter.WriteLine("#define EV_DASH_DIAL_EMBED_H $dialEmbedH")
        $headerWriter.WriteLine("#define EV_DASH_DIAL_W $dialEmbedW")
        $headerWriter.WriteLine("#define EV_DASH_DIAL_H $dialEmbedH")
        $headerWriter.WriteLine("#define EV_DASH_NEEDLE_W $needleW")
        $headerWriter.WriteLine("#define EV_DASH_NEEDLE_H $needleH")
        $headerWriter.WriteLine("#define EV_DASH_ASSETS_ID `"$dialAssetsSuffix`"")
        $headerWriter.WriteLine('')
        $headerWriter.WriteLine('#endif')
        $headerWriter.WriteLine('')
    }
    finally {
        $headerWriter.Close()
    }
    Write-Host "Generated $assetsHeader"
}
finally {
    $faceBmp.Dispose()
    $borderBmp.Dispose()
    $needleBmp.Dispose()
}

Write-Host ""
Write-Host "Embedded at native sc_ sizes (no upscale):"
foreach ($path in @($facePng, $borderPng, $needlePng)) {
    $b = [System.Drawing.Bitmap]::FromFile($path)
    Write-Host ("  {0,-30} {1}x{2}" -f [IO.Path]::GetFileName($path), $b.Width, $b.Height)
    $b.Dispose()
}
