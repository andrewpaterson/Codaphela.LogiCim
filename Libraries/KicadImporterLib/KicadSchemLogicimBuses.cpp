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
#include "BaseLib/StringHelper.h"
#include "BaseLib/PointerFunctions.h"
#include "LogicimLib/TunnelView.h"
#include "LogicimLib/SplitterView.h"
#include "LogicimLib/TraceAutoConnector.h"
#include "KicadSchemLogicimBuses.h"


//////////////////////////////////////////////////////////////////////////
//
// Adds to pcModel's circuit, counting what it adds in psCounts.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::Init(CKicadSchemNetlist* pcNetlist, CCircuitFonts* pcFonts, CKicadSchemLogicimModel* pcModel, SKicadSchemLogicimCounts* psCounts)
{
	mpcNetlist = pcNetlist;
	mpcFonts = pcFonts;
	mpcModel = pcModel;
	mpsCounts = psCounts;
	masSplitters.Init();
	masSplitterPairs.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::Kill(void)
{
	masSplitterPairs.Kill();
	masSplitters.Kill();
	mpsCounts = NULL;
	mpcModel = NULL;
	mpcFonts = NULL;
	mpcNetlist = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::AddBuses(void)
{
	size	i;

	for (i = 0; i < mpcNetlist->NumBuses(); i++)
	{
		AddBus(mpcNetlist->GetBus(i));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The way from a tap's bus point to its wire point, across the bus (a
// TRACE_DIRECTION): East or West for a bus going up and down, South or
// North for one going across.
//
//////////////////////////////////////////////////////////////////////////
static int GetKicadSchemTapSide(SKicadSchemBusTap* psTap)
{
	if (psTap->bVertical)
	{
		return (psTap->sWirePoint.x >= psTap->sBusPoint.x) ? TRACE_DIRECTION_EAST : TRACE_DIRECTION_WEST;
	}
	return (psTap->sWirePoint.y >= psTap->sBusPoint.y) ? TRACE_DIRECTION_SOUTH : TRACE_DIRECTION_NORTH;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static float64 GetKicadSchemTapAlong(SKicadSchemBusTap* psTap)
{
	return psTap->bVertical ? psTap->sBusPoint.y : psTap->sBusPoint.x;
}


//////////////////////////////////////////////////////////////////////////
//
// Where the bus is across the way it runs at the tap.
//
//////////////////////////////////////////////////////////////////////////
static float64 GetKicadSchemTapAcross(SKicadSchemBusTap* psTap)
{
	return psTap->bVertical ? psTap->sBusPoint.x : psTap->sBusPoint.y;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the tap goes before the other: by side, then where the bus is
// across, then along it.
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemTapBefore(SKicadSchemBusTap* psTap, SKicadSchemBusTap* psOther)
{
	if (psTap->bVertical != psOther->bVertical)
	{
		return psTap->bVertical;
	}
	if (GetKicadSchemTapSide(psTap) != GetKicadSchemTapSide(psOther))
	{
		return GetKicadSchemTapSide(psTap) < GetKicadSchemTapSide(psOther);
	}
	if (fabs(GetKicadSchemTapAcross(psTap) - GetKicadSchemTapAcross(psOther)) > KICAD_SCHEM_POINT_TOLERANCE)
	{
		return GetKicadSchemTapAcross(psTap) < GetKicadSchemTapAcross(psOther);
	}
	return GetKicadSchemTapAlong(psTap) < GetKicadSchemTapAlong(psOther);
}


//Orders a bus's taps, by index, as IsKicadSchemTapBefore does.
struct SKicadSchemTapOrder
{
	CKicadSchemBus*	pcBus;

	bool	IsBefore(int iTap, int iOther);
};


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool SKicadSchemTapOrder::IsBefore(int iTap, int iOther)
{
	return IsKicadSchemTapBefore(pcBus->GetTap(iTap), pcBus->GetTap(iOther));
}


//////////////////////////////////////////////////////////////////////////
//
// The bus's taps of its members, sorted (see IsKicadSchemTapBefore).
//
//////////////////////////////////////////////////////////////////////////
static void SortKicadSchemBusTaps(CKicadSchemBus* pcBus, CArrayInt* paiTaps)
{
	size					i;
	SKicadSchemBusTap*		psTap;
	SKicadSchemTapOrder		sOrder;

	sOrder.pcBus = pcBus;
	for (i = 0; i < pcBus->NumTaps(); i++)
	{
		psTap = pcBus->GetTap(i);
		if (pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName()) != ARRAY_ELEMENT_NOT_FOUND)
		{
			paiTaps->InsertInOrder((int)i, &sOrder);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A splitter for each run of the bus's taps on one side of it, and a tunnel at each of
// its labels if its name is on another bus too.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::AddBus(CKicadSchemBus* pcBus)
{
	CArrayInt	aiTaps;

	if (pcBus->NumMembers() == 0)
	{
		return;
	}

	AddBusTunnels(pcBus);
	aiTaps.Init();
	SortKicadSchemBusTaps(pcBus, &aiTaps);
	AddPinSplitters(pcBus, &aiTaps);
	AddRunSplitters(pcBus, &aiTaps);
	aiTaps.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// A tunnel at each of the bus's labels if its name is on another bus too.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::AddBusTunnels(CKicadSchemBus* pcBus)
{
	size				i;
	size				uiNamed;
	CKicadSchemLabel*	pcLabel;
	CTunnelView*		pcTunnel;
	size				uiItem;

	uiNamed = 0;
	if (pcBus->GetName() != NULL)
	{
		for (i = 0; i < mpcNetlist->NumBuses(); i++)
		{
			if ((mpcNetlist->GetBus(i)->GetName() != NULL) && (StringCompare(mpcNetlist->GetBus(i)->GetName(), pcBus->GetName()) == 0))
			{
				uiNamed++;
			}
		}
	}
	if (uiNamed <= 1)
	{
		return;
	}
	for (i = 0; i < pcBus->NumLabels(); i++)
	{
		pcLabel = pcBus->GetLabel(i);
		pcTunnel = NewMalloc<CTunnelView>();
		pcTunnel->Init(0, 0, GetKicadSchemRotation(ROT_East, pcLabel->GetAngle()), pcBus->GetName(), false, mpcFonts);
		uiItem = mpcModel->AddItem(pcTunnel, KSLIK_Tunnel, pcLabel->GetPosition(), pcTunnel->GetPortView((size)0), NULL);
		mpcModel->AddTerminal(pcBus->GetSubnet(), pcLabel->GetPosition(), pcTunnel->GetPortView((size)0), uiItem, true);
		mpsCounts->uiTunnels++;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The sorted taps wired to a component's pins: one splitter for each
// component and way its pins face (one end for each bit, so a member
// twice starts another), beside it.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::AddPinSplitters(CKicadSchemBus* pcBus, CArrayInt* paiTaps)
{
	CArrayTemplatePtr<CPortView>	apcGrouped;
	CArrayInt						aiRun;
	CArrayInt						aiUsed;
	size							i;
	size							j;
	size							uiMember;
	CPortView*						pcPin;
	CPortView*						pcOtherPin;
	SKicadSchemBusTap*				psTap;
	SKicadSchemBusTap*				psFirst;

	apcGrouped.Init();
	for (i = 0; i < paiTaps->NumElements(); i++)
	{
		apcGrouped.Add(GetTapPin(pcBus->GetTap(paiTaps->GetValue(i))));
	}
	aiRun.Init();
	aiUsed.Init();
	for (i = 0; i < paiTaps->NumElements(); i++)
	{
		pcPin = apcGrouped.GetPtr(i);
		if (pcPin == NULL)
		{
			continue;
		}
		aiRun.ReInit();
		aiUsed.ReInit();
		for (j = i; j < paiTaps->NumElements(); j++)
		{
			pcOtherPin = apcGrouped.GetPtr(j);
			if ((pcOtherPin == NULL) || (pcOtherPin->GetOwner() != pcPin->GetOwner()) || (CTraceAutoConnector::GetPortFacing(pcOtherPin) != CTraceAutoConnector::GetPortFacing(pcPin)))
			{
				continue;
			}
			psTap = pcBus->GetTap(paiTaps->GetValue(j));
			uiMember = pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName());
			if (aiUsed.Find((int)uiMember) != ARRAY_ELEMENT_NOT_FOUND)
			{
				continue;
			}
			aiRun.Add(paiTaps->GetValue(j));
			aiUsed.Add((int)uiMember);
			apcGrouped.SetPtr(j, NULL);
		}
		psFirst = pcBus->GetTap(aiRun.GetValue(0));
		AddBusSplitter(pcBus, &aiRun, GetKicadSchemTapSide(psFirst));
	}
	aiUsed.Kill();
	aiRun.Kill();
	apcGrouped.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The sorted taps wired to no component's pin, in runs: the same side of
// the same piece of the bus, each tap no more than two entries' spacing
// from the last, and no member twice.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::AddRunSplitters(CKicadSchemBus* pcBus, CArrayInt* paiTaps)
{
	CArrayInt			aiRest;
	CArrayInt			aiRun;
	CArrayInt			aiUsed;
	size				i;
	SKicadSchemBusTap*	psTap;
	SKicadSchemBusTap*	psLast;

	aiRest.Init();
	for (i = 0; i < paiTaps->NumElements(); i++)
	{
		if (GetTapPin(pcBus->GetTap(paiTaps->GetValue(i))) == NULL)
		{
			aiRest.Add(paiTaps->GetValue(i));
		}
	}
	aiRun.Init();
	aiUsed.Init();
	for (i = 0; i < aiRest.NumElements(); i++)
	{
		psTap = pcBus->GetTap(aiRest.GetValue(i));
		if (aiRun.NumElements() > 0)
		{
			psLast = pcBus->GetTap(aiRun.GetValue(aiRun.NumElements() - 1));
			if ((psTap->bVertical != psLast->bVertical) || (GetKicadSchemTapSide(psTap) != GetKicadSchemTapSide(psLast)) ||
				(fabs(GetKicadSchemTapAcross(psTap) - GetKicadSchemTapAcross(psLast)) > KICAD_SCHEM_POINT_TOLERANCE) || (GetKicadSchemTapAlong(psTap) - GetKicadSchemTapAlong(psLast) > 2 * KICAD_SCHEM_LOGICIM_GRID + KICAD_SCHEM_POINT_TOLERANCE) ||
				(aiUsed.Find((int)pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName())) != ARRAY_ELEMENT_NOT_FOUND))
			{
				AddBusSplitter(pcBus, &aiRun, GetKicadSchemTapSide(psLast));
				aiRun.ReInit();
				aiUsed.ReInit();
			}
		}
		aiRun.Add(aiRest.GetValue(i));
		aiUsed.Add((int)pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName()));
	}
	if (aiRun.NumElements() > 0)
	{
		psLast = pcBus->GetTap(aiRun.GetValue(aiRun.NumElements() - 1));
		AddBusSplitter(pcBus, &aiRun, GetKicadSchemTapSide(psLast));
	}
	aiUsed.Kill();
	aiRun.Kill();
	aiRest.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The first component pin the tap's member is wired to (not a tunnel's,
// supply's or splitter's), NULL if none.
//
//////////////////////////////////////////////////////////////////////////
CPortView* CKicadSchemLogicimBuses::GetTapPin(SKicadSchemBusTap* psTap)
{
	return GetComponentPin(psTap->pcMember, NULL, TRACE_DIRECTION_NONE);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the terminal is a component's pin: not a bus's, a tunnel's,
// supply's or splitter's.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimBuses::IsComponentPin(SKicadSchemLogicimTerminal* psTerminal)
{
	SKicadSchemLogicimItem*		psItem;

	if (psTerminal->bBus)
	{
		return false;
	}
	psItem = mpcModel->GetItem(psTerminal->uiItem);
	return (psItem->pcPort == NULL) && (psItem->eKind != KSLIK_Splitter);
}


//////////////////////////////////////////////////////////////////////////
//
// The first component pin (see IsComponentPin) the subnet is wired to, on
// pcOwner and facing iFacing (a TRACE_DIRECTION) unless they're NULL and
// TRACE_DIRECTION_NONE.  NULL if none.
//
//////////////////////////////////////////////////////////////////////////
CPortView* CKicadSchemLogicimBuses::GetComponentPin(CKicadSchemSubnet* pcSubnet, CComponentView* pcOwner, int iFacing)
{
	size							i;
	SKicadSchemLogicimTerminal*		psTerminal;

	for (i = mpcModel->FirstSubnetTerminal(pcSubnet); i != ARRAY_ELEMENT_NOT_FOUND; i = mpcModel->NextSubnetTerminal(i))
	{
		psTerminal = mpcModel->GetTerminal(i);
		if (IsComponentPin(psTerminal) &&
			((pcOwner == NULL) || (psTerminal->pcPortView->GetOwner() == pcOwner)) &&
			((iFacing == TRACE_DIRECTION_NONE) || (CTraceAutoConnector::GetPortFacing(psTerminal->pcPortView) == iFacing)))
		{
			return psTerminal->pcPortView;
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether one of the subnet's terminals is a tunnel's.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimBuses::HasTunnel(CKicadSchemSubnet* pcSubnet)
{
	size							i;
	SKicadSchemLogicimTerminal*		psTerminal;

	for (i = mpcModel->FirstSubnetTerminal(pcSubnet); i != ARRAY_ELEMENT_NOT_FOUND; i = mpcModel->NextSubnetTerminal(i))
	{
		psTerminal = mpcModel->GetTerminal(i);
		if (mpcModel->GetItem(psTerminal->uiItem)->eKind == KSLIK_Tunnel)
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The component pins' terminals (see IsComponentPin) the run's taps'
// members are wired to, tap by tap.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::GetTapPinTerminals(CKicadSchemBus* pcBus, CArrayInt* paiTaps, CArrayTemplatePtr<SKicadSchemLogicimTerminal>* papsTerminals)
{
	size							i;
	size							j;
	SKicadSchemBusTap*				psTap;
	SKicadSchemLogicimTerminal*		psTerminal;

	for (i = 0; i < paiTaps->NumElements(); i++)
	{
		psTap = pcBus->GetTap(paiTaps->GetValue(i));
		for (j = mpcModel->FirstSubnetTerminal(psTap->pcMember); j != ARRAY_ELEMENT_NOT_FOUND; j = mpcModel->NextSubnetTerminal(j))
		{
			psTerminal = mpcModel->GetTerminal(j);
			if (IsComponentPin(psTerminal))
			{
				papsTerminals->Add(psTerminal);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The way most of the pins face, a TRACE_DIRECTION, TRACE_DIRECTION_NONE
// if there are none.
//
//////////////////////////////////////////////////////////////////////////
static int GetKicadSchemMostFacing(CArrayTemplatePtr<SKicadSchemLogicimTerminal>* papsTerminals)
{
	int		aiFacings[4] = { 0, 0, 0, 0 };
	int		iFacing;
	size	i;

	for (i = 0; i < papsTerminals->NumElements(); i++)
	{
		aiFacings[CTraceAutoConnector::GetPortFacing(papsTerminals->GetPtr(i)->pcPortView)]++;
	}
	iFacing = TRACE_DIRECTION_NONE;
	for (i = 0; i < 4; i++)
	{
		if ((aiFacings[i] > 0) && ((iFacing == TRACE_DIRECTION_NONE) || (aiFacings[i] > aiFacings[iFacing])))
		{
			iFacing = (int)i;
		}
	}
	return iFacing;
}


//////////////////////////////////////////////////////////////////////////
//
// The component most of the pins facing iFacing are on, the first of
// them if more than one has as many.  NULL if no pin faces that way.
//
//////////////////////////////////////////////////////////////////////////
CComponentView* CKicadSchemLogicimBuses::GetMostPinsOwner(CArrayTemplatePtr<SKicadSchemLogicimTerminal>* papsTerminals, int iFacing)
{
	size				i;
	size				j;
	size				uiPins;
	size				uiMost;
	CComponentView*		pcView;
	CComponentView*		pcMost;

	pcMost = NULL;
	uiMost = 0;
	for (i = 0; i < papsTerminals->NumElements(); i++)
	{
		if (CTraceAutoConnector::GetPortFacing(papsTerminals->GetPtr(i)->pcPortView) != iFacing)
		{
			continue;
		}
		pcView = mpcModel->GetItem(papsTerminals->GetPtr(i)->uiItem)->pcView;
		uiPins = 0;
		for (j = 0; j < papsTerminals->NumElements(); j++)
		{
			if ((CTraceAutoConnector::GetPortFacing(papsTerminals->GetPtr(j)->pcPortView) == iFacing) && (mpcModel->GetItem(papsTerminals->GetPtr(j)->uiItem)->pcView == pcView))
			{
				uiPins++;
			}
		}
		if (uiPins > uiMost)
		{
			uiMost = uiPins;
			pcMost = pcView;
		}
	}
	return pcMost;
}


//////////////////////////////////////////////////////////////////////////
//
// The component pins the run's taps' members are wired to, by tap (NULL
// for a tap wired to none), and the way most of them face, a
// TRACE_DIRECTION (TRACE_DIRECTION_NONE if none are).  Each tap's pin is
// the first facing that way on the component most of them are on.
//
//////////////////////////////////////////////////////////////////////////
int CKicadSchemLogicimBuses::GetBusPins(CKicadSchemBus* pcBus, CArrayInt* paiTaps, CArrayTemplatePtr<CPortView>* papcPins)
{
	CArrayTemplatePtr<SKicadSchemLogicimTerminal>	apsTerminals;
	size											i;
	int												iFacing;
	CComponentView*									pcOwner;

	apsTerminals.Init();
	GetTapPinTerminals(pcBus, paiTaps, &apsTerminals);
	iFacing = GetKicadSchemMostFacing(&apsTerminals);
	pcOwner = (iFacing != TRACE_DIRECTION_NONE) ? GetMostPinsOwner(&apsTerminals, iFacing) : NULL;
	apsTerminals.Kill();

	for (i = 0; i < paiTaps->NumElements(); i++)
	{
		papcPins->Add((pcOwner != NULL) ? GetComponentPin(pcBus->GetTap(paiTaps->GetValue(i))->pcMember, pcOwner, iFacing) : NULL);
	}
	return (pcOwner != NULL) ? iFacing : TRACE_DIRECTION_NONE;
}


//Orders a run's taps, by index, by where their pins are along the side (bAlongY when it's an East or West side), the
//taps wired to no pin after them.  If the pins don't face one way (bPinned) the taps keep their order.
struct SKicadSchemSplitterTapOrder
{
	CArrayTemplatePtr<CPortView>*	papcPins;
	bool							bPinned;
	bool							bAlongY;

	bool	IsBefore(int iTap, int iOther);
	int32	GetAlong(CPortView* pcPin);
};


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool SKicadSchemSplitterTapOrder::IsBefore(int iTap, int iOther)
{
	CPortView*	pcPin;
	CPortView*	pcOther;

	pcPin = papcPins->GetPtr(iTap);
	if (!bPinned || (pcPin == NULL))
	{
		return false;
	}
	pcOther = papcPins->GetPtr(iOther);
	return (pcOther == NULL) || (GetAlong(pcPin) < GetAlong(pcOther));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int32 SKicadSchemSplitterTapOrder::GetAlong(CPortView* pcPin)
{
	return bAlongY ? pcPin->GetGridPosition().y : pcPin->GetGridPosition().x;
}


//////////////////////////////////////////////////////////////////////////
//
// The order the splitter's ends go to the run's taps (indices into the
// taps): the order of the taps' pins along the side (bAlongY when it's an
// East or West side) if they're wired to pins (iPinFacing isn't
// TRACE_DIRECTION_NONE), with taps wired to none after them, otherwise the
// taps' order.
//
//////////////////////////////////////////////////////////////////////////
static void OrderKicadSchemSplitterTaps(CArrayTemplatePtr<CPortView>* papcPins, int iPinFacing, bool bAlongY, CArrayInt* paiOrder)
{
	size							i;
	SKicadSchemSplitterTapOrder		sOrder;

	sOrder.papcPins = papcPins;
	sOrder.bPinned = iPinFacing != TRACE_DIRECTION_NONE;
	sOrder.bAlongY = bAlongY;
	for (i = 0; i < papcPins->NumElements(); i++)
	{
		paiOrder->InsertInOrder((int)i, &sOrder);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// How far apart the splitter's ends are: as far as the pins they're lined
// up with (two grid units for an instance's pins, one for a 74 part's),
// at most KICAD_SCHEM_LOGICIM_SPLITTER_SPACING.
//
//////////////////////////////////////////////////////////////////////////
static int32 GetKicadSchemSplitterSpacing(CArrayTemplatePtr<CPortView>* papcPins, CArrayInt* paiOrder, int iPinFacing, bool bAlongY)
{
	size		i;
	int32		iSpacing;
	int32		iGap;
	CPortView*	pcPrevious;
	CPortView*	pcPin;

	iSpacing = KICAD_SCHEM_LOGICIM_SPLITTER_SPACING;
	for (i = 1; (i < paiOrder->NumElements()) && (iPinFacing != TRACE_DIRECTION_NONE); i++)
	{
		pcPrevious = papcPins->GetPtr(paiOrder->GetValue(i - 1));
		pcPin = papcPins->GetPtr(paiOrder->GetValue(i));
		if ((pcPrevious != NULL) && (pcPin != NULL))
		{
			iGap = bAlongY ? pcPin->GetGridPosition().y - pcPrevious->GetGridPosition().y : pcPin->GetGridPosition().x - pcPrevious->GetGridPosition().x;
			if ((iGap > 0) && (iGap < iSpacing))
			{
				iSpacing = iGap;
			}
		}
	}
	return iSpacing;
}


//Orders a splitter's ends, by index, by where they are along the bus.
struct SKicadSchemSplitterEndOrder
{
	CArrayInt*	paiAlongs;

	bool	IsBefore(int iFan, int iOther);
};


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool SKicadSchemSplitterEndOrder::IsBefore(int iFan, int iOther)
{
	return paiAlongs->GetValue(iFan) < paiAlongs->GetValue(iOther);
}


//////////////////////////////////////////////////////////////////////////
//
// The rotation that has a splitter's ends facing iSide (East if none
// does), and where its ends are along the bus (bAlongY: in y), paiAlongs.
// The ends' own order along it if none does.
//
//////////////////////////////////////////////////////////////////////////
static ERotation GetKicadSchemSplitterEndAlongs(int32 iBitWidth, int32 iFanOut, int32 iSpacing, int iSide, bool bAlongY, CArrayInt* paiAlongs)
{
	CSplitterView*	pcSplitter;
	CPortView*		pcEnd;
	ERotation		eRotation;
	int				iDirection;
	int32			iFan;

	for (iDirection = 0; iDirection < 4; iDirection++)
	{
		eRotation = DirectionToRotation(iDirection);
		pcSplitter = NewMalloc<CSplitterView>();
		pcSplitter->Init(0, 0, eRotation, "", iBitWidth, iFanOut, iSpacing, SA_LeftHanded, 0);
		pcEnd = pcSplitter->GetEndPortView(0);
		if ((pcEnd != NULL) && (CTraceAutoConnector::GetPortFacing(pcEnd) == iSide))
		{
			for (iFan = 0; iFan < iFanOut; iFan++)
			{
				pcEnd = pcSplitter->GetEndPortView(iFan);
				paiAlongs->Add((pcEnd == NULL) ? 0 : (bAlongY ? pcEnd->GetGridPosition().y : pcEnd->GetGridPosition().x));
			}
			SafeKill(pcSplitter);
			return eRotation;
		}
		SafeKill(pcSplitter);
	}
	for (iFan = 0; iFan < iFanOut; iFan++)
	{
		paiAlongs->Add(iFan);
	}
	return ROT_East;
}


//////////////////////////////////////////////////////////////////////////
//
// The rotation that has a splitter's ends facing iSide (East if none
// does), and its ends (fans) in order along the bus.
//
//////////////////////////////////////////////////////////////////////////
static ERotation OrderKicadSchemSplitterEnds(int32 iBitWidth, int32 iFanOut, int32 iSpacing, int iSide, bool bAlongY, CArrayInt* paiFans)
{
	ERotation					eRotation;
	CArrayInt					aiAlongs;
	int32						iFan;
	SKicadSchemSplitterEndOrder	sOrder;

	aiAlongs.Init();
	eRotation = GetKicadSchemSplitterEndAlongs(iBitWidth, iFanOut, iSpacing, iSide, bAlongY, &aiAlongs);
	sOrder.paiAlongs = &aiAlongs;
	for (iFan = 0; iFan < iFanOut; iFan++)
	{
		paiFans->InsertInOrder(iFan, &sOrder);
	}
	aiAlongs.Kill();
	return eRotation;
}


//////////////////////////////////////////////////////////////////////////
//
// The run's taps (paiTaps, indices into the bus's), with nothing found
// for them yet.
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemLogicimBusRun::Init(CKicadSchemBus* pcBus, CArrayInt* paiTaps)
{
	this->pcBus = pcBus;
	this->paiTaps = paiTaps;
	apcPins.Init();
	iPinFacing = TRACE_DIRECTION_NONE;
	aiOrder.Init();
	aiFans.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemLogicimBusRun::Kill(void)
{
	aiFans.Kill();
	aiOrder.Kill();
	apcPins.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size SKicadSchemLogicimBusRun::NumTaps(void)
{
	return paiTaps->NumElements();
}


//////////////////////////////////////////////////////////////////////////
//
// The run's uiIndex'th tap.
//
//////////////////////////////////////////////////////////////////////////
SKicadSchemBusTap* SKicadSchemLogicimBusRun::GetTap(size uiIndex)
{
	return pcBus->GetTap(paiTaps->GetValue(uiIndex));
}


//////////////////////////////////////////////////////////////////////////
//
// The end each of the bus's iBitWidth members goes to (the taps' in the
// run's order, the rest on the end after them), and where the splitter
// goes: the middle of the taps' points on the bus.
//
//////////////////////////////////////////////////////////////////////////
static void GetKicadSchemSplitIndices(SKicadSchemLogicimBusRun* psRun, int32 iBitWidth, int32 iFanOut, CArrayInt* paiSplitIndices, SFloat64Vec2* psCentre)
{
	size					i;
	int32					iRestFan;
	SKicadSchemBusTap*		psTap;
	SKicadSchemPointMean	sMean;

	iRestFan = ((size)iFanOut > psRun->NumTaps()) ? psRun->aiFans.GetValue(psRun->NumTaps()) : 0;
	for (i = 0; i < (size)iBitWidth; i++)
	{
		paiSplitIndices->Add(iRestFan);
	}
	sMean.Init();
	for (i = 0; i < psRun->NumTaps(); i++)
	{
		psTap = psRun->GetTap(psRun->aiOrder.GetValue(i));
		paiSplitIndices->SetValue(psRun->pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName()), psRun->aiFans.GetValue(i));
		sMean.Add(&psTap->sBusPoint);
	}
	sMean.Get(psCentre);
}


//////////////////////////////////////////////////////////////////////////
//
// A splitter of the whole bus for a run of taps (in order along the bus)
// on the side iSide of it, its ends KICAD_SCHEM_LOGICIM_SPLITTER_SPACING
// apart, or as close as the pins they line up with if those are closer.
// It faces the component pins its members are wired to (the way most of
// them face, see GetBusPins), its ends given to the taps in the order of
// their pins along that side, and is kept beside that component (see
// CKicadSchemLogicimPlacer::PlaceSplitters).  A run wired to no pins faces
// the members as KiCad draws them, its ends in the order of the taps along
// the bus.  The bus's other members go to an end of their own, after the
// taps'.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::AddBusSplitter(CKicadSchemBus* pcBus, CArrayInt* paiTaps, int iSide)
{
	SKicadSchemLogicimBusRun	sRun;
	ERotation					eRotation;
	CSplitterView*				pcSplitter;
	CArrayInt					aiSplitIndices;
	int32						iBitWidth;
	int32						iFanOut;
	int32						iSpacing;
	bool						bAlongY;
	size						uiItem;
	SFloat64Vec2				sCentre;

	iBitWidth = (int32)pcBus->NumMembers();
	iFanOut = (int32)paiTaps->NumElements() + (((size)iBitWidth > paiTaps->NumElements()) ? 1 : 0);

	//The pins it goes to decide the way its ends face, and the order its ends go to the taps.
	sRun.Init(pcBus, paiTaps);
	sRun.iPinFacing = GetBusPins(pcBus, paiTaps, &sRun.apcPins);
	if (sRun.iPinFacing != TRACE_DIRECTION_NONE)
	{
		iSide = OppositeDirection(sRun.iPinFacing);
	}
	bAlongY = IsDirectionHorizontal(iSide);
	OrderKicadSchemSplitterTaps(&sRun.apcPins, sRun.iPinFacing, bAlongY, &sRun.aiOrder);
	iSpacing = GetKicadSchemSplitterSpacing(&sRun.apcPins, &sRun.aiOrder, sRun.iPinFacing, bAlongY);
	eRotation = OrderKicadSchemSplitterEnds(iBitWidth, iFanOut, iSpacing, iSide, bAlongY, &sRun.aiFans);

	aiSplitIndices.Init();
	GetKicadSchemSplitIndices(&sRun, iBitWidth, iFanOut, &aiSplitIndices, &sCentre);
	pcSplitter = NewMalloc<CSplitterView>();
	pcSplitter->Init(0, 0, eRotation, "", iBitWidth, iFanOut, iSpacing, SA_LeftHanded, 0, aiSplitIndices.GetData());
	aiSplitIndices.Kill();
	uiItem = mpcModel->AddItem(pcSplitter, KSLIK_Splitter, &sCentre, NULL, NULL);
	AddBusSplitterTerminals(&sRun, pcSplitter, uiItem);
	sRun.Kill();
	mpsCounts->uiSplitters++;
}


//////////////////////////////////////////////////////////////////////////
//
// The splitter's start on the bus and its ends on the run's members, and
// its record (see SKicadSchemLogicimSplitter): each end lined up with the
// pin its tap's member is wired to.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::AddBusSplitterTerminals(SKicadSchemLogicimBusRun* psRun, CSplitterView* pcSplitter, size uiItem)
{
	SKicadSchemLogicimSplitter*		psSplitter;
	SKicadSchemLogicimSplitterPair*	psPair;
	SKicadSchemBusTap*				psTap;
	CPortView*						pcEnd;
	CPortView*						pcPin;
	size							i;
	size							uiTap;

	psTap = psRun->GetTap(0);
	mpcModel->AddTerminal(psRun->pcBus->GetSubnet(), &psTap->sBusPoint, pcSplitter->GetStartPortView(), uiItem, true);
	psSplitter = masSplitters.Add();
	psSplitter->pcSplitter = pcSplitter;
	psSplitter->iFacing = psRun->iPinFacing;
	psSplitter->iGap = KICAD_SCHEM_LOGICIM_SPLITTER_GAP;
	for (i = 0; i < psRun->NumTaps(); i++)
	{
		if (HasTunnel(psRun->GetTap(i)->pcMember))
		{
			psSplitter->iGap = KICAD_SCHEM_LOGICIM_SPLITTER_TUNNEL_GAP;
		}
	}
	psSplitter->uiFirstPair = masSplitterPairs.NumElements();
	psSplitter->uiNumPairs = 0;
	for (i = 0; i < psRun->NumTaps(); i++)
	{
		uiTap = psRun->aiOrder.GetValue(i);
		psTap = psRun->GetTap(uiTap);
		pcEnd = pcSplitter->GetEndPortView(psRun->aiFans.GetValue(i));
		pcPin = psRun->apcPins.GetPtr(uiTap);
		mpcModel->AddTerminal(psTap->pcMember, &psTap->sWirePoint, pcEnd, uiItem);
		if ((psRun->iPinFacing != TRACE_DIRECTION_NONE) && (pcPin != NULL) && (pcEnd != NULL))
		{
			psPair = masSplitterPairs.Add();
			psPair->pcPin = pcPin;
			psPair->pcEnd = pcEnd;
			psSplitter->uiNumPairs++;
			mpcModel->GetItem(uiItem)->bPlacedSplitter = true;
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A tunnel named as its net at each label of each subnet of a net that
// has more than one, unless they're all members of one bus, which joins
// them (see IsJoinedByBus).
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimBuses::AddTunnels(void)
{
	size				i;
	size				j;
	size				k;
	CKicadSchemNet*		pcNet;
	CKicadSchemSubnet*	pcSubnet;
	CKicadSchemLabel*	pcLabel;
	CTunnelView*		pcTunnel;
	size				uiItem;

	for (i = 0; i < mpcNetlist->NumNets(); i++)
	{
		pcNet = mpcNetlist->GetNet(i);
		if ((pcNet->NumSubnets() < 2) || IsJoinedByBus(pcNet))
		{
			continue;
		}
		for (j = 0; j < pcNet->NumSubnets(); j++)
		{
			pcSubnet = pcNet->GetSubnet(j);
			for (k = 0; k < pcSubnet->NumLabels(); k++)
			{
				pcLabel = pcSubnet->GetLabel(k);
				if (mpcNetlist->IsBusLabel(pcLabel))
				{
					continue;
				}
				pcTunnel = NewMalloc<CTunnelView>();
				pcTunnel->Init(0, 0, GetKicadSchemRotation(ROT_East, pcLabel->GetAngle()), pcNet->GetName(), false, mpcFonts);
				uiItem = mpcModel->AddItem(pcTunnel, KSLIK_Tunnel, pcLabel->GetPosition(), pcTunnel->GetPortView((size)0), NULL);
				mpcModel->AddTerminal(pcSubnet, pcLabel->GetPosition(), pcTunnel->GetPortView((size)0), uiItem);
				mpsCounts->uiTunnels++;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The bus the subnet is a member of (one of its taps' wires is the
// subnet), NULL if none.
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemBus* CKicadSchemLogicimBuses::GetMemberBus(CKicadSchemSubnet* pcSubnet)
{
	size				i;
	size				j;
	CKicadSchemBus*		pcBus;

	for (i = 0; i < mpcNetlist->NumBuses(); i++)
	{
		pcBus = mpcNetlist->GetBus(i);
		for (j = 0; j < pcBus->NumTaps(); j++)
		{
			if (pcBus->GetTap(j)->pcMember == pcSubnet)
			{
				return pcBus;
			}
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether every subnet of the net is a member of one bus (or of buses of
// the same name, which their tunnels join), so the splitters join them
// and its labels (in KiCad they name the bus's members) need no tunnels.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimBuses::IsJoinedByBus(CKicadSchemNet* pcNet)
{
	size				i;
	CKicadSchemBus*		pcFirst;
	CKicadSchemBus*		pcBus;

	pcFirst = NULL;
	for (i = 0; i < pcNet->NumSubnets(); i++)
	{
		pcBus = GetMemberBus(pcNet->GetSubnet(i));
		if (pcBus == NULL)
		{
			return false;
		}
		if (pcFirst == NULL)
		{
			pcFirst = pcBus;
		}
		else if ((pcBus != pcFirst) && ((pcBus->GetName() == NULL) || (pcFirst->GetName() == NULL) || (StringCompare(pcBus->GetName(), pcFirst->GetName()) != 0)))
		{
			return false;
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the port is a component's pin a splitter beside it is wired
// straight to.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimBuses::IsSplitterPin(CPortView* pcPortView)
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
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimBuses::NumSplitters(void) { return masSplitters.NumElements(); }
SKicadSchemLogicimSplitter* CKicadSchemLogicimBuses::GetSplitter(size uiIndex) { return masSplitters.SafeGet(uiIndex); }
size CKicadSchemLogicimBuses::NumSplitterPairs(void) { return masSplitterPairs.NumElements(); }
SKicadSchemLogicimSplitterPair* CKicadSchemLogicimBuses::GetSplitterPair(size uiIndex) { return masSplitterPairs.SafeGet(uiIndex); }

