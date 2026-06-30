Add-Type -AssemblyName System.Drawing

$path = Join-Path $PSScriptRoot "..\ui\images\_debug_dial_full.png"
$bmp = [System.Drawing.Bitmap]::FromFile($path)
$w = $bmp.Width
$h = $bmp.Height
$minX = $w; $minY = $h; $maxX = -1; $maxY = -1
$nonRed = 0
for ($y = 0; $y -lt $h; $y++) {
    for ($x = 0; $x -lt $w; $x++) {
        $c = $bmp.GetPixel($x, $y)
        if ($c.R -lt 250 -or $c.G -gt 5 -or $c.B -gt 5) {
            $nonRed++
            if ($x -lt $minX) { $minX = $x }
            if ($y -lt $minY) { $minY = $y }
            if ($x -gt $maxX) { $maxX = $x }
            if ($y -gt $maxY) { $maxY = $y }
        }
    }
}
$bmp.Dispose()
$cx = ($minX + $maxX) / 2.0
$cy = ($minY + $maxY) / 2.0
Write-Output "canvas ${w}x${h}"
Write-Output "non-red bbox: $minX,$minY..$maxX,$maxY"
Write-Output "non-red center: $cx , $cy (canvas center: $($w/2), $($h/2))"
Write-Output "non-red pixels: $nonRed"
