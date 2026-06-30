$c = Get-Content (Join-Path $PSScriptRoot "..\ui\images\dial_speed_dial_data.c") -Raw
$m = [regex]::Matches($c, '0x[0-9a-f]{2}')
$bytes = New-Object System.Collections.Generic.List[int]
foreach ($x in $m) { [void]$bytes.Add([Convert]::ToInt32($x.Value, 16)) }

$w = 439
$stride = 878
$h = 439
$expectedRgb565 = $w * $h * 2
$expectedRgb565a8 = $expectedRgb565 + ($w * $h)

if ($c -match '\.cf = (LV_COLOR_FORMAT_\w+)') { Write-Output "color format: $($Matches[1])" }

if ($bytes.Count -eq $expectedRgb565a8) {
    Write-Output "total bytes: $($bytes.Count) (RGB565A8 ok)"
} elseif ($bytes.Count -eq $expectedRgb565) {
    Write-Output "total bytes: $($bytes.Count) (RGB565 ok)"
} else {
    Write-Output "total bytes: $($bytes.Count) expected rgb565a8=$expectedRgb565a8 or rgb565=$expectedRgb565"
}

foreach ($row in 0, 1, 218, 219, 437, 438) {
    $off = $row * $stride
    if ($off + $stride -gt $bytes.Count) {
        Write-Output "row $row MISSING (offset $off + $stride > $($bytes.Count))"
        continue
    }
    $first8 = ($bytes.GetRange($off, 8) | ForEach-Object { '{0:x2}' -f $_ }) -join ','
    Write-Output "row $row first8: $first8"
}

# Check header struct in file
if ($c -match '\.stride = (\d+)') { Write-Output "header stride: $($Matches[1])" }
if ($c -match '\.data_size = sizeof') { Write-Output "data_size uses sizeof(map)" }
