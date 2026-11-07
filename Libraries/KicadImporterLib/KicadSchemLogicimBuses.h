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
#ifndef __KICAD_SCHEM_LOGICIM_BUSES_H__
#define __KICAD_SCHEM_LOGICIM_BUSES_H__
#include "BaseLib/ArrayTemplate.h"
#include "BaseLib/ArrayTemplatePtr.h"
#include "BaseLib/ArrayInt.h"
#include "BaseLib/ArraySize.h"
#include "LogicimLib/CircuitFonts.h"
#include "KicadLib/KicadSchemNetlist.h"
#include "KicadSchemLogicimModel.h"
#include "KicadSchemLogicimCounts.h"


class CSplitterView;


//The grid units between a bus splitter's ends (as the pins they go to) and between its ends and those pins.
#define KICAD_SCHEM_LOGICIM_SPLITTER_SPACING	2
#define KICAD_SCHEM_LOGICIM_SPLITTER_GAP		3

//The gap kept instead when a member wired to the splitter also has a tunnel, which needs room to be wired onto it.
#define KICAD_SCHEM_LOGICIM_SPLITTER_TUNNEL_GAP	8


//A bus splitter kept beside the component most of its ends are wired to: its ends face the way that component's pins
//face (iFacing, a TRACE_DIRECTION, is the pins'), each end lined up with its pin.  Its pairs are uiNumPairs from
//uiFirstPair in the buses' splitter pairs.
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


//A run of a bus's taps being made into a splitter (see CKicadSchemLogicimBuses::AddBusSplitter): the component pins its
//taps' members are wired to (by tap, NULL for a tap wired to none) and the way they face (a TRACE_DIRECTION,
//TRACE_DIRECTION_NONE if none do), the order its ends go to the taps (indices into pauiTaps) and its ends in order along
//the bus.
struct SKicadSchemLogicimBusRun
{
	CKicadSchemBus*					pcBus;
	CArraySize*						pauiTaps;
	CArrayTemplatePtr<CPortView>	apcPins;
	int								iPinFacing;
	CArraySize						auiOrder;
	CArrayInt						aiFans;

	void				Init(CKicadSchemBus* pcBus, CArraySize* pauiTaps);
	void				Kill(void);
	size				NumTaps(void);
	SKicadSchemBusTap*	GetTap(size uiIndex);
};


typedef CArrayTemplate<SKicadSchemLogicimSplitter>		CArrayKicadSchemLogicimSplitter;
typedef CArrayTemplate<SKicadSchemLogicimSplitterPair>	CArrayKicadSchemLogicimSplitterPair;


//Makes the tunnels that join a net's subnets, and the splitters and tunnels of the buses (see
//CKicadSchemLogicimConverter), adding them and their terminals to the model.
class CKicadSchemLogicimBuses
{
protected:
	CKicadSchemNetlist*					mpcNetlist;
	CCircuitFonts*						mpcFonts;
	CKicadSchemLogicimModel*			mpcModel;
	SKicadSchemLogicimCounts*			mpsCounts;
	CArrayKicadSchemLogicimSplitter		masSplitters;
	CArrayKicadSchemLogicimSplitterPair	masSplitterPairs;

public:
	void							Init(CKicadSchemNetlist* pcNetlist, CCircuitFonts* pcFonts, CKicadSchemLogicimModel* pcModel, SKicadSchemLogicimCounts* psCounts);
	void							Kill(void);

	void							AddTunnels(void);
	void							AddBuses(void);

	bool							IsSplitterPin(CPortView* pcPortView);
	size							NumSplitters(void);
	SKicadSchemLogicimSplitter*		GetSplitter(size uiIndex);
	size							NumSplitterPairs(void);
	SKicadSchemLogicimSplitterPair*	GetSplitterPair(size uiIndex);

protected:
	CKicadSchemBus*					GetMemberBus(CKicadSchemSubnet* pcSubnet);
	bool							IsJoinedByBus(CKicadSchemNet* pcNet);
	void							AddBus(CKicadSchemBus* pcBus);
	void							AddBusTunnels(CKicadSchemBus* pcBus);
	void							AddPinSplitters(CKicadSchemBus* pcBus, CArraySize* pauiTaps);
	void							AddRunSplitters(CKicadSchemBus* pcBus, CArraySize* pauiTaps);
	void							AddBusSplitter(CKicadSchemBus* pcBus, CArraySize* pauiTaps, int iSide);
	void							AddBusSplitterTerminals(SKicadSchemLogicimBusRun* psRun, CSplitterView* pcSplitter, size uiItem);
	int								GetBusPins(CKicadSchemBus* pcBus, CArraySize* pauiTaps, CArrayTemplatePtr<CPortView>* papcPins);
	void							GetTapPinTerminals(CKicadSchemBus* pcBus, CArraySize* pauiTaps, CArrayTemplatePtr<SKicadSchemLogicimTerminal>* papsTerminals);
	CComponentView*					GetMostPinsOwner(CArrayTemplatePtr<SKicadSchemLogicimTerminal>* papsTerminals, int iFacing);
	CPortView*						GetTapPin(SKicadSchemBusTap* psTap);
	bool							IsComponentPin(SKicadSchemLogicimTerminal* psTerminal);
	CPortView*						GetComponentPin(CKicadSchemSubnet* pcSubnet, CComponentView* pcOwner, int iFacing);
	bool							HasTunnel(CKicadSchemSubnet* pcSubnet);
};


#endif // __KICAD_SCHEM_LOGICIM_BUSES_H__

