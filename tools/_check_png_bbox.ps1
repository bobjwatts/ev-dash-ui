Add-Type -AssemblyName System.Drawing

function Get-AlphaBBox {
    param([string]$Path)
    $bmp = [System.Drawing.Bitmap]::FromFile($Path)
    $w = $bmp.Width
    $h = $bmp.Height
    $minX = $w
    $minY = $h
    $maxX = -1
    $maxY = -1
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            if ($bmp.GetPixel($x, $y).A -gt 0) {
                if ($x -lt $minX) { $minX = $x }
                if ($y -lt $minY) { $minY = $y }
                if ($x -gt $maxX) { $maxX = $x }
                if ($y -gt $maxY) { $maxY = $y }
            }
        }
    }
    $strideTest = $null
    $rect = [System.Drawing.Rectangle]::new(0, 0, $w, $h)
    $data = $bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $strideTest = $data.Stride
    $bmp.UnlockBits($data)
    $bmp.Dispose()
    [PSCustomObject]@{
        File = Split-Path $Path -Leaf
        Size = "${w}x${h}"
        BBox = "$minX,$minY..$maxX,$maxY"
        LockBitsStride = $strideTest
        TightStride = ($w * 4)
    }
}

$root = Join-Path $PSScriptRoot "..\ui\images"
foreach ($name in @('sc_dial_speed_face.png', 'sc_dial_speed_face_border.png', '_debug_dial_full.png')) {
    Get-AlphaBBox (Join-Path $root $name)
}
