# LogiCim Design

LogicimLib is LogiCim's circuit library: the views that draw a circuit's components and wires, the wiring that
makes a running simulation of them in MicroprocessorLib, the circuit and simulation files, and the editor.  It
draws onto SupportLib's maps and knows nothing of windows; the application ([The LogiCim Application](app.md)) puts
it in a WindowLib window.  Each class says in its header which Logicim (Java) class it is and where it does
something different.

## Views

A component on a circuit is a view: a [`CComponentView`], Logicim's `#!text ComponentView`.  A view knows its
position, rotation and mirroring, its shapes and its port views ([`CPortView`], one view standing for every bit of
a bus), and makes its component in a simulation ([`CComponentView::CreateComponent`]).  The domain classes it makes
are MicroprocessorLib's ([Logic Simulation](../libraries/microprocessorlib/logic.md)).

| Views | Are |
|---|---|
| [`CAndGateView`], [`COrGateView`] (and XOR with its flag), [`CBufferView`], their NAND, NOR, XNOR and inverter kinds | Logicim's gates, any number of inputs of any width |
| [`CLogic74PackageView`], [`CLogic74UnitView`] | a real 74 series part whole, or one of its units, from the table in [`LogicimLib/Logic74Parts.h`] |
| [`CLogicBlockView`]'s kinds: [`CRegisterView`], [`CLatchView`], [`CMultiplexerView`], [`CDecoderView`], [`CAdderView`] ... | generic chips drawn as a box of labelled pins |
| [`CDTypeFlipFlopView`], [`CCounterView`], [`CClockView`], [`CConstantView`], [`CProbeView`], [`COscilloscopeView`] | Logicim's chips and instruments |
| [`CW65C816View`], [`C65C64View`] | the processors, with their registers and flags shown live |
| [`CPositivePowerView`], [`CGroundView`], [`CPinView`] | supplies (on at start or not, a rise time), and a subcircuit's pins |
| [`CRegulatorView`] | a voltage regulator: a three pin box, an upside down tombstone, its IN, OUT and GND wired like any chip's pins, its voltage, dropout and part |
| [`CResetSupervisorView`] | a power-on reset supervisor, a DS1813: its RST, its trip voltage and reset time |
| [`CPullView`] | a pull-up or pull-down: a resistor to a supply bar or a ground symbol, on a rail or with a supply of its own, its net's capacitance carried for its release curve; a trace only pulls hold is drawn fainter |
| [`CRCFilterView`], [`CAnalogComparatorView`] | an RC element (a resistor into a capacitor to ground, a diode across the resistor if it has one, its time constant shown; no power) and an analog comparator, a MAX9010 |
| [`CSplitterView`], [`CTunnelView`], [`CLabelView`] | wiring that makes no component, and text |
| [`CSubcircuitInstanceView`] | an instance of another circuit of the project |

A wire is a [`CTraceView`]: one straight horizontal or vertical piece between grid points, kept as Logicim keeps
them, merged along each other and split where one ends on another or on a port.

[`CComponentView::GetProperties`] and [`CComponentView::InitFromProperties`] stand in for Logicim's reflection:
each view lists its own properties ([`CComponentProperties`]), and a properties dialog's answer makes a new view
that replaces the old one.  [`CComponentView::InitPlaced`] sets up a view placed new, as Logicim's
`#!text createInitialProperties` does (the LVC family, implicit power, two inputs on a gate, the next free
reference).  The editor's choosers list what can be placed from the libraries in
[`LogicimLib/ComponentLibraries.h`].  A chip's family is its part's ([`CIntegratedCircuitView::GetPartType`],
[`CFamilyVoltageConfigurationStore::GetPart`]): a Schmitt trigger 74 series part, a 74LVC14 or 74LVC1G17, reads its
inputs at its VT- and VT+.

### Bounding boxes and text

