/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela KicadImporterLib

Codaphela KicadImporterLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela KicadImporterLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela KicadImporterLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#ifndef __KICAD_SCHEM_LOGICIM_CONVERTER_H__
#define __KICAD_SCHEM_LOGICIM_CONVERTER_H__
#include "BaseLib/ArrayTemplate.h"
#include "BaseLib/ArrayChars.h"
#include "LogicimLib/CircuitViews.h"
#include "LogicimLib/CircuitFonts.h"
#include "KicadLib/KicadSchemNetlist.h"
#include "KicadSchemLogicFunction.h"
#include "KicadSchemLogicimModel.h"
#include "KicadLib/KicadSchemSubnetGraph.h"


class CKicadSchemBoard;
class CKicadSchemBoardConnector;
class CKicadSchemBoardSource;
class CPinView;
class CSplitterView;
class CTraceAutoConnector;


//KiCad's 2.54mm (100 mil) pin spacing, its grid.
#define KICAD_SCHEM_LOGICIM_GRID			2.54

//Logicim grid units to one of KiCad's.  Logicim's parts are bigger than KiCad's symbols for their pins.
#define KICAD_SCHEM_LOGICIM_SCALE			2.0

//The grid units between the pins of the parts made for KiCad's symbols (subcircuit instances, 74 series parts and
//gates): a KiCad grid unit, so a tunnel fits on each pin and a splitter's ends line up with them.
#define KICAD_SCHEM_LOGICIM_PIN_SPACING		2

//The grid units between a bus splitter's ends (as the pins they go to) and between its ends and those pins.
#define KICAD_SCHEM_LOGICIM_SPLITTER_SPACING	2
#define KICAD_SCHEM_LOGICIM_SPLITTER_GAP		3

//The gap kept instead when a member wired to the splitter also has a tunnel, which needs room to be wired onto it.
#define KICAD_SCHEM_LOGICIM_SPLITTER_TUNNEL_GAP	8

//How far the components are spread out from where KiCad has them (scaled), at most and at least, and how many times the
//spread between is halved looking for the least that wires as well as the most.
#define KICAD_SCHEM_LOGICIM_SPREAD_MAXIMUM	1.5
#define KICAD_SCHEM_LOGICIM_SPREAD_MINIMUM	1.0
#define KICAD_SCHEM_LOGICIM_SPREAD_HALVINGS	3

//The gap kept between components' boxes.
#define KICAD_SCHEM_LOGICIM_SPACING			2


//How much more a component lined up with another (see SKicadSchemLogicimAlignment) would rather be moved along the
//line than off it when it's moved clear of the others.
#define KICAD_SCHEM_LOGICIM_ALIGN_WEIGHT	4


//Knots are wire turns and crossings.  An area of DENSE_WINDOW grid units square with more than DENSE_KNOTS is dense,
//and is exploded: EXPLODE_SPACE grid units are put in across it each way.
#define KICAD_SCHEM_LOGICIM_DENSE_WINDOW	12
#define KICAD_SCHEM_LOGICIM_DENSE_KNOTS		24
#define KICAD_SCHEM_LOGICIM_DENSE_AREAS		3
#define KICAD_SCHEM_LOGICIM_DENSE_SCAN		12
#define KICAD_SCHEM_LOGICIM_EXPLODE_SPACE	4
#define KICAD_SCHEM_LOGICIM_EXPLODE_PASSES	4


//How far a component is moved to keep it clear of the others before giving up.
#define KICAD_SCHEM_LOGICIM_SEPARATE_RADIUS	400


//Two components that are all of a subnet, wired by one straight wire in KiCad (their pins level or one above the
//other): their ports are lined up when they're placed so the wire stays straight.
struct SKicadSchemLogicimAlignment
{
	size		uiFromItem;
	size		uiToItem;
	CPortView*	pcFromPort;
	CPortView*	pcToPort;
	bool		bHorizontal;
};


//A bus splitter kept beside the component most of its ends are wired to: its ends face the way that component's pins
//face (iFacing, a TRACE_DIRECTION, is the pins'), each end lined up with its pin.  Its pairs are uiNumPairs from
//uiFirstPair in the converter's masSplitterPairs.
struct SKicadSchemLogicimSplitter
{
	CComponentView*		pcSplitter;
	int					iFacing;
	int32				iGap;  //KICAD_SCHEM_LOGICIM_SPLITTER_GAP, or more if a member has a tunnel too.
	size				uiFirstPair;
	size				uiNumPairs;
};


//A splitter's end (see SKicadSchemLogicimSplitter) and the component's pin it's wired straight to.
struct SKicadSchemLogicimSplitterPair
{
	CPortView*	pcPin;
	CPortView*	pcEnd;
};


