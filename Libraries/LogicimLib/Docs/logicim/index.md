# LogiCim

LogiCim is the C++ version of Logicim, the user's Java circuit editor and simulator: digital circuits drawn on a
grid, simulated as they are edited, with voltages, logic families and propagation times, down to the HCP65's
processors running programs.  Its interface is IntelliJ IDEA's style, on WindowLib, where Logicim's is KiCad's.  It
imports KiCad schematics, so the HCP65's boards can be simulated as they are drawn.  The Java version is the
inspiration, not ported line by line: each class says in its header which Logicim class it is and where it differs.

## Parts

| Part | Folder | Is |
|---|---|---|
| LogiCim | `#!text LogiCim/LogiCim` | The application: the window, its panes, dialogs and commands ([The LogiCim Application](app.md)). |
| LogicimLib | `#!text Libraries/LogicimLib` | The views of components and wires, drawing, wiring a simulation, files, and the editor ([LogiCim Design](design.md)). |
| KicadImporterLib | `#!text Libraries/KicadImporterLib` | KiCad schematics, read with CommonLibraries' KicadLib, made into LogiCim circuits ([KiCad import](design.md#kicad-import)). |
| TestLogicimLib, TestKicadImporterLib | `#!text Tests` | The libraries' tests. |
| Checks | [`LogiCim/Checks/LogicimChecks.py`] | The application run with scripted steps ([Checks](app.md#checks)). |

## What it is built on

The simulation is CommonLibraries' MicroprocessorLib: the timeline, voltages, traces, ports, the chips and the
processors, a port of Logicim's simulation domain ([Logic Simulation](../libraries/microprocessorlib/logic.md)).
MicroprocessorLib depends only on BaseLib and StandardLib, so everything that draws or edits is here: a view makes
its component in a [`CCircuitSimulation`], and LogicimLib wires the components' ports with traces.

Circuits are drawn into SupportLib's maps ([Images and Maps](../libraries/supportlib/images.md#maps)): shapes into
a [`CGeometryMap`], text as glyphs from SupportLib's fonts into a [`CSpriteMap`] over it.  LogicimLib knows nothing
of windows; the application shows the maps in a WindowLib canvas.

BaseLib gives the XML reader and writer the files use, the undo stack and the union-find the wiring uses;
StandardLib the objects; ThreadLib the threads a KiCad import lays its boards out on.

## Pages

- [LogiCim Design](design.md): LogicimLib and KicadImporterLib.
- [The LogiCim Application](app.md): the window, navigation, checks and the WindowLib port.

## Plans

Navigation.txt planned the Simulation and Subcircuits trees, places, tabs, Back and Forward and Go To (2026-11-14).
It was finished, nothing left open, and its design is now in [Navigation](app.md#navigation); the plan is in
LogiCim's history up to ba23c5b.
