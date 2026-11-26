# The LogiCim Application

The LogiCim application puts LogicimLib's editor ([LogiCim Design](design.md)) in a WindowLib window laid out like
Logicim's, in IntelliJ IDEA's dark style.  It runs on Windows (GDI) and Linux (X11).

## The window

[`CLogicimApp`] makes the window, its content and the input, and runs them.  The content,
[`CLogicimWindowContent`], is the menu bar, the main toolbar (projects, the simulation's controls, rotate and
flip), a dock holding the circuit between a view stripe on the left and an add stripe on the right, and the status
bar.  It holds a [`CLogicimCircuit`], which is the editor ([`CCircuitEditor`]) and the project in it.

The dock's panes are the HCP65 IDE's ([Docking](../libraries/windowlib/docking.md)): the circuit is the one
document, which can't be closed, with Subcircuits on its left and Simulation below it, each movable, floatable,
auto hidden or hidden.  Window, Reset Window Layout puts them back.  The status bar shows the circuit edited, the
simulation and the path into it on the left, what undo has to say, and the pointer's grid position and the zoom on
the right.

[`CCircuitCanvas`] passes the pointer, buttons and keys to the editor in grid space, zooms with the wheel and pans
with button three; [`CCircuitCanvasDraw`] draws the background, grid and circuit through the geometry map.  The
canvas is drawn whole every paint ([`CCanvas::SetDrawnEveryPaint`]): its draw reads the circuit, which edits, the
simulation and the view change without telling it.  The rest of the window draws only what changed
([Drawing and Speed](../libraries/windowlib/drawing.md)).

The dialogs (a component's properties, a new subcircuit, choosing a component, Go To, the file choosers, Settings)
are native windows shown modally ([Components](../libraries/windowlib/components.md)).  They are shown from the
window's tick ([`CLogicimWindowTick`]), not from the listener of the button or menu item that asked, because a
modal dialog reads the input itself.  A properties dialog is made from the view's [`CComponentProperties`]: a text
field, check box or combo box a property, a titled divider between groups.

## Commands and keys

Every menu item, tool button and key runs a command ([Input](../libraries/windowlib/input.md)).  Over the circuit
canvas the Circuit context's bare letters place components, as Logicim's keys do, and turn (r, R), flip (x, y) and
move (m) them, unless a text field has the focus.  Each library component and each subcircuit has a Place command;
a subcircuit's keys are the project's, saved with its circuit.  Their keys are given in the component chooser
([`CLogicimComponentChooser`], laid out as KiCad's symbol chooser), not in Settings' Keymap, which keeps the other
commands' keys in the user's Keymap.xml.

A running simulation is stepped every 16 ms by the time since the last tick times the speed, 0.25 simulated
nanoseconds a millisecond by default, doubled or halved by the speed commands.

## Projects

LogiCim starts with an empty circuit, Main, simulated on its top path.  New Project, Load Project, Import KiCad
Schematic and File, Recent Projects replace it; Save Project writes it as a circuits file (`#!text *.xml`).  Each
project's start is kept in the editor's undo stack, so undo goes back past a load or an import to the project
before it, with its file.  A project with changes asks Save, Don't Save or Cancel before it's closed, and before
LogiCim exits.

What LogiCim keeps between runs is in the user's folder (`#!text %APPDATA%\LogiCim`, or `#!text ~/.config/LogiCim`
on Linux), written as soon as it changes ([`CLogicimSettings`]):

| File | Holds |
|---|---|
| Settings.xml | where Import KiCad Schematic and Load Project last opened |
| RecentProjects.xml | the projects loaded or saved last, latest first |
| Layout.xml | the window and its dock layout |
| Keymap.xml | the keys changed in Settings' Keymap |

## Navigation

A place ([`SCircuitPlace`]) is the circuit simulated, the path of instances into the circuit edited, and the zoom
and centre.  [`CLogicimWindowContent::ShowPlace`] is the one way the editor moves: it simulates the place's circuit
if it isn't the one simulated, enters each instance on the path ([`CCircuitEditor::EnterInstancePath`]) and sets
the viewport.  A place that's gone (its circuit removed, an instance on its path deleted) is refused and nothing
changes.

- **The Simulation tree** is the circuit simulated and its instances, nested, following the place edited; a double
  click enters an instance.
- **The Subcircuits tree** is what uses what: at the top the circuits nothing uses, under each the distinct
  circuits it uses.  Choosing one edits it, simulated on its own.
- **Tabs**: each is a place with its own canvas.  There is one editor, one simulation and one undo history: the
  active tab moves the editor there.  A visible tab that isn't active draws its own place, with the simulation's
  state if its place is in the circuit simulated.  Tabs that aren't edited follow the project: their paths are kept
  up to date, trimmed to what's left, and a tab whose circuit goes is closed (the first tab goes to the top
  instead).  The tabs are saved in the project's `#!text <Editor>` element.  Open in New Tab is on an instance's
  menu and both trees' menus.
