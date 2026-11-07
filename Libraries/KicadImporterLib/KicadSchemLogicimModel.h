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
#ifndef __KICAD_SCHEM_LOGICIM_MODEL_H__
#define __KICAD_SCHEM_LOGICIM_MODEL_H__
#include "BaseLib/ArrayTemplate.h"
#include "BaseLib/ArrayInt.h"
#include "BaseLib/MapBlock.h"
#include "BaseLib/MapStringTemplate.h"
#include "LogicimLib/CircuitViews.h"
#include "LogicimLib/Bounds.h"
#include "KicadLib/KicadSchemNetlist.h"


//KiCad's 2.54mm (100 mil) pin spacing, its grid.
#define KICAD_SCHEM_LOGICIM_GRID			2.54

//Logicim grid units to one of KiCad's.  Logicim's parts are bigger than KiCad's symbols for their pins.
#define KICAD_SCHEM_LOGICIM_SCALE			2.0


//KiCad's angles as Logicim's rotations.
int			GetKicadSchemQuarterTurns(float64 fAngle);
ERotation	GetKicadSchemRotation(ERotation eUpright, float64 fAngle);

//The rotations, turning right from East, to try something in each of.
extern ERotation	gaeKicadSchemLogicimRotations[];


//The middle of the points added to it.
struct SKicadSchemPointMean
{
	SFloat64Vec2		sSum;
	size				uiCount;

	void	Init(void);
	void	Add(SFloat64Vec2* psPoint);
	void	Get(SFloat64Vec2* psMean);
};


//Which way a component keeps to the line it's lined up on with another.
enum EKicadSchemLogicimAlign
{
	KSLA_None,
	KSLA_KeepY,		//A horizontal wire: moved across, not up or down.
	KSLA_KeepX,		//A vertical wire.
};


//What the converter made an item for.
enum EKicadSchemLogicimItemKind
{
	KSLIK_Part,			//A symbol's component: a gate, flip flop, 74 part, logic block...
	KSLIK_Supply,		//A power symbol's supply.
	KSLIK_Tunnel,		//A label's or a bus's tunnel.
	KSLIK_Splitter,		//A bus's splitter.
	KSLIK_BoardPin,		//A connector pin's subcircuit pin.
	KSLIK_Subcircuit,	//A board or placeholder instance.
};


//A component made in the Logicim circuit and where KiCad has it, in Logicim grid units (see KICAD_SCHEM_LOGICIM_SCALE).
//The anchor is the point of the view that goes there (a power symbol's port, otherwise the view's position), from its position.  A tunnel or power
//supply has one port; if KiCad has it right on a pin it's attached to that pin's port: it goes port to port with it
//(Logicim joins ports in the same place), pointing away, and moves with it.
struct SKicadSchemLogicimItem
{
	CComponentView*				pcView;
	EKicadSchemLogicimItemKind	eKind;
	bool						bPlacedSplitter;  //A splitter PlaceSplitters puts beside its component.
	SFloat64Vec2				sKicadPosition;
	SInt32Vec2					sAnchor;
	SInt32Vec2					sOffset;
	CPortView*					pcPort;
	CPortView*					pcAttachedTo;
	EKicadSchemLogicimAlign		eAlign;
	CKicadSchemSymbol*			pcSymbol;  //The symbol it was made for.  NULL for tunnels and splitters.
	SFloat64Bounds				sImportBox;  //Its import box (see CComponentView::GetImportBoundingBox), from its position.
	bool						bImportBox;
};


//A port in the Logicim circuit standing for a pin, power symbol or label of a subnet, or (bBus) a bus's splitter or
//tunnel, which carries all its members.
struct SKicadSchemLogicimTerminal
{
	CKicadSchemSubnet*	pcSubnet;
	SFloat64Vec2		sKicadPosition;
	CPortView*			pcPortView;
	size				uiItem;
	bool				bBus;
};


//Two terminals of a subnet to be wired, the from terminal to the to terminal (already wired to the rest of the
//subnet), with the bend KiCad's wire between them has.
struct SKicadSchemLogicimConnection
{
	CKicadSchemSubnet*	pcSubnet;
	size				uiFrom;
	size				uiTo;
	float64				fBend;
	bool				bConnected;
};


