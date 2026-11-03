# Drives LogiCim (a native Win32 window) with injected mouse and keyboard input and PrintWindow
# screenshots.  Dot-source it in Windows PowerShell 5.1:  . .claude\skills\run-logicim\driver.ps1
#
# Positions on the circuit canvas are grid units (Logicim's: grid 0,0 is the canvas centre, ten pixels a unit at
# zoom 1).  The canvas layout constants assume the window's default 1280x800 client area; Start-LogiCim checks it.

Add-Type -AssemblyName System.Drawing
if (-not ("LogiCimU" -as [type])) {
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class LogiCimU {
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, int x, int y, uint d, IntPtr e);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint f, IntPtr e);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint type);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
}
"@
}
[LogiCimU]::SetProcessDPIAware() | Out-Null

$LogiCimUnit = (Resolve-Path "$PSScriptRoot\..\..\..\LogiCim").Path
# The canvas centre in client pixels (grid 0,0 at zoom 1, unscrolled) and the left toolbar's zoom buttons.
$LogiCimCentre = @(740, 347)
$LogiCimZoomIn = @(20, 148)
$LogiCimResetZoom = @(20, 224)
# Each tool call is a new shell, so the zoom the driver set is kept in a file, not only in the session.
$LogiCimZoomFile = "$env:TEMP\logicim-driver-zoom.txt"
$Script:LogiCimPixelsPerGrid = if (Test-Path $LogiCimZoomFile) { [int](Get-Content $LogiCimZoomFile) } else { 10 }
function Save-PixelsPerGrid([int]$Pixels) { $Script:LogiCimPixelsPerGrid = $Pixels; Set-Content -Path $LogiCimZoomFile -Value $Pixels }

function Get-LogiCimWindow { (Get-Process "LogiCim" -ErrorAction Stop | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1).MainWindowHandle }

# Launches a built configuration from the project folder (it loads Resources\ relative to it) and waits for the window.
# A LogiCim already running is stopped first, so there's only ever one window to drive.
function Start-LogiCim([string]$Config = "Debug", [string]$Platform = "x64") {
  if (Get-Process "LogiCim" -ErrorAction SilentlyContinue) { Stop-LogiCim; Start-Sleep -Seconds 1 }
  $dir = if ($Platform -eq "x64") { "$LogiCimUnit\x64\$Config" } else { "$LogiCimUnit\$Config" }
  Start-Process -FilePath "$dir\LogiCim.exe" -WorkingDirectory "$LogiCimUnit\LogiCim" | Out-Null
  for ($i = 0; $i -lt 50; $i++) {
    Start-Sleep -Milliseconds 200
    $p = Get-Process "LogiCim" -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 }
    if ($p) { break }
  }
  Start-Sleep -Seconds 1
  $r = New-Object LogiCimU+RECT; [LogiCimU]::GetClientRect((Get-LogiCimWindow), [ref]$r) | Out-Null
  if (($r.R -ne 1280) -or ($r.B -ne 800)) { Write-Warning "Client area is $($r.R)x$($r.B), not 1280x800: the canvas constants are off." }
  Save-PixelsPerGrid 10
  Focus-LogiCim
}

function Stop-LogiCim { Stop-Process -Name "LogiCim" -Confirm:$false -ErrorAction SilentlyContinue }

# Windows refuses SetForegroundWindow while the user is busy in another window, and then keys would go to that window
# (the terminal: an Escape there cancels a tool call).  A tapped Alt lifts the foreground lock; if LogiCim still isn't
# in front, stop rather than type into something else.
function Focus-LogiCim {
  $h = Get-LogiCimWindow
  for ($i = 0; $i -lt 5; $i++) {
    if ([LogiCimU]::GetForegroundWindow() -eq $h) { return }
    [LogiCimU]::keybd_event(0x12, [byte][LogiCimU]::MapVirtualKey(0x12, 0), 0, [IntPtr]::Zero)
    [LogiCimU]::keybd_event(0x12, [byte][LogiCimU]::MapVirtualKey(0x12, 0), 2, [IntPtr]::Zero)
    [LogiCimU]::ShowWindow($h, 9) | Out-Null; [LogiCimU]::BringWindowToTop($h) | Out-Null
    [LogiCimU]::SetForegroundWindow($h) | Out-Null; Start-Sleep -Milliseconds 200
  }
  if ([LogiCimU]::GetForegroundWindow() -ne $h) { throw "LogiCim couldn't be brought to the front; not sending input." }
}

# Every key checks LogiCim is still in front first, so a key can't land in another window.
function Assert-LogiCimForeground {
  if ([LogiCimU]::GetForegroundWindow() -ne (Get-LogiCimWindow)) { throw "LogiCim isn't the foreground window; not sending keys." }
}

