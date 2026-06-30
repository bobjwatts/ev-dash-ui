Add-Type -AssemblyName System.Drawing
. (Join-Path $PSScriptRoot "gen_image_data.ps1")  # won't work - functions only

# Inline minimal roundtrip test
function Read-BitmapArgb-Buggy {
    param([System.Drawing.Bitmap]$Bmp)
    $w = $Bmp.Width; $h = $Bmp.Height
    $rect = [System.Drawing.Rectangle]::new(0, 0, $w, $h)
    $data = $Bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $argbBytes = New-Object byte[] ($w * $h * 4)
    [Runtime.InteropServices.Marshal]::Copy($data.Scan0, $argbBytes, 0, $argbBytes.Length)
    $Bmp.UnlockBits($data)
    return @{ Width = $w; Height = $h; Argb = $argbBytes; Stride = $data.Stride }
}

function Read-BitmapArgb-Fixed {
    param([System.Drawing.Bitmap]$Bmp)
    $w = $Bmp.Width; $h = $Bmp.Height
    $rect = [System.Drawing.Rectangle]::new(0, 0, $w, $h)
    $data = $Bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $stride = $data.Stride
    $rowBytes = $w * 4
    $argbBytes = New-Object byte[] ($rowBytes * $h)
    for ($y = 0; $y -lt $h; $y++) {
        [Runtime.InteropServices.Marshal]::Copy($data.Scan0 + ($y * $stride), $argbBytes, ($y * $rowBytes), $rowBytes)
    }
    $Bmp.UnlockBits($data)
    return @{ Width = $w; Height = $h; Argb = $argbBytes; Stride = $stride }
}

function Rgb565ToBitmap {
    param([byte[]]$Bytes, [int]$W, [int]$H, [int]$Stride)
    $bmp = New-Object System.Drawing.Bitmap $W, $H, ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    for ($y = 0; $y -lt $H; $y++) {
        for ($x = 0; $x -lt $W; $x++) {
            $co = ($y * $Stride) + ($x * 2)
            $px = $Bytes[$co] -bor ($Bytes[$co + 1] -shl 8)
            $r = (($px -shr 11) -band 0x1F) * 255 / 31
            $g = (($px -shr 5) -band 0x3F) * 255 / 63
            $b = ($px -band 0x1F) * 255 / 31
            $bmp.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($r, $g, $b))
        }
    }
    return $bmp
}

# load convert functions from gen by dot-sourcing partial - duplicate Convert-ArgbToRgb565Opaque minimally
function Convert-ArgbToRgb565Opaque {
    param([byte[]]$Argb, [int]$Width, [int]$Height, [int]$BgR, [int]$BgG, [int]$BgB)
    $stride = $Width * 2
    $out = New-Object byte[] ($stride * $Height)
    for ($y = 0; $y -lt $Height; $y++) {
        for ($x = 0; $x -lt $Width; $x++) {
            $i = ($y * $Width + $x) * 4
            $b = $Argb[$i]; $g = $Argb[$i+1]; $r = $Argb[$i+2]; $a = $Argb[$i+3]
            if ($a -lt 255) {
                $na = $a / 255.0
                $r = [int]($r * $na + $BgR * (1.0 - $na))
                $g = [int]($g * $na + $BgG * (1.0 - $na))
                $b = [int]($b * $na + $BgB * (1.0 - $na))
            }
            $rgb565 = (($r -shr 3) -band 0x1F) -shl 11 -bor (($g -shr 2) -band 0x3F) -shl 5 -bor (($b -shr 3) -band 0x1F)
            $co = ($y * $stride) + ($x * 2)
            $out[$co] = $rgb565 -band 0xFF
            $out[$co+1] = ($rgb565 -shr 8) -band 0xFF
        }
    }
    return $out
}

$path = Join-Path $PSScriptRoot "..\ui\images\_debug_dial_full.png"
$src = [System.Drawing.Bitmap]::FromFile($path)
$buggy = Read-BitmapArgb-Buggy $src
$fixed = Read-BitmapArgb-Fixed $src
Write-Output "LockBits stride: $($buggy.Stride) tight=$($src.Width*4)"
$rgbBug = Convert-ArgbToRgb565Opaque $buggy.Argb 439 439 255 0 0
$rgbFix = Convert-ArgbToRgb565Opaque $fixed.Argb 439 439 255 0 0
$outDir = Join-Path $PSScriptRoot "..\ui\images"
Rgb565ToBitmap $rgbBug 439 439 878 | ForEach-Object { $_.Save((Join-Path $outDir "_debug_roundtrip_buggy.png")); $_.Dispose() }
Rgb565ToBitmap $rgbFix 439 439 878 | ForEach-Object { $_.Save((Join-Path $outDir "_debug_roundtrip_fixed.png")); $_.Dispose() }
$src.Dispose()
Write-Output "Wrote _debug_roundtrip_buggy.png and _debug_roundtrip_fixed.png"
