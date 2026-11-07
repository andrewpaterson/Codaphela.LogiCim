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
#include <stdlib.h>
#include "BaseLib/ConstructorCall.h"
#include "BaseLib/PointerFunctions.h"
#include "BaseLib/Logger.h"
#include "BaseLib/XMLFile.h"
#include "BaseLib/XMLWriter.h"
#include "BaseLib/Markup.h"
#include "BaseLib/LogString.h"
#include "BaseLib/ArrayElementNotFound.h"
#include "StandardLib/Objects.h"
#include "MicroprocessorLib/CircuitSimulation.h"
#include "CircuitPainter.h"
#include "CircuitWiring.h"
#include "CircuitFile.h"
#include "SimulationStateXML.h"
#include "ComponentPlacement.h"
#include "ComponentViewFactory.h"
#include "MoveComponentsEdit.h"
#include "SelectionEdit.h"
#include "WirePullEdit.h"
#include "TraceAutoConnector.h"
#include "SubcircuitInstanceView.h"
#include "CircuitEditor.h"
#include "CircuitEditorInternal.h"


//////////////////////////////////////////////////////////////////////////
//
// Views being moved aren't selected while they move.  Moved bConnected,
// the wires that aren't moving are followed out from each moved port and
// each end of each moved wire (see FollowFrom), and then the moved wires'
// ends still joined to something that isn't moving are anchored.
// Otherwise views already in the circuit are detached from their wires.
// New views (being placed) are in no simulation yet, so neither is done.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::StartMoveViews(CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcTraces, bool bNew, bool bConnected, CArrayTraceViewPtr* papcFollowing)
{
	size				i;
	size				j;
	CComponentView*		pcComponent;
	CTraceView*			pcTrace;
	CArrayTraceViewPtr	apcMoving;

	mcSelection.Clear();
	KillLeads();
	masAnchors.ReInit();
	mbDetachedMove = false;
	if (bNew)
	{
		return;
	}
	if (!bConnected)
	{
		mbDetachedMove = true;
		Rewire(papcComponents, papcTraces);
		return;
	}

	apcMoving.Init();
	for (i = 0; i < papcTraces->NumElements(); i++)
	{
		apcMoving.Add(papcTraces->GetPtr(i));
	}
	for (i = 0; i < papcComponents->NumElements(); i++)
	{
		pcComponent = papcComponents->GetPtr(i);
		for (j = 0; j < pcComponent->NumPortViews(); j++)
		{
			FollowFrom(pcComponent->GetPortView(j)->GetGridPosition(), pcComponent->GetPortView(j), papcComponents, &apcMoving, papcFollowing);
		}
	}
	for (i = 0; i < papcTraces->NumElements(); i++)
	{
		pcTrace = papcTraces->GetPtr(i);
		FollowFrom(pcTrace->GetStart(), NULL, papcComponents, &apcMoving, papcFollowing);
		FollowFrom(pcTrace->GetEnd(), NULL, papcComponents, &apcMoving, papcFollowing);
	}
	FindAnchors(papcComponents, &apcMoving);
	apcMoving.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Follows the wires that aren't moving from sPoint, a moved port (pcPort)
// or the end of a moved wire (pcPort NULL), see FollowChain.  Wires that
// follow are added to papcFollowing (and papcMoving), to be moved with the
// views.  A port's wires that stop at a junction or a port are its lead,
// to be routed again as it moves (see MovedViews); a moved wire's are
// left, and it's anchored to them (see FindAnchors).
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::FollowFrom(SInt32Vec2 sPoint, CPortView* pcPort, CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcMoving, CArrayTraceViewPtr* papcFollowing)
{
	CArrayTraceViewPtr	apcChain;
	SInt32Vec2			sEnd;
	CPortView*			pcPortAt;
	EMoveChain			eChain;
	SMoveLead*			psLead;
	size				i;

	apcChain.Init();
	eChain = FollowChain(sPoint, papcComponents, papcMoving, &apcChain, &sEnd, &pcPortAt);
	if (eChain == MC_Follows)
	{
		for (i = 0; i < apcChain.NumElements(); i++)
		{
			papcFollowing->Add(apcChain.GetPtr(i));
			mapcFollowing.Add(apcChain.GetPtr(i));
			papcMoving->Add(apcChain.GetPtr(i));
		}
	}
	else if ((eChain == MC_Stops) && (pcPort != NULL))
	{
		psLead = masLeads.Add();
		psLead->pcPort = pcPort;
		psLead->sTarget = sEnd;
		psLead->apcTraces.Init();
		for (i = 0; i < apcChain.NumElements(); i++)
		{
			psLead->apcTraces.Add(apcChain.GetPtr(i));
		}
		psLead->pcNetPort = FindNetPort(sEnd, &apcChain, papcComponents);
		psLead->pcTargetPort = (pcPortAt != NULL) ? pcPortAt : psLead->pcNetPort;
		if (psLead->pcNetPort == NULL)
		{
			psLead->pcNetPort = pcPortAt;
		}
	}
	apcChain.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The port at sPoint that isn't moving (isn't on one of papcComponents),
// NULL if there's none.  Each port there that is moving is counted in
// *puiMoving.
//
//////////////////////////////////////////////////////////////////////////
static CPortView* FindStillPortAt(SInt32Vec2 sPoint, CArrayPortViewPtr* papcPortViews, CArrayComponentViewPtr* papcComponents, size* puiMoving)
{
	size		i;
	CPortView*	pcPortView;
	CPortView*	pcStill;

	pcStill = NULL;
	for (i = 0; i < papcPortViews->NumElements(); i++)
	{
		pcPortView = papcPortViews->GetPtr(i);
		if (pcPortView->GetGridPosition() == sPoint)
		{
			if (papcComponents->Find(pcPortView->GetOwner()) == ARRAY_ELEMENT_NOT_FOUND)
			{
				pcStill = pcPortView;
			}
			else
			{
				(*puiMoving)++;
			}
		}
	}
	return pcStill;
}


//////////////////////////////////////////////////////////////////////////
//
// A wire on sPoint that isn't moving, not pcFrom or already in the chain,
// NULL if there's none.  The wires there that aren't moving are counted
// in *puiStill and those that are in *puiMoving.
//
//////////////////////////////////////////////////////////////////////////
CTraceView* CCircuitEditor::FindStillTraceAt(SInt32Vec2 sPoint, CTraceView* pcFrom, CArrayTraceViewPtr* papcMoving, CArrayTraceViewPtr* papcChain, size* puiStill, size* puiMoving)
{
	size			i;
	CTraceView*		pcTrace;
	CTraceView*		pcStill;

	pcStill = NULL;
	for (i = 0; i < mpcCircuit->NumTraces(); i++)
	{
		pcTrace = mpcCircuit->GetTrace(i);
		if ((pcTrace != pcFrom) && pcTrace->Contains(sPoint) && (papcChain->Find(pcTrace) == ARRAY_ELEMENT_NOT_FOUND))
		{
			if (papcMoving->Find(pcTrace) != ARRAY_ELEMENT_NOT_FOUND)
			{
				(*puiMoving)++;
			}
			else
			{
				pcStill = pcTrace;
				(*puiStill)++;
			}
		}
	}
	return pcStill;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the chain FollowChain is following ends at sPoint, and how, in
// *peChain.  bStart if sPoint is where it started, bPortAt if a port
// that isn't moving is there, uiStill and uiMoving how many still and
// moving things are joined there, and pcNext one of the still wires.
//
//////////////////////////////////////////////////////////////////////////
static bool IsMoveChainEnd(SInt32Vec2 sPoint, bool bStart, bool bPortAt, size uiStill, size uiMoving, CTraceView* pcNext, EMoveChain* peChain)
{
	//The moved port or wire at sPoint itself is one of the moving there.
	if (bStart && (uiMoving > 1))
	{
		*peChain = MC_None;
	}
	else if (bPortAt)
	{
		*peChain = MC_Stops;
	}
	else if (bStart && (uiStill == 0))
	{
		*peChain = MC_None;
	}
	else if (!bStart && (uiMoving > 0))
	{
		*peChain = (uiStill == 0) ? MC_Follows : MC_Stops;
	}
	else if (uiStill == 0)
	{
		*peChain = MC_Follows;
	}
	else if ((uiStill > 1) || !pcNext->IsEnd(sPoint))
	{
		*peChain = MC_Stops;
	}
	else
	{
		return false;
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Follows the wires that aren't moving from sPoint (a moved port, or a
// moved wire's end) on through the points where one wire ends on another
// and nothing else is, into papcChain.  MC_Follows if they end on nothing
// or on something else that's moving: they can move with it.  MC_Stops,
// with where they stop in *psEnd and the port there (that isn't moving)
// in *ppcPortAt, if they end on a junction or a port; a junction or port
// right at sPoint stops there with no wires.  MC_None if nothing that
// isn't moving is joined at sPoint, or something else moving is.
//
//////////////////////////////////////////////////////////////////////////
EMoveChain CCircuitEditor::FollowChain(SInt32Vec2 sPoint, CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcMoving, CArrayTraceViewPtr* papcChain, SInt32Vec2* psEnd, CPortView** ppcPortAt)
{
	CTraceView*			pcFrom;
	CTraceView*			pcNext;
	size				uiStill;
	size				uiMoving;
	CArrayPortViewPtr	apcPortViews;
	EMoveChain			eChain;

	apcPortViews.Init();
	mpcCircuit->CollectPortViews(&apcPortViews);
	pcFrom = NULL;
	for (;;)
	{
		*psEnd = sPoint;
		uiStill = 0;
		uiMoving = 0;
		*ppcPortAt = FindStillPortAt(sPoint, &apcPortViews, papcComponents, &uiMoving);
		pcNext = FindStillTraceAt(sPoint, pcFrom, papcMoving, papcChain, &uiStill, &uiMoving);
		if (IsMoveChainEnd(sPoint, pcFrom == NULL, *ppcPortAt != NULL, uiStill, uiMoving, pcNext, &eChain))
		{
			apcPortViews.Kill();
			return eChain;
		}

		papcChain->Add(pcNext);
		pcFrom = pcNext;
		sPoint = (pcFrom->GetStart() == sPoint) ? pcFrom->GetEnd() : pcFrom->GetStart();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Called by the move each time the views have moved (or turned or
// flipped): the leads are routed to where they are now.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::MovedViews(void)
{
	if (!mbDetachedMove)
	{
		RouteLeads();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The ends of the moved wires that are joined to something that isn't
// moving, see SMoveAnchor.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::FindAnchors(CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcTraces)
{
	size			i;
	int				iEnd;
	CTraceView*		pcTrace;
	SInt32Vec2		sEnd;
	SMoveAnchor*	psAnchor;

	for (i = 0; i < papcTraces->NumElements(); i++)
	{
		pcTrace = papcTraces->GetPtr(i);
		for (iEnd = 0; iEnd < 2; iEnd++)
		{
			sEnd = (iEnd == 0) ? pcTrace->GetStart() : pcTrace->GetEnd();
			if (IsJoinedAt(sEnd, papcComponents, papcTraces))
			{
				psAnchor = masAnchors.Add();
				psAnchor->pcTrace = pcTrace;
				psAnchor->bStart = (iEnd == 0);
				psAnchor->sOriginal = sEnd;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a wire that isn't moving is on the grid point, or a port of a
// component that isn't.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::IsJoinedAt(SInt32Vec2 sPoint, CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcTraces)
{
	size				i;
	size				j;
	CTraceView*			pcTrace;
	CComponentView*		pcComponent;

	for (i = 0; i < mpcCircuit->NumTraces(); i++)
	{
		pcTrace = mpcCircuit->GetTrace(i);
		if (pcTrace->Contains(sPoint) && (papcTraces->Find(pcTrace) == ARRAY_ELEMENT_NOT_FOUND))
		{
			return true;
		}
	}
	for (i = 0; i < mpcCircuit->NumComponents(); i++)
	{
		pcComponent = mpcCircuit->GetComponent(i);
		if (papcComponents->Find(pcComponent) == ARRAY_ELEMENT_NOT_FOUND)
		{
			for (j = 0; j < pcComponent->NumPortViews(); j++)
			{
				if (pcComponent->GetPortView(j)->GetGridPosition() == sPoint)
				{
					return true;
				}
			}
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The new wire joining the anchor's end, where it's been moved to, back to
// where it was: first square to its wire, then along it, so a wire moved
// sideways stretches straight back.  None if it hasn't moved.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::GetAnchorLines(SMoveAnchor* psAnchor, CArrayTraceLine* pasLines)
{
	SInt32Vec2	sMoved;
	SInt32Vec2	sCorner;
	STraceLine	sWire;

	sMoved = psAnchor->bStart ? psAnchor->pcTrace->GetStart() : psAnchor->pcTrace->GetEnd();
	if (sMoved == psAnchor->sOriginal)
	{
		return;
	}
	sWire.Init(psAnchor->pcTrace->GetStart(), psAnchor->pcTrace->GetEnd());
	if (sWire.IsHorizontal())
	{
		sCorner.Init(sMoved.x, psAnchor->sOriginal.y);
	}
	else
	{
		sCorner.Init(psAnchor->sOriginal.x, sMoved.y);
	}
	if (sCorner != sMoved)
	{
		pasLines->Add()->Init(sMoved, sCorner);
	}
	if (sCorner != psAnchor->sOriginal)
	{
		pasLines->Add()->Init(sCorner, psAnchor->sOriginal);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's doneMoveComponents: new components are made and started
// in the simulations.  If anything changed the leads' routes are kept, the
// moved wires, and the wires at the moved components' ports, are merged
// and split again with the wires they touch, and the simulations are
// rewired, which joins detached views to what they're on.  If nothing
// changed the leads are put back and detached views joined again where
// they were.  The views are selected if bSelect: the components and the
// wires along where the moved wires went.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::DoneMoveViews(CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcTraces, bool bSelect, bool bNew, bool bChanged)
{
	size				i;
	CArrayTraceLine		asMovedLines;
	CArrayTraceLine		asAnchorLines;
	CArrayTraceViewPtr	apcSeeds;
	CArrayTraceViewPtr	apcAdded;

	ClearHover();
	if (bNew)
	{
		CreateComponents(papcComponents);

		//A unit of a part placed as its units is followed by the part's next unit (see ButtonReleased).
		if (papcComponents->NumElements() == 1)
		{
			mpcNextUnit = papcComponents->GetPtr(0)->NewNextUnit();
		}
	}

	asAnchorLines.Init();
	if (bChanged)
	{
		for (i = 0; i < masAnchors.NumElements(); i++)
		{
			GetAnchorLines(masAnchors.Get(i), &asAnchorLines);
		}
	}
	masAnchors.ReInit();

	asMovedLines.Init();
	GetMovedLines(papcTraces, &asMovedLines);
	mapcFollowing.ReInit();

	apcSeeds.Init();
	if (bChanged)
	{
		FinishLeads(&apcSeeds);
	}
	else
	{
		RestoreLeads();
	}
	KillLeads();

	apcAdded.Init();
	if (bNew || bChanged)
	{
		for (i = 0; i < papcTraces->NumElements(); i++)
		{
			apcSeeds.Add(papcTraces->GetPtr(i));
		}
		AddPortTraces(papcComponents, &apcSeeds);
		mpcCircuit->ReplaceWires(&asAnchorLines, &apcSeeds, &apcAdded);
		Rewire();
	}
	else
	{
		for (i = 0; i < papcTraces->NumElements(); i++)
		{
			apcAdded.Add(papcTraces->GetPtr(i));
		}
		if (mbDetachedMove)
		{
			Rewire();
		}
	}
	mbDetachedMove = false;
	apcSeeds.Kill();

	mcSelection.Clear();
	if (bSelect)
	{
		SelectMoved(papcComponents, &apcAdded, &asMovedLines);
	}
	apcAdded.Kill();
	asMovedLines.Kill();
	asAnchorLines.Kill();

	if (bNew || bChanged)
	{
		PushUndo(false);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The lines of the wires moved (not those following a component).
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::GetMovedLines(CArrayTraceViewPtr* papcTraces, CArrayTraceLine* pasMovedLines)
{
	size			i;
	CTraceView*		pcTrace;
	STraceLine*		psLine;

	for (i = 0; i < papcTraces->NumElements(); i++)
	{
		pcTrace = papcTraces->GetPtr(i);
		if (mapcFollowing.Find(pcTrace) == ARRAY_ELEMENT_NOT_FOUND)
		{
			psLine = pasMovedLines->Add();
			psLine->Init(pcTrace->GetStart(), pcTrace->GetEnd());
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Adds the wires through the components' ports to the seeds, once each.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::AddPortTraces(CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcSeeds)
{
	size				i;
	size				j;
	size				k;
	CComponentView*		pcComponent;
	CTraceView*			pcTrace;
	SInt32Vec2			sPort;

	for (i = 0; i < papcComponents->NumElements(); i++)
	{
		pcComponent = papcComponents->GetPtr(i);
		for (j = 0; j < pcComponent->NumPortViews(); j++)
		{
			sPort = pcComponent->GetPortView(j)->GetGridPosition();
			for (k = 0; k < mpcCircuit->NumTraces(); k++)
			{
				pcTrace = mpcCircuit->GetTrace(k);
				if (pcTrace->Contains(sPort) && (papcSeeds->Find(pcTrace) == ARRAY_ELEMENT_NOT_FOUND))
				{
					papcSeeds->Add(pcTrace);
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Selects the moved components and the wires added along where the moved
// wires went.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::SelectMoved(CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcAdded, CArrayTraceLine* pasMovedLines)
{
	size			i;
	size			j;
	CTraceView*		pcTrace;
	STraceLine		sAdded;

	for (i = 0; i < papcComponents->NumElements(); i++)
	{
		mcSelection.Add(papcComponents->GetPtr(i));
	}
	for (i = 0; i < papcAdded->NumElements(); i++)
	{
		pcTrace = papcAdded->GetPtr(i);
		sAdded.Init(pcTrace->GetStart(), pcTrace->GetEnd());
		for (j = 0; j < pasMovedLines->NumElements(); j++)
		{
			if (sAdded.Overlaps(pasMovedLines->Get(j), false))
			{
				mcSelection.Add(pcTrace);
				break;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Adds the pulled wire to the circuit, merged and split with the wires it
// touches, and rewires the simulations.  A wire pulled from one
// component's port to another's is routed around the components instead
// (see CTraceAutoConnector), unless no route can be found.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::PullWire(CWirePullEdit* pcPull)
{
	CArrayTraceLine		asLines;

	ClearHover();
	mcSelection.Clear();
	if (!RouteWire(pcPull->GetFirst(), pcPull->GetEnd()))
	{
		asLines.Init();
		pcPull->GetLines(&asLines);
		mpcCircuit->ReplaceWires(&asLines, NULL, NULL);
		asLines.Kill();
	}
	Rewire();
	PushUndo(false);
}


//////////////////////////////////////////////////////////////////////////
//
// A port of a component that isn't moving on the wires joined to those
// through the grid point, not counting the excluded wires.  NULL if there
// is none.
//
//////////////////////////////////////////////////////////////////////////
CPortView* CCircuitEditor::FindNetPort(SInt32Vec2 sPoint, CArrayTraceViewPtr* papcExcluded, CArrayComponentViewPtr* papcComponents)
{
	CArrayTraceViewPtr	apcNet;
	CArrayPortViewPtr	apcPortViews;
	CTraceView*			pcTrace;
	CPortView*			pcPortView;
	CPortView*			pcFound;
	size				i;
	size				j;

	apcNet.Init();
	for (i = 0; i < mpcCircuit->NumTraces(); i++)
	{
		pcTrace = mpcCircuit->GetTrace(i);
		if (pcTrace->Contains(sPoint) && (papcExcluded->Find(pcTrace) == ARRAY_ELEMENT_NOT_FOUND))
		{
			apcNet.Add(pcTrace);
		}
	}
	for (i = 0; i < apcNet.NumElements(); i++)
	{
		for (j = 0; j < mpcCircuit->NumTraces(); j++)
		{
			pcTrace = mpcCircuit->GetTrace(j);
			if (pcTrace->Touches(apcNet.GetPtr(i)) && (papcExcluded->Find(pcTrace) == ARRAY_ELEMENT_NOT_FOUND) && (apcNet.Find(pcTrace) == ARRAY_ELEMENT_NOT_FOUND))
			{
				apcNet.Add(pcTrace);
			}
		}
	}

	pcFound = NULL;
	apcPortViews.Init();
	mpcCircuit->CollectPortViews(&apcPortViews);
	for (i = 0; (i < apcPortViews.NumElements()) && (pcFound == NULL); i++)
	{
		pcPortView = apcPortViews.GetPtr(i);
		if (papcComponents->Find(pcPortView->GetOwner()) == ARRAY_ELEMENT_NOT_FOUND)
		{
			for (j = 0; j < apcNet.NumElements(); j++)
			{
				if (apcNet.GetPtr(j)->Contains(pcPortView->GetGridPosition()))
				{
					pcFound = pcPortView;
					break;
				}
			}
		}
	}
	apcPortViews.Kill();
	apcNet.Kill();
	return pcFound;
}


//////////////////////////////////////////////////////////////////////////
//
// Takes the leads' wires out of the circuit, remembering where each was
// in its wires so they can be put back as they were (see RestoreLeads).
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::LiftLeads(void)
{
	size			i;
	size			j;
	size			k;
	SLiftedLead		sLifted;

	for (i = 0; i < masLeads.NumElements(); i++)
	{
		for (j = 0; j < masLeads.Get(i)->apcTraces.NumElements(); j++)
		{
			sLifted.pcTrace = masLeads.Get(i)->apcTraces.GetPtr(j);
			sLifted.uiIndex = mpcCircuit->FindTraceIndex(sLifted.pcTrace);
			if ((sLifted.uiIndex != ARRAY_ELEMENT_NOT_FOUND) && !IsLiftedLead(sLifted.pcTrace))
			{
				for (k = 0; (k < masLiftedLeads.NumElements()) && (masLiftedLeads.Get(k)->uiIndex < sLifted.uiIndex); k++)
				{
				}
				masLiftedLeads.InsertAt(&sLifted, k);
			}
		}
	}
	for (i = 0; i < masLiftedLeads.NumElements(); i++)
	{
		mpcCircuit->RemoveTrace(masLiftedLeads.Get(i)->pcTrace);
	}
	mbLeadsLifted = true;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the wire is one of the leads' taken out (see LiftLeads).
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::IsLiftedLead(CTraceView* pcTrace)
{
	size	i;

	for (i = 0; i < masLiftedLeads.NumElements(); i++)
	{
		if (masLiftedLeads.Get(i)->pcTrace == pcTrace)
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Each lead with a port to go to added to the router, those to the same
// net (pcNetPort) in one of its nets.  pauiConnections gets each lead's
// connection in the router, ARRAY_ELEMENT_NOT_FOUND for a lead that isn't
// routed.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::AddLeadConnections(CTraceAutoConnector* pcConnector, CArrayTemplatePrimitive<size>* pauiConnections)
{
	size			i;
	SMoveLead*		psLead;

	for (i = 0; i < masLeads.NumElements(); i++)
	{
		psLead = masLeads.Get(i);
		if (psLead->pcTargetPort == NULL)
		{
			pauiConnections->Add(ARRAY_ELEMENT_NOT_FOUND);
		}
		else
		{
			pauiConnections->Add(pcConnector->AddConnection(pcConnector->FindOrAddNet(psLead->pcNetPort), psLead->pcPort, psLead->pcTargetPort, TRACE_AUTO_CONNECT_NO_BEND));
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The lead wired straight from its port to where it went: along and then
// across.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::AddStraightLead(SMoveLead* psLead)
{
	SInt32Vec2	sPort;

	sPort = psLead->pcPort->GetGridPosition();
	if (sPort.x != psLead->sTarget.x)
	{
		mpcCircuit->AddTrace(sPort.x, sPort.y, psLead->sTarget.x, sPort.y);
	}
	if (sPort.y != psLead->sTarget.y)
	{
		mpcCircuit->AddTrace(psLead->sTarget.x, sPort.y, psLead->sTarget.x, psLead->sTarget.y);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Routes each moved port again to where its lead went, where the ports
// are now, or wires it there straight (see AddStraightLead) if it can't
// be routed.  Leads that end on the same net (pcNetPort) are routed as one
// net, as a port can only be on one.  The routes replace those of the
// last time and are left in the circuit, not merged, split or simulated,
// so they're drawn as disconnected wires.  The leads' wires are taken out
// first.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::RouteLeads(void)
{
	size							i;
	size							uiFirstRoute;
	CArrayTemplatePrimitive<size>	auiConnections;
	CTraceAutoConnector				cConnector;

	if (masLeads.NumElements() == 0)
	{
		return;
	}
	if (!mbLeadsLifted)
	{
		LiftLeads();
	}
	KillLeadRoutes();

	//The router and AddTrace only add traces, after those already in the circuit.
	uiFirstRoute = mpcCircuit->NumTraces();

	auiConnections.Init();
	cConnector.Init(mpcCircuit);
	AddLeadConnections(&cConnector, &auiConnections);
	cConnector.Connect();
	for (i = 0; i < masLeads.NumElements(); i++)
	{
		if (!cConnector.IsConnected(auiConnections.GetValue(i)))
		{
			AddStraightLead(masLeads.Get(i));
		}
	}
	cConnector.Kill();
	auiConnections.Kill();
	mpcCircuit->CollectTracesFrom(uiFirstRoute, &mapcLeadRoutes);
}


//////////////////////////////////////////////////////////////////////////
//
// Takes the leads' routes out of the circuit and frees them.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::KillLeadRoutes(void)
{
	size			i;
	CTraceView*		pcTrace;

	for (i = 0; i < mapcLeadRoutes.NumElements(); i++)
	{
		pcTrace = mapcLeadRoutes.GetPtr(i);
		mpcCircuit->RemoveTrace(pcTrace);
		SafeKill(pcTrace);
	}
	mapcLeadRoutes.ReInit();
}


//////////////////////////////////////////////////////////////////////////
//
// The leads' routes are taken out and their wires put back where they
// were, for a move that ends where it started.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::RestoreLeads(void)
{
	size			i;
	CTraceView*		pcTrace;

	KillLeadRoutes();
	for (i = 0; i < masLiftedLeads.NumElements(); i++)
	{
		pcTrace = masLiftedLeads.Get(i)->pcTrace;
		mpcCircuit->InsertTrace(pcTrace, masLiftedLeads.Get(i)->uiIndex);
	}
	masLiftedLeads.ReInit();
	mbLeadsLifted = false;
}


//////////////////////////////////////////////////////////////////////////
//
// The leads' routes, where the moved ports ended up, are kept and their
// old wires freed.  The routes, and the wires at the leads' ends, are
// added to papcSeeds to be merged and split.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::FinishLeads(CArrayTraceViewPtr* papcSeeds)
{
	size			i;
	size			j;
	CTraceView*		pcTrace;

	if (masLeads.NumElements() == 0)
	{
		return;
	}
	RouteLeads();
	for (i = 0; i < masLiftedLeads.NumElements(); i++)
	{
		pcTrace = masLiftedLeads.Get(i)->pcTrace;
		mcSelection.Remove(pcTrace);
		SafeKill(pcTrace);
	}
	masLiftedLeads.ReInit();
	mbLeadsLifted = false;

	for (i = 0; i < mapcLeadRoutes.NumElements(); i++)
	{
		papcSeeds->Add(mapcLeadRoutes.GetPtr(i));
	}
	mapcLeadRoutes.ReInit();
	for (i = 0; i < mpcCircuit->NumTraces(); i++)
	{
		pcTrace = mpcCircuit->GetTrace(i);
		if (papcSeeds->Find(pcTrace) == ARRAY_ELEMENT_NOT_FOUND)
		{
			for (j = 0; j < masLeads.NumElements(); j++)
			{
				if (pcTrace->Contains(masLeads.Get(j)->sTarget))
				{
					papcSeeds->Add(pcTrace);
					break;
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::KillLeads(void)
{
	size			i;
	CTraceView*		pcTrace;

	for (i = 0; i < masLeads.NumElements(); i++)
	{
		masLeads.Get(i)->apcTraces.Kill();
	}
	masLeads.ReInit();
	for (i = 0; i < masLiftedLeads.NumElements(); i++)
	{
		pcTrace = masLiftedLeads.Get(i)->pcTrace;
		SafeKill(pcTrace);
	}
	masLiftedLeads.ReInit();
	mapcLeadRoutes.ReInit();
	mbLeadsLifted = false;
}


//////////////////////////////////////////////////////////////////////////
//
// Routes a wire from the port on sFrom to the port on sTo, if they're
// ports of different components, and merges and splits the new wires with
// those they touch.  Fails, adding nothing, if they aren't or if there's
// no route.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::RouteWire(SInt32Vec2 sFrom, SInt32Vec2 sTo)
{
	CPortView*			pcFrom;
	CPortView*			pcTo;
	CTraceAutoConnector	cConnector;
	CArrayTraceViewPtr	apcAdded;
	CArrayTraceLine		asNoLines;
	size				uiFirstRoute;
	size				uiConnection;
	bool				bConnected;

	pcFrom = mpcCircuit->GetPortViewAt(sFrom);
	pcTo = mpcCircuit->GetPortViewAt(sTo);
	if ((pcFrom == NULL) || (pcTo == NULL) || (pcFrom->GetOwner() == pcTo->GetOwner()))
	{
		return false;
	}

	uiFirstRoute = mpcCircuit->NumTraces();
	cConnector.Init(mpcCircuit);
	uiConnection = cConnector.AddConnection(cConnector.AddNet(), pcFrom, pcTo, TRACE_AUTO_CONNECT_NO_BEND);
	cConnector.Connect();
	bConnected = cConnector.IsConnected(uiConnection);
	cConnector.Kill();

	apcAdded.Init();
	mpcCircuit->CollectTracesFrom(uiFirstRoute, &apcAdded);

	if (apcAdded.NumElements() > 0)
	{
		asNoLines.Init();
		mpcCircuit->ReplaceWires(&asNoLines, &apcAdded, NULL);
		asNoLines.Kill();
	}
	apcAdded.Kill();
	return bConnected;
}


//////////////////////////////////////////////////////////////////////////
//
// The wires with an end on the grid point.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::GetTracesEndingAt(SInt32Vec2 sPoint, CArrayTraceViewPtr* papcTraces)
{
	size			i;
	CTraceView*		pcTrace;

	for (i = 0; i < mpcCircuit->NumTraces(); i++)
	{
		pcTrace = mpcCircuit->GetTrace(i);
		if (pcTrace->IsEnd(sPoint))
		{
			papcTraces->Add(pcTrace);
		}
	}
}
