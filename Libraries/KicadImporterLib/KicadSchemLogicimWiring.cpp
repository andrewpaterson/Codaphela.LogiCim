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
#include "LogicimLib/TraceAutoConnector.h"
#include "KicadLib/KicadSchemSubnetGraph.h"
#include "KicadSchemLogicimWiring.h"


//////////////////////////////////////////////////////////////////////////
//
// Wires pcModel's terminals, the splitters' ends to their pins as
// pcBuses has them.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimWiring::Init(CKicadSchemNetlist* pcNetlist, CKicadSchemLogicimModel* pcModel, CKicadSchemLogicimBuses* pcBuses)
{
	mpcNetlist = pcNetlist;
	mpcModel = pcModel;
	mpcBuses = pcBuses;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimWiring::Kill(void)
{
	mpcBuses = NULL;
	mpcModel = NULL;
	mpcNetlist = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimWiring::AddConnections(void)
{
	size	i;

	for (i = 0; i < mpcNetlist->NumSubnets(); i++)
	{
		AddSubnetConnections(mpcNetlist->GetSubnet(i));
	}
	for (i = 0; i < mpcNetlist->NumBuses(); i++)
	{
		AddSubnetConnections(mpcNetlist->GetBus(i)->GetSubnet());
	}
	JoinSplitterPins();
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a connection joins the two terminals, either way round.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimWiring::IsConnectionBetween(size uiTerminal1, size uiTerminal2)
{
	size							i;
	SKicadSchemLogicimConnection*	psConnection;

	for (i = 0; i < mpcModel->NumConnections(); i++)
	{
		psConnection = mpcModel->GetConnection(i);
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
bool CKicadSchemLogicimWiring::ReachesTerminal(size uiFrom, size uiTo, size uiWithout)
{
	CArraySize						auiReached;
	size							uiHead;
	size							uiNext;
	size							i;
	SKicadSchemLogicimConnection*	psConnection;

	auiReached.Init();
	auiReached.Add(uiFrom);
	for (uiHead = 0; uiHead < auiReached.NumElements(); uiHead++)
	{
		for (i = 0; i < mpcModel->NumConnections(); i++)
		{
			psConnection = mpcModel->GetConnection(i);
			if (i == uiWithout)
			{
				continue;
			}
			if (psConnection->uiFrom == auiReached.GetValue(uiHead))
			{
				uiNext = psConnection->uiTo;
			}
			else if (psConnection->uiTo == auiReached.GetValue(uiHead))
			{
				uiNext = psConnection->uiFrom;
			}
			else
			{
				continue;
			}
			if (uiNext == uiTo)
			{
				auiReached.Kill();
				return true;
			}
			if (auiReached.Find(uiNext) == ARRAY_ELEMENT_NOT_FOUND)
			{
				auiReached.Add(uiNext);
			}
		}
	}
	auiReached.Kill();
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The first connection from the splitter's end to another terminal that
// still reaches the pin without it, joined to the pin instead (see
// JoinSplitterPins).
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimWiring::JoinSplitterEnd(size uiEnd, size uiPin)
{
	size							i;
	size							uiOther;
	SKicadSchemLogicimConnection*	psConnection;

	for (i = 0; i < mpcModel->NumConnections(); i++)
	{
		psConnection = mpcModel->GetConnection(i);
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
void CKicadSchemLogicimWiring::JoinSplitterPins(void)
{
	size	i;
	size	uiPin;
	size	uiEnd;

	for (i = 0; i < mpcBuses->NumSplitterPairs(); i++)
	{
		uiPin = mpcModel->FindPortTerminal(mpcBuses->GetSplitterPair(i)->pcPin);
		uiEnd = mpcModel->FindPortTerminal(mpcBuses->GetSplitterPair(i)->pcEnd);
		if ((uiPin != ARRAY_ELEMENT_NOT_FOUND) && (uiEnd != ARRAY_ELEMENT_NOT_FOUND) && !IsConnectionBetween(uiEnd, uiPin))
		{
			JoinSplitterEnd(uiEnd, uiPin);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Of the terminals at paiNodes not yet in pauiConnected, the one fewest
// wire pieces from those that are, its path in paiBestPath.
// ARRAY_ELEMENT_NOT_FOUND if none of them reaches by wire.
//
//////////////////////////////////////////////////////////////////////////
static size FindKicadSchemNearestTerminal(CKicadSchemSubnetGraph* pcGraph, CArrayInt* paiNodes, CArraySize* pauiConnected, CArrayInt* paiBestPath)
{
	CArrayInt	aiTargets;
	CArrayInt	aiPath;
	size		i;
	size		j;
	size		uiBest;

	aiTargets.Init();
	for (i = 0; i < pauiConnected->NumElements(); i++)
	{
		aiTargets.Add(paiNodes->GetValue(pauiConnected->GetValue(i)));
	}

	uiBest = ARRAY_ELEMENT_NOT_FOUND;
	paiBestPath->ReInit();
	for (i = 0; i < paiNodes->NumElements(); i++)
	{
		if (pauiConnected->Find(i) != ARRAY_ELEMENT_NOT_FOUND)
		{
			continue;
		}
		aiPath.Init();
		if (pcGraph->FindPath(paiNodes->GetValue(i), &aiTargets, &aiPath))
		{
			if ((uiBest == ARRAY_ELEMENT_NOT_FOUND) || (aiPath.NumElements() < paiBestPath->NumElements()))
			{
				uiBest = i;
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
	return uiBest;
}


//////////////////////////////////////////////////////////////////////////
//
// The first of the uiNumTerminals terminals not in pauiConnected.
//
//////////////////////////////////////////////////////////////////////////
static size GetKicadSchemFirstUnconnected(size uiNumTerminals, CArraySize* pauiConnected)
{
	size	i;

	for (i = 0; i < uiNumTerminals; i++)
	{
		if (pauiConnected->Find(i) == ARRAY_ELEMENT_NOT_FOUND)
		{
			break;
		}
	}
	return i;
}


//////////////////////////////////////////////////////////////////////////
//
// The terminal of pauiConnected at the graph's node iNode, 0 if none is.
//
//////////////////////////////////////////////////////////////////////////
static size GetKicadSchemConnectedAt(CArrayInt* paiNodes, CArraySize* pauiConnected, int iNode)
{
	size	i;

	for (i = 0; i < pauiConnected->NumElements(); i++)
	{
		if (paiNodes->GetValue(pauiConnected->GetValue(i)) == iNode)
		{
			return pauiConnected->GetValue(i);
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
	CArrayFloat64Vec2		asPath;
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
// The subnet's terminals as a tree (see ConnectSubnetTerminals), each a
// node of a graph of its wires.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimWiring::AddSubnetConnections(CKicadSchemSubnet* pcSubnet)
{
	CArraySize				auiTerminals;
	CArrayInt				aiNodes;
	CKicadSchemSubnetGraph	cGraph;
	size					i;

	auiTerminals.Init();
	mpcModel->GetSubnetTerminals(pcSubnet, &auiTerminals);
	if (auiTerminals.NumElements() >= 2)
	{
		cGraph.Init(pcSubnet);
		aiNodes.Init();
		for (i = 0; i < auiTerminals.NumElements(); i++)
		{
			aiNodes.Add((int)cGraph.AddPoint(&mpcModel->GetTerminal(auiTerminals.GetValue(i))->sKicadPosition));
		}
		cGraph.Build();
		ConnectSubnetTerminals(pcSubnet, &cGraph, &auiTerminals, &aiNodes);
		aiNodes.Kill();
		cGraph.Kill();
	}
	auiTerminals.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The subnet's terminals (pauiTerminals, at the graph's nodes paiNodes) as
// a tree: each next is the terminal fewest wire pieces from those already
// in it, connected to the one it reaches.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimWiring::ConnectSubnetTerminals(CKicadSchemSubnet* pcSubnet, CKicadSchemSubnetGraph* pcGraph, CArraySize* pauiTerminals, CArrayInt* paiNodes)
{
	CArraySize						auiConnected;
	CArrayInt						aiBestPath;
	size							uiBest;
	size							uiTo;
	SKicadSchemLogicimConnection*	psConnection;

	auiConnected.Init();
	auiConnected.Add(0);
	aiBestPath.Init();
	while (auiConnected.NumElements() < pauiTerminals->NumElements())
	{
		uiBest = FindKicadSchemNearestTerminal(pcGraph, paiNodes, &auiConnected, &aiBestPath);
		psConnection = mpcModel->AddConnection();
		psConnection->pcSubnet = pcSubnet;
		if (uiBest == ARRAY_ELEMENT_NOT_FOUND)
		{
			//Not joined by wires (only by being at the same place): straight to the first.
			uiBest = GetKicadSchemFirstUnconnected(pauiTerminals->NumElements(), &auiConnected);
			uiTo = 0;
			psConnection->fBend = 0.5;
		}
		else
		{
			uiTo = GetKicadSchemConnectedAt(paiNodes, &auiConnected, aiBestPath.GetValue(aiBestPath.NumElements() - 1));
			psConnection->fBend = GetKicadSchemPathBend(pcGraph, &aiBestPath);
		}
		psConnection->bConnected = false;
		psConnection->uiFrom = pauiTerminals->GetValue(uiBest);
		psConnection->uiTo = pauiTerminals->GetValue(uiTo);
		auiConnected.Add(uiBest);
	}
	aiBestPath.Kill();
	auiConnected.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The box the router keeps each item's wires out of, where the item has
// one.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimWiring::SetRouteBoxes(CTraceAutoConnector* pcConnector)
{
	size						i;
	SKicadSchemLogicimItem*		psItem;
	SFloat64Bounds				sBox;

	for (i = 0; i < mpcModel->NumItems(); i++)
	{
		psItem = mpcModel->GetItem(i);
		if (psItem->bImportBox && mpcModel->GetItemImportBox(i, &sBox))
		{
			pcConnector->SetComponentBox(psItem->pcView, &sBox);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each connection added to the router, in its subnet's net and in the
// run of the connections between the same two items.  pauiRouted gets
// each one's connection in the router.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimWiring::AddRouteConnections(CTraceAutoConnector* pcConnector, CArrayTemplatePrimitive<size>* pauiRouted)
{
	size							i;
	SKicadSchemLogicimConnection*	psConnection;
	SKicadSchemLogicimTerminal*		psFrom;
	SKicadSchemLogicimTerminal*		psTo;
	size							uiNet;
	size							uiRun;

	for (i = 0; i < mpcModel->NumConnections(); i++)
	{
		psConnection = mpcModel->GetConnection(i);
		psFrom = mpcModel->GetTerminal(psConnection->uiFrom);
		psTo = mpcModel->GetTerminal(psConnection->uiTo);
		uiNet = pcConnector->FindOrAddNet(psFrom->pcSubnet);
		uiRun = pcConnector->FindOrAddRun(mpcModel->GetItem(psFrom->uiItem)->pcView, mpcModel->GetItem(psTo->uiItem)->pcView);
		pauiRouted->Add(pcConnector->AddConnection(uiNet, psFrom->pcPortView, psTo->pcPortView, psConnection->fBend, uiRun));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Rewires the circuit from nothing and returns how many connections
// couldn't be routed.  Each subnet is a net, and connections between the
// same two items are a run.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimWiring::Route(void)
{
	CTraceAutoConnector				cConnector;
	CArrayTemplatePrimitive<size>	auiRouted;
	size							i;
	SKicadSchemLogicimConnection*	psConnection;
	size							uiUnconnected;

	mpcModel->GetCircuit()->KillTraces();
	cConnector.Init(mpcModel->GetCircuit());
	SetRouteBoxes(&cConnector);
	auiRouted.Init();
	AddRouteConnections(&cConnector, &auiRouted);
	cConnector.Connect();

	uiUnconnected = 0;
	for (i = 0; i < mpcModel->NumConnections(); i++)
	{
		psConnection = mpcModel->GetConnection(i);
		psConnection->bConnected = cConnector.IsConnected(auiRouted.GetValue(i));
		if (!psConnection->bConnected)
		{
			uiUnconnected++;
		}
	}
	auiRouted.Kill();
	cConnector.Kill();
	return uiUnconnected;
}