//A logic part's pins (not its power pins), each split into its gate number (an index into aszIndices) and letters,
//with whether it's negated and where KiCad has it.
struct SKicadSchemLogicPins
{
	CArrayChars							aszIndices;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	CArrayInt							aiGroups;
	CArrayChars							aszLetters;
	CArrayInt							aiNegated;
	CArrayFloat64Vec2					asPositions;

	void	Init(void);
	void	Kill(void);
};


//How a gate's pins lie: its inputs (indices into the pins) in letter order, its output, and the centres of all its
//pins, of its inputs and of its outputs.
struct SKicadSchemLogicLayout
{
	CArrayInt			aiInputs;
	int					iOutput;
	SFloat64Vec2		sCentre;
	SFloat64Vec2		sInputs;
	SFloat64Vec2		sOutputs;
};


//A voltage regulator's output: the net it drives and its voltage.
struct SKicadSchemLogicimRegulated
{
	CKicadSchemNet*		pcNet;
	float32				fVoltage;
};


typedef CArrayTemplate<SKicadSchemLogicimRegulated>		CArrayKicadSchemLogicimRegulated;
typedef CArrayTemplate<SKicadSchemLogicimSplitter>		CArrayKicadSchemLogicimSplitter;
typedef CArrayTemplate<SKicadSchemLogicimSplitterPair>	CArrayKicadSchemLogicimSplitterPair;
typedef CArrayTemplate<SKicadSchemLogicimAlignment>		CArrayKicadSchemLogicimAlignment;


//Makes a Logicim circuit from one KiCad sheet.
//
//Each symbol becomes the components Logicim has for it.  A real 74 series part (see SLogic74Part) is placed as KiCad
//has it: each unit of a part drawn as units (a 74LVC00's "_Multi" symbol) is a 74 series unit, and a part drawn whole is
//a 74 series package, both named as the part.  A gate part's pins are found by their names ("1A", "1B" and "1Y"; "A"
//and "Y" on a single gate part, which is a unit) or, if they have no names, by their types; any other part's pins by
//the labels of its package's pins (see AddLogic74Block).  A 74 series gate or flip flop that isn't a part there's a row
//for becomes a generic gate (or D flip flop) for each of its gates.  Power pins are left out: the parts have implied
//power.  A power symbol becomes a ground or a positive supply of its voltage ("+3V3" is 3.3V).  A voltage regulator
//isn't drawn: the ICs whose power pins its output reaches are given its voltage as their implied Vcc (see AddRegulator
//and ApplyRegulatedVoltages).  A connector (a board's edge connector) becomes the circuit's pins, one for
//each net on it that isn't a supply's (see AddBoardPins).  Resistors and resistor networks, capacitors and capacitor
//networks, inductors, diodes, LEDs, mounting holes, net ties, test points and power flags are left out.  Any other
//symbol becomes an instance of a placeholder subcircuit made for its library symbol and unit: an empty circuit with a
//pin for each of its shown pins, on the side KiCad draws it, so it can be wired and its logic filled in later.
//Hierarchical sheets are left out.
//
//A subnet that is part of a bigger net (it's joined to the rest by labels) gets a tunnel named as the net at each of
//its labels, so each subnet is wired on its own, as KiCad drew it.  A tunnel or supply on a subnet with nothing else drawn on it (wired only to
//parts' left out power pins, or to nothing) isn't made.  A tunnel or supply KiCad has right on a pin is
//attached to the pin's port: port to port, pointing away from it, and moving with it.  The terminals of each subnet are
//wired as a tree in the order KiCad's wires join them, each connection hinted to bend as KiCad's wire first
//bends.  Connections between the same two components are routed as a run (see CTraceAutoConnector).
//
//A bus becomes a bus of its members in Logicim.  Each run of its entries on one side of it (a column of entries
//tapping a bus going up and down) becomes a splitter, its ends in the order KiCad's entries are, joined to the
//members' wires.  A splitter has an end for each member it taps and, if the bus has others, an end for the rest.  The
//splitters of a bus are wired to each other along it, and a bus whose name is on other buses gets a tunnel at each of
//its labels.
//
//Two components that are all of a subnet, joined by one straight wire in KiCad, are lined up wherever they're placed
//(the one with fewer connections is moved), and moving them clear of the others would rather move them along their
//line than off it (see KICAD_SCHEM_LOGICIM_ALIGN_WEIGHT).
//
//The components are first placed spread out from where KiCad has them (like raisins in rising dough,
//KICAD_SCHEM_LOGICIM_SPREAD_MAXIMUM) and wired, then at the least spread (KICAD_SCHEM_LOGICIM_SPREAD_MINIMUM).  If that
//wires as many connections it's kept; otherwise the spread halfway between the closest that wired as many and the
//widest that didn't is tried, KICAD_SCHEM_LOGICIM_SPREAD_HALVINGS times, and the closest that wired as many is kept.
//
//Then dense areas, where the wires knot (turn and cross a lot), are exploded a few times: space is put in across
//each one, up and down and left and right (so what's lined up stays lined up), and it's rewired.  An explosion is
//kept if it wires as well and leaves fewer knots over the limit in the dense areas.
class CKicadSchemLogicimConverter
{
protected:
	CKicadSchemFile*					mpcFile;
	CKicadSchemNetlist*					mpcNetlist;
	CCircuitViews*						mpcCircuits;
	CCircuitFonts*						mpcFonts;
	CCircuitView*						mpcCircuit;
	CKicadSchemLogicimModel				mcModel;
	CArrayKicadSchemLogicimAlignment	masAlignments;
	CArrayKicadSchemLogicimRegulated	masRegulated;
	CArrayKicadSchemLogicimSplitter		masSplitters;
	CArrayKicadSchemLogicimSplitterPair	masSplitterPairs;
	CKicadSchemSymbol*					mpcAddingSymbol;
	CKicadSchemBoard*					mpcBoard;  //The board the sheet is, its connectors' pins recorded.  NULL for none.
	CKicadSchemBoardSource*				mpcBoardSource;  //Where boards drawn as symbols come from.  NULL to make them placeholders.
	size								muiBoards;
	size								muiRegulators;
	float64								mfSpread;
	size								muiUnconnected;
	size								muiGates;
	size								muiPlaceholders;
	size								muiPowers;
	size								muiTunnels;
	size								muiSplitters;
	size								muiBlocks;
	size								muiBoardPins;
	size								muiSkipped;
	size								muiOverlaps;
	size								muiExplosions;
	size								muiExcessBefore;
	size								muiExcessAfter;

public:
	void							Init(CKicadSchemFile* pcFile, CKicadSchemNetlist* pcNetlist, CCircuitViews* pcCircuits, CCircuitFonts* pcFonts);
	void							Kill(void);

