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
#include <math.h>
#include "BaseLib/ConstructorCall.h"
#include "BaseLib/StringHelper.h"
#include "BaseLib/Logger.h"
#include "BaseLib/PointerFunctions.h"
#include "LogicimLib/AndGateView.h"
#include "LogicimLib/OrGateView.h"
#include "LogicimLib/BufferView.h"
#include "LogicimLib/Logic74View.h"
#include "LogicimLib/FlipFlopView.h"
#include "LogicimLib/PowerSourceView.h"
#include "LogicimLib/TunnelView.h"
#include "LogicimLib/CircuitTextMarkupParser.h"
#include "LogicimLib/SplitterView.h"
#include "LogicimLib/ValueView.h"
#include "LogicimLib/SubcircuitInstanceView.h"
#include "LogicimLib/TraceAutoConnector.h"
#include "LogicimLib/ComponentShapes.h"
#include "KicadSchemLogicimConverter.h"
#include "KicadSchemBoards.h"
#include "KicadSchemLogicimConverterInternal.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::Init(CKicadSchemFile* pcFile, CKicadSchemNetlist* pcNetlist, CCircuitViews* pcCircuits, CCircuitFonts* pcFonts)
{
	mpcFile = pcFile;
	mpcNetlist = pcNetlist;
	mpcCircuits = pcCircuits;
	mpcFonts = pcFonts;
	mcModel.Init();
	masRegulated.Init();
	mpcBoard = NULL;
	mpcBoardSource = NULL;
	msCounts.Init();
	mcBuses.Init(pcNetlist, pcFonts, &mcModel, &msCounts);
	mcWiring.Init(pcNetlist, &mcModel, &mcBuses);
	mcPlacer.Init(pcFonts, &mcModel, &mcBuses, &mcWiring, &msCounts);
}


