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
	msCounts.uiExcessKnotsBefore = uiExcess;
	bOnlyDensest = false;
	for (uiPass = 0; (uiPass < KICAD_SCHEM_LOGICIM_EXPLODE_PASSES) && (asCentres.NumElements() > 0); uiPass++)
	{
		asOffsets.ReInit();
		for (i = 0; i < mcModel.NumItems(); i++)
		{
			asOffsets.Add(&mcModel.GetItem(i)->sOffset);
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
		if ((uiUnconnected > msCounts.uiUnconnected) || (uiNewExcess >= uiExcess))
		{
			//Worse: back to how it was.
			for (i = 0; i < mcModel.NumItems(); i++)
			{
				mcModel.GetItem(i)->sOffset = *asOffsets.Get(i);
			}
			Place(mfSpread);
			msCounts.uiUnconnected = Route();
			if (bOnlyDensest)
			{
				break;
			}
			bOnlyDensest = true;
			asCentres.ReInit();
			FindDenseAreas(&asCentres);
			continue;
		}
		msCounts.uiUnconnected = uiUnconnected;
		uiExcess = uiNewExcess;
		msCounts.uiExplosions++;
		bOnlyDensest = false;
	}
	msCounts.uiExcessKnotsAfter = uiExcess;
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

	for (i = 0; i < mcModel.NumItems(); i++)
	{
		psItem = mcModel.GetItem(i);
		if (psItem->pcAttachedTo != NULL)
		{
			continue;
		}
		sPosition = psItem->pcView->GetPosition() + psItem->sAnchor;
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
	for (i = 0; i < mcModel.NumConnections(); i++)
	{
		psConnection = mcModel.GetConnection(i);
		uiCount = 0;
		for (j = 0; j < mcModel.NumConnections(); j++)
		{
			psOther = mcModel.GetConnection(j);
			if (psOther->pcSubnet == psConnection->pcSubnet)
			{
				uiCount++;
			}
		}
		if (uiCount != 1)
		{
			continue;
		}

		psFrom = mcModel.GetTerminal(psConnection->uiFrom);
		psTo = mcModel.GetTerminal(psConnection->uiTo);
		if (psFrom->bBus || psTo->bBus || (psFrom->uiItem == psTo->uiItem) || (mcModel.GetItem(psFrom->uiItem)->pcAttachedTo != NULL) || (mcModel.GetItem(psTo->uiItem)->pcAttachedTo != NULL))
		{
			continue;
		}
		//A splitter beside its component is put there, not lined up.
		if (mcModel.GetItem(psFrom->uiItem)->bPlacedSplitter || mcModel.GetItem(psTo->uiItem)->bPlacedSplitter)
		{
			continue;
		}
		bHorizontal = fabs(psFrom->sKicadPosition.y - psTo->sKicadPosition.y) < 0.001;
		bVertical = fabs(psFrom->sKicadPosition.x - psTo->sKicadPosition.x) < 0.001;
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

	for (i = 0; i < mcModel.NumItems(); i++)
	{
		mcModel.GetItem(i)->eAlign = KSLA_None;
	}
	if (masAlignments.NumElements() == 0)
	{
		return;
	}

	aiTerminals.Init();
	aiFixed.Init();
	for (i = 0; i < mcModel.NumItems(); i++)
	{
		aiTerminals.Add(0);
		aiFixed.Add(0);
	}
	for (j = 0; j < mcModel.NumTerminals(); j++)
	{
		i = mcModel.GetTerminal(j)->uiItem;
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

		psFrom = mcModel.GetItem(psAlignment->uiFromItem);
		psTo = mcModel.GetItem(psAlignment->uiToItem);
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

	for (i = 0; i < mcModel.NumItems(); i++)
	{
		psItem = mcModel.GetItem(i);
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
// Whether the port is a component's pin a splitter beside it is wired
// straight to.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsSplitterPin(CPortView* pcPortView)
{
	size	i;

	for (i = 0; i < masSplitterPairs.NumElements(); i++)
	{
		if (masSplitterPairs.Get(i)->pcPin == pcPortView)
		{
			return true;
		}
	}
	return false;
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
	SKicadSchemLogicimSplitterPair*	psPair;
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
			psPair = masSplitterPairs.Get(psSplitter->uiFirstPair + j);
			sPin = psPair->pcPin->GetGridPosition();
			sEnd = psPair->pcEnd->GetGridPosition();
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
			psPair = masSplitterPairs.Get(psSplitter->uiFirstPair + j);
			sPin = psPair->pcPin->GetGridPosition();
			sEnd = psPair->pcEnd->GetGridPosition();
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

	for (i = 0; i < mcModel.NumItems(); i++)
	{
		psItem = mcModel.GetItem(i);
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

	for (i = 0; i < mcModel.NumItems(); i++)
	{
		psItem = mcModel.GetItem(i);
		if (psItem->pcPort == NULL)
		{
			continue;
		}

		pcTarget = NULL;
		for (j = mcModel.FirstItemTerminal(i); (j != ARRAY_ELEMENT_NOT_FOUND) && (pcTarget == NULL); j = mcModel.NextItemTerminal(j))
		{
			psTerminal = mcModel.GetTerminal(j);
			for (k = mcModel.FirstSubnetTerminal(psTerminal->pcSubnet); k != ARRAY_ELEMENT_NOT_FOUND; k = mcModel.NextSubnetTerminal(k))
			{
				psOther = mcModel.GetTerminal(k);
				if ((mcModel.GetItem(psOther->uiItem)->pcPort == NULL) && psOther->sKicadPosition.CloselyEqual(&psTerminal->sKicadPosition, KICAD_SCHEM_POINT_TOLERANCE))
				{
					pcTarget = psOther->pcPortView;
					break;
				}
			}
		}
		//A pin wired to a splitter beside it keeps its way out for that wire; its tunnel stands on its own, wired too.
		if ((pcTarget == NULL) || IsSplitterPin(pcTarget))
		{
			continue;
		}

		mcModel.AttachItem(i, pcTarget);
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

	for (i = 0; i < mcModel.NumItems(); i++)
	{
		psItem = mcModel.GetItem(i);
		psItem->bImportBox = psItem->pcView->GetImportBoundingBox(mpcFonts, &psItem->sImportBox);
		if (psItem->bImportBox)
		{
			sPosition = psItem->pcView->GetPosition();
			psItem->sImportBox.Move(SFloat64Vec2(-sPosition.x, -sPosition.y));
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The item's import box and the import boxes of the items attached to it.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::GetItemBox(size uiItem, SFloat64Bounds* psBox)
{
	size				i;
	SFloat64Bounds		sOther;

	if (!mcModel.GetItemImportBox(uiItem, psBox))
	{
		return false;
	}
	for (i = mcModel.FirstAttached(uiItem); i != ARRAY_ELEMENT_NOT_FOUND; i = mcModel.NextAttached(i))
	{
		if (mcModel.GetItemImportBox(i, &sOther))
		{
			psBox->Include(&sOther);
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the box, moved and spaced, overlaps any of the boxes placed so
// far.
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemLogicimBoxFree(SFloat64Bounds* psBox, int32 iMoveX, int32 iMoveY, CArrayTemplate<SFloat64Bounds>* pasPlaced)
{
	size				i;
	SFloat64Bounds*		psOther;

	for (i = 0; i < pasPlaced->NumElements(); i++)
	{
		psOther = pasPlaced->Get(i);
		if ((psBox->sMinimum.x + iMoveX < psOther->sMaximum.x + KICAD_SCHEM_LOGICIM_SPACING) && (psOther->sMinimum.x < psBox->sMaximum.x + iMoveX + KICAD_SCHEM_LOGICIM_SPACING) &&
			(psBox->sMinimum.y + iMoveY < psOther->sMaximum.y + KICAD_SCHEM_LOGICIM_SPACING) && (psOther->sMinimum.y < psBox->sMaximum.y + iMoveY + KICAD_SCHEM_LOGICIM_SPACING))
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
// The best clear move of a box found so far, and how far it is (-1 until
// one is found).
//
//////////////////////////////////////////////////////////////////////////
struct SKicadSchemLogicimBestMove
{
	SFloat64Bounds*						psBox;
	EKicadSchemLogicimAlign				eAlign;
	CArrayTemplate<SFloat64Bounds>*		pasPlaced;
	int32								iDistance;
	SInt32Vec2							sMove;

	void	Init(SFloat64Bounds* psBox, EKicadSchemLogicimAlign eAlign, CArrayTemplate<SFloat64Bounds>* pasPlaced);
	void	Try(int32 iMoveX, int32 iMoveY);
	bool	IsNearer(int32 iDistance);
};


void SKicadSchemLogicimBestMove::Init(SFloat64Bounds* psBox, EKicadSchemLogicimAlign eAlign, CArrayTemplate<SFloat64Bounds>* pasPlaced)
{
	this->psBox = psBox;
	this->eAlign = eAlign;
	this->pasPlaced = pasPlaced;
	iDistance = -1;
	sMove.x = 0;
	sMove.y = 0;
}


bool SKicadSchemLogicimBestMove::IsNearer(int32 iDistance)
{
	return (this->iDistance == -1) || (iDistance < this->iDistance);
}


void SKicadSchemLogicimBestMove::Try(int32 iMoveX, int32 iMoveY)
{
	int32	iMoveDistance;

	iMoveDistance = GetKicadSchemLogicimMoveCost(eAlign, iMoveX, iMoveY);
	if (IsNearer(iMoveDistance) && IsKicadSchemLogicimBoxFree(psBox, iMoveX, iMoveY, pasPlaced))
	{
		iDistance = iMoveDistance;
		sMove.x = iMoveX;
		sMove.y = iMoveY;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The nearest move (see GetKicadSchemLogicimMoveCost) that puts the box
// clear of those placed, searching out in rings.  False if there's none
// within KICAD_SCHEM_LOGICIM_SEPARATE_RADIUS.
//
//////////////////////////////////////////////////////////////////////////
static bool FindKicadSchemLogicimFreeMove(SFloat64Bounds* psBox, EKicadSchemLogicimAlign eAlign, CArrayTemplate<SFloat64Bounds>* pasPlaced, SInt32Vec2* psMove)
{
	SKicadSchemLogicimBestMove	sBest;
	int32						iRadius;
	int32						iStep;

	psMove->x = 0;
	psMove->y = 0;
	if (IsKicadSchemLogicimBoxFree(psBox, 0, 0, pasPlaced))
	{
		return true;
	}

	//Rings out, while a ring could still have a nearer place than the best found (its moves cost at least its radius).
	sBest.Init(psBox, eAlign, pasPlaced);
	for (iRadius = 1; (iRadius <= KICAD_SCHEM_LOGICIM_SEPARATE_RADIUS) && sBest.IsNearer(iRadius); iRadius++)
	{
		for (iStep = -iRadius; iStep <= iRadius; iStep++)
		{
			sBest.Try(iStep, -iRadius);
			sBest.Try(iStep, iRadius);
			sBest.Try(-iRadius, iStep);
			sBest.Try(iRadius, iStep);
		}
	}
	if (sBest.iDistance == -1)
	{
		return false;
	}
	*psMove = sBest.sMove;
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The order SeparateItems places the items in: the parts and splitters
// first, then the tunnels and supplies standing on their own, which are
// small and move aside.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::GetSeparateOrder(CArrayInt* paiOrder)
{
	size	i;
	size	uiPass;

	for (uiPass = 0; uiPass < 2; uiPass++)
	{
		for (i = 0; i < mcModel.NumItems(); i++)
		{
			if ((mcModel.GetItem(i)->pcPort == NULL) == (uiPass == 0))
			{
				paiOrder->Add((int)i);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Places the items one at a time (see GetSeparateOrder), each where it is
// if that's clear of the ones already placed, otherwise at the nearest
// place that is (see FindKicadSchemLogicimFreeMove).  An item's box takes
// in the items attached to it.  Items that couldn't be placed clear are
// counted as overlaps.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::SeparateItems(void)
{
	CArrayTemplate<SFloat64Bounds>	asPlaced;
	CArrayInt						aiOrder;
	size							i;
	size							uiOrder;
	SKicadSchemLogicimItem*			psItem;
	SFloat64Bounds					sBox;
	SInt32Vec2						sMove;
	SInt32Vec2						sPosition;

	PlaceAttachedItems();
	asPlaced.Init();
	msCounts.uiOverlaps = 0;

	aiOrder.Init();
	GetSeparateOrder(&aiOrder);
	for (uiOrder = 0; uiOrder < aiOrder.NumElements(); uiOrder++)
	{
		i = (size)aiOrder.GetValue(uiOrder);
		psItem = mcModel.GetItem(i);
		if ((psItem->pcAttachedTo != NULL) || !GetItemBox(i, &sBox))
		{
			continue;
		}

		if (!FindKicadSchemLogicimFreeMove(&sBox, psItem->eAlign, &asPlaced, &sMove))
		{
			msCounts.uiOverlaps++;
		}
		sPosition = psItem->pcView->GetPosition();
		psItem->pcView->SetPosition(sPosition.x + sMove.x, sPosition.y + sMove.y);
		sBox.Move(SFloat64Vec2(sMove.x, sMove.y));
		asPlaced.Add(&sBox);
	}
	aiOrder.Kill();
	PlaceAttachedItems();
	asPlaced.Kill();
}
