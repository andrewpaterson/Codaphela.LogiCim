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
	size							j;
	size							uiKept;
	SKicadSchemLogicimTerminal*		psTerminal;
	SKicadSchemLogicimTerminal*		psOther;
	SKicadSchemLogicimItem*			psItem;
	CArrayInt						aiNewIndices;
	bool							bDrawn;

	aiNewIndices.Init();
	for (i = 0; i < masItems.NumElements(); i++)
	{
		aiNewIndices.Add(0);
	}

	//Which items go: -1.
	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		psTerminal = masTerminals.Get(i);
		if (psTerminal->bBus || (masItems.Get(psTerminal->uiItem)->pcPort == NULL))
		{
			continue;
		}
		bDrawn = false;
		for (j = 0; (j < masTerminals.NumElements()) && !bDrawn; j++)
		{
			psOther = masTerminals.Get(j);
			if ((psOther->pcSubnet == psTerminal->pcSubnet) && (psOther->bBus || (masItems.Get(psOther->uiItem)->pcPort == NULL)))
			{
				bDrawn = true;
			}
		}
		if (!bDrawn)
		{
			aiNewIndices.SetValue(psTerminal->uiItem, -1);
		}
	}

	//The items kept, in order, and where each is now.
	uiKept = 0;
	for (i = 0; i < masItems.NumElements(); i++)
	{
		psItem = masItems.Get(i);
		if (aiNewIndices.GetValue(i) == -1)
		{
			if (psItem->eKind == KSLIK_Tunnel)
			{
				muiTunnels--;
			}
			else
			{
				muiPowers--;
			}
			mpcCircuit->Remove(psItem->pcView);
			SafeKill(psItem->pcView);
			continue;
		}
		aiNewIndices.SetValue(i, (int)uiKept);
		if (uiKept != i)
		{
			*masItems.Get(uiKept) = *psItem;
		}
		uiKept++;
	}
	masItems.SetUsedElements(uiKept);

	uiKept = 0;
	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		psTerminal = masTerminals.Get(i);
		if (aiNewIndices.GetValue(psTerminal->uiItem) == -1)
		{
			continue;
		}
		psTerminal->uiItem = (size)aiNewIndices.GetValue(psTerminal->uiItem);
		if (uiKept != i)
		{
			*masTerminals.Get(uiKept) = *psTerminal;
		}
		uiKept++;
	}
	masTerminals.SetUsedElements(uiKept);
	aiNewIndices.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddConnections(void)
{
	size	i;

	for (i = 0; i < mpcNetlist->NumSubnets(); i++)
	{
		AddSubnetConnections(mpcNetlist->GetSubnet(i), i);
	}
	for (i = 0; i < mpcNetlist->NumBuses(); i++)
	{
		AddSubnetConnections(mpcNetlist->GetBus(i)->GetSubnet(), mpcNetlist->NumSubnets() + i);
	}
	JoinSplitterPins();
}