function Move-Client([int]$x, [int]$y) {
  $p = New-Object LogiCimU+POINT; $p.X = $x; $p.Y = $y
  [LogiCimU]::ClientToScreen((Get-LogiCimWindow), [ref]$p) | Out-Null
  [LogiCimU]::SetCursorPos($p.X, $p.Y) | Out-Null; Start-Sleep -Milliseconds 60
}
function Click-Client([int]$x, [int]$y) {
  Move-Client $x $y
  [LogiCimU]::mouse_event(0x2, 0, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 60
  [LogiCimU]::mouse_event(0x4, 0, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 100
}

# Grid units to client pixels at the current zoom.  The zoom buttons zoom about the centre, so it stays put.
function Grid-ToClient([double]$gx, [double]$gy) {
  @([int]($LogiCimCentre[0] + $Script:LogiCimPixelsPerGrid * $gx), [int]($LogiCimCentre[1] + $Script:LogiCimPixelsPerGrid * $gy))
}

# Zoom 1, then up by whole steps: Set-Zoom 2 is twenty pixels a grid unit.
function Set-Zoom([int]$Zoom) {
  Click-Client $LogiCimResetZoom[0] $LogiCimResetZoom[1]
  for ($i = 1; $i -lt $Zoom; $i++) { Click-Client $LogiCimZoomIn[0] $LogiCimZoomIn[1] }
  Save-PixelsPerGrid (10 * $Zoom)
}

function Move-Grid([double]$gx, [double]$gy) { $c = Grid-ToClient $gx $gy; Move-Client $c[0] $c[1] }
function Click-Grid([double]$gx, [double]$gy) { $c = Grid-ToClient $gx $gy; Click-Client $c[0] $c[1] }

# Button one held through each grid point in turn, in small steps, as a wire is drawn: Drag-Grid @(-7,-3) @(-3,-3)
function Drag-Grid {
  $pts = $args
  Move-Grid $pts[0][0] $pts[0][1]
  [LogiCimU]::mouse_event(0x2, 0, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 60
  for ($i = 1; $i -lt $pts.Count; $i++) {
    $a = $pts[$i - 1]; $b = $pts[$i]
    for ($s = 1; $s -le 8; $s++) { Move-Grid ($a[0] + ($b[0] - $a[0]) * $s / 8) ($a[1] + ($b[1] - $a[1]) * $s / 8) }
  }
  [LogiCimU]::mouse_event(0x4, 0, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 100
}

# Left control down or up.  A drag with it held moves views connected (their wires rubber band); without, detached.
function Set-Control([bool]$Down) {
  if ($Down) { Assert-LogiCimForeground }
  $sc = [byte][LogiCimU]::MapVirtualKey(0x11, 0)
  [LogiCimU]::keybd_event(0x11, $sc, $(if ($Down) { 0 } else { 2 }), [IntPtr]::Zero); Start-Sleep -Milliseconds 60
}

# Drag-Grid with left control held: Drag-GridControl @(0,0) @(0,8)
function Drag-GridControl { Set-Control $true; Drag-Grid @args; Set-Control $false }

# Button one pressed at a grid point and left down, to look at a drag before it's dropped; Move-Grid to drag, then
# Release-Grid.  -Control holds left control until the release.
function Press-Grid([double]$gx, [double]$gy, [switch]$Control) {
  if ($Control) { Set-Control $true }
  Move-Grid $gx $gy
  [LogiCimU]::mouse_event(0x2, 0, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 60
}

function Release-Grid([double]$gx, [double]$gy) {
  Move-Grid $gx $gy
  [LogiCimU]::mouse_event(0x4, 0, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 100
  Set-Control $false
}

# A key by virtual key code.  Raw input reads the scan code, so it must be sent; extended keys (Delete, arrows) need -Extended.
function Send-VKey([byte]$Vk, [switch]$Shift, [switch]$Extended) {
  Assert-LogiCimForeground
  $sc = [byte][LogiCimU]::MapVirtualKey($Vk, 0); $shiftSc = [byte][LogiCimU]::MapVirtualKey(0x10, 0)
  $ext = if ($Extended) { 1 } else { 0 }
  if ($Shift) { [LogiCimU]::keybd_event(0x10, $shiftSc, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 30 }
  [LogiCimU]::keybd_event($Vk, $sc, $ext, [IntPtr]::Zero); Start-Sleep -Milliseconds 30
  [LogiCimU]::keybd_event($Vk, $sc, $ext -bor 2, [IntPtr]::Zero); Start-Sleep -Milliseconds 30
  if ($Shift) { [LogiCimU]::keybd_event(0x10, $shiftSc, 2, [IntPtr]::Zero) }
  Start-Sleep -Milliseconds 100
}

# A letter, shifted if it's a capital: Send-Char 'F' starts placing a flip flop at the pointer.
function Send-Char([char]$c) { Send-VKey ([byte][char]::ToUpper($c)) -Shift:([char]::IsUpper($c)) }

function Send-Delete { Send-VKey 0x2E -Extended }

# Places a component by its canvas key (see CCircuitCanvas::Printable) at a grid point.
function Place-Grid([char]$Key, [double]$gx, [double]$gy) {
  Move-Grid $gx $gy; Send-Char $Key
  Move-Grid ($gx + 1) $gy; Move-Grid $gx $gy
  Click-Grid $gx $gy
}

# The whole window, frame included, as a PNG.  The client area starts 8 pixels in and 31 down in the image.
function Save-Screenshot([string]$Path) {
  $h = Get-LogiCimWindow; $r = New-Object LogiCimU+RECT; [LogiCimU]::GetWindowRect($h, [ref]$r) | Out-Null
  $bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
  $g = [System.Drawing.Graphics]::FromImage($bmp); $hdc = $g.GetHdc()
  [LogiCimU]::PrintWindow($h, $hdc, 2) | Out-Null
  $g.ReleaseHdc($hdc); $g.Dispose(); $bmp.Save($Path); $bmp.Dispose()
}
