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
#include "KicadSchemLogicPins.h"
#include "KicadSchemLogicimModel.h"
#include "KicadSchemLogicimCounts.h"
#include "KicadSchemLogicimBuses.h"
#include "KicadSchemLogicimWiring.h"
#include "KicadSchemLogicimPlacer.h"


class CKicadSchemBoard;
class CKicadSchemBoardConnector;
class CKicadSchemBoardSource;
class CPinView;
class CSubcircuitInstanceView;


//The grid units between the pins of the parts made for KiCad's symbols (subcircuit instances, 74 series parts and
//gates): a KiCad grid unit, so a tunnel fits on each pin and a splitter's ends line up with them.
#define KICAD_SCHEM_LOGICIM_PIN_SPACING		2

//How far the components are spread out from where KiCad has them (scaled), at most and at least, and how many times the
//spread between is halved looking for the least that wires as well as the most.
#define KICAD_SCHEM_LOGICIM_SPREAD_MAXIMUM	1.5
#define KICAD_SCHEM_LOGICIM_SPREAD_MINIMUM	1.0
#define KICAD_SCHEM_LOGICIM_SPREAD_HALVINGS	3


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


//A symbol's logic part as its name says: a real 74 series part (see Parse74Part) or the function of its gates or flip
//flops (KSLF_None if it isn't parsed as one), with its family ("LVC") and part type ("74LVC00").
struct SKicadSchemLogicPart
{
	CKicadSchemSymbol*			pcSymbol;
	CKicadSchemLibSymbol*		pcLibSymbol;
	EKicadSchemLogicFunction	eFunction;
	CChars						szFamily;
	CChars						szPartType;

	void	Init(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	void	Kill(void);
	bool	Parse74Part(void);
	bool	ParseFunction(void);
};


//A voltage regulator's output: the net it drives and its voltage.
struct SKicadSchemLogicimRegulated
{
	CKicadSchemNet*		pcNet;
	float32				fVoltage;
};


typedef CArrayTemplate<SKicadSchemLogicimRegulated>		CArrayKicadSchemLogicimRegulated;


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
	CKicadSchemLogicimModel				mcModel;
	CArrayKicadSchemLogicimRegulated	masRegulated;
	CKicadSchemLogicimBuses				mcBuses;
	CKicadSchemLogicimWiring			mcWiring;
	CKicadSchemLogicimPlacer			mcPlacer;
	CKicadSchemBoard*					mpcBoard;  //The board the sheet is, its connectors' pins recorded.  NULL for none.
	CKicadSchemBoardSource*				mpcBoardSource;  //Where boards drawn as symbols come from.  NULL to make them placeholders.
	SKicadSchemLogicimCounts			msCounts;

public:
	void							Init(CKicadSchemFile* pcFile, CKicadSchemNetlist* pcNetlist, CCircuitViews* pcCircuits, CCircuitFonts* pcFonts);
	void							Kill(void);

	void							SetBoards(CKicadSchemBoard* pcBoard, CKicadSchemBoardSource* pcBoardSource);
	bool							Convert(const char* szTypeName);

	CCircuitView*					GetCircuit(void);
	SKicadSchemLogicimCounts*		GetCounts(void);
	CKicadSchemLogicimModel*		GetModel(void);

protected:
	bool							AddSymbol(CKicadSchemSymbol* pcSymbol);
	bool							AddPower(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	bool							AddBoardPins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	CKicadSchemBoardConnector*		AddBoardConnector(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	void							AddBoardPin(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CKicadSchemBoardConnector* pcConnector);
	void							JoinBoardPin(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CKicadSchemBoardConnector* pcConnector, size uiItem);
	void							AddBoardPinView(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CKicadSchemBoardConnector* pcConnector, const char* szName);
	ERotation						GetBoardPinFacing(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, SFloat64Vec2* psOuter);
	bool							AddRegulator(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	void							AddRegulatedNets(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, float32 fVoltage);
	bool							AddLogicParts(SKicadSchemLogicPart* psPart);
	bool							AddLogic74Block(SKicadSchemLogicPart* psPart);
	void							AddLogic74BlockView(SKicadSchemLogicPart* psPart, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayFloat64Vec2* pasPositions, CComponentView* pcView, bool bUnit);
	void							CollectLogic74Pins(SKicadSchemLogicPart* psPart, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayFloat64Vec2* pasPositions);
	void							CollectLogicPins(SKicadSchemLogicPart* psPart, SKicadSchemLogicPins* psPins);
	void							AddLogicPart(SKicadSchemLogicPart* psPart, SKicadSchemLogicPins* psPins, size uiGroup);
	bool							AddLogic74Part(SKicadSchemLogicPart* psPart, SKicadSchemLogicPins* psPins);
	void							AddLogic74Unit(SKicadSchemLogicPart* psPart, int iUnit, SKicadSchemLogicPins* psPins, SKicadSchemLogicLayout* psLayout, ERotation eRotation);
	void							AddLogic74Package(SKicadSchemLogicPart* psPart, SKicadSchemLogicPins* psPins, SKicadSchemLogicLayout* psLayout, ERotation eRotation);
	bool							AddBoard(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	void							JoinBoardPins(CKicadSchemSymbol* pcSymbol, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CKicadSchemBoardConnector* pcConnector, CSubcircuitInstanceView* pcInstance, size uiItem);
	CSubcircuitInstanceView*		AddSubcircuitInstance(CKicadSchemSymbol* pcSymbol, CCircuitView* pcSubcircuit, size* puiItem);
	void							LayOutBoardPins(CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CKicadSchemBoardConnector* pcConnector);
	bool							AddPlaceholder(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	CCircuitView*					GetPlaceholderCircuit(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol);
	void							GetPlaceholderPins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayChars* paszNames);
	void							ApplyRegulatedVoltages(void);
	float32							GetRegulatedVoltage(CKicadSchemSymbol* pcSymbol);
	float32							GetNetRegulatedVoltage(CKicadSchemNet* pcNet);
	void							RemoveUnconnectedItems(void);
	bool							IsDrawnTerminal(SKicadSchemLogicimTerminal* psTerminal);
	bool							IsSubnetDrawn(CKicadSchemSubnet* pcSubnet);
	void							PlaceAndRoute(void);

	void							AddPinTerminal(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CPortView* pcPortView, size uiItem);
};


#endif // __KICAD_SCHEM_LOGICIM_CONVERTER_H__

