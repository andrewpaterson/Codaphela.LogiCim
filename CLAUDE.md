# LogiCim

A digital logic simulator: the C++ version of Logicim, with an IntelliJ-style interface on CommonLibraries'
WindowLib, and KiCad import.  It needs CommonLibraries checked out beside it (`../CommonLibraries`).

## Layout

- Design pages: `Libraries/LogicimLib/Docs/logicim` (`index.md`, `design.md`: LogicimLib and KicadImporterLib,
  `app.md`: the application, its navigation and the WindowLib port's gaps); the website gathers them.  Read them
  before changing what they cover.
- `LogiCim/`: the app (`LogiCim.sln`) and its checks (`Checks/LogicimChecks.py`; the `LOGICIM_*` variables are in
  `app.md`).
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

## Rules

- Circuit text is drawn with SupportLib's fonts (`CFontFamily`, `CFont`; turned text through `EFontTurn`), as
  sprites; zoomed sizes round to whole pixels.

## Working in these repositories

<!-- This section is the same in every Codaphela repository's CLAUDE.md: change them together. -->

- Several Claude sessions work in these checkouts at once, on the main branch (no private branches or worktrees).
  Run `git status` before touching a file and leave other sessions' uncommitted work alone.  Stage and commit only
  your own files, by path: `git add <paths>` and `git commit -m "..." -- <paths>`; never `git add -u`/`-A` or
  `commit -a`, and not `repos.py commit` (it stages everything).  Check `git show --stat HEAD` after.
- Commit messages have no Co-Authored-By line.  Make commits logical: one change each, its tests with it.
- Bring work from a copy into the main checkout with a 3-way merge (`git merge-file`, or `diff -u base mine |
  patch`), never by copying files over it: another session may have changed the target since.
- Before building a step of a shared plan, `git fetch` and read `git log HEAD..origin/<branch>` for the plan's
  files: a session on another machine may have done it.
- The shared build folders (`../build/*`) can hold binaries linked from another session's half-finished edits.  A
  failure seen only there: build the same sources in a private build folder; if that passes, rebuild the shared one
  clean.  Tests rewrite `Output/` in their project folder: run from a copy of that folder when another session may
  be running the same suite.
- Build, run and measure in Release x64 by default; Debug for full test sweeps or chasing a problem.  Say which build
  a number came from.  A speed claim that feeds a decision needs a like-for-like A/B (same commit but the change, in
  the whole test program) judged by instruction counts (callgrind) with the clock beside them.
- A decision the user states while planning settles that question only: plan with them, and wait to be told to
  build.
- Put decisions to the user one at a time, each explained plainly (what changes, what it costs), with the
  recommended option first, especially outside their speciality.
- Show images as full `file://` paths to the real files; don't make enlarged copies.
- Code files are LF.  `.sln`, `.vcxproj` and `.filters` are CRLF with a BOM: edit them as bytes and check `git diff
  --stat` shows only your lines.  Test `Input` files are kept byte for byte.
