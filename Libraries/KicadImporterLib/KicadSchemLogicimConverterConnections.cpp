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
			if (StringCompare(psItem->pcView->GetXMLName(), "Tunnel") == 0)
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
// Each end of a splitter beside its component wired straight to its pin,
// the short wire between them.  KiCad's wires can join the end to the pin
// through another terminal (a label on the pin): that connection is
// joined to the pin instead, so long as the other terminal still reaches
// the pin by the other connections.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::JoinSplitterPins(void)
{
	size							i;
	size							j;
	size							k;
	size							uiPin;
	size							uiEnd;
	size							uiOther;
	SKicadSchemLogicimConnection*	psConnection;
	SKicadSchemLogicimConnection*	psOther;
	bool							bJoined;
	bool							bOtherJoined;
	CArrayInt						aiReached;
	size							uiHead;
	size							uiNext;

	aiReached.Init();
	for (i = 0; i < mapcSplitterPins.NumElements(); i++)
	{
		uiPin = FindPortTerminal(mapcSplitterPins.GetPtr(i));
		uiEnd = FindPortTerminal(mapcSplitterEnds.GetPtr(i));
		if ((uiPin == ARRAY_ELEMENT_NOT_FOUND) || (uiEnd == ARRAY_ELEMENT_NOT_FOUND))
		{
			continue;
		}

		bJoined = false;
		for (j = 0; (j < masConnections.NumElements()) && !bJoined; j++)
		{
			psConnection = masConnections.Get(j);
			bJoined = ((psConnection->uiFrom == uiEnd) && (psConnection->uiTo == uiPin)) || ((psConnection->uiFrom == uiPin) && (psConnection->uiTo == uiEnd));
		}
		for (j = 0; (j < masConnections.NumElements()) && !bJoined; j++)
		{
			psConnection = masConnections.Get(j);
			if ((psConnection->uiFrom != uiEnd) && (psConnection->uiTo != uiEnd))
			{
				continue;
			}
			uiOther = (psConnection->uiFrom == uiEnd) ? psConnection->uiTo : psConnection->uiFrom;

			//The other terminal must still reach the pin without this connection, or the subnet would come apart.
			aiReached.ReInit();
			aiReached.Add((int)uiOther);
			bOtherJoined = false;
			for (uiHead = 0; (uiHead < aiReached.NumElements()) && !bOtherJoined; uiHead++)
			{
				for (k = 0; k < masConnections.NumElements(); k++)
				{
					psOther = masConnections.Get(k);
					if (k == j)
					{
						continue;
					}
					if (psOther->uiFrom == (size)aiReached.GetValue(uiHead))
					{
						uiNext = psOther->uiTo;
					}
					else if (psOther->uiTo == (size)aiReached.GetValue(uiHead))
					{
						uiNext = psOther->uiFrom;
					}
					else
					{
						continue;
					}
					if (uiNext == uiPin)
					{
						bOtherJoined = true;
						break;
					}
					if (aiReached.Find((int)uiNext) == ARRAY_ELEMENT_NOT_FOUND)
					{
						aiReached.Add((int)uiNext);
					}
				}
			}
			if (!bOtherJoined)
			{
				continue;
			}
			if (psConnection->uiFrom == uiEnd)
			{
				psConnection->uiTo = uiPin;
			}
			else
			{
				psConnection->uiFrom = uiPin;
			}
			psConnection->fBend = 0.5;
			bJoined = true;
		}
	}
	aiReached.Kill();
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
	CArrayInt						aiTargets;
	CArrayInt						aiPath;
	CArrayInt						aiBestPath;
	CArrayKicadSchemPoint			asPath;
	CKicadSchemSubnetGraph			cGraph;
	size							i;
	size							j;
	int								iBest;
	int								iTo;
	SKicadSchemLogicimConnection*	psConnection;
	bool							bHorizontal;

	aiTerminals.Init();
	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		if (masTerminals.Get(i)->pcSubnet == pcSubnet)
		{
			aiTerminals.Add((int)i);
		}
	}
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
	aiTargets.Init();
	aiBestPath.Init();
	while (aiConnected.NumElements() < aiTerminals.NumElements())
	{
		aiTargets.ReInit();
		for (i = 0; i < aiConnected.NumElements(); i++)
		{
			aiTargets.Add(aiNodes.GetValue(aiConnected.GetValue(i)));
		}

		iBest = -1;
		aiBestPath.ReInit();
		for (i = 0; i < aiTerminals.NumElements(); i++)
		{
			if (aiConnected.Find((int)i) != ARRAY_ELEMENT_NOT_FOUND)
			{
				continue;
			}
			aiPath.Init();
			if (cGraph.FindPath(aiNodes.GetValue(i), &aiTargets, &aiPath))
			{
				if ((iBest == -1) || (aiPath.NumElements() < aiBestPath.NumElements()))
				{
					iBest = (int)i;
					aiBestPath.ReInit();
					for (j = 0; j < aiPath.NumElements(); j++)
					{
						aiBestPath.Add(aiPath.GetValue(j));
					}
				}
			}
			aiPath.Kill();
		}

		psConnection = masConnections.Add();
		psConnection->uiSubnet = uiSubnet;
		if (iBest == -1)
		{
			//Not joined by wires (only by being at the same place): straight to the first.
			for (i = 0; i < aiTerminals.NumElements(); i++)
			{
				if (aiConnected.Find((int)i) == ARRAY_ELEMENT_NOT_FOUND)
				{
					break;
				}
			}
			iBest = (int)i;
			iTo = 0;
			psConnection->fBend = 0.5;
		}
		else
		{
			iTo = 0;
			for (i = 0; i < aiConnected.NumElements(); i++)
			{
				if (aiNodes.GetValue(aiConnected.GetValue(i)) == aiBestPath.GetValue(aiBestPath.NumElements() - 1))
				{
					iTo = aiConnected.GetValue(i);
					break;
				}
			}
			asPath.Init();
			for (i = 0; i < aiBestPath.NumElements(); i++)
			{
				asPath.Add(cGraph.GetPoint(aiBestPath.GetValue(i)));
			}
			psConnection->fBend = CKicadSchemSubnetGraph::GetBendFraction(&asPath, &bHorizontal);
			asPath.Kill();
		}
		psConnection->bConnected = false;
		psConnection->uiFrom = aiTerminals.GetValue(iBest);
		psConnection->uiTo = aiTerminals.GetValue(iTo);
		aiConnected.Add(iBest);
	}

	aiBestPath.Kill();
	aiTargets.Kill();
	aiConnected.Kill();
	aiNodes.Kill();
	cGraph.Kill();
	aiTerminals.Kill();
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
	CArrayInt						aiSubnetNets;
	CArrayInt						aiRunFrom;
	CArrayInt						aiRunTo;
	CArrayInt						aiRunCounts;
	CArrayInt						aiRuns;
	CArrayInt						aiConnectorIndices;
	size							i;
	size							j;
	SKicadSchemLogicimConnection*	psConnection;
	SKicadSchemLogicimTerminal*		psFrom;
	SKicadSchemLogicimTerminal*		psTo;
	int								iNet;
	int								iRun;
	int								iFromItem;
	int								iToItem;
	size							uiUnconnected;
	SKicadSchemLogicimItem*			psItem;
	SFloat64Vec2					sBoxMinimum;
	SFloat64Vec2					sBoxMaximum;

	mpcCircuit->KillTraces();

	//The connections between each pair of items.
	aiRunFrom.Init();
	aiRunTo.Init();
	aiRunCounts.Init();
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		iFromItem = (int)masTerminals.Get(psConnection->uiFrom)->uiItem;
		iToItem = (int)masTerminals.Get(psConnection->uiTo)->uiItem;
		for (j = 0; j < aiRunFrom.NumElements(); j++)
		{
			if ((aiRunFrom.GetValue(j) == iFromItem) && (aiRunTo.GetValue(j) == iToItem))
			{
				break;
			}
		}
		if (j == aiRunFrom.NumElements())
		{
			aiRunFrom.Add(iFromItem);
			aiRunTo.Add(iToItem);
			aiRunCounts.Add(0);
		}
		aiRunCounts.SetValue(j, aiRunCounts.GetValue(j) + 1);
	}

	cConnector.Init(mpcCircuit);
	for (i = 0; i < masItems.NumElements(); i++)
	{
		psItem = masItems.Get(i);
		if (psItem->bImportBox && GetItemImportBox(psItem, &sBoxMinimum, &sBoxMaximum))
		{
			cConnector.SetComponentBox(psItem->pcView, sBoxMinimum, sBoxMaximum);
		}
	}
	aiRuns.Init();
	for (i = 0; i < aiRunCounts.NumElements(); i++)
	{
		aiRuns.Add((aiRunCounts.GetValue(i) > 1) ? (int)cConnector.AddRun() : -1);
	}
	aiSubnetNets.Init();
	for (i = 0; i < mpcNetlist->NumSubnets() + mpcNetlist->NumBuses(); i++)
	{
		aiSubnetNets.Add(-1);
	}

	aiConnectorIndices.Init();
	for (i = 0; i < masConnections.NumElements(); i++)
	{
		psConnection = masConnections.Get(i);
		iNet = aiSubnetNets.GetValue(psConnection->uiSubnet);
		if (iNet == -1)
		{
			iNet = (int)cConnector.AddNet();
			aiSubnetNets.SetValue(psConnection->uiSubnet, iNet);
		}
		psFrom = masTerminals.Get(psConnection->uiFrom);
		psTo = masTerminals.Get(psConnection->uiTo);
		iRun = -1;
		for (j = 0; j < aiRunFrom.NumElements(); j++)
		{
			if ((aiRunFrom.GetValue(j) == (int)psFrom->uiItem) && (aiRunTo.GetValue(j) == (int)psTo->uiItem))
			{
				iRun = aiRuns.GetValue(j);
				break;
			}
		}
		aiConnectorIndices.Add(cConnector.AddConnection(iNet, psFrom->pcPortView, psTo->pcPortView, psConnection->fBend, iRun) ? (int)(cConnector.NumConnections() - 1) : -1);
	}
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

	aiSubnetNets.Kill();
	aiRuns.Kill();
	aiRunCounts.Kill();
	aiRunTo.Kill();
	aiRunFrom.Kill();
	return uiUnconnected;
}
