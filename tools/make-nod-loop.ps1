# Cuts the 5x3 nod sheet (15 frames, 582x964 cells) into assets/nod_loop.png: 375x463 cells, 9 per row,
# every frame scaled by the same amount so the duck matches the idle loop's size, feet on y=458, centred on the frames' shared box.
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System; using System.Drawing; using System.Drawing.Imaging; using System.Runtime.InteropServices;
public static class Bbox {
  public static int[] Of(Bitmap b, int x0, int y0, int w, int h) {
    var d = b.LockBits(new Rectangle(0,0,b.Width,b.Height), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
    byte[] px = new byte[d.Stride * b.Height]; Marshal.Copy(d.Scan0, px, 0, px.Length); b.UnlockBits(d);
    int minx = w, miny = h, maxx = -1, maxy = -1;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
      if (px[(y0+y)*d.Stride + (x0+x)*4 + 3] > 24) { if (x<minx) minx=x; if (x>maxx) maxx=x; if (y<miny) miny=y; if (y>maxy) maxy=y; }
    return new int[]{minx,miny,maxx,maxy};
  }
}
'@ -ReferencedAssemblies System.Drawing
$assets = 'D:\Claude Projects\The Ducker VST\assets'
$CW = 582; $CH = 964; $cols = 5; $n = 15
$W = 375; $H = 463; $oc = 9; $feet = 458; $targetH = 440
$src = [System.Drawing.Bitmap]::FromFile("$assets\nod_layers_spritesheet.png")
$ux0 = 9999; $uy0 = 9999; $ux1 = -1; $uy1 = -1
for ($i = 0; $i -lt $n; $i++) {
  $b = [Bbox]::Of($src, ($i % $cols) * $CW, [math]::Floor($i / $cols) * $CH, $CW, $CH)
  $ux0 = [math]::Min($ux0, $b[0]); $uy0 = [math]::Min($uy0, $b[1]); $ux1 = [math]::Max($ux1, $b[2]); $uy1 = [math]::Max($uy1, $b[3])
}
$bw = $ux1 - $ux0 + 1; $bh = $uy1 - $uy0 + 1
$s = $targetH / $bh
"union box $bw x $bh at ($ux0,$uy0); scale $([math]::Round($s,3)); scaled width $([math]::Round($bw*$s))"
$rows = [math]::Ceiling($n / $oc)
$out = New-Object System.Drawing.Bitmap ($oc * $W), ($rows * $H), ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($out)
$g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$dw = [int][math]::Round($bw * $s); $dh = [int][math]::Round($bh * $s)
$dx = [int][math]::Floor(($W - $dw) / 2); $dy = $feet - $dh + 5
for ($i = 0; $i -lt $n; $i++) {
  $sr = New-Object System.Drawing.Rectangle ((($i % $cols) * $CW) + $ux0), (([math]::Floor($i / $cols) * $CH) + $uy0), $bw, $bh
  $dr = New-Object System.Drawing.Rectangle ((($i % $oc) * $W) + $dx), (([math]::Floor($i / $oc) * $H) + $dy), $dw, $dh
  $g.DrawImage($src, $dr, $sr, [System.Drawing.GraphicsUnit]::Pixel)
}
$g.Dispose(); $src.Dispose()
$out.Save("$assets\nod_loop.png", [System.Drawing.Imaging.ImageFormat]::Png); $out.Dispose()
"$n frames, $($oc*$W) x $($rows*$H), $([math]::Round((Get-Item "$assets\nod_loop.png").Length/1MB,2)) MB"