- **Back and Forward** ([`CLogicimNavigationHistory`], 50 places) step through the moves between places: entering
  or leaving an instance, choosing a circuit, a tab, a tree's node, Go To.  Zooming and panning aren't steps.
- **Go To** (Control+N) finds every component in the circuit simulated and in each of its instances, by name or by
  part ("U12 74HC00", with "Main > U1 > U3" after it), tunnels and labels by name, and circuits by name
  ([`CLogicimGoTo`]).  A component inside two instances is two results.  Choosing one enters its path, selects it
  and centres on it.

Moving isn't a change to undo.  Undo and redo go through WindowLib's [`CRevealingUndo`]: a change made where the
editor is is undone at once; one made elsewhere is shown by the first press, the status bar saying "Ctrl+Z again to
undo the change in Gate in U3", and undone by the second.

## KiCad import

An import builds its boards' circuits on the window's thread, then lays them out on a pool of threads (see
[KiCad import](design.md#kicad-import)).  Meanwhile a progress dialog ([`CProgressDialog`]) says which board and
step it is on; its Cancel gives the import up, each circuit stopping at its next step, and leaves the project as it
was.

`#!text LogiCim --decode <board.kicad_sch>` (on Linux, run from `LogiCim/LogiCim` for its resources) opens no
window: it simulates the board's address decode ([`CKicadSchemLogicimDecode`], see
[KiCad import](design.md#kicad-import)) and prints each chip select's address ranges in each mode, a line each.  The
website's build checks the HCP65's memory map pages against the HCP65 MPU Address Decode board's.

## Checks

[`LogiCim/Checks/LogicimChecks.py`] runs LogiCim with steps in `#!text LOGICIM_CHECK` ([`CLogicimApp::Check`]),
which it runs once the window is set up, without showing it, and checks what they print: the panes, the trees,
places, tabs, Back and Forward, Go To, the keymap, closing with changes, the dock layout.  A scripted run uses none
of the user's files unless their variables name them.

| Variable | Does |
|---|---|
| `#!text LOGICIM_CHECK` | The check steps, comma separated. |
| `#!text LOGICIM_IMPORT` | A KiCad schematic imported at the first tick, zoomed to fit. |
| `#!text LOGICIM_VIEW` | `#!text x,y,size`: after the import, show that square of grid units instead. |
| `#!text LOGICIM_SNAPSHOT`, `#!text LOGICIM_SNAPSHOT_TICKS` | Save the window as a PNG after that many ticks (60) and close. |
| `#!text LOGICIM_VERIFY_REDRAW` | Check each partial paint against drawing the whole window. |
| `#!text LOGICIM_SETTINGS`, `#!text LOGICIM_RECENT`, `#!text LOGICIM_LAYOUT`, `#!text LOGICIM_KEYMAP` | Where the user's files are. |

## The WindowLib port

The goal (the user, 2026-10-22): a C++ WindowLib version of Logicim's interface in IntelliJ IDEA's style rather
than Logicim's KiCad style.  The target agreed then: a main toolbar with a Run widget and a simulation combo; a
Project tool window (the subcircuits) and a Simulations tool window, both trees; editor tabs per subcircuit; a
status bar with a breadcrumb, X, Y and zoom; real menus with accelerators; context menus; an Add Component search
popup; IntelliJ form dialogs (label and editor rows, titled separators, Save Defaults, OK and Cancel, Control+Enter
and Escape).

WindowLib lacked most of what that needed, so the gaps were listed in order: the core (timers, double clicks, key
repeat, cursors, enabled and visible flags, Tab traversal, a window keymap, the OS clipboard, many windows and
modal loops); text fields, check boxes, combo boxes, separators, a form helper and message boxes; toolbars, status
bars and tool tips; splitters, trees, tabs and tool window stripes; search popups, file choosers and menu
accelerators, disabled and check items; a shared theme; and on the canvas dashed strokes, turned text and a zoom
transform.  All but menu check items was built, in WindowLib ([WindowLib](../libraries/windowlib/index.md)),
SupportLib (turned text) and LogicimLib (the viewport's zoom; the selection rectangle's dashes, drawn as pieces of
line).  Where LogiCim differs from the target is in Not done.

## Decisions

- An IntelliJ IDEA style C++ version of Logicim's interface on WindowLib (the user, 2026-10-22).
- Dialogs are real native windows, with many windows and a modal loop in WindowLib, not overlays inside the window
  (the user, 2026-10-22).
- Toolbars, status bars and tool tips are WindowLib classes shared with the HCP65 IDE (the user, 2026-11-08).
- LogiCim's actions are commands, which menus, tool buttons and keys run (2026-11-07); its tool windows are dock
  panes, as the IDE's (2026-11-14).
- The circuit canvas is drawn every paint and the rest of the window only where it changed (2026-11-07).
- The project file is XML with the `#!text .xml` extension, offered as Logicim Projects.
- Navigation (the user, 2026-11-14, Navigation.txt): the Simulation tree replaces the placeholder Simulations pane;
  the Subcircuits pane is a tree of what uses what; a tab is a place, with one editor, simulation and undo history;
  the tabs are saved in the project; Go To finds each instance in the simulation, by name and by type; Back and
  Forward step between places, each with its zoom and centre, zooming and panning not steps, on Control+Alt+Left
  and Right as the IDE's.
- As built (2026-11-14): a place is LogicimLib's, so the editor keeps the tabs with the project; the commands are
  in the View menu, as LogiCim has no Navigate menu; Back and Forward have no toolbar buttons, as the toolbar's
  icons are KiCad's and have none; the first tab is the one that can't be closed; there is no middle click.
- The Simulation tree makes all its nodes at once: on the Audio Board import (27 instances, 2204 components) the
  trees and Go To take no time to notice, so it has no lazy nodes (2026-11-14).
- A path names each instance as well as its index, so a place survives other components coming and going
  (2026-11-14).
- Moving isn't a change to undo; a change made out of view is shown before it's undone (2026-11-14).
- A KiCad import's progress is in its dialog alone, and it can be cancelled (2026-11-19).
- Scripted runs (`#!text LOGICIM_CHECK`, `#!text LOGICIM_IMPORT`, `#!text LOGICIM_SNAPSHOT`) don't read or change
  the user's files unless told where they are: the user's own aren't what's checked.

## Not done

- Cut, Copy, Paste and Duplicate, and the Add Trace, Add Connection and Add Subcircuit buttons: they only log
  "isn't done yet".
- One simulation at a time: New Simulation only names it.  Logicim's several simulations of a circuit, and the
  target's Run widget and simulation combo on the main toolbar, aren't built.
- The properties dialog has no Save Defaults.
- Menu items can't show a tick (Toggle Point View doesn't show whether the grid is on).
- Components are added from a chooser dialog, KiCad's style, not an IntelliJ search popup.
- Partial redraw hasn't been checked by hand in LogiCim, and its checks never show the window
  ([Drawing and Speed](../libraries/windowlib/drawing.md#not-done)).