A view's bounding box is its shapes and ports, never its text, so a component's size doesn't depend on the font
it's drawn with.  Logicim measured labels into the box, so its components changed size with the font.  A view
Logicim sized to its text (a constant as long as its widest value, a tunnel as long as its name, a subcircuit
instance's box) measures the text once when it's made, and the box is saved with it
([`CComponentView::SaveXMLBox`]), so a loaded circuit keeps its layout whatever fonts draw it.
[`CComponentView::GetImportBoundingBox`] is the box of everything a view draws, text included, measured by painting
it; the KiCad importer places and routes with it.

## Circuits, paths and simulations

A [`CCircuitView`] is one circuit's views and wires, which it owns, named by its type name.  A project's circuits
are a [`CCircuitViews`]: any of them can be simulated on its own and placed in the others as instances.

A circuit used in several places has one set of views and a component of each per use, as in Logicim.  A
[`CViewPath`] is one use: the top circuit, then each instance down to the circuit.  A view keeps the component it
made, and a port view or wire its ports or traces, for each view path and circuit simulation it is in
([`CViewPathPointers`]).  The paths belong to a [`CViewPaths`], made as they're asked for; a subcircuit can't be
inside itself.  A path is written for places and files as [`LogicimLib/InstancePath.h`] has it, each step an
instance's index and name (`#!text 1:U2/1:U3`), so it still finds its instance after other components come and go.

[`CCircuitView::CreateSimulation`] makes each view's component for the path, and those of the subcircuits through
their instances, starts the simulation, and only then connects the ports, live.  Connecting before the start lost a
supply wired to a gate's input: gates act only on transitions.  Without starting it wires them for a saved
simulation to be loaded into.

[`CCircuitWiring`] finds the nets, Logicim's `#!text WireListFinder`, in two steps over every level of the path.
Wires, port views, tunnels of one name and the port view of each instance's pin with the pin's inside are joined
into nets, each a bus as wide as its widest port view.  Then the bits that components join (a splitter joins each
bit of its bus to its bit of an end, [`CComponentView::GetBitJoins`]) are joined, and each set of joined bits gets
one trace ([`CTrace`]).  A signal through a subcircuit's pin is one trace on both sides.  Both steps are
union-finds ([`CUnionFind`]).

A chip's power ports are hidden, and it gets its own supply at its implied VCC, unless it is on power rails
([`CIntegratedCircuitView::SetPowerRails`]): its hidden VCC and GND port views ([`CPortView::SetHidden`]) then join
only the nets of the rails' names, as tunnels of those names would.  A supply on a rail
([`CPowerSourceView::SetRail`]) joins its rail's nets wherever it is, as KiCad's power symbols do, so imported boards'
chips power up from their rails as the rails ramp (the user: "Hidden power pins join their named rail", 2026-11-26).
Only a regulator's OUT is on its rail: its IN and GND join only what they are wired to, and it is on while its GND is
at 0V and its IN is at its voltage plus its dropout, so where its power comes from is always drawn (the user:
"regulators should not have hidden / implied pins, the power source they're connected to must be obvious",
2026-11-27).

Editing a simulated circuit keeps it running: added views' components are made in every simulation of every path
through the circuit and started ([`CCircuitView::CreateComponents`]), removed ones taken out
([`CCircuitView::DestroyComponent`]), and the simulation wired again ([`CCircuitWiring::Rewire`]), so components
that didn't move keep their state.  Views being dragged are detached ([`CCircuitWiring::SetDetached`]): they join
nothing, their components stay in the simulation.

## Drawing

Circuits are drawn into SupportLib's maps ([Images and Maps](../libraries/supportlib/images.md#maps)): shapes into
a [`CGeometryMap`], text as glyph sprites into a [`CSpriteMap`] blitted over it.

| Class | Does |
|---|---|
| [`CCircuitViewport`] | Where the grid is on the screen: a grid unit is ten pixels at zoom one, grid 0, 0 in the centre, zoom 0.2 to 6.  Screen points are pixel centres and aren't rounded to whole pixels as Logicim's are. |
| [`CComponentShapes`] | A view's shapes in its own grid space, facing East: lines, polygons, arcs, circles and text, changed through their index. |
| [`CCircuitPainter`] | Turns and moves the shapes ([`CCircuitPainter::SetTransform`]), puts them through the viewport into the maps; arcs are flattened to 0.1 pixel. |
| [`CCircuitColours`] | Logicim's colours: voltages 0 to 7 volts in tenths, the background and grid.  [`CCircuitColours::InitLogicim`] is Logicim's light editor, [`CCircuitColours::InitDark`] (the application's) sits on IntelliJ's dark editor. |
| [`CCircuitFonts`] | Liberation Sans regular and bold and Liberation Mono, Logicim's faces, as SupportLib font families: each size and turn rasterised when first drawn. |
| [`CCircuitGrid`] | Logicim's point grid as tiles of one row of dots each, pixel for pixel as drawing the dots. |
| [`CComponentSpriteCache`] | Components' shapes rasterised once and blitted after. |

A component is drawn facing East (its output to the right) and turned about its position; mirroring is top to
bottom before turning, so the eight ways a component can lie are a rotation and a mirror
([`LogicimLib/Rotation.h`]).  Text keeps upright: across when the component faces East or West, down when North or
South.  Circuit text is KiCad's markup ([`CCircuitTextMarkupParser`]: `#!text ~{...}` overlined, `#!text _{...}`
and `#!text ^{...}` sub and superscript); overlines are rectangles in the geometry map.  Zoomed text sizes round to
whole pixels.

The sprite cache keys a component's sprite by the bytes of what it painted: the same shapes, colours, rotation,
zoom and position within a pixel paint the same pixels, so components that look alike share a sprite, and a shape
that changes (a clock's wave colour) is another.  Text and ports, which change as the circuit runs, are drawn every
paint.  Sprites are packed on atlas pages and kept at every zoom, so zooming back is free; past the cache's size
(64 MB by default) the least recently used page is reused, never one drawn from this frame.  A component's memo
([`SComponentSpriteMemo`]) finds its sprite again without painting while its state hasn't changed.

## Files

Circuits are XML written by BaseLib's [`CXMLWriter`] and read with its parser: a `#!text <Logicim version="2">`
root holding a `#!text <Circuits>` element (a project) or one `#!text <Circuit>`, and optionally a
`#!text <Simulation>` ([`LogicimLib/CircuitFile.h`]).  A view saves itself as an element named by
[`CComponentView::GetXMLName`] with its properties as attributes; [`CComponentView::LoadXML`] is called on a view
allocated by [`NewComponentView`] but not initialised, reads the attributes and initialises itself, and a view that
fails is freed without Kill.  A wire is a `#!text <Trace>`.  An instance loaded knows only its subcircuit's type
name; the circuits are joined up once all are read.  The editor adds an `#!text <Editor>` element (see below).  A
project's `#!text <Circuits>` has `#!text powerUp` and `#!text powerUpSeed` attributes when its chips don't power up
at Zero ([`CCircuitViews::SetPowerUp`]); the editor gives them to each simulation it makes.

A running simulation is saved beside the circuit ([`LogicimLib/SimulationStateXML.h`]): the timeline's times, then
for each component the state of the component its view made, then the same for each instance's path.  The view
picks the helper ([`CComponentView::GetComponentStateXML`]): [`CIntegratedCircuitStateXML`] saves a chip's pending
events and each port's output and events, [`CPowerSourceStateXML`] a supply's switch when it isn't simply on, and
[`CPassiveStateXML`] nothing.  What a chip remembers it saves itself
([`CIntegratedCircuit::SaveXML`], in an element [`CIntegratedCircuit::GetStateXMLName`] names), the processors
their whole core.  Events ([`LogicimLib/EventXML.h`]) are saved with their time and ID, and loaded in time then ID
order so each time's events run in the order they did.  Wires keep no state: their traces are made again from the
views.  A circuit loaded with no saved simulation is started afresh.

## The editor

[`CCircuitEditor`] edits a project's circuits while one of them is simulated, Logicim's `#!text CircuitEditor`: it
owns the circuits and the simulation, and the pointer is given to it in grid space.  An edit in progress is a
[`CStatefulEdit`]: a selection rectangle ([`CSelectionEdit`]), a wire pulled from a port ([`CWirePullEdit`]), a
move ([`CMoveComponentsEdit`]), a placement.

- A wire pulled from one component's port and released on another's is routed around the components
  ([`CTraceAutoConnector`]) rather than drawn as pulled.
- Dragged on their own, components come off their wires and join whatever they land on.  Dragged with Control, or
  turned or flipped, they stay connected: each moved port's lead (its wires to the next junction or port) is lifted
  and routed again at each step, and each moved wire's end is joined back by new wire.  Logicim has neither.
- Turning and flipping act on what is moving or being placed, else the component under the pointer, else the
  selection, about the pointer.
- A subcircuit instance can be entered, as KiCad enters a sheet: the editor then edits that circuit as it is
  simulated inside the instance, and its edits are to every instance of it.  After an edit, every instance of a
  circuit whose pins changed is laid out again ([`CCircuitEditor::UpdateInstancePins`]).
- A saved project is the circuits file with an `#!text <Editor>` element: the circuit edited, the rotation new
  components are placed with, the selection, and an application's tabs ([`SCircuitPlace`]), which undo leaves
  alone.

[`CTraceAutoConnector`] routes horizontal and vertical wires over the grid, charging for length, turns, crossings,
running beside a component or another net's wire, and turning away from a hint.  A route may cross another net's
wire but never run along it, turn or end on it, or pass over another net's port.  The connections between the same
two components (a bus) are a run, routed together with their bends stepped into a stair.  A connection that can't
be routed is tried again through the other nets' new wires; those nets are ripped up and routed again, and one that
still can't is left unconnected.

Undo is Logicim's: after each change the whole project, simulation and all, is saved onto BaseLib's [`CUndoStack`],
and undoing loads one again.  A change only to the selection is skipped by Undo and Redo, not by stepping.  Moving
(another circuit, an instance, a tab) isn't a change: each snapshot remembers where its change was made
([`CCircuitEditor::GetUndoPlace`]), and undo goes back there.  The editor tells a [`CCircuitEditorListener`] after
each change and each undo or redo.

## KiCad import

KicadImporterLib makes LogiCim circuits from KiCad schematics read by CommonLibraries' KicadLib.
[`CKicadSchemLogicimConverter`] makes one sheet's circuit: a real 74 series part becomes a 74 series package or units
as KiCad draws it, any other 74 series gate or flip flop generic ones, a power symbol a supply, a board's edge
connector the circuit's pins, and any other symbol an instance of an empty placeholder circuit with its pins, to be
filled in later.  Power flags, test points and the like are left out, and resistors and capacitors aren't drawn.  A
resistor between two nets that aren't supplies (a series termination, a network's element) joins them into one net
([`CKicadSchemLogicimConverter::JoinSeriesResistors`]): its labels' tunnels and one at each of the resistor's pins
take the name of its label-named net (the connector's side first, else the driven side's,
[`CKicadSchemLogicimModel::GetNetName`]).  Not when it makes an RC filter of 1ns or more with what it drives: the
capacitors drawn from that net to a supply, or else an estimate, each input on the net at its family's input
capacitance (PropagationTimes.csv's CI) plus 3pF of wiring and 10pF a connector pin; that becomes an RC element from
the driven side to the other, where the resistor is ([`CKicadSchemLogicimConverter::AddRCFilters`]).  Resistors in
parallel between the same nets are one RC, and a diode across it (anode to cathode, [`GetKicadSchemDiodeElements`])
charges its capacitor quickly that way, through 25 ohms.  A value that can't be read joins, warned.  A resistor from a
supply to a net logic is on (a part, a board drawn on the sheet, a connector pin; not a placeholder) is a pull, on the
supply's rail if it is one, carrying its net's capacitance, all of what series resistors joined it to
([`CKicadSchemLogicimConverter::AddPulls`]); to a net only an LED or a placeholder is on it's left out, and an
unreadable value is 10k, warned.  A bussed network (a 4609X, a 4816P-1) is an element from each pin to its common pin,
which joins no signals; a network numbered only (a CA0612 capacitor array) pairs its pins across the package.
[`GetKicadSchemResistance`] and [`GetKicadSchemCapacitance`] read every value HCP65/Kicad writes, resistor networks'
part codes too; [`GetKicadSchemPassiveElements`] pairs an isolated network's pins by name.  A chip whose VCC and GND nets have
supplies is on those nets as power rails, its power pins hidden ([`CKicadSchemLogicimConverter::ApplyPowerRails`]); a
voltage regulator becomes a Regulator on its output's rail, its IN, OUT and GND wired to what its pins are on, with
its part's dropout and start-up ([`CKicadSchemLogicimConverter::AddRegulatorSupplies`]); one with nothing on its input
stays off; a DS1813 a reset supervisor; a MAX9010 an analog comparator
([`IsKicadSchemComparatorPart`]'s list); and every supply made rises
over 1ms as the simulation starts.  A subnet joined to the rest of its net by labels gets a tunnel at
each label.  A net a label names as a supply gets one supply, as a power symbol would give it, beside the first such
label on a part of it that's drawn and wired to that part; its labels' tunnels take it to the rest
([`CKicadSchemLogicimConverter::AddSupplyLabels`]).  A supply label is one whose whole text is GND,
`#!text GND_{OUT1}` and the like (0V), a voltage (`#!text 3.3V`, `#!text 18V`, `#!text 1.25V`) or VCC, VCCIO or VCCINT
(5V) ([`IsKicadSchemSupplyLabel`]).  A negative voltage (the Audio DAC's `#!text -5V`) stays an ordinary net: LogiCim's
supplies are grounds and positive supplies.  The pieces are [`CKicadSchemLogicimModel`] (the items, terminals and
connections), [`CKicadSchemLogicimBuses`], [`CKicadSchemLogicimPlacer`] (spread out from KiCad's places, lined up,
moved clear, dense areas exploded) and [`CKicadSchemLogicimWiring`], which routes with [`CTraceAutoConnector`].

[`CKicadSchemBoardImporter`] imports a board with every board drawn on it (a symbol with a Schematic property),
each made once as a circuit whose instances are its symbols.  Building every board's circuit is quick and done on
the caller's thread; laying each out is nearly all the time and touches only its own circuit, so each is a job on a
thread pool, and the result is the same file byte for byte whichever finishes first.  An import can be cancelled
and tells a [`CKicadSchemImportProgress`] its steps.

[`CKicadSchemLogicimDecode`] reads a board's address decode by simulating it.  The sheet's circuit is built but not
laid out: its components are moved apart and wired by the netlist instead, a tunnel named as its net on each port the
converter made for a pin, label or supply, by the converter's joins, RC filters joined too
([`CKicadSchemLogicimConverter::SetJoinFilters`]: a chip select leaves a board through a series resistor, and the
decode reads logic, not timing).  Its supplies are the
converter's, power symbols' and supply labels' (a GND label ties the board's unused '138 enables low).  The board's
pins on nets that only inputs read are driven, those named A and a number as the address and the rest as modes,
through every combination, and the pins on nets a part drives are read once it settles; an output neither high nor
low is an error.  Each output's active addresses are written as ranges, a range repeating through address lines the
board hasn't on one line (`LogiCim --decode`, [The LogiCim Application](app.md#kicad-import)).

## Tests

| Test | Covers |
|---|---|
| [`TestLogicimLib/TestGateViews.cpp`], [`TestLogicimLib/TestTextViews.cpp`], [`TestLogicimLib/TestLogic74Views.cpp`] | views drawn, compared with reference images in Input |
| [`TestLogicimLib/TestCircuitFile.cpp`], [`TestLogicimLib/TestSimulationState.cpp`] | every view type round trip; a ring oscillator, one through an RC element, and a W65C816 saved mid-run run on identically once loaded; both processors' cores round trip; pulls on their own supply and on a rail |
| [`TestLogicimLib/TestWiring.cpp`], [`TestLogicimLib/TestViewPaths.cpp`], [`TestLogicimLib/TestSubcircuits.cpp`] | nets, tunnels, splitters, paths and instances |
| [`TestLogicimLib/TestComponentSprites.cpp`], [`TestLogicimLib/TestCircuitGrid.cpp`] | the sprite cache and grid tiles draw as painting does |
| [`TestLogicimLib/TestTraceAutoConnector.cpp`], [`TestLogicimLib/TestCircuitEditor.cpp`] | routing, and editing with undo |
| [`TestKicadImporterLib/TestKicadSchemLogicim.cpp`] | each converted schematic compared with its expected circuit in Input/Logicim; supply labels; boards on threads and cancelled |
| [`TestKicadImporterLib/TestKicadSchemPassives.cpp`] | every value form in HCP65/Kicad read; a test sheet's resistors, networks and RCs joined or made RC elements; the Clock Doubler's pulse at each edge, the PS2 Timing's packet pulse held through a burst and let go along R3 x C3; the Address Buffer 16bit's pulls, a line let go curving up over 10k x 13pF |
| [`TestKicadImporterLib/TestKicadSchemPower.cpp`] | Power and Reset and the Processor Reset Device imported and powered up: rails, regulators, the DS1813, the reset through R1 |
| [`TestKicadImporterLib/TestKicadSchemDecode.cpp`] | the decode of a small board (a series resistor, a pull up, a GND label) and of the HCP65 MPU Address Decode board |

The reference images of the first views were checked against Java Logicim's own renders when they were made
(2026-10-24): positions match, text sits about a pixel lower and a few hinted widths differ.

## Decisions

- Logicim's user interface goes in its own library, LogicimLib (now in this repository), not MicroprocessorLib,
  which stays simulation only on BaseLib and StandardLib (the user, 2026-10-24).
- Views draw onto SupportLib's maps, mainly [`CGeometryMap`], not as WindowLib components: LogicimLib has no
  WindowLib dependency (the user, 2026-10-24).
- The Java version is the inspiration, not ported one to one, and looks are checked against it (the user,
  2026-10-24).
- Bounding boxes leave text out, and views sized to text keep the box they measured, saved with them (the user,
  2026-10-24: Logicim's component sizes depend on the font, through `#!text TextView`'s measuring).
- Files are a new XML format with SaveXML and LoadXML on the objects, not Java's reflective `#!text .logic` data
  objects (the user, 2026-10-24).
- A simulation's state is saved by helper classes beside the domain's (the user, 2026-10-24); on 2026-10-26 the
  user moved what each chip remembers onto the chip ([`CIntegratedCircuit::SaveXML`]), keeping the event and
  component helpers here.  The processors' cores are saved as a plain state struct rather than through an accessor
  for each field (accepted by the user, 2026-10-24).
- [`CCircuitView::CreateSimulation`] starts the simulation itself, then connects every port live, as Logicim's
  reset does (2026-10-24, when the user asked why a supply wired to a gate before the start was never seen).
- A circuit used in several places has a component per view path, as Logicim's (2026-10-24), replacing one
  component per view.
- North is up: the rotations were renamed so (2026-10-25).
- A splitter maps bit b of its bus to the end its split indices say, in order; Logicim paired bits by port name,
  which only matched for indices in order (2026-10-24).  A clock with an inverse output keeps its wave in the
  middle of its box; Logicim's moved off it facing North or South (2026-10-24).
- Any port can have a clock notch ([`CPortView::SetClock`]), drawn as Logicim draws it on subcircuit pins; Logicim
  never drew it on chips (the user asked for the W65C816's, 2026-10-24).  A 65C64 view was added with no Java
  original (the user, 2026-10-24).
- Tunnels match their names with case, without the spaces around them; Logicim ignores case.
- Components' shapes are cached as sprites and the grid drawn as tiles (2026-11-03): a frame costs little more than
  copying its components.  On a 990 pixel wide canvas, grid tiles 256 pixels wide drew within 2% of tiles the
  canvas' width, the narrowest that did (64 pixels took 10% longer).
- Circuit text and KiCad positions are worked out the same in every build (2026-11-09): optimised builds'
  `#!text /fp:fast` turned a divide by 10 into a multiply by 0.1, so a tunnel's box, and a KiCad import's layout,
  differed between Release and Debug in the last digit.
- A KiCad import lays its boards out on several threads (2026-11-19): the Audio Board's thirteen boards took 12.2 s
  and about 6 s after (the build wasn't recorded).
- Labels named as supplies are supplies (the user, 2026-11-26): "GND, 3.3V, 5V, 12V, 18V (and few other slightly
  more unusual values) should be treated as supplies.  I think KiCad does some annoying global level power symbol
  stuff that broke a schematic with sub-schematics so stopped using them."  Then "GND_{OUT*} yes, its power label.
  VCC, VCCIO and VCCINT yes", and VCC is always 5V: "The CPLD in this case is the Altera MAX 7000 series which has
  VCCINT of 5V.  There is also a MAX II programmer which is 3.3V but it's fine if that is misinterpreted as 5V".  The
  names like them are ordinary nets: `#!text GND_{A} EN` to `#!text GND_{D} EN` enable the Pico boards' switched
  grounds `#!text GNDa` to `#!text GNDd`, and `#!text GND_{TOP}` and `#!text GND_{1}` to `#!text GND_{4}` are what
  they switch.  A net has one supply, beside a label, rather than a supply in place of each label's tunnel, as a
  power symbol on each pin would be: grounds on a 74573's pins left too little room to route the pins beside them.

## Not done

- The circuit canvas is drawn whole every paint: drawing less needs change tracking in LogicimLib
  ([Drawing and Speed](../libraries/windowlib/drawing.md#not-done)).
- KiCad's hierarchical sheets are left out of an import, and placeholder circuits have no logic until it's drawn.
