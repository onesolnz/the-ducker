Add-Type -AssemblyName System.Drawing
$assets = 'D:\Claude Projects\The Ducker VST\assets'
$W = 375; $H = 463; $cols = 9
# sheet, cell w, cell h, dx, dy (feet onto y=458, centred), source frames
$parts = @(
  @('tier1', 244, 454, 58, 9, @(0..19 | ForEach-Object { $_ * 2 })),          # E: 0-38 every 2nd
  @('tier2', 375, 463, 18, 1, @(0..35 | ForEach-Object { $_ * 2 })),          # H: 0-70 every 2nd
  @('tier3', 375, 463,  0, 9, @(0..23 | ForEach-Object { 23 + $_ * 2 }))      # M: 23-69 every 2nd
)
$total = 0; foreach ($p in $parts) { $total += $p[5].Count }
$rows = [math]::Ceiling($total / $cols)
$out = New-Object System.Drawing.Bitmap ($cols * $W), ($rows * $H), ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($out)
$g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$k = 0
foreach ($p in $parts) {
  $src = [System.Drawing.Bitmap]::FromFile("$assets\$($p[0])_spritesheet_1x.png")
  foreach ($i in $p[5]) {
    $sr = New-Object System.Drawing.Rectangle (($i % 9) * $p[1]), ([math]::Floor($i / 9) * $p[2]), $p[1], $p[2]
    # clip the shifted cell to its destination cell so nothing spills into a neighbour
    $dx = $p[3]; $dy = $p[4]
    $cw = [math]::Min($p[1], $W - $dx); $ch = [math]::Min($p[2], $H - $dy)
    $sr.Width = $cw; $sr.Height = $ch
    $dr = New-Object System.Drawing.Rectangle ((($k % $cols) * $W) + $dx), (([math]::Floor($k / $cols) * $H) + $dy), $cw, $ch
    $g.DrawImage($src, $dr, $sr, [System.Drawing.GraphicsUnit]::Pixel)
    $k++
  }
  $src.Dispose()
}
$g.Dispose()
$dest = "$assets\duck_loops.png"
$out.Save($dest, [System.Drawing.Imaging.ImageFormat]::Png)
$out.Dispose()
"$k frames, $($cols*$W) x $($rows*$H), $([math]::Round((Get-Item $dest).Length/1MB,2)) MB"
