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
#include "LogicimLib/CircuitViews.h"
#include "LogicimLib/Bounds.h"
#include "KicadLib/KicadSchemNetlist.h"


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


//A component made in the Logicim circuit and where KiCad has it, in Logicim grid units (see KICAD_SCHEM_LOGICIM_SCALE).  The anchor is the point of the view
//that goes there (a power symbol's port, otherwise the view's position), from its position.  A tunnel or power
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
	size		uiSubnet;
	size		uiFrom;
	size		uiTo;
	float64		fBend;
	bool		bConnected;
};


typedef CArrayTemplate<SKicadSchemLogicimItem>			CArrayKicadSchemLogicimItem;
typedef CArrayTemplate<SKicadSchemLogicimTerminal>		CArrayKicadSchemLogicimTerminal;
typedef CArrayTemplate<SKicadSchemLogicimConnection>	CArrayKicadSchemLogicimConnection;


//What the converter has made so far: the items (the Logicim components), their terminals (the ports standing for
//KiCad's pins, labels and supplies on each subnet) and the connections to be wired between terminals.
class CKicadSchemLogicimModel
{
protected:
	CArrayKicadSchemLogicimItem			masItems;
	CArrayKicadSchemLogicimTerminal		masTerminals;
	CArrayKicadSchemLogicimConnection	masConnections;

public:
	void							Init(void);
	void							Kill(void);

	size							AddItem(CComponentView* pcView, EKicadSchemLogicimItemKind eKind, SFloat64Vec2* psPosition, CPortView* pcAnchor, CKicadSchemSymbol* pcSymbol);
	void							AddTerminal(CKicadSchemSubnet* pcSubnet, SFloat64Vec2* psKicadPosition, CPortView* pcPortView, size uiItem, bool bBus = false);
	SKicadSchemLogicimConnection*	AddConnection(void);
	void							RemoveItems(CArrayInt* paiRemoved);

	size							NumItems(void);
	SKicadSchemLogicimItem*			GetItem(size uiIndex);
	size							NumTerminals(void);
	SKicadSchemLogicimTerminal*		GetTerminal(size uiIndex);
	size							NumConnections(void);
	SKicadSchemLogicimConnection*	GetConnection(size uiIndex);
};


#endif // __KICAD_SCHEM_LOGICIM_MODEL_H__

