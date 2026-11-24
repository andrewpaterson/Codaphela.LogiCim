# LogiCim

A digital logic simulator: the C++ version of Logicim, with an IntelliJ-style interface on CommonLibraries'
WindowLib, and KiCad import.  It needs CommonLibraries checked out beside it (`../CommonLibraries`).

## Layout

- `LogiCim/`: the app (`LogiCim.sln`), its checks (`Checks/LogicimChecks.py`) and `Navigation.txt`.
- `Libraries/LogicimLib`: the interface and views (gates, ports, traces, the circuit view) on SupportLib's
  CGeometryMap.  `Libraries/KicadImporterLib`: reads KiCad schematics into circuits.
- `Tests/TestLogicimLib`, `Tests/TestKicadImporterLib`.
- The simulation itself (timeline, voltages, traces, ports, ICs, gates) is CommonLibraries'
  MicroprocessorLib, which depends on BaseLib and StandardLib only: interface code belongs here, not there.

## Building and testing

- `cmake -S LogiCim -B ../build/LogiCimRelease -DCMAKE_BUILD_TYPE=Release`, then `cmake --build`.
- On Windows the `run-logicim` skill (`.claude/skills/run-logicim`) builds it with MSBuild and drives the
  window: placing components, drawing wires, screenshots.
- Tests build with CMake as in CommonLibraries (`cmake -S Tests/TestLogicimLib -B ../build/TestLogicimLib64
  -DBASELIB_64=ON`, then `cmake --build` and `ctest --test-dir`).