//The model's item: the item, and its terminals, the items attached to it and the next attached to the same item, by
//index (ARRAY_ELEMENT_NOT_FOUND for none).
struct SKicadSchemLogicimModelItem
{
	SKicadSchemLogicimItem	sItem;
	size					uiFirstTerminal;
	size					uiLastTerminal;
	size					uiFirstAttached;
	size					uiLastAttached;
	size					uiNextAttached;
};


//The model's terminal: the terminal, and the next of its subnet's terminals and of its item's
//(ARRAY_ELEMENT_NOT_FOUND after the last).
struct SKicadSchemLogicimModelTerminal
{
	SKicadSchemLogicimTerminal	sTerminal;
	size						uiNextOnSubnet;
	size						uiNextOfItem;
};


//The first and last of a subnet's terminals (see CKicadSchemLogicimModel::FirstSubnetTerminal).
struct SKicadSchemLogicimSubnetTerminals
{
	size	uiFirst;
	size	uiLast;
};


typedef CArrayTemplate<SKicadSchemLogicimModelItem>		CArrayKicadSchemLogicimModelItem;
typedef CArrayTemplate<SKicadSchemLogicimModelTerminal>	CArrayKicadSchemLogicimModelTerminal;
typedef CArrayTemplate<SKicadSchemLogicimConnection>	CArrayKicadSchemLogicimConnection;


//What the converter has made so far: the circuit, its items (the Logicim components), their terminals (the ports
//standing for KiCad's pins, labels and supplies on each subnet) and the connections to be wired between terminals.
class CKicadSchemLogicimModel
{
protected:
	CCircuitView*							mpcCircuit;
	CArrayKicadSchemLogicimModelItem		masItems;
	CArrayKicadSchemLogicimModelTerminal	masTerminals;
	CArrayKicadSchemLogicimConnection		masConnections;
	CMapBlock								mcSubnetTerminals;  //A subnet to its SKicadSchemLogicimSubnetTerminals.
	CMapBlock								mcPortTerminals;    //A port to its first terminal.
	CMapBlock								mcViewItems;        //A view to its item.
	CMapStringTemplate<size>				mcBoardPins;        //A board pin's name to its item.

	void							LinkItem(size uiItem);
	void							LinkTerminal(size uiTerminal);
	void							LinkAttached(size uiItem);
	void							Index(void);

public:
	void							Init(void);
	void							Kill(void);

	void							SetCircuit(CCircuitView* pcCircuit);
	CCircuitView*					GetCircuit(void);

	size							AddItem(CComponentView* pcView, EKicadSchemLogicimItemKind eKind, SFloat64Vec2* psKicadPosition, CPortView* pcAnchor, CKicadSchemSymbol* pcSymbol);
	void							AddTerminal(CKicadSchemSubnet* pcSubnet, SFloat64Vec2* psKicadPosition, CPortView* pcPortView, size uiItem, bool bBus = false);
	SKicadSchemLogicimConnection*	AddConnection(void);
	void							RemoveItems(CArrayInt* paiRemoved);
	void							AttachItem(size uiItem, CPortView* pcTo);

	size							NumItems(void);
	SKicadSchemLogicimItem*			GetItem(size uiIndex);
	size							NumTerminals(void);
	SKicadSchemLogicimTerminal*		GetTerminal(size uiIndex);
	size							NumConnections(void);
	SKicadSchemLogicimConnection*	GetConnection(size uiIndex);

	size							FirstSubnetTerminal(CKicadSchemSubnet* pcSubnet);
	size							NextSubnetTerminal(size uiTerminal);
	size							FirstItemTerminal(size uiItem);
	size							NextItemTerminal(size uiTerminal);
	size							NumItemTerminals(size uiItem);
	size							FirstAttached(size uiItem);
	size							NextAttached(size uiItem);
	size							FindPortTerminal(CPortView* pcPortView);
	size							FindItem(CComponentView* pcView);
	size							FindBoardPin(const char* szName);
	void							GetSubnetTerminals(CKicadSchemSubnet* pcSubnet, CArrayInt* paiTerminals);
	bool							GetItemImportBox(size uiItem, SFloat64Bounds* psBox);
};


#endif // __KICAD_SCHEM_LOGICIM_MODEL_H__

