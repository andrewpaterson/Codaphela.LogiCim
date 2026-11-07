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
	CArrayInt						aiReached;
	size							uiHead;
	size							uiNext;
	size							i;
	SKicadSchemLogicimConnection*	psConnection;

	aiReached.Init();
	aiReached.Add((int)uiFrom);
	for (uiHead = 0; uiHead < aiReached.NumElements(); uiHead++)
	{
		for (i = 0; i < mpcModel->NumConnections(); i++)
		{
			psConnection = mpcModel->GetConnection(i);
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
// The subnet's terminals as a tree: each next is the terminal fewest wire
// pieces from those already in it, connected to the one it reaches.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimWiring::AddSubnetConnections(CKicadSchemSubnet* pcSubnet)
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
	mpcModel->GetSubnetTerminals(pcSubnet, &aiTerminals);
	if (aiTerminals.NumElements() < 2)
	{
		aiTerminals.Kill();
		return;
	}

	cGraph.Init(pcSubnet);
	aiNodes.Init();
	for (i = 0; i < aiTerminals.NumElements(); i++)
	{
		aiNodes.Add((int)cGraph.AddPoint(&mpcModel->GetTerminal(aiTerminals.GetValue(i))->sKicadPosition));
	}
	cGraph.Build();

	aiConnected.Init();
	aiConnected.Add(0);
	aiBestPath.Init();
	while (aiConnected.NumElements() < aiTerminals.NumElements())
	{
		iBest = FindKicadSchemNearestTerminal(&cGraph, &aiNodes, &aiConnected, &aiBestPath);
		psConnection = mpcModel->AddConnection();
		psConnection->pcSubnet = pcSubnet;
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
		pauiRouted->Add(pcConnector->AddConnection(uiNet, psFrom->pcPortView, psTo->pcPortView, psConnection->fBend, (int)uiRun));
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
