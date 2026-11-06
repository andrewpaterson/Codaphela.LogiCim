---
name: run-logicim
description: Build, run, launch, drive, click, type into and screenshot LogiCim (the C++ port of Logicim, a circuit editor and simulator in a native Win32 window) - place components, draw wires, zoom, and capture the canvas to check a change in the real app.
---

# Run LogiCim

A native Win32 (GDI) window: a Logicim-style circuit editor over a running simulation. Agents drive it with
`.claude/skills/run-logicim/driver.ps1`, which injects mouse and keyboard input and takes `PrintWindow`
screenshots. Positions are given in circuit grid units. Paths here are relative to `LogiCim/` (the git repo).

Use the **PowerShell** tool (Windows PowerShell 5.1), not Bash: the driver is a dot-sourced PowerShell script.

## Build

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe" "D:\gameengine\LogiCim\LogiCim\LogiCim.sln" /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo | Select-String "LogiCim.exe| error "
```

The exe lands in `LogiCim\x64\Debug\LogiCim.exe` (Win32 configurations in `LogiCim\Debug\` / `LogiCim\Release\`).

## Run (agent path)

Each tool call is a fresh shell: dot-source the driver at the start of every call.

```powershell
Set-Location D:\gameengine\LogiCim; . .claude\skills\run-logicim\driver.ps1; Start-LogiCim; Set-Zoom 2; Click-Grid 15 10; Place-Grid 'F' 0 0; Place-Grid 'C' -8 -3; Drag-Grid @(-7,-3) @(-5,-3) @(-3,-3); Drag-Grid @(4,1) @(6,1) @(6,6); Drag-Grid @(6,6) @(-6,6) @(-6,1); Drag-Grid @(-6,1) @(-3,1); Place-Grid 'v' -10 0
```

That places a D flip flop at grid (0,0), a clock wired to its CP, a wire from QB round under it into D, and a 3.3V
supply. A later call carries on with the same zoom:

```powershell
Set-Location D:\gameengine\LogiCim; . .claude\skills\run-logicim\driver.ps1; Focus-LogiCim; Drag-Grid @(-9,0) @(-9,-1) @(-4,-1); Move-Grid 14 8; Start-Sleep -Seconds 1; Save-Screenshot "$env:TEMP\logicim.png"
```

Then Read the PNG to look at it. Stop with `Stop-LogiCim`.

| Function | Does |
|---|---|
| `Start-LogiCim [-Config Debug] [-Platform x64]` | Stops any running LogiCim, launches from `LogiCim\LogiCim\` (it loads `Resources\`), waits for the window, warns if the client isn't 1280x800 |
| `Stop-LogiCim` / `Focus-LogiCim` | Kills it / brings it to the front |
| `Set-Zoom n` | Reset zoom then zoom in to n (whole steps, about the centre) |
| `Move-Grid x y` / `Click-Grid x y` | Pointer to a grid point (fractions allowed) / button one click there |
| `Drag-Grid @(x,y) @(x,y) ...` | Button one held through the points: draws a wire from a port or wire end (one or two lines per drag), moves a component (detached from its wires) or the selection, or drags a selection rectangle from empty space |
| `Drag-GridControl @(x,y) @(x,y) ...` | As `Drag-Grid` with left control held: a move keeps the wires connected, routing them live |
| `Press-Grid x y [-Control]` / `Release-Grid x y` | Button one down and left down (`Move-Grid` to drag, `Save-Screenshot` to see it mid-drag), then up |
| `Place-Grid 'F' x y` | Canvas key then click to put it down. Keys (CCircuitCanvas::Printable): `F` flip flop, `C` clock, `v` 3.3V, `g` ground, `p` pin, `c` constant, `A`/`O`/`X`/`N` gates, `P` probe, `S` splitter, `T` tunnel, `L` label; `r` rotate anticlockwise, `R` clockwise, `x`/`y` flip horizontally/vertically, `e` properties, `m` move the selection (or the component under the pointer) until the next click |
| `Send-Char 'x'` / `Send-VKey vk [-Shift] [-Extended]` / `Send-Delete` | Keys |
| `Click-Client x y` / `Move-Client x y` | Raw client-area pixels, for toolbars and menus |
| `RightClick-Client x y` / `DoubleClick-Client x y` | Button two (a context menu) / a double click of button one, at client pixels |
| `Save-Screenshot path` | Whole window as PNG; the client area starts 8 px in and 31 px down in the image |

Port positions for a D flip flop at (0,0) facing East: CP (-3,-3), SD (-4,-1), D (-3,1), RD (-4,3), Q (3,-1),
QB (4,1). A clock's output is one unit right of it; a 3.3V supply placed at (-10,0) has its port at (-9,0).

## Run (human path)

Launch `LogiCim\x64\Debug\LogiCim.exe` with the working directory `LogiCim\LogiCim\`. The simulation starts
running ("View Path: Top (running)" in the status bar).

## Test

The editor and drawing logic is in `Libraries\LogicimLib`, tested by `Tests\TestLogicimLib` (run the exe from
its project folder, results at the end of `Log.txt`):

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe" D:\gameengine\LogiCim\Tests\TestLogicimLib\TestLogicimLib.sln /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo | Select-String " error "; Set-Location D:\gameengine\LogiCim\Tests\TestLogicimLib\TestLogicimLib; & ..\x64\Release\TestLogicimLib.exe | Out-Null; Get-Content Log.txt -Tail 4
```

## Gotchas

- **Keys go through raw input (`WM_INPUT`), which reads the scan code.** `keybd_event` with scan code 0 is silently
  ignored: the mouse works, keys do nothing. `Send-VKey` fills the scan code from `MapVirtualKey`.
- **Every tool call is a new shell**, so the driver's idea of the zoom would reset to 1 while the app stays zoomed and
  every grid position lands at the wrong scale (the status bar's X/Y shows half what you aimed at). The zoom is kept
  in `$env:TEMP\logicim-driver-zoom.txt`; only change zoom with `Set-Zoom`, and don't scroll (button three), or the
  centre the mapping assumes moves.
- **The canvas mapping is constants** for the default 1280x800 client: canvas centre at client (740,347), zoom
  buttons at (20,148) in and (20,224) reset. Check a mapping by moving there and reading X/Y in the status bar.
- Grey wires and port dots are undriven; dark/bright green are low/high. Sample several screenshots a few hundred
  ms apart to see a clock or divider move: the flip flop divider above shows Q changing within four shots.

## Troubleshooting

- **Keys do nothing, clicks work** -> scan code missing (see above), or the window isn't foreground: `Focus-LogiCim`.
- **Wires and placements land in the wrong place after the first call** -> the zoom file is stale or the view was
  scrolled: `Set-Zoom 2` again (resets about the centre).