//////////////////////////////////////////////////////////////////////////
//
// The circuits keep what was made.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::Kill(void)
{
	mcPlacer.Kill();
	mcWiring.Kill();
	mcBuses.Kill();
	masRegulated.Kill();
	mcModel.Kill();
	mpcBoardSource = NULL;
	mpcBoard = NULL;
	mpcFonts = NULL;
	mpcCircuits = NULL;
	mpcNetlist = NULL;
	mpcFile = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// pcBoard records the pins of the sheet's connectors, so the sheet can be
// drawn as a symbol on another board.  A symbol with a Schematic property
// (another board drawn on this one) is an instance of the board
// pcBoardSource makes from it.  Either may be NULL.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::SetBoards(CKicadSchemBoard* pcBoard, CKicadSchemBoardSource* pcBoardSource)
{
	mpcBoard = pcBoard;
	mpcBoardSource = pcBoardSource;
}


//////////////////////////////////////////////////////////////////////////
//
// Makes the circuit szTypeName in the circuits from the sheet.  The
// netlist must be built.  Fails if there is already a circuit of the type.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::Convert(const char* szTypeName)
{
	size			i;
	CCircuitView*	pcCircuit;

	pcCircuit = mpcCircuits->Add(szTypeName);
	if (pcCircuit == NULL)
	{
		return false;
	}
	mcModel.SetCircuit(pcCircuit);

	for (i = 0; i < mpcFile->NumSymbols(); i++)
	{
		if (!AddSymbol(mpcFile->GetSymbol(i)))
		{
			return false;
		}
	}
	ApplyRegulatedVoltages();
	mcBuses.AddTunnels();
	mcBuses.AddBuses();
	RemoveUnconnectedItems();
	mcPlacer.AttachItems();
	mcPlacer.MeasureImportBoxes();
	mcPlacer.GroupConnectorPins();
	mcWiring.AddConnections();
	mcPlacer.FindAlignments();
	PlaceAndRoute();
	mcPlacer.Explode();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Placed and wired spread out, then as close as it wires as well: the
// least spread if that does, otherwise halfway between the closest that
// does and the widest that doesn't, a few times.  The circuit is left
// placed and wired at the chosen spread.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::PlaceAndRoute(void)
{
	size	i;
	size	uiFirstUnconnected;
	float64	fClosest;
	float64	fWorse;
	float64	fSpread;
	bool	bRouted;

	mcPlacer.Place(KICAD_SCHEM_LOGICIM_SPREAD_MAXIMUM);
	uiFirstUnconnected = mcWiring.Route();
	fClosest = KICAD_SCHEM_LOGICIM_SPREAD_MAXIMUM;
	mcPlacer.Place(KICAD_SCHEM_LOGICIM_SPREAD_MINIMUM);
	msCounts.uiUnconnected = mcWiring.Route();
	if (msCounts.uiUnconnected <= uiFirstUnconnected)
	{
		return;
	}

	fWorse = KICAD_SCHEM_LOGICIM_SPREAD_MINIMUM;
	bRouted = false;
	for (i = 0; i < KICAD_SCHEM_LOGICIM_SPREAD_HALVINGS; i++)
	{
		fSpread = (fClosest + fWorse) / 2;
		mcPlacer.Place(fSpread);
		msCounts.uiUnconnected = mcWiring.Route();
		if (msCounts.uiUnconnected > uiFirstUnconnected)
		{
			fWorse = fSpread;
			bRouted = false;
		}
		else
		{
			fClosest = fSpread;
			bRouted = true;
		}
	}
	if (!bRouted)
	{
		mcPlacer.Place(fClosest);
		msCounts.uiUnconnected = mcWiring.Route();
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddSymbol(CKicadSchemSymbol* pcSymbol)
{
	CKicadSchemLibSymbol*		pcLibSymbol;
	SKicadSchemLogicPart		sPart;
	bool						bAdded;

	pcLibSymbol = mpcFile->GetLibSymbol(pcSymbol);
	if (pcLibSymbol == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Symbol [", pcSymbol->GetReference(), "] has no library symbol.", NULL);
	}
	if (pcLibSymbol->IsPower())
	{
		return AddPower(pcSymbol, pcLibSymbol);
	}
	if (IsKicadSchemSkippedSymbol(pcSymbol))
	{
		msCounts.uiSkipped++;
		return true;
	}
	if (AddRegulator(pcSymbol, pcLibSymbol) || AddBoardPins(pcSymbol, pcLibSymbol) || AddBoard(pcSymbol, pcLibSymbol))
	{
		return true;
	}

	//A real 74 series part that isn't gates, or gates or flip flops.
	sPart.Init(pcSymbol, pcLibSymbol);
	bAdded = (sPart.Parse74Part() && AddLogic74Block(&sPart)) || (sPart.ParseFunction() && AddLogicParts(&sPart));
	sPart.Kill();
	if (bAdded)
	{
		return true;
	}
	return AddPlaceholder(pcSymbol, pcLibSymbol);
}


//////////////////////////////////////////////////////////////////////////
//
// A tunnel or supply whose subnet has nothing drawn on it but tunnels and
// supplies (a supply wired only to parts' power pins, which are left out
// as the parts have implied power, or a label on a wire to nothing) would
// be drawn joined to nothing, so it's taken out with its terminal, and so
// isn't wired.  Bus tunnels are kept: they're on their bus's splitters.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::RemoveUnconnectedItems(void)
{
	size							i;
	SKicadSchemLogicimTerminal*		psTerminal;
	SKicadSchemLogicimItem*			psItem;
	CArrayInt						aiRemoved;

	aiRemoved.Init();
	for (i = 0; i < mcModel.NumItems(); i++)
	{
		aiRemoved.Add(0);
	}
	for (i = 0; i < mcModel.NumTerminals(); i++)
	{
		psTerminal = mcModel.GetTerminal(i);
		if (!IsDrawnTerminal(psTerminal) && !IsSubnetDrawn(psTerminal->pcSubnet))
		{
			aiRemoved.SetValue(psTerminal->uiItem, 1);
		}
	}

	for (i = 0; i < mcModel.NumItems(); i++)
	{
		psItem = mcModel.GetItem(i);
		if (aiRemoved.GetValue(i) != 0)
		{
			if (psItem->eKind == KSLIK_Tunnel)
			{
				msCounts.uiTunnels--;
			}
			else
			{
				msCounts.uiPowers--;
			}
		}
	}
	mcModel.RemoveItems(&aiRemoved);
	aiRemoved.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the terminal is something drawn: a component's pin or a bus's,
// not a tunnel's or a supply's.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsDrawnTerminal(SKicadSchemLogicimTerminal* psTerminal)
{
	return psTerminal->bBus || (mcModel.GetItem(psTerminal->uiItem)->pcPort == NULL);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether one of the subnet's terminals is drawn (see IsDrawnTerminal).
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsSubnetDrawn(CKicadSchemSubnet* pcSubnet)
{
	size	i;

	for (i = mcModel.FirstSubnetTerminal(pcSubnet); i != ARRAY_ELEMENT_NOT_FOUND; i = mcModel.NextSubnetTerminal(i))
	{
		if (IsDrawnTerminal(mcModel.GetTerminal(i)))
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The way from the inputs to the output.
//
//////////////////////////////////////////////////////////////////////////
ERotation GetKicadSchemFacing(SFloat64Vec2* psFrom, SFloat64Vec2* psTo)
{
	float64		fDX;
	float64		fDY;

	fDX = psTo->x - psFrom->x;
	fDY = psTo->y - psFrom->y;
	if (fabs(fDX) >= fabs(fDY))
	{
		return (fDX >= 0) ? ROT_East : ROT_West;
	}
	return (fDY > 0) ? ROT_South : ROT_North;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddPinTerminal(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CPortView* pcPortView, size uiItem)
{
	SFloat64Vec2		sPosition;

	mpcFile->GetPinPosition(&sPosition, pcSymbol, pcPin);
	mcModel.AddTerminal(mpcNetlist->GetPinSubnet(pcSymbol, pcPin), &sPosition, pcPortView, uiItem);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CKicadSchemLogicimConverter::GetCircuit(void) { return mcModel.GetCircuit(); }
SKicadSchemLogicimCounts* CKicadSchemLogicimConverter::GetCounts(void) { return &msCounts; }
CKicadSchemLogicimModel* CKicadSchemLogicimConverter::GetModel(void) { return &mcModel; }