//////////////////////////////////////////////////////////////////////////
//
// The terminal of the port, ARRAY_ELEMENT_NOT_FOUND if it has none.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimConverter::FindPortTerminal(CPortView* pcPortView)
{
	size	i;

	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		if (masTerminals.Get(i)->pcPortView == pcPortView)
		{
			return i;
		}
	}
	return ARRAY_ELEMENT_NOT_FOUND;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a connection joins the two terminals, either way round.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsConnectionBetween(size uiTerminal1, size uiTerminal2)
{
	size							i;
	SKicadSchemLogicimConnection*	psConnection;

	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		if (((psConnection->uiFrom == uiTerminal1) && (psConnection->uiTo == uiTerminal2)) || ((psConnection->uiFrom == uiTerminal2) && (psConnection->uiTo == uiTerminal1)))
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the connections, all but uiWithout, join terminal uiFrom to
// terminal uiTo.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::ReachesTerminal(size uiFrom, size uiTo, size uiWithout)
{
	CArrayInt						aiReached;
	size							uiHead;
	size							uiNext;
	size							i;
	SKicadSchemLogicimConnection*	psConnection;

	aiReached.Init();
	aiReached.Add((int)uiFrom);
	for (uiHead = 0; uiHead < aiReached.NumElements(); uiHead++)
	{
		for (i = 0; i < masConnections.NumElements(); i++)
		{
			psConnection = masConnections.Get(i);
			if (i == uiWithout)
			{
				continue;
			}
			if (psConnection->uiFrom == (size)aiReached.GetValue(uiHead))
			{
				uiNext = psConnection->uiTo;
			}
			else if (psConnection->uiTo == (size)aiReached.GetValue(uiHead))
			{
				uiNext = psConnection->uiFrom;
			}
			else
			{
				continue;
			}
			if (uiNext == uiTo)
			{
				aiReached.Kill();
				return true;
			}
			if (aiReached.Find((int)uiNext) == ARRAY_ELEMENT_NOT_FOUND)
			{
				aiReached.Add((int)uiNext);
			}
		}
	}
	aiReached.Kill();
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The first connection from the splitter's end to another terminal that
// still reaches the pin without it, joined to the pin instead (see
// JoinSplitterPins).
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::JoinSplitterEnd(size uiEnd, size uiPin)
{
	size							i;
	size							uiOther;
	SKicadSchemLogicimConnection*	psConnection;

	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		if ((psConnection->uiFrom != uiEnd) && (psConnection->uiTo != uiEnd))
		{
			continue;
		}
		uiOther = (psConnection->uiFrom == uiEnd) ? psConnection->uiTo : psConnection->uiFrom;

		//The other terminal must still reach the pin without this connection, or the subnet would come apart.
		if (ReachesTerminal(uiOther, uiPin, i))
		{
			if (psConnection->uiFrom == uiEnd)
			{
				psConnection->uiTo = uiPin;
			}
			else
			{
				psConnection->uiFrom = uiPin;
			}
			psConnection->fBend = 0.5;
			return;
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each end of a splitter beside its component wired straight to its pin,
// the short wire between them.  KiCad's wires can join the end to the pin
// through another terminal (a label on the pin): that connection is
// joined to the pin instead, so long as the other terminal still reaches
// the pin by the other connections.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::JoinSplitterPins(void)
{
	size	i;
	size	uiPin;
	size	uiEnd;

	for (i = 0; i < mapcSplitterPins.NumElements(); i++)
	{
		uiPin = FindPortTerminal(mapcSplitterPins.GetPtr(i));
		uiEnd = FindPortTerminal(mapcSplitterEnds.GetPtr(i));
		if ((uiPin != ARRAY_ELEMENT_NOT_FOUND) && (uiEnd != ARRAY_ELEMENT_NOT_FOUND) && !IsConnectionBetween(uiEnd, uiPin))
		{
			JoinSplitterEnd(uiEnd, uiPin);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The terminals of the subnet, added to paiTerminals.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::GetSubnetTerminals(CKicadSchemSubnet* pcSubnet, CArrayInt* paiTerminals)
{
	size	i;

	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		if (masTerminals.Get(i)->pcSubnet == pcSubnet)
		{
			paiTerminals->Add((int)i);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Of the terminals at paiNodes not yet in paiConnected, the one fewest
// wire pieces from those that are, its path in paiBestPath.  -1 if none of
// them reaches by wire.
//
//////////////////////////////////////////////////////////////////////////
static int FindKicadSchemNearestTerminal(CKicadSchemSubnetGraph* pcGraph, CArrayInt* paiNodes, CArrayInt* paiConnected, CArrayInt* paiBestPath)
{
	CArrayInt	aiTargets;
	CArrayInt	aiPath;
	size		i;
	size		j;
	int			iBest;

	aiTargets.Init();
	for (i = 0; i < paiConnected->NumElements(); i++)
	{
		aiTargets.Add(paiNodes->GetValue(paiConnected->GetValue(i)));
	}

	iBest = -1;
	paiBestPath->ReInit();
	for (i = 0; i < paiNodes->NumElements(); i++)
	{
		if (paiConnected->Find((int)i) != ARRAY_ELEMENT_NOT_FOUND)
		{
			continue;
		}
		aiPath.Init();
		if (pcGraph->FindPath(paiNodes->GetValue(i), &aiTargets, &aiPath))
		{
			if ((iBest == -1) || (aiPath.NumElements() < paiBestPath->NumElements()))
			{
				iBest = (int)i;
				paiBestPath->ReInit();
				for (j = 0; j < aiPath.NumElements(); j++)
				{
					paiBestPath->Add(aiPath.GetValue(j));
				}
			}
		}
		aiPath.Kill();
	}
	aiTargets.Kill();
	return iBest;
}


//////////////////////////////////////////////////////////////////////////
//
// The first of the uiNumTerminals terminals not in paiConnected.
//
//////////////////////////////////////////////////////////////////////////
static int GetKicadSchemFirstUnconnected(size uiNumTerminals, CArrayInt* paiConnected)
{
	size	i;

	for (i = 0; i < uiNumTerminals; i++)
	{
		if (paiConnected->Find((int)i) == ARRAY_ELEMENT_NOT_FOUND)
		{
			break;
		}
	}
	return (int)i;
}


//////////////////////////////////////////////////////////////////////////
//
// The terminal of paiConnected at the graph's node iNode, 0 if none is.
//
//////////////////////////////////////////////////////////////////////////
static int GetKicadSchemConnectedAt(CArrayInt* paiNodes, CArrayInt* paiConnected, int iNode)
{
	size	i;

	for (i = 0; i < paiConnected->NumElements(); i++)
	{
		if (paiNodes->GetValue(paiConnected->GetValue(i)) == iNode)
		{
			return paiConnected->GetValue(i);
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
//
// The bend KiCad's wire along the graph's path paiPath has.
//
//////////////////////////////////////////////////////////////////////////
static float64 GetKicadSchemPathBend(CKicadSchemSubnetGraph* pcGraph, CArrayInt* paiPath)
{
	CArrayKicadSchemPoint	asPath;
	size					i;
	bool					bHorizontal;
	float64					fBend;

	asPath.Init();
	for (i = 0; i < paiPath->NumElements(); i++)
	{
		asPath.Add(pcGraph->GetPoint(paiPath->GetValue(i)));
	}
	fBend = CKicadSchemSubnetGraph::GetBendFraction(&asPath, &bHorizontal);
	asPath.Kill();
	return fBend;
}


//////////////////////////////////////////////////////////////////////////
//
// The subnet's terminals as a tree: each next is the terminal fewest wire
// pieces from those already in it, connected to the one it reaches.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddSubnetConnections(CKicadSchemSubnet* pcSubnet, size uiSubnet)
{
	CArrayInt						aiTerminals;
	CArrayInt						aiNodes;
	CArrayInt						aiConnected;
	CArrayInt						aiBestPath;
	CKicadSchemSubnetGraph			cGraph;
	size							i;
	int								iBest;
	int								iTo;
	SKicadSchemLogicimConnection*	psConnection;

	aiTerminals.Init();
	GetSubnetTerminals(pcSubnet, &aiTerminals);
	if (aiTerminals.NumElements() < 2)
	{
		aiTerminals.Kill();
		return;
	}

	cGraph.Init(pcSubnet);
	aiNodes.Init();
	for (i = 0; i < aiTerminals.NumElements(); i++)
	{
		aiNodes.Add((int)cGraph.AddPoint(&masTerminals.Get(aiTerminals.GetValue(i))->sKicadPosition));
	}
	cGraph.Build();

	aiConnected.Init();
	aiConnected.Add(0);
	aiBestPath.Init();
	while (aiConnected.NumElements() < aiTerminals.NumElements())
	{
		iBest = FindKicadSchemNearestTerminal(&cGraph, &aiNodes, &aiConnected, &aiBestPath);
		psConnection = masConnections.Add();
		psConnection->uiSubnet = uiSubnet;
		if (iBest == -1)
		{
			//Not joined by wires (only by being at the same place): straight to the first.
			iBest = GetKicadSchemFirstUnconnected(aiTerminals.NumElements(), &aiConnected);
			iTo = 0;
			psConnection->fBend = 0.5;
		}
		else
		{
			iTo = GetKicadSchemConnectedAt(&aiNodes, &aiConnected, aiBestPath.GetValue(aiBestPath.NumElements() - 1));
			psConnection->fBend = GetKicadSchemPathBend(&cGraph, &aiBestPath);
		}
		psConnection->bConnected = false;
		psConnection->uiFrom = aiTerminals.GetValue(iBest);
		psConnection->uiTo = aiTerminals.GetValue(iTo);
		aiConnected.Add(iBest);
	}

	aiBestPath.Kill();
	aiConnected.Kill();
	aiNodes.Kill();
	cGraph.Kill();
	aiTerminals.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The run of connections from item iFromItem to item iToItem,
// ARRAY_ELEMENT_NOT_FOUND if there isn't one yet.
//
//////////////////////////////////////////////////////////////////////////
static size FindKicadSchemRun(CArrayKicadSchemLogicimRun* pasRuns, int iFromItem, int iToItem)
{
	size						i;
	SKicadSchemLogicimRun*		psRun;

	for (i = 0; i < pasRuns->NumElements(); i++)
	{
		psRun = pasRuns->Get(i);
		if ((psRun->iFromItem == iFromItem) && (psRun->iToItem == iToItem))
		{
			return i;
		}
	}
	return ARRAY_ELEMENT_NOT_FOUND;
}


//////////////////////////////////////////////////////////////////////////
//
// The connections between each pair of items, counted in pasRuns.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::CountRuns(CArrayKicadSchemLogicimRun* pasRuns)
{
	size							i;
	size							uiRun;
	SKicadSchemLogicimConnection*	psConnection;
	SKicadSchemLogicimRun*			psRun;
	int								iFromItem;
	int								iToItem;

	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		iFromItem = (int)masTerminals.Get(psConnection->uiFrom)->uiItem;
		iToItem = (int)masTerminals.Get(psConnection->uiTo)->uiItem;
		uiRun = FindKicadSchemRun(pasRuns, iFromItem, iToItem);
		if (uiRun == ARRAY_ELEMENT_NOT_FOUND)
		{
			psRun = pasRuns->Add();
			psRun->iFromItem = iFromItem;
			psRun->iToItem = iToItem;
			psRun->iCount = 0;
			psRun->iRun = -1;
		}
		else
		{
			psRun = pasRuns->Get(uiRun);
		}
		psRun->iCount++;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The box the router keeps each item's wires out of, where the item has
// one.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::SetRouteBoxes(CTraceAutoConnector* pcConnector)
{
	size						i;
	SKicadSchemLogicimItem*		psItem;
	SFloat64Vec2				sBoxMinimum;
	SFloat64Vec2				sBoxMaximum;

	for (i = 0; i < masItems.NumElements(); i++)
	{
		psItem = masItems.Get(i);
		if (psItem->bImportBox && GetItemImportBox(psItem, &sBoxMinimum, &sBoxMaximum))
		{
			pcConnector->SetComponentBox(psItem->pcView, sBoxMinimum, sBoxMaximum);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each connection added to the router: in its subnet's net, and in its
// run if the run has more than one.  paiConnectorIndices gets each one's
// index in the router, -1 if the router wouldn't take it.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddRouteConnections(CTraceAutoConnector* pcConnector, CArrayKicadSchemLogicimRun* pasRuns, CArrayInt* paiConnectorIndices)
{
	CArrayInt						aiSubnetNets;
	size							i;
	size							uiRun;
	SKicadSchemLogicimConnection*	psConnection;
	SKicadSchemLogicimTerminal*		psFrom;
	SKicadSchemLogicimTerminal*		psTo;
	int								iNet;
	int								iRun;

	aiSubnetNets.Init();
	for (i = 0; i < mpcNetlist->NumSubnets() + mpcNetlist->NumBuses(); i++)
	{
		aiSubnetNets.Add(-1);
	}

	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		iNet = aiSubnetNets.GetValue(psConnection->uiSubnet);
		if (iNet == -1)
		{
			iNet = (int)pcConnector->AddNet();
			aiSubnetNets.SetValue(psConnection->uiSubnet, iNet);
		}
		psFrom = masTerminals.Get(psConnection->uiFrom);
		psTo = masTerminals.Get(psConnection->uiTo);
		uiRun = FindKicadSchemRun(pasRuns, (int)psFrom->uiItem, (int)psTo->uiItem);
		iRun = (uiRun != ARRAY_ELEMENT_NOT_FOUND) ? pasRuns->Get(uiRun)->iRun : -1;
		paiConnectorIndices->Add(pcConnector->AddConnection(iNet, psFrom->pcPortView, psTo->pcPortView, psConnection->fBend, iRun) ? (int)(pcConnector->NumConnections() - 1) : -1);
	}
	aiSubnetNets.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Rewires the circuit from nothing and returns how many connections
// couldn't be routed.  Each subnet is a net, and connections between the
// same two items are a run.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimConverter::Route(void)
{
	CTraceAutoConnector				cConnector;
	CArrayKicadSchemLogicimRun		asRuns;
	CArrayInt						aiConnectorIndices;
	size							i;
	SKicadSchemLogicimConnection*	psConnection;
	SKicadSchemLogicimRun*			psRun;
	size							uiUnconnected;

	mpcCircuit->KillTraces();

	asRuns.Init();
	CountRuns(&asRuns);

	cConnector.Init(mpcCircuit);
	SetRouteBoxes(&cConnector);
	for (i = 0; i < asRuns.NumElements(); i++)
	{
		psRun = asRuns.Get(i);
		if (psRun->iCount > 1)
		{
			psRun->iRun = (int)cConnector.AddRun();
		}
	}

	aiConnectorIndices.Init();
	AddRouteConnections(&cConnector, &asRuns, &aiConnectorIndices);
	cConnector.Connect();
	uiUnconnected = 0;
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		psConnection->bConnected = (aiConnectorIndices.GetValue(i) != -1) && cConnector.IsConnected(aiConnectorIndices.GetValue(i));
		if (!psConnection->bConnected)
		{
			uiUnconnected++;
		}
	}
	aiConnectorIndices.Kill();
	cConnector.Kill();
	asRuns.Kill();
	return uiUnconnected;
}