	void							SetBoards(CKicadSchemBoard* pcBoard, CKicadSchemBoardSource* pcBoardSource);
	bool							Convert(const char* szTypeName);

	CCircuitView*					GetCircuit(void);
	size							NumBoards(void);
	float64							GetSpread(void);
	size							NumConnections(void);
	size							NumUnconnected(void);
	SKicadSchemLogicimConnection*	GetConnection(size uiIndex);
	size							NumTerminals(void);
	SKicadSchemLogicimTerminal*		GetTerminal(size uiIndex);
	size							NumItems(void);
	size							NumGates(void);
	size							NumPlaceholders(void);
	size							NumPowers(void);
	size							NumRegulators(void);
	size							NumTunnels(void);
	size							NumSplitters(void);
	size							NumBlocks(void);
	size							NumBoardPins(void);
	size							NumSkipped(void);
	size							NumOverlaps(void);
	size							NumExplosions(void);
	size							GetExcessKnotsBefore(void);
	size							GetExcessKnotsAfter(void);
	size							FindDenseAreas(CArrayTemplate<SFloat64Vec2>* pasCentres);

	static bool						IsSkippedSymbol(CKicadSchemSymbol* pcSymbol);
	static bool						IsGroundName(const char* szName);
	static float32					GetSupplyVoltage(const char* szName);
	static ERotation				GetKicadRotation(ERotation eUpright, float64 fAngle);

protected:
	bool							AddSymbol(CKicadSchemSymbol* pcSymbol);
	bool							AddPower(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	bool							AddBoardPins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	CKicadSchemBoardConnector*		AddBoardConnector(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	void							AddBoardPin(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CKicadSchemBoardConnector* pcConnector);
	ERotation						GetBoardPinFacing(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, SFloat64Vec2* psOuter);
	bool							AddRegulator(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	bool							AddLogicParts(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, EKicadSchemLogicFunction eFunction, const char* szFamily, const char* szPartType);
	bool							AddLogic74Block(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, const char* szFamily, const char* szPartType);
	void							CollectLogic74Pins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayFloat64Vec2* pasPositions);
	void							CollectLogicPins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, SKicadSchemLogicPins* psPins);
	void							AddLogicPart(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, EKicadSchemLogicFunction eFunction, SKicadSchemLogicPins* psPins, size uiGroup, const char* szFamily);
	bool							AddLogic74Part(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, EKicadSchemLogicFunction eFunction, SKicadSchemLogicPins* psPins, const char* szFamily, const char* szPartType);
	void							AddLogic74Unit(CKicadSchemSymbol* pcSymbol, int iUnit, SKicadSchemLogicPins* psPins, SKicadSchemLogicLayout* psLayout, ERotation eRotation, const char* szFamily, const char* szPartType);
	void							AddLogic74Package(CKicadSchemSymbol* pcSymbol, SKicadSchemLogicPins* psPins, SKicadSchemLogicLayout* psLayout, ERotation eRotation, const char* szFamily, const char* szPartType);
	bool							AddBoard(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	void							LayOutBoardPins(CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CKicadSchemBoardConnector* pcConnector);
	bool							AddPlaceholder(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	CCircuitView*					GetPlaceholderCircuit(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	void							GetPlaceholderPins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayChars* paszNames);
	void							AddTunnels(void);
	CKicadSchemBus*					GetMemberBus(CKicadSchemSubnet* pcSubnet);
	bool							IsJoinedByBus(CKicadSchemNet* pcNet);
	void							ApplyRegulatedVoltages(void);
	float32							GetRegulatedVoltage(CKicadSchemSymbol* pcSymbol);
	float32							GetNetRegulatedVoltage(CKicadSchemNet* pcNet);
	void							RemoveUnconnectedItems(void);
	void							AddBuses(void);
	void							AddBus(CKicadSchemBus* pcBus, size uiBus);
	void							AddBusTunnels(CKicadSchemBus* pcBus);
	void							AddPinSplitters(CKicadSchemBus* pcBus, CArrayInt* paiTaps);
	void							AddRunSplitters(CKicadSchemBus* pcBus, CArrayInt* paiTaps);
	void							AddBusSplitter(CKicadSchemBus* pcBus, CArrayInt* paiTaps, bool bVertical, int iSide);
	void							AddBusSplitterTerminals(CKicadSchemBus* pcBus, CArrayInt* paiTaps, CSplitterView* pcSplitter, size uiItem, CArrayInt* paiOrder, CArrayInt* paiFans, CArrayTemplatePtr<CPortView>* papcPins, int iPinFacing);
	void							AddConnections(void);
	void							FindAlignments(void);
	void							AlignItems(void);
	void							AddSubnetConnections(CKicadSchemSubnet* pcSubnet);
	void							GetSubnetTerminals(CKicadSchemSubnet* pcSubnet, CArrayInt* paiTerminals);
	bool							IsConnectionBetween(size uiTerminal1, size uiTerminal2);
	bool							ReachesTerminal(size uiFrom, size uiTo, size uiWithout);
	void							JoinSplitterEnd(size uiEnd, size uiPin);

	size							AddItem(CComponentView* pcView, EKicadSchemLogicimItemKind eKind, SFloat64Vec2* psKicadPosition, CPortView* pcAnchor);
	void							AttachItems(void);
	void							PlaceAttachedItems(void);
	void							PlaceSplitters(void);
	bool							IsSplitterPin(CPortView* pcPortView);
	void							JoinSplitterPins(void);
	int								GetBusPins(CKicadSchemBus* pcBus, CArrayInt* paiTaps, CArrayTemplatePtr<CPortView>* papcPins);
	void							GetTapPinTerminals(CKicadSchemBus* pcBus, CArrayInt* paiTaps, CArrayTemplatePtr<SKicadSchemLogicimTerminal>* papsTerminals);
	CComponentView*					GetMostPinsOwner(CArrayTemplatePtr<SKicadSchemLogicimTerminal>* papsTerminals, int iFacing);
	CPortView*						GetTapPin(SKicadSchemBusTap* psTap);
	bool							IsComponentPin(SKicadSchemLogicimTerminal* psTerminal);
	CPortView*						GetComponentPin(CKicadSchemSubnet* pcSubnet, CComponentView* pcOwner, int iFacing);
	bool							HasTunnel(CKicadSchemSubnet* pcSubnet);
	bool							GetItemBox(size uiItem, SFloat64Bounds* psBox);
	void							MeasureImportBoxes(void);
	bool							GetItemImportBox(SKicadSchemLogicimItem* psItem, SFloat64Bounds* psBox);
	void							AddPinTerminal(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CPortView* pcPortView, size uiItem);

	void							Place(float64 fSpread);
	void							Explode(void);
	void							ExplodeAreas(CArrayTemplate<SFloat64Vec2>* pasCentres);
	void							SeparateItems(void);
	void							GetSeparateOrder(CArrayInt* paiOrder);
	size							Route(void);
	void							SetRouteBoxes(CTraceAutoConnector* pcConnector);
	void							AddRouteConnections(CTraceAutoConnector* pcConnector, CArrayTemplatePrimitive<size>* pauiRouted);
};


#endif // __KICAD_SCHEM_LOGICIM_CONVERTER_H__

