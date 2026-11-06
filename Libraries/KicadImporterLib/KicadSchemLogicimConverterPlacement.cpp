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
// Explodes the dense areas while that wires as well and leaves fewer
// knots over the dense areas' limit, as many times as
// KICAD_SCHEM_LOGICIM_EXPLODE_PASSES.  If exploding them all is worse,
// exploding just the densest is tried before giving up.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::Explode(void)
{
	CArrayTemplate<SFloat64Vec2>	asCentres;
	CArrayTemplate<SInt32Vec2>		asOffsets;
	size							uiPass;
	size							i;
	size							uiExcess;
	size							uiNewExcess;
	size							uiUnconnected;
	bool							bOnlyDensest;

	asCentres.Init();
	asOffsets.Init();
	uiExcess = FindDenseAreas(&asCentres);
	muiExcessBefore = uiExcess;
	bOnlyDensest = false;
	for (uiPass = 0; (uiPass < KICAD_SCHEM_LOGICIM_EXPLODE_PASSES) && (asCentres.NumElements() > 0); uiPass++)
	{
		asOffsets.ReInit();
		for (i = 0; i < masItems.NumElements(); i++)
		{
			asOffsets.Add(&masItems.Get(i)->sOffset);
		}

		if (bOnlyDensest)
		{
			while (asCentres.NumElements() > 1)
			{
				asCentres.RemoveTail();
			}
		}
		ExplodeAreas(&asCentres);
		Place(mfSpread);
		uiUnconnected = Route();
		asCentres.ReInit();
		uiNewExcess = FindDenseAreas(&asCentres);
		if ((uiUnconnected > muiUnconnected) || (uiNewExcess >= uiExcess))
		{
			//Worse: back to how it was.
			for (i = 0; i < masItems.NumElements(); i++)
			{
				masItems.Get(i)->sOffset = *asOffsets.Get(i);
			}
			Place(mfSpread);
			muiUnconnected = Route();
			if (bOnlyDensest)
			{
				break;
			}
			bOnlyDensest = true;
			asCentres.ReInit();
			FindDenseAreas(&asCentres);
			continue;
		}
		muiUnconnected = uiUnconnected;
		uiExcess = uiNewExcess;
		muiExplosions++;
		bOnlyDensest = false;
	}
	muiExcessAfter = uiExcess;
	asOffsets.Kill();
	asCentres.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Makes room through each dense area by putting space in along a line
// across it each way: everything right of its centre moves right and
// everything below it moves down, by KICAD_SCHEM_LOGICIM_EXPLODE_SPACE.
// Things on the same side of a line move together, so what was lined up
// stays lined up and a wire across a line only gets longer.  Attached
// items move with their pins.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::ExplodeAreas(CArrayTemplate<SFloat64Vec2>* pasCentres)
{
	size						i;
	size						j;
	SKicadSchemLogicimItem*		psItem;
	SInt32Vec2					sPosition;
	SFloat64Vec2*				psCentre;

	for (i = 0; i < masItems.NumElements(); i++)
	{
		psItem = masItems.Get(i);
		if (psItem->pcAttachedTo != NULL)
		{
			continue;
		}
		sPosition = psItem->pcView->GetPosition();
		sPosition.x += psItem->sAnchor.x;
		sPosition.y += psItem->sAnchor.y;
		for (j = 0; j < pasCentres->NumElements(); j++)
		{
			psCentre = pasCentres->Get(j);
			if (sPosition.x > psCentre->x)
			{
				psItem->sOffset.x += KICAD_SCHEM_LOGICIM_EXPLODE_SPACE;
			}
			if (sPosition.y > psCentre->y)
			{
				psItem->sOffset.y += KICAD_SCHEM_LOGICIM_EXPLODE_SPACE;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit's knots: where a wire turns or ends (but not on a port) or
// where two wires cross.
//
//////////////////////////////////////////////////////////////////////////
static void CollectKicadSchemKnots(CCircuitView* pcCircuit, CArrayTemplate<SInt32Vec2>* pasKnots)
{
	CArrayPortViewPtr	apcPorts;
	size				i;
	size				j;
	size				k;
	CTraceView*			pcTrace;
	CTraceView*			pcOther;
	SInt32Vec2			sPoint;
	bool				bPort;

	apcPorts.Init();
	pcCircuit->CollectPortViews(&apcPorts);
	for (i = 0; i < pcCircuit->NumTraces(); i++)
	{
		pcTrace = pcCircuit->GetTrace(i);
		for (j = 0; j < 2; j++)
		{
			sPoint = (j == 0) ? pcTrace->GetStart() : pcTrace->GetEnd();
			bPort = false;
			for (k = 0; k < apcPorts.NumElements(); k++)
			{
				if (apcPorts.GetPtr(k)->GetGridPosition() == sPoint)
				{
					bPort = true;
					break;
				}
			}
			if (!bPort)
			{
				pasKnots->Add(&sPoint);
			}
		}
		for (j = i + 1; j < pcCircuit->NumTraces(); j++)
		{
			pcOther = pcCircuit->GetTrace(j);
			if ((pcTrace->GetStart().y == pcTrace->GetEnd().y) && (pcOther->GetStart().x == pcOther->GetEnd().x))
			{
				sPoint.Init(pcOther->GetStart().x, pcTrace->GetStart().y);
			}
			else if ((pcTrace->GetStart().x == pcTrace->GetEnd().x) && (pcOther->GetStart().y == pcOther->GetEnd().y))
			{
				sPoint.Init(pcTrace->GetStart().x, pcOther->GetStart().y);
			}
			else
			{
				continue;
			}
			if (pcTrace->Contains(sPoint) && pcOther->Contains(sPoint) && !pcTrace->IsEnd(sPoint) && !pcOther->IsEnd(sPoint))
			{
				pasKnots->Add(&sPoint);
			}
		}
	}
	apcPorts.Kill();
}


//Knots counted into cells and summed, so any window's count is four sums.  Cell (x, y) is grid point
//(sMinimum.x - 1 + x, sMinimum.y - 1 + y); row and column 0 are a border of zeros.  The cells reach a window past the
//knots, so there's a window ending on every knot.
struct SKicadSchemKnotSums
{
	CArrayInt	aiSums;
	SInt32Vec2	sMinimum;
	int32		iWidth;
	int32		iHeight;

	void	Init(CArrayTemplate<SInt32Vec2>* pasKnots, int32 iWindow);
	void	Kill(void);
	size	Index(int32 x, int32 y);
	int32	Get(int32 x, int32 y);
	int32	CountWindow(int32 x, int32 y, int32 iWindow);
};


//////////////////////////////////////////////////////////////////////////
//
// There must be at least one knot.
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemKnotSums::Init(CArrayTemplate<SInt32Vec2>* pasKnots, int32 iWindow)
{
	SInt32Bounds	sBounds;
	SInt32Vec2*		psKnot;
	size			i;
	int32			x;
	int32			y;

	sBounds.Init();
	for (i = 0; i < pasKnots->NumElements(); i++)
	{
		psKnot = pasKnots->Get(i);
		sBounds.Include(psKnot->x, psKnot->y);
	}
	sMinimum = sBounds.sMinimum;
	iWidth = sBounds.sMaximum.x - sMinimum.x + 1 + iWindow;
	iHeight = sBounds.sMaximum.y - sMinimum.y + 1 + iWindow;

	aiSums.Init();
	aiSums.GrowToAtLeastNumElements((size)(iWidth + 1) * (size)(iHeight + 1), true, 0);
	for (i = 0; i < pasKnots->NumElements(); i++)
	{
		psKnot = pasKnots->Get(i);
		x = psKnot->x - sMinimum.x + 1;
		y = psKnot->y - sMinimum.y + 1;
		aiSums.SetValue(Index(x, y), Get(x, y) + 1);
	}
	for (y = 1; y <= iHeight; y++)
	{
		for (x = 1; x <= iWidth; x++)
		{
			aiSums.SetValue(Index(x, y), Get(x, y) + Get(x - 1, y) + Get(x, y - 1) - Get(x - 1, y - 1));
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemKnotSums::Kill(void)
{
	aiSums.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size SKicadSchemKnotSums::Index(int32 x, int32 y)
{
	return (size)(x + y * (iWidth + 1));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int32 SKicadSchemKnotSums::Get(int32 x, int32 y)
{
	return aiSums.GetValue(Index(x, y));
}


//////////////////////////////////////////////////////////////////////////
//
// The knots in the window of iWindow cells ending at cell (x, y).
//
//////////////////////////////////////////////////////////////////////////
int32 SKicadSchemKnotSums::CountWindow(int32 x, int32 y, int32 iWindow)
{
	return Get(x, y) - Get(x - iWindow, y) - Get(x, y - iWindow) + Get(x - iWindow, y - iWindow);
}


//////////////////////////////////////////////////////////////////////////
//
// The count of the densest window ending on a cell not used yet, and the
// cell (-1 if every cell is used).
//
//////////////////////////////////////////////////////////////////////////
static int32 FindKicadSchemDensestWindow(SKicadSchemKnotSums* psSums, CArrayInt* paiUsed, int32 iWindow, int32* piBestX, int32* piBestY)
{
	int32	x;
	int32	y;
	int32	iCount;
	int32	iBest;

	iBest = -1;
	*piBestX = 0;
	*piBestY = 0;
	for (y = iWindow; y <= psSums->iHeight; y++)
	{
		for (x = iWindow; x <= psSums->iWidth; x++)
		{
			if (paiUsed->GetValue(psSums->Index(x, y)) != 0)
			{
				continue;
			}
			iCount = psSums->CountWindow(x, y, iWindow);
			if (iCount > iBest)
			{
				iBest = iCount;
				*piBestX = x;
				*piBestY = y;
			}
		}
	}
	return iBest;
}


//////////////////////////////////////////////////////////////////////////
//
// Uses every cell a window ending there would overlap the window ending
// at (iBestX, iBestY) on.
//
//////////////////////////////////////////////////////////////////////////
static void UseKicadSchemWindow(SKicadSchemKnotSums* psSums, CArrayInt* paiUsed, int32 iWindow, int32 iBestX, int32 iBestY)
{
	int32	x;
	int32	y;

	for (y = iBestY - iWindow + 1; y < iBestY + iWindow; y++)
	{
		for (x = iBestX - iWindow + 1; x < iBestX + iWindow; x++)
		{
			if ((x >= 0) && (x <= psSums->iWidth) && (y >= 0) && (y <= psSums->iHeight))
			{
				paiUsed->SetValue(psSums->Index(x, y), 1);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The dense windows (with more knots than KICAD_SCHEM_LOGICIM_DENSE_KNOTS),
// densest first and none overlapping another: the centres of the first
// KICAD_SCHEM_LOGICIM_DENSE_AREAS, and the knots over the limit in all of
// them (up to KICAD_SCHEM_LOGICIM_DENSE_SCAN).  A knot is
// where a wire turns or ends (but not on a port) or where two wires cross.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimConverter::FindDenseAreas(CArrayTemplate<SFloat64Vec2>* pasCentres)
{
	CArrayTemplate<SInt32Vec2>	asKnots;
	SKicadSchemKnotSums			sSums;
	CArrayInt					aiUsed;
	size						i;
	int32						iWindow;
	int32						iBest;
	int32						iBestX;
	int32						iBestY;
	size						uiDensest;
	SFloat64Vec2				sCentre;

	asKnots.Init();
	CollectKicadSchemKnots(mpcCircuit, &asKnots);
	if (asKnots.NumElements() == 0)
	{
		asKnots.Kill();
		return 0;
	}

	iWindow = KICAD_SCHEM_LOGICIM_DENSE_WINDOW;
	sSums.Init(&asKnots, iWindow);
	aiUsed.Init();
	aiUsed.GrowToAtLeastNumElements((size)(sSums.iWidth + 1) * (size)(sSums.iHeight + 1), true, 0);
	uiDensest = 0;
	for (i = 0; i < KICAD_SCHEM_LOGICIM_DENSE_SCAN; i++)
	{
		iBest = FindKicadSchemDensestWindow(&sSums, &aiUsed, iWindow, &iBestX, &iBestY);
		if (iBest <= KICAD_SCHEM_LOGICIM_DENSE_KNOTS)
		{
			break;
		}
		uiDensest += (size)(iBest - KICAD_SCHEM_LOGICIM_DENSE_KNOTS);

		//The window covers the cells from iBest - iWindow + 1 to iBest, less one for the sums' border.
		if (i < KICAD_SCHEM_LOGICIM_DENSE_AREAS)
		{
			sCentre.Init(sSums.sMinimum.x - 1 + iBestX - (iWindow - 1) / 2.0, sSums.sMinimum.y - 1 + iBestY - (iWindow - 1) / 2.0);
			pasCentres->Add(&sCentre);
		}
		UseKicadSchemWindow(&sSums, &aiUsed, iWindow, iBestX, iBestY);
	}
	aiUsed.Kill();
	sSums.Kill();
	asKnots.Kill();
	return uiDensest;
}


//////////////////////////////////////////////////////////////////////////
//
// The pairs of items that are all of a subnet, wired by one straight wire
// in KiCad: their pins are level (a horizontal wire) or one above the
// other.  Items attached to a pin already sit on it, so aren't paired.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::FindAlignments(void)
{
	size							i;
	size							uiCount;
	SKicadSchemLogicimConnection*	psConnection;
	SKicadSchemLogicimConnection*	psOther;
	SKicadSchemLogicimTerminal*		psFrom;
	SKicadSchemLogicimTerminal*		psTo;
	SKicadSchemLogicimAlignment*	psAlignment;
	size							j;
	bool							bHorizontal;
	bool							bVertical;

	masAlignments.ReInit();
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		uiCount = 0;
		for (j = 0; j < masConnections.NumElements(); j++)
		{
			psOther = masConnections.Get(j);
			if (psOther->uiSubnet == psConnection->uiSubnet)
			{
				uiCount++;
			}
		}
		if (uiCount != 1)
		{
			continue;
		}

		psFrom = masTerminals.Get(psConnection->uiFrom);
		psTo = masTerminals.Get(psConnection->uiTo);
		if (psFrom->bBus || psTo->bBus || (psFrom->uiItem == psTo->uiItem) || (masItems.Get(psFrom->uiItem)->pcAttachedTo != NULL) || (masItems.Get(psTo->uiItem)->pcAttachedTo != NULL))
		{
			continue;
		}
		//A splitter beside its component is put there, not lined up.
		if (masItems.Get(psFrom->uiItem)->bPlacedSplitter || masItems.Get(psTo->uiItem)->bPlacedSplitter)
		{
			continue;
		}
		bHorizontal = fabs(psFrom->sKicadPosition.fY - psTo->sKicadPosition.fY) < 0.001;
		bVertical = fabs(psFrom->sKicadPosition.fX - psTo->sKicadPosition.fX) < 0.001;
		if (bHorizontal == bVertical)
		{
			continue;
		}

		psAlignment = masAlignments.Add();
		psAlignment->uiFromItem = psFrom->uiItem;
		psAlignment->uiToItem = psTo->uiItem;
		psAlignment->pcFromPort = psFrom->pcPortView;
		psAlignment->pcToPort = psTo->pcPortView;
		psAlignment->bHorizontal = bHorizontal;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Lines up the ports of each pair of items found by FindAlignments by
// moving the item with fewer terminals across the wire (the other if
// that's already been lined up with something), and marks both to keep
// to the line when they're moved clear of the others.  An item is moved
// only once.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AlignItems(void)
{
	size							i;
	size							j;
	SKicadSchemLogicimAlignment*	psAlignment;
	SKicadSchemLogicimItem*			psMover;
	SKicadSchemLogicimItem*			psFrom;
	SKicadSchemLogicimItem*			psTo;
	CPortView*						pcMoverPort;
	CPortView*						pcStillPort;
	CArrayInt						aiTerminals;
	CArrayInt						aiFixed;
	SInt32Vec2						sMover;
	SInt32Vec2						sStill;
	SInt32Vec2						sPosition;
	bool							bFromMoves;

	for (i = 0; i < masItems.NumElements(); i++)
	{
		masItems.Get(i)->eAlign = KSLA_None;
	}
	if (masAlignments.NumElements() == 0)
	{
		return;
	}

	aiTerminals.Init();
	aiFixed.Init();
	for (i = 0; i < masItems.NumElements(); i++)
	{
		aiTerminals.Add(0);
		aiFixed.Add(0);
	}
	for (j = 0; j < masTerminals.NumElements(); j++)
	{
		i = masTerminals.Get(j)->uiItem;
		aiTerminals.SetValue(i, aiTerminals.GetValue(i) + 1);
	}

	for (i = 0; i < masAlignments.NumElements(); i++)
	{
		psAlignment = masAlignments.Get(i);
		if (aiFixed.GetValue(psAlignment->uiFromItem) && aiFixed.GetValue(psAlignment->uiToItem))
		{
			continue;
		}
		if (aiFixed.GetValue(psAlignment->uiFromItem))
		{
			bFromMoves = false;
		}
		else if (aiFixed.GetValue(psAlignment->uiToItem))
		{
			bFromMoves = true;
		}
		else
		{
			bFromMoves = aiTerminals.GetValue(psAlignment->uiFromItem) <= aiTerminals.GetValue(psAlignment->uiToItem);
		}

		psFrom = masItems.Get(psAlignment->uiFromItem);
		psTo = masItems.Get(psAlignment->uiToItem);
		psMover = bFromMoves ? psFrom : psTo;
		pcMoverPort = bFromMoves ? psAlignment->pcFromPort : psAlignment->pcToPort;
		pcStillPort = bFromMoves ? psAlignment->pcToPort : psAlignment->pcFromPort;
		sMover = pcMoverPort->GetGridPosition();
		sStill = pcStillPort->GetGridPosition();
		sPosition = psMover->pcView->GetPosition();
		if (psAlignment->bHorizontal)
		{
			psMover->pcView->SetPosition(sPosition.x, sPosition.y + sStill.y - sMover.y);
		}
		else
		{
			psMover->pcView->SetPosition(sPosition.x + sStill.x - sMover.x, sPosition.y);
		}
		psFrom->eAlign = psAlignment->bHorizontal ? KSLA_KeepY : KSLA_KeepX;
		psTo->eAlign = psFrom->eAlign;
		aiFixed.SetValue(psAlignment->uiFromItem, 1);
		aiFixed.SetValue(psAlignment->uiToItem, 1);
	}
	aiFixed.Kill();
	aiTerminals.Kill();
	PlaceAttachedItems();
}


//////////////////////////////////////////////////////////////////////////
//
// Each item at its KiCad place spread out from the origin, lined up with
// the item a straight wire joins it to (see FindAlignments), and then
// moved apart where they overlap.  Attached items go with their pins.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::Place(float64 fSpread)
{
	size						i;
	SKicadSchemLogicimItem*		psItem;

	for (i = 0; i < masItems.NumElements(); i++)
	{
		psItem = masItems.Get(i);
		if (psItem->pcAttachedTo == NULL)
		{
			psItem->pcView->SetPosition((int32)floor(psItem->sKicadPosition.x * fSpread + 0.5) - psItem->sAnchor.x + psItem->sOffset.x, (int32)floor(psItem->sKicadPosition.y * fSpread + 0.5) - psItem->sAnchor.y + psItem->sOffset.y);
		}
	}
	PlaceSplitters();
	PlaceAttachedItems();
	AlignItems();
	SeparateItems();
}


//////////////////////////////////////////////////////////////////////////
//
// Each splitter beside the component its ends go to: its ends
// KICAD_SCHEM_LOGICIM_SPLITTER_GAP grid units out from the outermost of
// their pins (KICAD_SCHEM_LOGICIM_SPLITTER_TUNNEL_GAP if a member has a
// tunnel too), and along the side as near as they can be to their pins.
// Ends that can't all line up with their pins are put that much further
// out again, so the wires have room to step across.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::PlaceSplitters(void)
{
	size							i;
	size							j;
	SKicadSchemLogicimSplitter*		psSplitter;
	SInt32Vec2						sPin;
	SInt32Vec2						sEnd;
	SInt32Vec2						sPosition;
	bool							bAcrossX;
	int								iOut;
	int32							iPinAcross;
	int32							iEndAcross;
	int32							iAcross;
	float64							fAlong;
	int32							iAlong;
	int32							iStep;
	int32							iOffset;

	for (i = 0; i < masSplitters.NumElements(); i++)
	{
		psSplitter = masSplitters.Get(i);
		if (psSplitter->uiNumPairs == 0)
		{
			continue;
		}
		bAcrossX = (psSplitter->iFacing == TRACE_DIRECTION_EAST) || (psSplitter->iFacing == TRACE_DIRECTION_WEST);
		iOut = ((psSplitter->iFacing == TRACE_DIRECTION_EAST) || (psSplitter->iFacing == TRACE_DIRECTION_SOUTH)) ? 1 : -1;

		iPinAcross = 0;
		iEndAcross = 0;
		fAlong = 0;
		for (j = 0; j < psSplitter->uiNumPairs; j++)
		{
			sPin = mapcSplitterPins.GetPtr(psSplitter->uiFirstPair + j)->GetGridPosition();
			sEnd = mapcSplitterEnds.GetPtr(psSplitter->uiFirstPair + j)->GetGridPosition();
			iAcross = bAcrossX ? sPin.x : sPin.y;
			if ((j == 0) || (iAcross * iOut > iPinAcross * iOut))
			{
				iPinAcross = iAcross;
			}
			iEndAcross = bAcrossX ? sEnd.x : sEnd.y;
			fAlong += bAcrossX ? (sPin.y - sEnd.y) : (sPin.x - sEnd.x);
		}
		iAlong = (int32)floor(fAlong / (float64)psSplitter->uiNumPairs + 0.5);

		//Ends further apart than their pins (a 74 part's pins are a grid unit apart) need room for the wires to step in.
		iStep = 0;
		for (j = 0; j < psSplitter->uiNumPairs; j++)
		{
			sPin = mapcSplitterPins.GetPtr(psSplitter->uiFirstPair + j)->GetGridPosition();
			sEnd = mapcSplitterEnds.GetPtr(psSplitter->uiFirstPair + j)->GetGridPosition();
			iOffset = bAcrossX ? (sPin.y - sEnd.y - iAlong) : (sPin.x - sEnd.x - iAlong);
			iOffset = (iOffset < 0) ? -iOffset : iOffset;
			if (iOffset > iStep)
			{
				iStep = iOffset;
			}
		}
		iAcross = iPinAcross + iOut * (psSplitter->iGap + iStep) - iEndAcross;

		sPosition = psSplitter->pcSplitter->GetPosition();
		if (bAcrossX)
		{
			psSplitter->pcSplitter->SetPosition(sPosition.x + iAcross, sPosition.y + iAlong);
		}
		else
		{
			psSplitter->pcSplitter->SetPosition(sPosition.x + iAlong, sPosition.y + iAcross);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each attached item with its port on the port it's attached to.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::PlaceAttachedItems(void)
{
	size						i;
	SKicadSchemLogicimItem*		psItem;
	SInt32Vec2					sPort;
	SInt32Vec2					sTarget;
	SInt32Vec2					sPosition;

	for (i = 0; i < masItems.NumElements(); i++)
	{
		psItem = masItems.Get(i);
		if (psItem->pcAttachedTo != NULL)
		{
			sPort = psItem->pcPort->GetGridPosition();
			sTarget = psItem->pcAttachedTo->GetGridPosition();
			sPosition = psItem->pcView->GetPosition();
			psItem->pcView->SetPosition(sPosition.x + sTarget.x - sPort.x, sPosition.y + sTarget.y - sPort.y);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A tunnel or supply that KiCad has on a pin (of a gate, flip flop or
// placeholder) is attached to its port, turned so its port faces the
// pin's.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AttachItems(void)
{
	size							i;
	size							j;
	size							k;
	SKicadSchemLogicimItem*			psItem;
	SKicadSchemLogicimTerminal*		psTerminal;
	SKicadSchemLogicimTerminal*		psOther;
	CPortView*						pcTarget;
	int								iWanted;
	ERotation						eOriginal;

	for (i = 0; i < masItems.NumElements(); i++)
	{
		psItem = masItems.Get(i);
		if (psItem->pcPort == NULL)
		{
			continue;
		}

		pcTarget = NULL;
		for (j = 0; (j < masTerminals.NumElements()) && (pcTarget == NULL); j++)
		{
			psTerminal = masTerminals.Get(j);
			if (psTerminal->uiItem != i)
			{
				continue;
			}
			for (k = 0; k < masTerminals.NumElements(); k++)
			{
				psOther = masTerminals.Get(k);
				if ((psOther->pcSubnet == psTerminal->pcSubnet) && (masItems.Get(psOther->uiItem)->pcPort == NULL) && psOther->sKicadPosition.Equals(&psTerminal->sKicadPosition))
				{
					pcTarget = psOther->pcPortView;
					break;
				}
			}
		}
		//A pin wired to a splitter beside it keeps its way out for that wire; its tunnel stands on its own, wired too.
		if ((pcTarget == NULL) || (mapcSplitterPins.Find(pcTarget) != ARRAY_ELEMENT_NOT_FOUND))
		{
			continue;
		}

		psItem->pcAttachedTo = pcTarget;
		iWanted = CTraceAutoConnector::OppositeDirection(CTraceAutoConnector::GetPortFacing(pcTarget));
		eOriginal = psItem->pcView->GetRotation();
		for (k = 0; k < 4; k++)
		{
			psItem->pcView->SetRotation(gaeKicadSchemLogicimRotations[k]);
			if (CTraceAutoConnector::GetPortFacing(psItem->pcPort) == iWanted)
			{
				break;
			}
		}
		if (k == 4)
		{
			psItem->pcView->SetRotation(eOriginal);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each item's import box: its bounding box and the text it draws outside
// it.  Placed items only move, so it's measured once, from the item's
// position, once the attached items are turned.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::MeasureImportBoxes(void)
{
	size						i;
	SKicadSchemLogicimItem*		psItem;
	SInt32Vec2					sPosition;

	for (i = 0; i < masItems.NumElements(); i++)
	{
		psItem = masItems.Get(i);
		psItem->bImportBox = psItem->pcView->GetImportBoundingBox(mpcFonts, &psItem->sImportMinimum, &psItem->sImportMaximum);
		if (psItem->bImportBox)
		{
			sPosition = psItem->pcView->GetPosition();
			psItem->sImportMinimum.Init(psItem->sImportMinimum.x - sPosition.x, psItem->sImportMinimum.y - sPosition.y);
			psItem->sImportMaximum.Init(psItem->sImportMaximum.x - sPosition.x, psItem->sImportMaximum.y - sPosition.y);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The item's import box where it is now.  Its bounding box if it hasn't
// been measured.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::GetItemImportBox(SKicadSchemLogicimItem* psItem, SFloat64Vec2* psMinimum, SFloat64Vec2* psMaximum)
{
	SInt32Vec2	sPosition;

	if (!psItem->bImportBox)
	{
		return psItem->pcView->GetBoundingBox(psMinimum, psMaximum);
	}
	sPosition = psItem->pcView->GetPosition();
	psMinimum->Init(psItem->sImportMinimum.x + sPosition.x, psItem->sImportMinimum.y + sPosition.y);
	psMaximum->Init(psItem->sImportMaximum.x + sPosition.x, psItem->sImportMaximum.y + sPosition.y);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The item's import box and the import boxes of the items attached to it.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::GetItemBox(size uiItem, SFloat64Vec2* psMinimum, SFloat64Vec2* psMaximum)
{
	size						i;
	SKicadSchemLogicimItem*		psItem;
	SKicadSchemLogicimItem*		psOther;
	SFloat64Vec2				sMinimum;
	SFloat64Vec2				sMaximum;
	SFloat64Bounds				sBounds;

	psItem = masItems.Get(uiItem);
	if (!GetItemImportBox(psItem, psMinimum, psMaximum))
	{
		return false;
	}
	sBounds.Init();
	sBounds.Include(*psMinimum, *psMaximum);
	for (i = 0; i < masItems.NumElements(); i++)
	{
		psOther = masItems.Get(i);
		if ((psOther->pcAttachedTo != NULL) && (psOther->pcAttachedTo->GetOwner() == psItem->pcView) && GetItemImportBox(psOther, &sMinimum, &sMaximum))
		{
			sBounds.Include(sMinimum, sMaximum);
		}
	}
	*psMinimum = sBounds.sMinimum;
	*psMaximum = sBounds.sMaximum;
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the box, spaced, overlaps any of the boxes placed so far.
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemLogicimBoxFree(SFloat64Vec2* psMinimum, SFloat64Vec2* psMaximum, int32 iMoveX, int32 iMoveY, CArrayTemplate<SFloat64Vec2>* pasMinimums, CArrayTemplate<SFloat64Vec2>* pasMaximums, size uiPlaced)
{
	size			i;
	SFloat64Vec2*	psOtherMinimum;
	SFloat64Vec2*	psOtherMaximum;

	for (i = 0; i < uiPlaced; i++)
	{
		psOtherMinimum = pasMinimums->Get(i);
		psOtherMaximum = pasMaximums->Get(i);
		if ((psMinimum->x + iMoveX < psOtherMaximum->x + KICAD_SCHEM_LOGICIM_SPACING) && (psOtherMinimum->x < psMaximum->x + iMoveX + KICAD_SCHEM_LOGICIM_SPACING) &&
			(psMinimum->y + iMoveY < psOtherMaximum->y + KICAD_SCHEM_LOGICIM_SPACING) && (psOtherMinimum->y < psMaximum->y + iMoveY + KICAD_SCHEM_LOGICIM_SPACING))
		{
			return false;
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// How far a move is for an item lined up eAlign: moving off its line
// counts KICAD_SCHEM_LOGICIM_ALIGN_WEIGHT times.
//
//////////////////////////////////////////////////////////////////////////
static int32 GetKicadSchemLogicimMoveCost(EKicadSchemLogicimAlign eAlign, int32 iMoveX, int32 iMoveY)
{
	if (eAlign == KSLA_KeepY)
	{
		return abs(iMoveX) + abs(iMoveY) * KICAD_SCHEM_LOGICIM_ALIGN_WEIGHT;
	}
	else if (eAlign == KSLA_KeepX)
	{
		return abs(iMoveX) * KICAD_SCHEM_LOGICIM_ALIGN_WEIGHT + abs(iMoveY);
	}
	return abs(iMoveX) + abs(iMoveY);
}


//////////////////////////////////////////////////////////////////////////
//
// Places the items one at a time (parts and splitters, then the tunnels
// and supplies standing on their own), each where it is if that's clear of the
// ones already placed, otherwise at the nearest place that is (searching
// out in rings).  An item lined up with another counts moving off its line
// KICAD_SCHEM_LOGICIM_ALIGN_WEIGHT times as far.  An item's box takes in the items attached to it.  Items
// that couldn't be placed clear are counted as overlaps.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::SeparateItems(void)
{
	CArrayTemplate<SFloat64Vec2>	asMinimums;
	CArrayTemplate<SFloat64Vec2>	asMaximums;
	size							i;
	size							uiPlaced;
	SKicadSchemLogicimItem*			psItem;
	SFloat64Vec2					sMinimum;
	SFloat64Vec2					sMaximum;
	int32							iRadius;
	int32							iStep;
	int32							iMoveX;
	int32							iMoveY;
	int32							iBestX;
	int32							iBestY;
	int32							iBestDistance;
	int32							iDistance;
	bool							bFound;
	SInt32Vec2						sPosition;
	CArrayInt						aiOrder;
	size							uiPass;
	size							uiOrder;

	PlaceAttachedItems();
	asMinimums.Init();
	asMaximums.Init();
	uiPlaced = 0;
	muiOverlaps = 0;

	//The parts and splitters first, then the tunnels and supplies standing on their own, which are small and move aside.
	aiOrder.Init();
	for (uiPass = 0; uiPass < 2; uiPass++)
	{
		for (i = 0; i < masItems.NumElements(); i++)
		{
			if ((masItems.Get(i)->pcPort == NULL) == (uiPass == 0))
			{
				aiOrder.Add((int)i);
			}
		}
	}

	for (uiOrder = 0; uiOrder < aiOrder.NumElements(); uiOrder++)
	{
		i = (size)aiOrder.GetValue(uiOrder);
		psItem = masItems.Get(i);
		if ((psItem->pcAttachedTo != NULL) || !GetItemBox(i, &sMinimum, &sMaximum))
		{
			continue;
		}

		iBestX = 0;
		iBestY = 0;
		iBestDistance = -1;
		bFound = IsKicadSchemLogicimBoxFree(&sMinimum, &sMaximum, 0, 0, &asMinimums, &asMaximums, uiPlaced);
		//Rings out, while a ring could still have a nearer place than the best found (its moves cost at least its radius).
		for (iRadius = 1; !bFound && (iRadius <= KICAD_SCHEM_LOGICIM_SEPARATE_RADIUS) && ((iBestDistance == -1) || (iRadius < iBestDistance)); iRadius++)
		{
			for (iStep = -iRadius; iStep <= iRadius; iStep++)
			{
				for (iMoveY = -iRadius; iMoveY <= iRadius; iMoveY += 2 * iRadius)
				{
					iMoveX = iStep;
					iDistance = GetKicadSchemLogicimMoveCost(psItem->eAlign, iMoveX, iMoveY);
					if (((iBestDistance == -1) || (iDistance < iBestDistance)) && IsKicadSchemLogicimBoxFree(&sMinimum, &sMaximum, iMoveX, iMoveY, &asMinimums, &asMaximums, uiPlaced))
					{
						iBestDistance = iDistance;
						iBestX = iMoveX;
						iBestY = iMoveY;
					}
				}
				for (iMoveX = -iRadius; iMoveX <= iRadius; iMoveX += 2 * iRadius)
				{
					iMoveY = iStep;
					iDistance = GetKicadSchemLogicimMoveCost(psItem->eAlign, iMoveX, iMoveY);
					if (((iBestDistance == -1) || (iDistance < iBestDistance)) && IsKicadSchemLogicimBoxFree(&sMinimum, &sMaximum, iMoveX, iMoveY, &asMinimums, &asMaximums, uiPlaced))
					{
						iBestDistance = iDistance;
						iBestX = iMoveX;
						iBestY = iMoveY;
					}
				}
			}
		}
		bFound = bFound || (iBestDistance != -1);
		if (!bFound)
		{
			muiOverlaps++;
			iBestX = 0;
			iBestY = 0;
		}

		sPosition = psItem->pcView->GetPosition();
		psItem->pcView->SetPosition(sPosition.x + iBestX, sPosition.y + iBestY);
		sMinimum.x += iBestX;
		sMaximum.x += iBestX;
		sMinimum.y += iBestY;
		sMaximum.y += iBestY;
		asMinimums.Add(&sMinimum);
		asMaximums.Add(&sMaximum);
		uiPlaced++;
	}
	aiOrder.Kill();
	PlaceAttachedItems();
	asMaximums.Kill();
	asMinimums.Kill();
}
