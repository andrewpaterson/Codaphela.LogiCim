/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela LogicimLib

Codaphela LogicimLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela LogicimLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela LogicimLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include <math.h>
#include "BaseLib/Logger.h"
#include "BaseLib/IntegerHelper.h"
#include "BaseLib/PointerFunctions.h"
#include "TraceAutoConnector.h"


//What is under a grid cell besides wires: nothing, a component's body or a port (or the way out of its component)
//that isn't on any of the nets being connected.  Anything else is the net it's kept for.
#define OWNER_FREE		-1
#define OWNER_FOREIGN	-2
#define OWNER_BODY		-3


static int32 gaiDirectionX[4] = { 1, 0, -1, 0 };
static int32 gaiDirectionY[4] = { 0, 1, 0, -1 };


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static bool IsDirectionHorizontal(int iDirection)
{
	return (iDirection == TRACE_DIRECTION_EAST) || (iDirection == TRACE_DIRECTION_WEST);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int CTraceAutoConnector::OppositeDirection(int iDirection)
{
	return (iDirection + 2) % 4;
}


//////////////////////////////////////////////////////////////////////////
//
// The set the wire is in, for the circuit's wires joined into sets.
//
//////////////////////////////////////////////////////////////////////////
static int FindTraceGroup(CArrayInt* paiParents, int iTrace)
{
	while (paiParents->GetValue(iTrace) != iTrace)
	{
		iTrace = paiParents->GetValue(iTrace);
	}
	return iTrace;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::Init(CCircuitView* pcCircuit)
{
	mpcCircuit = pcCircuit;
	masConnections.Init();
	muiNumNets = 0;
	muiNumRuns = 0;
	masPorts.Init();
	mcPortIndices.Init();
	mcBoxes.Init();
	mcNetKeys.Init();
	mcRunKeys.Init();

	msGridMinimum.Init(0, 0);
	miGridWidth = 0;
	miGridHeight = 0;
	maiOwners.Init();
	maiHorizontal.Init();
	maiVertical.Init();
	maiNodes.Init();
	maiNear.Init();

	maiCosts.Init();
	maiFrom.Init();
	maiStamps.Init();
	miStamp = 0;
	masHeap.Init();
	maiGoalStamps.Init();
	maiGoalQueue.Init();

	mapcAddedTraces.Init();
	maiAddedTraceNets.Init();
	maiNetsWithExisting.Init();
	maiNetRuns.Init();
	maiNetWires.Init();
	muiRips = 0;
	mpiOwners = NULL;
	mpiHorizontal = NULL;
	mpiVertical = NULL;
	mpiNodes = NULL;
	mpiNear = NULL;
	mpiCosts = NULL;
	mpiFrom = NULL;
	mpiStamps = NULL;
	mpiGoalStamps = NULL;
	mpiNetRuns = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::Kill(void)
{
	maiNetWires.Kill();
	maiNetRuns.Kill();
	maiNetsWithExisting.Kill();
	maiAddedTraceNets.Kill();
	mapcAddedTraces.Kill();
	maiGoalQueue.Kill();
	maiGoalStamps.Kill();
	masHeap.Kill();
	maiStamps.Kill();
	maiFrom.Kill();
	maiCosts.Kill();

	maiNear.Kill();
	maiNodes.Kill();
	maiVertical.Kill();
	maiHorizontal.Kill();
	maiOwners.Kill();

	mcRunKeys.Kill();
	mcNetKeys.Kill();
	mcBoxes.Kill();
	mcPortIndices.Kill();
	masPorts.Kill();
	masConnections.Kill();
	mpcCircuit = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The index the map has for the key, ARRAY_ELEMENT_NOT_FOUND if none.
//
//////////////////////////////////////////////////////////////////////////
static size GetTraceAutoConnectIndex(CMapBlock* pcMap, const void* pvKey, size uiKeySize)
{
	size*	puiIndex;

	puiIndex = (size*)pcMap->Get(pvKey, uiKeySize);
	return (puiIndex != NULL) ? *puiIndex : ARRAY_ELEMENT_NOT_FOUND;
}


//////////////////////////////////////////////////////////////////////////
//
// A new net.
//
//////////////////////////////////////////////////////////////////////////
size CTraceAutoConnector::AddNet(void)
{
	muiNumNets++;
	return muiNumNets - 1;
}


//////////////////////////////////////////////////////////////////////////
//
// The net the caller names by pvKey (a subnet, a port), added the first
// time it's asked for.
//
//////////////////////////////////////////////////////////////////////////
size CTraceAutoConnector::FindOrAddNet(void* pvKey)
{
	size	uiNet;

	uiNet = GetTraceAutoConnectIndex(&mcNetKeys, &pvKey, sizeof(pvKey));
	if (uiNet == ARRAY_ELEMENT_NOT_FOUND)
	{
		uiNet = AddNet();
		mcNetKeys.Put(&pvKey, sizeof(pvKey), &uiNet, sizeof(uiNet));
	}
	return uiNet;
}


//////////////////////////////////////////////////////////////////////////
//
// A new run.  A run that gets only one connection is no run: the
// connection is routed alone (see DropLoneRuns).
//
//////////////////////////////////////////////////////////////////////////
size CTraceAutoConnector::AddRun(void)
{
	muiNumRuns++;
	return muiNumRuns - 1;
}


//////////////////////////////////////////////////////////////////////////
//
// The run the caller names by a pair of keys (the components a bus runs
// from and to), added the first time it's asked for.
//
//////////////////////////////////////////////////////////////////////////
size CTraceAutoConnector::FindOrAddRun(void* pvFrom, void* pvTo)
{
	STraceAutoConnectRunKey		sKey;
	size						uiRun;

	sKey.pvFrom = pvFrom;
	sKey.pvTo = pvTo;
	uiRun = GetTraceAutoConnectIndex(&mcRunKeys, &sKey, sizeof(sKey));
	if (uiRun == ARRAY_ELEMENT_NOT_FOUND)
	{
		uiRun = AddRun();
		mcRunKeys.Put(&sKey, sizeof(sKey), &uiRun, sizeof(uiRun));
	}
	return uiRun;
}


//////////////////////////////////////////////////////////////////////////
//
// The new connection's index (see IsConnected), ARRAY_ELEMENT_NOT_FOUND
// if either port is already on another net.  fBend is from 0 (at the
// from component) to 1 (at the to component) or
// TRACE_AUTO_CONNECT_NO_BEND.
//
//////////////////////////////////////////////////////////////////////////
size CTraceAutoConnector::AddConnection(size uiNet, CPortView* pcFrom, CPortView* pcTo, float64 fBend, int iRun)
{
	STraceAutoConnection*	psConnection;

	if (uiNet >= muiNumNets)
	{
		gcLogger.Error2(__METHOD__, " There is no net [", IntToString((int)uiNet), "].", NULL);
		return ARRAY_ELEMENT_NOT_FOUND;
	}
	if (!AddPort(pcFrom, uiNet) || !AddPort(pcTo, uiNet))
	{
		return ARRAY_ELEMENT_NOT_FOUND;
	}

	psConnection = masConnections.Add();
	psConnection->uiNet = uiNet;
	psConnection->pcFrom = pcFrom;
	psConnection->pcTo = pcTo;
	psConnection->fBend = fBend;
	psConnection->iRun = iRun;
	psConnection->bHorizontal = true;
	psConnection->iBend = 0;
	psConnection->bHasBend = false;
	psConnection->bConnected = false;
	psConnection->iLength = 0;
	psConnection->bFailed = false;
	psConnection->uiFailedRips = 0;
	psConnection->iFailedNetWires = 0;
	return masConnections.NumElements() - 1;
}


//////////////////////////////////////////////////////////////////////////
//
// The box, in grid units, that wires keep out of for the component
// instead of its bounding box: an importer's box that holds the text
// drawn outside the component too (see
// CComponentView::GetImportBoundingBox).  Its ports still have their way
// out.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::SetComponentBox(CComponentView* pcComponent, SFloat64Vec2 sMinimum, SFloat64Vec2 sMaximum)
{
	STraceAutoConnectBox	sBox;

	sBox.sMinimum = sMinimum;
	sBox.sMaximum = sMaximum;
	mcBoxes.Put(&pcComponent, sizeof(pcComponent), &sBox, sizeof(sBox));
}


//////////////////////////////////////////////////////////////////////////
//
// The box given for the component, otherwise its bounding box.
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::GetComponentBox(CComponentView* pcComponent, SFloat64Vec2* psMinimum, SFloat64Vec2* psMaximum)
{
	STraceAutoConnectBox*	psBox;

	psBox = (STraceAutoConnectBox*)mcBoxes.Get(&pcComponent, sizeof(pcComponent));
	if (psBox != NULL)
	{
		*psMinimum = psBox->sMinimum;
		*psMaximum = psBox->sMaximum;
		return true;
	}
	return pcComponent->GetBoundingBox(psMinimum, psMaximum);
}


//////////////////////////////////////////////////////////////////////////
//
// The port is connected on the net.  Fails if it's already on another.
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::AddPort(CPortView* pcPortView, size uiNet)
{
	int						iNet;
	STraceAutoConnectPort*	psPort;
	size					uiIndex;

	iNet = GetPortNet(pcPortView);
	if (iNet == -1)
	{
		uiIndex = masPorts.NumElements();
		psPort = masPorts.Add();
		psPort->pcPort = pcPortView;
		psPort->uiNet = uiNet;
		mcPortIndices.Put(&pcPortView, sizeof(pcPortView), &uiIndex, sizeof(uiIndex));
		return true;
	}
	if (iNet != (int)uiNet)
	{
		return gcLogger.Error2(__METHOD__, " Port [", pcPortView->GetPortName(0), "] of [", pcPortView->GetOwner()->GetName(), "] is already on another net.", NULL);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// -1 if the port isn't being connected.
//
//////////////////////////////////////////////////////////////////////////
int CTraceAutoConnector::GetPortNet(CPortView* pcPortView)
{
	size	uiIndex;

	uiIndex = GetTraceAutoConnectIndex(&mcPortIndices, &pcPortView, sizeof(pcPortView));
	if (uiIndex == ARRAY_ELEMENT_NOT_FOUND)
	{
		return -1;
	}
	return (int)masPorts.Get(uiIndex)->uiNet;
}


//////////////////////////////////////////////////////////////////////////
//
// A run that got only one connection is no run: the connection is routed
// alone, after the runs (see SortConnections).
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::DropLoneRuns(void)
{
	CArrayInt				aiCounts;
	size					i;
	STraceAutoConnection*	psConnection;

	aiCounts.Init();
	for (i = 0; i < muiNumRuns; i++)
	{
		aiCounts.Add(0);
	}
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		if (IsInRun(psConnection))
		{
			aiCounts.SetValue(psConnection->iRun, aiCounts.GetValue(psConnection->iRun) + 1);
		}
	}
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		if (IsInRun(psConnection) && (aiCounts.GetValue(psConnection->iRun) == 1))
		{
			psConnection->iRun = -1;
		}
	}
	aiCounts.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Routes every connection and adds their wires to the circuit.  False if
// any couldn't be routed.
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::Connect(void)
{
	CArrayInt	aiOrder;
	CArrayInt	aiFailed;
	size		i;

	if (masConnections.NumElements() == 0)
	{
		return true;
	}

	DropLoneRuns();
	InitNets();
	InitSearch();

	aiOrder.Init();
	aiFailed.Init();
	SortConnections(&aiOrder);
	for (i = 0; i < aiOrder.NumElements(); i++)
	{
		if (!Route(masConnections.Get(aiOrder.GetValue(i)), false, NULL))
		{
			aiFailed.Add(aiOrder.GetValue(i));
		}
	}
	RipAndReroute(&aiOrder, &aiFailed);

	//Ripping a net unconnects all of its connections; when the rips run out some of them may not have been routed again.
	for (i = 0; i < aiOrder.NumElements(); i++)
	{
		if (!masConnections.Get(aiOrder.GetValue(i))->bConnected)
		{
			Route(masConnections.Get(aiOrder.GetValue(i)), false, NULL);
		}
	}
	aiFailed.Kill();
	aiOrder.Kill();

	return NumUnconnected() == 0;
}


//////////////////////////////////////////////////////////////////////////
//
// Each net with no existing wires or new wires yet, and its first
// connection's run.  Nothing has been ripped up and no wires added.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::InitNets(void)
{
	size					i;
	STraceAutoConnection*	psConnection;

	maiNetsWithExisting.ReInit();
	maiNetRuns.ReInit();
	maiNetWires.ReInit();
	for (i = 0; i < muiNumNets; i++)
	{
		maiNetsWithExisting.Add(0);
		maiNetRuns.Add(-1);
		maiNetWires.Add(0);
	}
	muiRips = 0;
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		if (maiNetRuns.GetValue(psConnection->uiNet) == -1)
		{
			maiNetRuns.SetValue(psConnection->uiNet, psConnection->iRun);
		}
	}
	mapcAddedTraces.ReInit();
	maiAddedTraceNets.ReInit();
}


//////////////////////////////////////////////////////////////////////////
//
// The grid with the circuit marked on it, and the search's arrays, one
// element for each cell and direction.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::InitSearch(void)
{
	size	uiStates;

	CreateGrid();
	MarkComponents();
	MarkPorts();
	MarkExistingTraces();
	CalculateBends();

	uiStates = (size)miGridWidth * (size)miGridHeight * 4;
	maiCosts.Kill();
	maiFrom.Kill();
	maiStamps.Kill();
	maiCosts.Init();
	maiFrom.Init();
	maiStamps.Init();
	maiCosts.GrowToAtLeastNumElements(uiStates, true, 0);
	maiFrom.GrowToAtLeastNumElements(uiStates, true, 0);
	maiStamps.GrowToAtLeastNumElements(uiStates, true, 0);
	maiGoalStamps.ReInit();
	maiGoalStamps.GrowToAtLeastNumElements(uiStates / 2, true, 0);
	miStamp = 0;
	CacheArrays();
}


//////////////////////////////////////////////////////////////////////////
//
// Rips up what's in the way of each connection that failed and routes it
// again.  The connections of a net ripped up are added to the failed
// ones, to be routed again in their turn, up to a limit on rips.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::RipAndReroute(CArrayInt* paiOrder, CArrayInt* paiFailed)
{
	CArrayInt				aiConflicts;
	size					i;
	size					j;
	size					uiHead;
	size					uiRips;
	size					uiMaxRips;
	STraceAutoConnection*	psConnection;
	int						iNet;

	aiConflicts.Init();
	uiRips = 0;
	uiMaxRips = 2 * paiFailed->NumElements() + 8;
	for (uiHead = 0; (uiHead < paiFailed->NumElements()) && (uiRips < uiMaxRips); uiHead++)
	{
		psConnection = masConnections.Get(paiFailed->GetValue(uiHead));
		if (psConnection->bConnected)
		{
			continue;
		}
		//A plain search that failed fails again unless a net has been ripped up since (clearing the way) or its net has
		//new wires it could end on.
		if ((!psConnection->bFailed || (psConnection->uiFailedRips != muiRips) || (psConnection->iFailedNetWires != maiNetWires.GetValue(psConnection->uiNet))) && Route(psConnection, false, NULL))
		{
			continue;
		}
		aiConflicts.ReInit();
		if (!Route(psConnection, true, &aiConflicts) || (aiConflicts.NumElements() == 0))
		{
			continue;
		}
		for (i = 0; i < aiConflicts.NumElements(); i++)
		{
			iNet = aiConflicts.GetValue(i);
			RipNet(iNet);
			for (j = 0; j < paiOrder->NumElements(); j++)
			{
				if ((int)masConnections.Get(paiOrder->GetValue(j))->uiNet == iNet)
				{
					paiFailed->Add(paiOrder->GetValue(j));
				}
			}
		}
		Route(psConnection, false, NULL);
		uiRips++;
	}
	aiConflicts.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The grid's and the search's arrays don't change size while connecting,
// so the search reads their elements directly.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::CacheArrays(void)
{
	mpiOwners = maiOwners.GetData();
	mpiHorizontal = maiHorizontal.GetData();
	mpiVertical = maiVertical.GetData();
	mpiNodes = maiNodes.GetData();
	mpiNear = maiNear.GetData();
	mpiCosts = maiCosts.GetData();
	mpiFrom = maiFrom.GetData();
	mpiStamps = maiStamps.GetData();
	mpiGoalStamps = maiGoalStamps.GetData();
	mpiNetRuns = maiNetRuns.GetData();
}


//////////////////////////////////////////////////////////////////////////
//
// Around the components, their ports and the wires, with a margin.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::CreateGrid(void)
{
	SInt32Bounds		sBounds;
	size				i;
	size				j;
	CComponentView*		pcComponent;
	SFloat64Vec2		sBoxMinimum;
	SFloat64Vec2		sBoxMaximum;
	SInt32Vec2			sPosition;
	CTraceView*			pcTrace;
	size				uiCells;

	sBounds.Init();
	for (i = 0; i < mpcCircuit->NumComponents(); i++)
	{
		pcComponent = mpcCircuit->GetComponent(i);
		if (GetComponentBox(pcComponent, &sBoxMinimum, &sBoxMaximum))
		{
			sBounds.Include((int32)floor(sBoxMinimum.x), (int32)floor(sBoxMinimum.y));
			sBounds.Include((int32)ceil(sBoxMaximum.x), (int32)ceil(sBoxMaximum.y));
		}
		for (j = 0; j < pcComponent->NumPortViews(); j++)
		{
			sPosition = pcComponent->GetPortView(j)->GetGridPosition();
			sBounds.Include(sPosition.x, sPosition.y);
		}
	}
	for (i = 0; i < mpcCircuit->NumTraces(); i++)
	{
		pcTrace = mpcCircuit->GetTrace(i);
		sBounds.Include(pcTrace->GetStart().x, pcTrace->GetStart().y);
		sBounds.Include(pcTrace->GetEnd().x, pcTrace->GetEnd().y);
	}

	msGridMinimum.Init(sBounds.sMinimum.x - TRACE_AUTO_CONNECT_MARGIN, sBounds.sMinimum.y - TRACE_AUTO_CONNECT_MARGIN);
	miGridWidth = (sBounds.sMaximum.x - sBounds.sMinimum.x) + 1 + 2 * TRACE_AUTO_CONNECT_MARGIN;
	miGridHeight = (sBounds.sMaximum.y - sBounds.sMinimum.y) + 1 + 2 * TRACE_AUTO_CONNECT_MARGIN;
	uiCells = (size)miGridWidth * (size)miGridHeight;

	maiOwners.Kill();
	maiHorizontal.Kill();
	maiVertical.Kill();
	maiNodes.Kill();
	maiNear.Kill();
	maiOwners.Init();
	maiHorizontal.Init();
	maiVertical.Init();
	maiNodes.Init();
	maiNear.Init();
	maiOwners.GrowToAtLeastNumElements(uiCells, true, 0xff);
	maiHorizontal.GrowToAtLeastNumElements(uiCells, true, 0xff);
	maiVertical.GrowToAtLeastNumElements(uiCells, true, 0xff);
	maiNodes.GrowToAtLeastNumElements(uiCells, true, 0xff);
	maiNear.GrowToAtLeastNumElements(uiCells, true, 0);
}


//////////////////////////////////////////////////////////////////////////
//
// Every cell in a component's box is its body, and the cells around a
// body are near it.  A box that doesn't end on the grid (a tunnel is 1.3
// high) only covers the cells inside it.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::MarkComponents(void)
{
	size				i;
	CComponentView*		pcComponent;
	SFloat64Vec2		sBoxMinimum;
	SFloat64Vec2		sBoxMaximum;
	int32				x;
	int32				y;
	int32				iMinimumX;
	int32				iMinimumY;
	int32				iMaximumX;
	int32				iMaximumY;
	int					iCell;
	int					iDirection;

	for (i = 0; i < mpcCircuit->NumComponents(); i++)
	{
		pcComponent = mpcCircuit->GetComponent(i);
		if (!GetComponentBox(pcComponent, &sBoxMinimum, &sBoxMaximum))
		{
			continue;
		}
		iMinimumX = (int32)ceil(sBoxMinimum.x);
		iMinimumY = (int32)ceil(sBoxMinimum.y);
		iMaximumX = (int32)floor(sBoxMaximum.x);
		iMaximumY = (int32)floor(sBoxMaximum.y);
		for (y = iMinimumY; y <= iMaximumY; y++)
		{
			for (x = iMinimumX; x <= iMaximumX; x++)
			{
				maiOwners.SetValue(GetCell(x, y), OWNER_BODY);
			}
		}
	}

	for (y = 0; y < miGridHeight; y++)
	{
		for (x = 0; x < miGridWidth; x++)
		{
			iCell = x + y * miGridWidth;
			if (maiOwners.GetValue(iCell) == OWNER_BODY)
			{
				continue;
			}
			for (iDirection = 0; iDirection < 4; iDirection++)
			{
				if ((x + gaiDirectionX[iDirection] >= 0) && (x + gaiDirectionX[iDirection] < miGridWidth) && (y + gaiDirectionY[iDirection] >= 0) && (y + gaiDirectionY[iDirection] < miGridHeight))
				{
					if (maiOwners.GetValue(iCell + gaiDirectionX[iDirection] + gaiDirectionY[iDirection] * miGridWidth) == OWNER_BODY)
					{
						maiNear.SetValue(iCell, 1);
					}
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A port, the way out from it to the edge of its component's body and the
// cells straight out in front of it are kept for its net; a port that isn't being
// connected is kept clear of.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::MarkPorts(void)
{
	size				i;
	size				j;
	CComponentView*		pcComponent;
	CPortView*			pcPortView;
	int					iNet;
	int					iOwner;
	SInt32Vec2			sPosition;
	int					iFacing;
	int					iCell;
	int					iApproach;

	for (i = 0; i < mpcCircuit->NumComponents(); i++)
	{
		pcComponent = mpcCircuit->GetComponent(i);
		for (j = 0; j < pcComponent->NumPortViews(); j++)
		{
			pcPortView = pcComponent->GetPortView(j);
			iNet = GetPortNet(pcPortView);
			iOwner = (iNet == -1) ? OWNER_FOREIGN : iNet;
			sPosition = pcPortView->GetGridPosition();
			iCell = GetCell(sPosition);
			maiOwners.SetValue(iCell, iOwner);
			maiNodes.SetValue(iCell, iOwner);

			iFacing = GetPortFacing(pcPortView);
			for (;;)
			{
				sPosition.x += gaiDirectionX[iFacing];
				sPosition.y += gaiDirectionY[iFacing];
				if (!IsInGrid(sPosition.x, sPosition.y))
				{
					break;
				}
				iCell = GetCell(sPosition);
				if (maiOwners.GetValue(iCell) != OWNER_BODY)
				{
					//The cells in front of a port being connected are kept for its net, so another net can't block the way in.
					for (iApproach = 0; (iNet != -1) && (iApproach < TRACE_AUTO_CONNECT_APPROACH); iApproach++)
					{
						if (!IsInGrid(sPosition.x, sPosition.y) || (maiOwners.GetValue(GetCell(sPosition)) != OWNER_FREE))
						{
							break;
						}
						maiOwners.SetValue(GetCell(sPosition), iNet);
						sPosition.x += gaiDirectionX[iFacing];
						sPosition.y += gaiDirectionY[iFacing];
					}
					break;
				}
				maiOwners.SetValue(iCell, iOwner);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit's wires joined into groups as Logicim's wiring joins them.
// A group on a port being connected is that port's net; any other group
// is a net of its own that routes keep clear of.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::MarkExistingTraces(void)
{
	CArrayInt		aiParents;
	CArrayInt		aiGroupNets;
	size			uiTraces;
	size			i;
	size			j;
	int				iRoot;
	int				iOther;
	int				iNet;
	CTraceView*		pcTrace;
	SInt32Vec2		sPosition;

	uiTraces = mpcCircuit->NumTraces();
	if (uiTraces == 0)
	{
		return;
	}

	aiParents.Init();
	aiGroupNets.Init();
	for (i = 0; i < uiTraces; i++)
	{
		aiParents.Add((int)i);
		aiGroupNets.Add(-1);
	}
	for (i = 0; i < uiTraces; i++)
	{
		for (j = i + 1; j < uiTraces; j++)
		{
			if (mpcCircuit->GetTrace(i)->Touches(mpcCircuit->GetTrace(j)))
			{
				iRoot = FindTraceGroup(&aiParents, (int)i);
				iOther = FindTraceGroup(&aiParents, (int)j);
				if (iRoot != iOther)
				{
					aiParents.SetValue(iOther, iRoot);
				}
			}
		}
	}

	for (i = 0; i < masPorts.NumElements(); i++)
	{
		sPosition = masPorts.Get(i)->pcPort->GetGridPosition();
		iNet = (int)masPorts.Get(i)->uiNet;
		for (j = 0; j < uiTraces; j++)
		{
			if (mpcCircuit->GetTrace(j)->Contains(sPosition))
			{
				iRoot = FindTraceGroup(&aiParents, (int)j);
				if (aiGroupNets.GetValue(iRoot) == -1)
				{
					aiGroupNets.SetValue(iRoot, iNet);
					maiNetsWithExisting.SetValue(iNet, 1);
				}
				else if (aiGroupNets.GetValue(iRoot) != iNet)
				{
					//Already joins two nets; it's no use to either.
					aiGroupNets.SetValue(iRoot, (int)(muiNumNets + iRoot));
				}
			}
		}
	}

	for (i = 0; i < uiTraces; i++)
	{
		iRoot = FindTraceGroup(&aiParents, (int)i);
		iNet = aiGroupNets.GetValue(iRoot);
		if (iNet == -1)
		{
			iNet = (int)(muiNumNets + iRoot);
		}
		pcTrace = mpcCircuit->GetTrace(i);
		MarkTrace(pcTrace->GetStart().x, pcTrace->GetStart().y, pcTrace->GetEnd().x, pcTrace->GetEnd().y, iNet);
	}
	aiGroupNets.Kill();
	aiParents.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// A wire that isn't horizontal or vertical only has its ends.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::MarkTrace(int32 x0, int32 y0, int32 x1, int32 y1, int iNet)
{
	int32	i;
	int32	iMinimum;
	int32	iMaximum;

	if (y0 == y1)
	{
		iMinimum = (x0 < x1) ? x0 : x1;
		iMaximum = (x0 > x1) ? x0 : x1;
		for (i = iMinimum; i <= iMaximum; i++)
		{
			maiHorizontal.SetValue(GetCell(i, y0), iNet);
		}
	}
	else if (x0 == x1)
	{
		iMinimum = (y0 < y1) ? y0 : y1;
		iMaximum = (y0 > y1) ? y0 : y1;
		for (i = iMinimum; i <= iMaximum; i++)
		{
			maiVertical.SetValue(GetCell(x0, i), iNet);
		}
	}
	maiNodes.SetValue(GetCell(x0, y0), iNet);
	maiNodes.SetValue(GetCell(x1, y1), iNet);
}


//////////////////////////////////////////////////////////////////////////
//
// Where each connection should turn, from its components' boxes.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::CalculateBends(void)
{
	size					i;
	STraceAutoConnection*	psConnection;
	SFloat64Vec2			sFromMinimum;
	SFloat64Vec2			sFromMaximum;
	SFloat64Vec2			sToMinimum;
	SFloat64Vec2			sToMaximum;
	SInt32Vec2				sFrom;
	SInt32Vec2				sTo;
	float64					fFromEdge;
	float64					fToEdge;
	int						iFacing;

	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		sFrom = psConnection->pcFrom->GetGridPosition();
		sTo = psConnection->pcTo->GetGridPosition();
		psConnection->iLength = IntAbs(sTo.x - sFrom.x) + IntAbs(sTo.y - sFrom.y);
		iFacing = GetPortFacing(psConnection->pcFrom);
		psConnection->bHorizontal = IsDirectionHorizontal(iFacing);
		psConnection->bHasBend = false;
		if (psConnection->fBend < 0)
		{
			continue;
		}
		if (!psConnection->pcFrom->GetOwner()->GetBoundingBox(&sFromMinimum, &sFromMaximum) || !psConnection->pcTo->GetOwner()->GetBoundingBox(&sToMinimum, &sToMaximum))
		{
			continue;
		}

		if (psConnection->bHorizontal)
		{
			if (sToMinimum.x + sToMaximum.x >= sFromMinimum.x + sFromMaximum.x)
			{
				fFromEdge = sFromMaximum.x;
				fToEdge = sToMinimum.x;
			}
			else
			{
				fFromEdge = sFromMinimum.x;
				fToEdge = sToMaximum.x;
			}
		}
		else
		{
			if (sToMinimum.y + sToMaximum.y >= sFromMinimum.y + sFromMaximum.y)
			{
				fFromEdge = sFromMaximum.y;
				fToEdge = sToMinimum.y;
			}
			else
			{
				fFromEdge = sFromMinimum.y;
				fToEdge = sToMaximum.y;
			}
		}
		psConnection->iBend = (int32)floor(fFromEdge + psConnection->fBend * (fToEdge - fFromEdge) + 0.5);
		psConnection->bHasBend = true;
	}

	for (i = 0; i < muiNumRuns; i++)
	{
		CalculateRunBends((int)i);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Steps the bends of a run's connections apart so they turn in a stair:
// going right and down the one from the top port turns last, so none
// crosses another.  They share the middle of their bends.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::CalculateRunBends(int iRun)
{
	CArrayInt				aiMembers;
	size					i;
	size					j;
	int						iMember;
	STraceAutoConnection*	psConnection;
	STraceAutoConnection*	psOther;
	int32					iAcross;
	int32					iAlong;
	int32					iBendSum;
	int						iSign;
	int						iCentre;
	SInt32Vec2				sFrom;
	SInt32Vec2				sTo;
	SInt32Vec2				sOtherFrom;
	bool					bHorizontal;

	aiMembers.Init();
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		if ((psConnection->iRun == iRun) && psConnection->bHasBend)
		{
			//Sorted across the way they leave.
			sFrom = psConnection->pcFrom->GetGridPosition();
			for (j = 0; j < aiMembers.NumElements(); j++)
			{
				psOther = masConnections.Get(aiMembers.GetValue(j));
				sOtherFrom = psOther->pcFrom->GetGridPosition();
				if ((psConnection->bHorizontal && (sFrom.y < sOtherFrom.y)) || (!psConnection->bHorizontal && (sFrom.x < sOtherFrom.x)))
				{
					break;
				}
			}
			aiMembers.InsertAt((int)i, j);
		}
	}
	if (aiMembers.NumElements() < 2)
	{
		aiMembers.Kill();
		return;
	}

	iAcross = 0;
	iAlong = 0;
	iBendSum = 0;
	bHorizontal = masConnections.Get(aiMembers.GetValue(0))->bHorizontal;
	for (i = 0; i < aiMembers.NumElements(); i++)
	{
		psConnection = masConnections.Get(aiMembers.GetValue(i));
		sFrom = psConnection->pcFrom->GetGridPosition();
		sTo = psConnection->pcTo->GetGridPosition();
		if (bHorizontal)
		{
			iAcross += sTo.y - sFrom.y;
			iAlong += sTo.x - sFrom.x;
		}
		else
		{
			iAcross += sTo.x - sFrom.x;
			iAlong += sTo.y - sFrom.y;
		}
		iBendSum += psConnection->iBend;
	}

	iSign = (iAcross > 0) ? -1 : 1;
	if (iAlong < 0)
	{
		iSign = -iSign;
	}
	iCentre = (int)(aiMembers.NumElements() - 1) / 2;
	for (i = 0; i < aiMembers.NumElements(); i++)
	{
		iMember = aiMembers.GetValue(i);
		psConnection = masConnections.Get(iMember);
		psConnection->iBend = (int32)floor((float64)iBendSum / (float64)aiMembers.NumElements() + 0.5) + iSign * ((int)i - iCentre);
	}
	aiMembers.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the connection is in one of the runs.
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::IsInRun(STraceAutoConnection* psConnection)
{
	return (psConnection->iRun >= 0) && (psConnection->iRun < (int)muiNumRuns);
}


//////////////////////////////////////////////////////////////////////////
//
// The runs that have connections, shortest first: a run is as long as its
// shortest connection.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::GetRunsByLength(CArrayInt* paiRuns)
{
	CArrayInt				aiRunLengths;
	STraceAutoConnection*	psConnection;
	size					i;
	size					j;
	int						iLength;

	aiRunLengths.Init();
	for (i = 0; i < muiNumRuns; i++)
	{
		aiRunLengths.Add(-1);
	}
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		if (IsInRun(psConnection))
		{
			iLength = aiRunLengths.GetValue(psConnection->iRun);
			if ((iLength == -1) || (psConnection->iLength < iLength))
			{
				aiRunLengths.SetValue(psConnection->iRun, psConnection->iLength);
			}
		}
	}

	for (i = 0; i < muiNumRuns; i++)
	{
		if (aiRunLengths.GetValue(i) == -1)
		{
			continue;
		}
		for (j = 0; (j < paiRuns->NumElements()) && (aiRunLengths.GetValue(i) >= aiRunLengths.GetValue(paiRuns->GetValue(j))); j++)
		{
		}
		paiRuns->InsertAt((int)i, j);
	}
	aiRunLengths.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The run's connections added to paiOrder in its stair's order: across
// the way they leave, as their bends are stepped.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::AddRunInStairOrder(int iRun, CArrayInt* paiOrder)
{
	STraceAutoConnection*	psConnection;
	SInt32Vec2				sFrom;
	SInt32Vec2				sOtherFrom;
	size					uiFirst;
	size					i;
	size					j;

	uiFirst = paiOrder->NumElements();
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		if (psConnection->iRun != iRun)
		{
			continue;
		}
		sFrom = psConnection->pcFrom->GetGridPosition();
		for (j = uiFirst; j < paiOrder->NumElements(); j++)
		{
			sOtherFrom = masConnections.Get(paiOrder->GetValue(j))->pcFrom->GetGridPosition();
			if ((psConnection->bHorizontal && (sFrom.y < sOtherFrom.y)) || (!psConnection->bHorizontal && (sFrom.x < sOtherFrom.x)))
			{
				break;
			}
		}
		paiOrder->InsertAt((int)i, j);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The connections not in a run added to paiOrder, shortest first.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::AddSinglesByLength(CArrayInt* paiOrder)
{
	STraceAutoConnection*	psConnection;
	size					uiFirst;
	size					i;
	size					j;

	uiFirst = paiOrder->NumElements();
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		if (IsInRun(psConnection))
		{
			continue;
		}
		for (j = uiFirst; (j < paiOrder->NumElements()) && (psConnection->iLength >= masConnections.Get(paiOrder->GetValue(j))->iLength); j++)
		{
		}
		paiOrder->InsertAt((int)i, j);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The runs, shortest first and each in its stair's order, and then the
// connections not in a run, shortest first.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::SortConnections(CArrayInt* paiOrder)
{
	CArrayInt	aiRuns;
	size		i;

	aiRuns.Init();
	GetRunsByLength(&aiRuns);
	for (i = 0; i < aiRuns.NumElements(); i++)
	{
		AddRunInStairOrder(aiRuns.GetValue(i), paiOrder);
	}
	aiRuns.Kill();
	AddSinglesByLength(paiOrder);
}


//////////////////////////////////////////////////////////////////////////
//
// A search from the from port, leaving the way it faces, to its to port
// (arriving the way it faces in) or to a wire already on its net.  While
// ripping it may go through other nets' new wires, at a cost, and the
// route isn't added: the nets in its way are collected.
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::Route(STraceAutoConnection* psConnection, bool bRipping, CArrayInt* paiConflicts)
{
	STraceAutoConnectSearch		sSearch;
	STraceAutoConnectHeapNode	sNode;
	int							iNet;
	int							iCell;
	int							iDirection;
	size						uiPops;
	size						uiMaxPops;

	iNet = (int)psConnection->uiNet;
	sSearch.psConnection = psConnection;
	sSearch.bRipping = bRipping;
	sSearch.iStart = GetCell(psConnection->pcFrom->GetGridPosition());
	sSearch.sTarget = psConnection->pcTo->GetGridPosition();
	sSearch.iTarget = GetCell(sSearch.sTarget);
	miStamp++;
	MarkGoal(sSearch.iTarget, iNet);
	if ((sSearch.iStart == sSearch.iTarget) || IsGoal(sSearch.iStart))
	{
		psConnection->bConnected = true;
		return true;
	}

	sSearch.iFacing = GetPortFacing(psConnection->pcFrom);
	sSearch.iArriving = OppositeDirection(GetPortFacing(psConnection->pcTo));

	masHeap.ReInit();
	SetVisited(sSearch.iStart * 4 + sSearch.iFacing, 0, -1);
	PushHeap(GetHeuristic(sSearch.iStart, sSearch.sTarget), sSearch.iStart * 4 + sSearch.iFacing);

	uiPops = 0;
	uiMaxPops = (size)miGridWidth * (size)miGridHeight * 4;
	while (PopHeap(&sNode))
	{
		uiPops++;
		if (uiPops > uiMaxPops)
		{
			break;
		}

		iCell = sNode.iState / 4;
		iDirection = sNode.iState % 4;
		if (sNode.iCost - GetHeuristic(iCell, sSearch.sTarget) > mpiCosts[sNode.iState])
		{
			//A cheaper way here was found after this was pushed.
			continue;
		}

		if ((iCell != sSearch.iStart) && (((iCell == sSearch.iTarget) && (iDirection == sSearch.iArriving)) || ((iCell != sSearch.iTarget) && IsGoal(iCell) && CanNode(iCell, iNet))))
		{
			if (bRipping)
			{
				CollectConflicts(sNode.iState, iNet, paiConflicts);
				return true;
			}
			AddRoute(sNode.iState, iNet);
			psConnection->bConnected = true;
			return true;
		}
		ExpandRoute(&sSearch, &sNode);
	}
	if (!bRipping)
	{
		psConnection->bFailed = true;
		psConnection->uiFailedRips = muiRips;
		psConnection->iFailedNetWires = maiNetWires.GetValue(iNet);
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Pushes each step on from the node's cell that the search can take: not
// back the way it came, out of the start only the way the port faces,
// turning only where the net can have a node and through only cells its
// wires can pass (or, ripping, other nets' new wires, at a cost).
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::ExpandRoute(STraceAutoConnectSearch* psSearch, STraceAutoConnectHeapNode* psNode)
{
	int		iNet;
	int		iCell;
	int		iDirection;
	int		iNewDirection;
	int		iNewCell;
	int		iNewState;
	int32	iCost;
	int32	x;
	int32	y;

	iNet = (int)psSearch->psConnection->uiNet;
	iCell = psNode->iState / 4;
	iDirection = psNode->iState % 4;
	x = iCell % miGridWidth;
	y = iCell / miGridWidth;
	for (iNewDirection = 0; iNewDirection < 4; iNewDirection++)
	{
		if (iNewDirection == OppositeDirection(iDirection))
		{
			continue;
		}
		if ((iCell == psSearch->iStart) && (iNewDirection != psSearch->iFacing))
		{
			continue;
		}
		iCost = 0;
		if ((iNewDirection != iDirection) && !CanNode(iCell, iNet))
		{
			if (!psSearch->bRipping || !CanNodeRipping(iCell, iNet))
			{
				continue;
			}
			iCost += TRACE_AUTO_CONNECT_RIP_COST;
		}
		if ((x + gaiDirectionX[iNewDirection] < 0) || (x + gaiDirectionX[iNewDirection] >= miGridWidth) || (y + gaiDirectionY[iNewDirection] < 0) || (y + gaiDirectionY[iNewDirection] >= miGridHeight))
		{
			continue;
		}
		iNewCell = iCell + gaiDirectionX[iNewDirection] + gaiDirectionY[iNewDirection] * miGridWidth;
		if (!CanPass(iNewCell, iNet, IsDirectionHorizontal(iNewDirection)))
		{
			if (!psSearch->bRipping || !CanPassRipping(iNewCell, iNet, IsDirectionHorizontal(iNewDirection)))
			{
				continue;
			}
			iCost += TRACE_AUTO_CONNECT_RIP_COST;
		}

		iCost += mpiCosts[psNode->iState] + GetStepCost(psSearch->psConnection, iNewCell, iNewDirection);
		if (iNewDirection != iDirection)
		{
			iCost += TRACE_AUTO_CONNECT_BEND_COST;
		}
		iNewState = iNewCell * 4 + iNewDirection;
		if (!IsVisited(iNewState) || (iCost < mpiCosts[iNewState]))
		{
			SetVisited(iNewState, iCost, psNode->iState);
			PushHeap(iCost + GetHeuristic(iNewCell, psSearch->sTarget), iNewState);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The route back from the goal as straight wires, one for each run
// between turns.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::AddRoute(int iGoalState, int iNet)
{
	int		iState;
	int		iPrevious;
	int		iCell;
	int		iEndCell;
	int		iDirection;

	maiNetWires.SetValue(iNet, maiNetWires.GetValue(iNet) + 1);
	iState = iGoalState;
	iEndCell = iState / 4;
	for (;;)
	{
		iPrevious = maiFrom.GetValue(iState);
		iDirection = iState % 4;
		if ((iPrevious == -1) || ((iPrevious % 4) != iDirection))
		{
			//The run that entered this state's cell in its direction starts at the previous state's cell.
			iCell = (iPrevious == -1) ? iState / 4 : iPrevious / 4;
			if (iCell != iEndCell)
			{
				mapcAddedTraces.Add(mpcCircuit->AddTrace(iCell % miGridWidth + msGridMinimum.x, iCell / miGridWidth + msGridMinimum.y, iEndCell % miGridWidth + msGridMinimum.x, iEndCell / miGridWidth + msGridMinimum.y));
				maiAddedTraceNets.Add(iNet);
				MarkTrace(iCell % miGridWidth + msGridMinimum.x, iCell / miGridWidth + msGridMinimum.y, iEndCell % miGridWidth + msGridMinimum.x, iEndCell / miGridWidth + msGridMinimum.y, iNet);
			}
			iEndCell = iCell;
		}
		if (iPrevious == -1)
		{
			break;
		}
		iState = iPrevious;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The nets a route found while ripping goes through: along their wires,
// or turning or ending on them, or over their turns and ends.  Crossing a
// wire is no conflict.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::CollectConflicts(int iGoalState, int iNet, CArrayInt* paiConflicts)
{
	int		iState;
	int		iCell;
	int		iDirection;
	int		iNextDirection;
	bool	bNode;

	iState = iGoalState;
	iNextDirection = -1;
	while (iState != -1)
	{
		iCell = iState / 4;
		iDirection = iState % 4;
		bNode = (iNextDirection == -1) || (iNextDirection != iDirection) || (maiFrom.GetValue(iState) == -1);
		AddConflict(maiNodes.GetValue(iCell), iNet, paiConflicts);
		if (bNode || IsDirectionHorizontal(iDirection))
		{
			AddConflict(maiHorizontal.GetValue(iCell), iNet, paiConflicts);
		}
		if (bNode || !IsDirectionHorizontal(iDirection))
		{
			AddConflict(maiVertical.GetValue(iCell), iNet, paiConflicts);
		}
		iNextDirection = iDirection;
		iState = maiFrom.GetValue(iState);
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::AddConflict(int iOccupant, int iNet, CArrayInt* paiConflicts)
{
	if ((iOccupant != iNet) && IsRippable(iOccupant))
	{
		paiConflicts->AddIfUnique(iOccupant);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Takes out every wire routed for the net and leaves its connections
// unconnected.  Its ports stay kept for it.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::RipNet(int iNet)
{
	size			i;
	CTraceView*		pcTrace;
	int				iCell;
	int				iCells;

	muiRips++;
	for (i = mapcAddedTraces.NumElements(); i > 0; i--)
	{
		if (maiAddedTraceNets.GetValue(i - 1) == iNet)
		{
			pcTrace = mapcAddedTraces.GetPtr(i - 1);
			mpcCircuit->RemoveTrace(pcTrace);
			SafeKill(pcTrace);
			mapcAddedTraces.RemoveAt(i - 1);
			maiAddedTraceNets.RemoveAt(i - 1);
		}
	}

	iCells = miGridWidth * miGridHeight;
	for (iCell = 0; iCell < iCells; iCell++)
	{
		if (maiHorizontal.GetValue(iCell) == iNet)
		{
			maiHorizontal.SetValue(iCell, -1);
		}
		if (maiVertical.GetValue(iCell) == iNet)
		{
			maiVertical.SetValue(iCell, -1);
		}
		if (maiNodes.GetValue(iCell) == iNet)
		{
			maiNodes.SetValue(iCell, -1);
		}
	}
	for (i = 0; i < masPorts.NumElements(); i++)
	{
		if ((int)masPorts.Get(i)->uiNet == iNet)
		{
			maiNodes.SetValue(GetCell(masPorts.Get(i)->pcPort->GetGridPosition()), iNet);
		}
	}
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		if ((int)masConnections.Get(i)->uiNet == iNet)
		{
			masConnections.Get(i)->bConnected = false;
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A net being connected that has only wires routed here (none of its
// wires were already in the circuit).
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::IsRippable(int iOccupant)
{
	return (iOccupant >= 0) && (iOccupant < (int)muiNumNets) && (maiNetsWithExisting.GetValue(iOccupant) == 0);
}


//////////////////////////////////////////////////////////////////////////
//
// As CanPass, but through the wires of nets that can be ripped up.
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::CanPassRipping(int iCell, int iNet, bool bHorizontal)
{
	int		iOwner;
	int		iNode;
	int		iAlong;

	iOwner = maiOwners.GetValue(iCell);
	if ((iOwner == OWNER_BODY) || (iOwner == OWNER_FOREIGN) || ((iOwner >= 0) && (iOwner != iNet)))
	{
		return false;
	}
	iNode = maiNodes.GetValue(iCell);
	if ((iNode != -1) && (iNode != iNet) && !IsRippable(iNode))
	{
		return false;
	}
	iAlong = bHorizontal ? maiHorizontal.GetValue(iCell) : maiVertical.GetValue(iCell);
	return (iAlong == -1) || (iAlong == iNet) || IsRippable(iAlong);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::CanNodeRipping(int iCell, int iNet)
{
	return CanPassRipping(iCell, iNet, true) && CanPassRipping(iCell, iNet, false);
}


//////////////////////////////////////////////////////////////////////////
//
// Which way the port faces out of its component: across the edge of the
// component's box it's nearest, left or right before up or down.
//
//////////////////////////////////////////////////////////////////////////
int CTraceAutoConnector::GetPortFacing(CPortView* pcPortView)
{
	SFloat64Vec2	sMinimum;
	SFloat64Vec2	sMaximum;
	SInt32Vec2		sPosition;
	float64			afDistances[4];
	int				iDirection;
	int				iBest;

	if (!pcPortView->GetOwner()->GetBoundingBox(&sMinimum, &sMaximum))
	{
		return TRACE_DIRECTION_EAST;
	}
	sPosition = pcPortView->GetGridPosition();
	afDistances[TRACE_DIRECTION_EAST] = sMaximum.x - sPosition.x;
	afDistances[TRACE_DIRECTION_SOUTH] = sMaximum.y - sPosition.y;
	afDistances[TRACE_DIRECTION_WEST] = sPosition.x - sMinimum.x;
	afDistances[TRACE_DIRECTION_NORTH] = sPosition.y - sMinimum.y;

	iBest = TRACE_DIRECTION_WEST;
	if (afDistances[TRACE_DIRECTION_EAST] < afDistances[iBest])
	{
		iBest = TRACE_DIRECTION_EAST;
	}
	for (iDirection = TRACE_DIRECTION_SOUTH; iDirection <= TRACE_DIRECTION_NORTH; iDirection += 2)
	{
		if (afDistances[iDirection] < afDistances[iBest])
		{
			iBest = iDirection;
		}
	}
	return iBest;
}


//////////////////////////////////////////////////////////////////////////
//
// A route can go through a cell along a way unless the cell is a
// component's, or kept for another net, or has another net's port, turn
// or wire end on it, or another net's wire along the same way.
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::CanPass(int iCell, int iNet, bool bHorizontal)
{
	int		iOwner;
	int		iNode;
	int		iAlong;

	iOwner = mpiOwners[iCell];
	if ((iOwner == OWNER_BODY) || (iOwner == OWNER_FOREIGN) || ((iOwner >= 0) && (iOwner != iNet)))
	{
		return false;
	}
	iNode = mpiNodes[iCell];
	if ((iNode != -1) && (iNode != iNet))
	{
		return false;
	}
	iAlong = bHorizontal ? mpiHorizontal[iCell] : mpiVertical[iCell];
	return (iAlong == -1) || (iAlong == iNet);
}


//////////////////////////////////////////////////////////////////////////
//
// A route can turn or end in a cell it can pass both ways.
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::CanNode(int iCell, int iNet)
{
	return CanPass(iCell, iNet, true) && CanPass(iCell, iNet, false);
}


//////////////////////////////////////////////////////////////////////////
//
// The cells of the net's wires that are joined to the target port, for
// this search: along a wire, and from one way to the other only where a
// wire ends or turns (wires of a net that cross aren't joined).
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::MarkGoal(int iTarget, int iNet)
{
	size	uiHead;
	int		iState;
	int		iCell;
	int		iAxis;
	int		iDirection;
	int		iNewCell;
	int32	x;
	int32	y;

	maiGoalQueue.ReInit();
	maiGoalStamps.SetValue(iTarget * 2, miStamp);
	maiGoalStamps.SetValue(iTarget * 2 + 1, miStamp);
	maiGoalQueue.Add(iTarget * 2);
	maiGoalQueue.Add(iTarget * 2 + 1);
	for (uiHead = 0; uiHead < maiGoalQueue.NumElements(); uiHead++)
	{
		iState = maiGoalQueue.GetValue(uiHead);
		iCell = iState / 2;
		iAxis = iState % 2;
		x = iCell % miGridWidth;
		y = iCell / miGridWidth;
		for (iDirection = iAxis; iDirection < 4; iDirection += 2)
		{
			if ((x + gaiDirectionX[iDirection] < 0) || (x + gaiDirectionX[iDirection] >= miGridWidth) || (y + gaiDirectionY[iDirection] < 0) || (y + gaiDirectionY[iDirection] >= miGridHeight))
			{
				continue;
			}
			iNewCell = iCell + gaiDirectionX[iDirection] + gaiDirectionY[iDirection] * miGridWidth;
			if ((iAxis == 0) ? ((maiHorizontal.GetValue(iCell) != iNet) || (maiHorizontal.GetValue(iNewCell) != iNet)) : ((maiVertical.GetValue(iCell) != iNet) || (maiVertical.GetValue(iNewCell) != iNet)))
			{
				continue;
			}
			if (maiGoalStamps.GetValue(iNewCell * 2 + iAxis) != miStamp)
			{
				maiGoalStamps.SetValue(iNewCell * 2 + iAxis, miStamp);
				maiGoalQueue.Add(iNewCell * 2 + iAxis);
			}
			if ((maiNodes.GetValue(iNewCell) == iNet) && (maiGoalStamps.GetValue(iNewCell * 2 + 1 - iAxis) != miStamp))
			{
				maiGoalStamps.SetValue(iNewCell * 2 + 1 - iAxis, miStamp);
				maiGoalQueue.Add(iNewCell * 2 + 1 - iAxis);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::IsGoal(int iCell)
{
	return (mpiGoalStamps[iCell * 2] == miStamp) || (mpiGoalStamps[iCell * 2 + 1] == miStamp);
}


//////////////////////////////////////////////////////////////////////////
//
// The cost of going into the cell in the direction, not counting a turn.
//
//////////////////////////////////////////////////////////////////////////
int CTraceAutoConnector::GetStepCost(STraceAutoConnection* psConnection, int iCell, int iDirection)
{
	int		iCost;
	int		iAcross;
	int		iNet;
	int32	iOffBend;
	bool	bHorizontal;
	int32	x;
	int32	y;

	iNet = (int)psConnection->uiNet;
	bHorizontal = IsDirectionHorizontal(iDirection);
	iCost = TRACE_AUTO_CONNECT_STEP_COST;
	if (mpiNear[iCell] != 0)
	{
		iCost += TRACE_AUTO_CONNECT_NEAR_COST;
	}
	iAcross = bHorizontal ? mpiVertical[iCell] : mpiHorizontal[iCell];
	if ((iAcross != -1) && (iAcross != iNet))
	{
		iCost += TRACE_AUTO_CONNECT_CROSS_COST;
	}

	//Another net's wire running alongside, either side.
	x = iCell % miGridWidth;
	y = iCell / miGridWidth;
	if (bHorizontal)
	{
		if ((y > 0) && IsCrowding(mpiHorizontal[iCell - miGridWidth], iNet))
		{
			iCost += TRACE_AUTO_CONNECT_CROWD_COST;
		}
		if ((y < miGridHeight - 1) && IsCrowding(mpiHorizontal[iCell + miGridWidth], iNet))
		{
			iCost += TRACE_AUTO_CONNECT_CROWD_COST;
		}
	}
	else
	{
		if ((x > 0) && IsCrowding(mpiVertical[iCell - 1], iNet))
		{
			iCost += TRACE_AUTO_CONNECT_CROWD_COST;
		}
		if ((x < miGridWidth - 1) && IsCrowding(mpiVertical[iCell + 1], iNet))
		{
			iCost += TRACE_AUTO_CONNECT_CROWD_COST;
		}
	}
	if (psConnection->bHasBend && (bHorizontal != psConnection->bHorizontal))
	{
		if (psConnection->bHorizontal)
		{
			iOffBend = IntAbs(iCell % miGridWidth + msGridMinimum.x - psConnection->iBend);
		}
		else
		{
			iOffBend = IntAbs(iCell / miGridWidth + msGridMinimum.y - psConnection->iBend);
		}
		iOffBend *= TRACE_AUTO_CONNECT_HINT_COST;
		iCost += (iOffBend < TRACE_AUTO_CONNECT_HINT_LIMIT) ? iOffBend : TRACE_AUTO_CONNECT_HINT_LIMIT;
	}
	return iCost;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a wire of the occupant beside a route of the net crowds it: it's
// another net, and not one in the same run.
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::IsCrowding(int iOccupant, int iNet)
{
	if ((iOccupant == -1) || (iOccupant == iNet))
	{
		return false;
	}
	if ((iOccupant < (int)muiNumNets) && (mpiNetRuns[iNet] != -1) && (mpiNetRuns[iOccupant] == mpiNetRuns[iNet]))
	{
		return false;
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The length to the to port.  It's not always the least the cost could be
// (a route can end on its net's wires before it), so a route is good
// rather than the cheapest.
//
//////////////////////////////////////////////////////////////////////////
int CTraceAutoConnector::GetHeuristic(int iCell, SInt32Vec2 sTarget)
{
	int32	x;
	int32	y;

	x = iCell % miGridWidth + msGridMinimum.x;
	y = iCell / miGridWidth + msGridMinimum.y;
	return (IntAbs(sTarget.x - x) + IntAbs(sTarget.y - y)) * TRACE_AUTO_CONNECT_STEP_COST;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::IsInGrid(int32 x, int32 y)
{
	x -= msGridMinimum.x;
	y -= msGridMinimum.y;
	return (x >= 0) && (x < miGridWidth) && (y >= 0) && (y < miGridHeight);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int CTraceAutoConnector::GetCell(int32 x, int32 y)
{
	return (x - msGridMinimum.x) + (y - msGridMinimum.y) * miGridWidth;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int CTraceAutoConnector::GetCell(SInt32Vec2 sPosition)
{
	return GetCell(sPosition.x, sPosition.y);
}


//////////////////////////////////////////////////////////////////////////
//
// A binary heap, cheapest first.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::PushHeap(int32 iCost, int32 iState)
{
	size						uiIndex;
	size						uiParent;
	STraceAutoConnectHeapNode*	psHeap;
	STraceAutoConnectHeapNode	sNode;

	masHeap.Add();
	psHeap = masHeap.GetData();
	sNode.iCost = iCost;
	sNode.iState = iState;
	uiIndex = masHeap.NumElements() - 1;
	while (uiIndex > 0)
	{
		uiParent = (uiIndex - 1) / 2;
		if (psHeap[uiParent].iCost <= iCost)
		{
			break;
		}
		psHeap[uiIndex] = psHeap[uiParent];
		uiIndex = uiParent;
	}
	psHeap[uiIndex] = sNode;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::PopHeap(STraceAutoConnectHeapNode* psNode)
{
	size						uiIndex;
	size						uiChild;
	size						uiNum;
	STraceAutoConnectHeapNode*	psHeap;
	STraceAutoConnectHeapNode	sLast;

	uiNum = masHeap.NumElements();
	if (uiNum == 0)
	{
		return false;
	}
	psHeap = masHeap.GetData();
	*psNode = psHeap[0];
	sLast = psHeap[uiNum - 1];
	masHeap.RemoveTail();
	uiNum--;
	if (uiNum == 0)
	{
		return true;
	}
	psHeap = masHeap.GetData();

	//The last node sinks from the top to where it belongs.
	uiIndex = 0;
	for (;;)
	{
		uiChild = uiIndex * 2 + 1;
		if (uiChild >= uiNum)
		{
			break;
		}
		if ((uiChild + 1 < uiNum) && (psHeap[uiChild + 1].iCost < psHeap[uiChild].iCost))
		{
			uiChild++;
		}
		if (sLast.iCost <= psHeap[uiChild].iCost)
		{
			break;
		}
		psHeap[uiIndex] = psHeap[uiChild];
		uiIndex = uiChild;
	}
	psHeap[uiIndex] = sLast;
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// States are visited in this search if they have its stamp.
//
//////////////////////////////////////////////////////////////////////////
void CTraceAutoConnector::SetVisited(int iState, int32 iCost, int32 iFrom)
{
	mpiStamps[iState] = miStamp;
	mpiCosts[iState] = iCost;
	mpiFrom[iState] = iFrom;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CTraceAutoConnector::IsVisited(int iState)
{
	return mpiStamps[iState] == miStamp;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CTraceAutoConnector::NumConnections(void) { return masConnections.NumElements(); }
bool CTraceAutoConnector::IsConnected(size uiIndex) { return (uiIndex < masConnections.NumElements()) && masConnections.Get(uiIndex)->bConnected; }


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CTraceAutoConnector::NumUnconnected(void)
{
	size	i;
	size	uiUnconnected;

	uiUnconnected = 0;
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		if (!masConnections.Get(i)->bConnected)
		{
			uiUnconnected++;
		}
	}
	return uiUnconnected;
}

