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
void CKicadSchemLogicimConverter::AddBuses(void)
{
	size	i;

	for (i = 0; i < mpcNetlist->NumBuses(); i++)
	{
		AddBus(mpcNetlist->GetBus(i), i);
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
		return (psTap->sWirePoint.fX >= psTap->sBusPoint.fX) ? TRACE_DIRECTION_EAST : TRACE_DIRECTION_WEST;
	}
	return (psTap->sWirePoint.fY >= psTap->sBusPoint.fY) ? TRACE_DIRECTION_SOUTH : TRACE_DIRECTION_NORTH;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static float64 GetKicadSchemTapAlong(SKicadSchemBusTap* psTap)
{
	return psTap->bVertical ? psTap->sBusPoint.fY : psTap->sBusPoint.fX;
}


//////////////////////////////////////////////////////////////////////////
//
// Where the bus is across the way it runs at the tap.
//
//////////////////////////////////////////////////////////////////////////
static float64 GetKicadSchemTapAcross(SKicadSchemBusTap* psTap)
{
	return psTap->bVertical ? psTap->sBusPoint.fX : psTap->sBusPoint.fY;
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


//////////////////////////////////////////////////////////////////////////
//
// The bus's taps of its members, sorted (see IsKicadSchemTapBefore).
//
//////////////////////////////////////////////////////////////////////////
static void SortKicadSchemBusTaps(CKicadSchemBus* pcBus, CArrayInt* paiTaps)
{
	size				i;
	size				j;
	SKicadSchemBusTap*	psTap;

	for (i = 0; i < pcBus->NumTaps(); i++)
	{
		psTap = pcBus->GetTap(i);
		if (pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName()) == ARRAY_ELEMENT_NOT_FOUND)
		{
			continue;
		}
		for (j = 0; j < paiTaps->NumElements(); j++)
		{
			if (IsKicadSchemTapBefore(psTap, pcBus->GetTap(paiTaps->GetValue(j))))
			{
				break;
			}
		}
		paiTaps->InsertAt((int)i, j);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A splitter for each run of the bus's taps on one side of it, and a tunnel at each of
// its labels if its name is on another bus too.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddBus(CKicadSchemBus* pcBus, size uiBus)
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
void CKicadSchemLogicimConverter::AddBusTunnels(CKicadSchemBus* pcBus)
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
		pcTunnel->Init(0, 0, GetKicadRotation(ROT_East, pcLabel->GetAngle()), pcBus->GetName(), false, mpcFonts);
		uiItem = AddItem(pcTunnel, KSLIK_Tunnel, pcLabel->GetPosition(), pcTunnel->GetPortView((size)0));
		AddTerminal(pcBus->GetSubnet(), pcLabel->GetPosition(), pcTunnel->GetPortView((size)0), uiItem, true);
		muiTunnels++;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The sorted taps wired to a component's pins: one splitter for each
// component and way its pins face (one end for each bit, so a member
// twice starts another), beside it.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddPinSplitters(CKicadSchemBus* pcBus, CArrayInt* paiTaps)
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
		AddBusSplitter(pcBus, &aiRun, psFirst->bVertical, GetKicadSchemTapSide(psFirst));
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
void CKicadSchemLogicimConverter::AddRunSplitters(CKicadSchemBus* pcBus, CArrayInt* paiTaps)
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
				AddBusSplitter(pcBus, &aiRun, psLast->bVertical, GetKicadSchemTapSide(psLast));
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
		AddBusSplitter(pcBus, &aiRun, psLast->bVertical, GetKicadSchemTapSide(psLast));
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
CPortView* CKicadSchemLogicimConverter::GetTapPin(SKicadSchemBusTap* psTap)
{
	return GetComponentPin(psTap->pcMember, NULL, -1);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the terminal is a component's pin: not a bus's, a tunnel's,
// supply's or splitter's.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsComponentPin(SKicadSchemLogicimTerminal* psTerminal)
{
	SKicadSchemLogicimItem*		psItem;

	if (psTerminal->bBus)
	{
		return false;
	}
	psItem = masItems.Get(psTerminal->uiItem);
	return (psItem->pcPort == NULL) && (psItem->eKind != KSLIK_Splitter);
}


//////////////////////////////////////////////////////////////////////////
//
// The first component pin (see IsComponentPin) the subnet is wired to, on
// pcOwner and facing iFacing (a TRACE_DIRECTION) unless they're NULL and
// -1.  NULL if none.
//
//////////////////////////////////////////////////////////////////////////
CPortView* CKicadSchemLogicimConverter::GetComponentPin(CKicadSchemSubnet* pcSubnet, CComponentView* pcOwner, int iFacing)
{
	size							i;
	SKicadSchemLogicimTerminal*		psTerminal;

	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		psTerminal = masTerminals.Get(i);
		if ((psTerminal->pcSubnet == pcSubnet) && IsComponentPin(psTerminal) &&
			((pcOwner == NULL) || (psTerminal->pcPortView->GetOwner() == pcOwner)) &&
			((iFacing == -1) || (CTraceAutoConnector::GetPortFacing(psTerminal->pcPortView) == iFacing)))
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
bool CKicadSchemLogicimConverter::HasTunnel(CKicadSchemSubnet* pcSubnet)
{
	size							i;
	SKicadSchemLogicimTerminal*		psTerminal;

	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		psTerminal = masTerminals.Get(i);
		if ((psTerminal->pcSubnet == pcSubnet) && (masItems.Get(psTerminal->uiItem)->eKind == KSLIK_Tunnel))
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
void CKicadSchemLogicimConverter::GetTapPinTerminals(CKicadSchemBus* pcBus, CArrayInt* paiTaps, CArrayTemplatePtr<SKicadSchemLogicimTerminal>* papsTerminals)
{
	size							i;
	size							j;
	SKicadSchemBusTap*				psTap;
	SKicadSchemLogicimTerminal*		psTerminal;

	for (i = 0; i < paiTaps->NumElements(); i++)
	{
		psTap = pcBus->GetTap(paiTaps->GetValue(i));
		for (j = 0; j < masTerminals.NumElements(); j++)
		{
			psTerminal = masTerminals.Get(j);
			if ((psTerminal->pcSubnet == psTap->pcMember) && IsComponentPin(psTerminal))
			{
				papsTerminals->Add(psTerminal);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The way most of the pins face, a TRACE_DIRECTION, -1 if there are none.
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
	iFacing = -1;
	for (i = 0; i < 4; i++)
	{
		if ((aiFacings[i] > 0) && ((iFacing == -1) || (aiFacings[i] > aiFacings[iFacing])))
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
CComponentView* CKicadSchemLogicimConverter::GetMostPinsOwner(CArrayTemplatePtr<SKicadSchemLogicimTerminal>* papsTerminals, int iFacing)
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
		pcView = masItems.Get(papsTerminals->GetPtr(i)->uiItem)->pcView;
		uiPins = 0;
		for (j = 0; j < papsTerminals->NumElements(); j++)
		{
			if ((CTraceAutoConnector::GetPortFacing(papsTerminals->GetPtr(j)->pcPortView) == iFacing) && (masItems.Get(papsTerminals->GetPtr(j)->uiItem)->pcView == pcView))
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
// TRACE_DIRECTION (-1 if none are).  Each tap's pin is the first facing
// that way on the component most of them are on.
//
//////////////////////////////////////////////////////////////////////////
int CKicadSchemLogicimConverter::GetBusPins(CKicadSchemBus* pcBus, CArrayInt* paiTaps, CArrayTemplatePtr<CPortView>* papcPins)
{
	CArrayTemplatePtr<SKicadSchemLogicimTerminal>	apsTerminals;
	size											i;
	int												iFacing;
	CComponentView*									pcOwner;

	apsTerminals.Init();
	GetTapPinTerminals(pcBus, paiTaps, &apsTerminals);
	iFacing = GetKicadSchemMostFacing(&apsTerminals);
	pcOwner = (iFacing != -1) ? GetMostPinsOwner(&apsTerminals, iFacing) : NULL;
	apsTerminals.Kill();

	for (i = 0; i < paiTaps->NumElements(); i++)
	{
		papcPins->Add((pcOwner != NULL) ? GetComponentPin(pcBus->GetTap(paiTaps->GetValue(i))->pcMember, pcOwner, iFacing) : NULL);
	}
	return (pcOwner != NULL) ? iFacing : -1;
}


//////////////////////////////////////////////////////////////////////////
//
// The order the splitter's ends go to the run's taps (indices into the
// taps): the order of the taps' pins along the side (bAlongY when it's an
// East or West side) if they're wired to pins (iPinFacing isn't -1), with
// taps wired to none after them, otherwise the taps' order.
//
//////////////////////////////////////////////////////////////////////////
static void OrderKicadSchemSplitterTaps(CArrayTemplatePtr<CPortView>* papcPins, int iPinFacing, bool bAlongY, CArrayInt* paiOrder)
{
	size		i;
	size		j;
	int32		iKey;
	int32		iOtherKey;
	CPortView*	pcPin;
	CPortView*	pcOther;

	for (i = 0; i < papcPins->NumElements(); i++)
	{
		j = paiOrder->NumElements();
		pcPin = papcPins->GetPtr(i);
		if ((iPinFacing != -1) && (pcPin != NULL))
		{
			iKey = bAlongY ? pcPin->GetGridPosition().y : pcPin->GetGridPosition().x;
			for (j = 0; j < paiOrder->NumElements(); j++)
			{
				pcOther = papcPins->GetPtr(paiOrder->GetValue(j));
				if (pcOther == NULL)
				{
					break;
				}
				iOtherKey = bAlongY ? pcOther->GetGridPosition().y : pcOther->GetGridPosition().x;
				if (iKey < iOtherKey)
				{
					break;
				}
			}
		}
		paiOrder->InsertAt((int)i, j);
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
	for (i = 1; (i < paiOrder->NumElements()) && (iPinFacing != -1); i++)
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


//////////////////////////////////////////////////////////////////////////
//
// The rotation that has a splitter's ends facing iSide (East if none
// does), and its ends (fans) in order along the bus.
//
//////////////////////////////////////////////////////////////////////////
static ERotation OrderKicadSchemSplitterEnds(int32 iBitWidth, int32 iFanOut, int32 iSpacing, int iSide, bool bAlongY, CArrayInt* paiFans)
{
	ERotation		eRotation;
	CSplitterView*	pcSplitter;
	CPortView*		pcEnd;
	CArrayInt		aiAlongs;
	size			i;
	size			j;
	int32			iFan;

	eRotation = ROT_East;
	aiAlongs.Init();
	for (i = 0; i < 4; i++)
	{
		pcSplitter = NewMalloc<CSplitterView>();
		pcSplitter->Init(0, 0, gaeKicadSchemLogicimRotations[i], "", iBitWidth, iFanOut, iSpacing, SA_LeftHanded, 0);
		pcEnd = pcSplitter->GetEndPortView(0);
		if ((pcEnd != NULL) && (CTraceAutoConnector::GetPortFacing(pcEnd) == iSide))
		{
			eRotation = gaeKicadSchemLogicimRotations[i];
			for (iFan = 0; iFan < iFanOut; iFan++)
			{
				pcEnd = pcSplitter->GetEndPortView(iFan);
				aiAlongs.Add((pcEnd == NULL) ? 0 : (bAlongY ? pcEnd->GetGridPosition().y : pcEnd->GetGridPosition().x));
			}
			SafeKill(pcSplitter);
			break;
		}
		SafeKill(pcSplitter);
	}
	if (aiAlongs.NumElements() == 0)
	{
		for (iFan = 0; iFan < iFanOut; iFan++)
		{
			aiAlongs.Add(iFan);
		}
	}

	for (iFan = 0; iFan < iFanOut; iFan++)
	{
		for (j = 0; j < paiFans->NumElements(); j++)
		{
			if (aiAlongs.GetValue(iFan) < aiAlongs.GetValue(paiFans->GetValue(j)))
			{
				break;
			}
		}
		paiFans->InsertAt(iFan, j);
	}
	aiAlongs.Kill();
	return eRotation;
}


//////////////////////////////////////////////////////////////////////////
//
// A splitter of the whole bus for a run of taps (in order along the bus)
// on the side iSide of it, its ends KICAD_SCHEM_LOGICIM_SPLITTER_SPACING
// apart, or as close as the pins they line up with if those are closer.  It faces the component pins its members are wired to (the way
// most of them face, see GetBusPins), its ends given to the taps in the
// order of their pins along that side, and is kept beside that component
// (see PlaceSplitters).  A run wired to no pins faces the members as KiCad
// draws them, its ends in the order of the taps along the bus.  The bus's
// other members go to an end of their own, after the taps'.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddBusSplitter(CKicadSchemBus* pcBus, CArrayInt* paiTaps, bool bVertical, int iSide)
{
	ERotation						eRotation;
	CSplitterView*					pcSplitter;
	CArrayInt						aiFans;
	CArrayInt						aiOrder;
	CArrayTemplatePtr<CPortView>	apcPins;
	int*							aiSplitIndices;
	int32							iBitWidth;
	int32							iFanOut;
	int32							iRestFan;
	int32							iSpacing;
	int								iPinFacing;
	bool							bAlongY;
	size							i;
	size							uiTaps;
	size							uiItem;
	SKicadSchemBusTap*				psTap;
	SKicadSchemPoint				sCentre;
	SKicadSchemPointMean			sMean;

	uiTaps = paiTaps->NumElements();
	iBitWidth = (int32)pcBus->NumMembers();
	iFanOut = (int32)uiTaps + (((size)iBitWidth > uiTaps) ? 1 : 0);

	//The pins it goes to decide the way its ends face, and the order its ends go to the taps.
	apcPins.Init();
	iPinFacing = GetBusPins(pcBus, paiTaps, &apcPins);
	if (iPinFacing != -1)
	{
		iSide = CTraceAutoConnector::OppositeDirection(iPinFacing);
	}
	bAlongY = (iSide == TRACE_DIRECTION_EAST) || (iSide == TRACE_DIRECTION_WEST);
	aiOrder.Init();
	OrderKicadSchemSplitterTaps(&apcPins, iPinFacing, bAlongY, &aiOrder);
	iSpacing = GetKicadSchemSplitterSpacing(&apcPins, &aiOrder, iPinFacing, bAlongY);
	aiFans.Init();
	eRotation = OrderKicadSchemSplitterEnds(iBitWidth, iFanOut, iSpacing, iSide, bAlongY, &aiFans);

	//Each member's end: the taps' in order, the rest on the end after them.  It's where the taps are.
	aiSplitIndices = (int*)malloc(sizeof(int) * (iBitWidth + 1));
	iRestFan = ((size)iFanOut > uiTaps) ? aiFans.GetValue(uiTaps) : 0;
	for (i = 0; i < (size)iBitWidth; i++)
	{
		aiSplitIndices[i] = iRestFan;
	}
	sMean.Init();
	for (i = 0; i < uiTaps; i++)
	{
		psTap = pcBus->GetTap(paiTaps->GetValue(aiOrder.GetValue(i)));
		aiSplitIndices[pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName())] = aiFans.GetValue(i);
		sMean.Add(&psTap->sBusPoint);
	}
	sMean.Get(&sCentre);

	pcSplitter = NewMalloc<CSplitterView>();
	pcSplitter->Init(0, 0, eRotation, "", iBitWidth, iFanOut, iSpacing, SA_LeftHanded, 0, aiSplitIndices);
	free(aiSplitIndices);
	uiItem = AddItem(pcSplitter, KSLIK_Splitter, &sCentre, NULL);
	AddBusSplitterTerminals(pcBus, paiTaps, pcSplitter, uiItem, &aiOrder, &aiFans, &apcPins, iPinFacing);

	aiOrder.Kill();
	apcPins.Kill();
	aiFans.Kill();
	muiSplitters++;
}


//////////////////////////////////////////////////////////////////////////
//
// The splitter's start on the bus and its ends on the taps' members, and
// its record (see SKicadSchemLogicimSplitter): each end lined up with the
// pin its tap's member is wired to (papcPins, facing iPinFacing).
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddBusSplitterTerminals(CKicadSchemBus* pcBus, CArrayInt* paiTaps, CSplitterView* pcSplitter, size uiItem, CArrayInt* paiOrder, CArrayInt* paiFans, CArrayTemplatePtr<CPortView>* papcPins, int iPinFacing)
{
	SKicadSchemLogicimSplitter*		psSplitter;
	SKicadSchemLogicimSplitterPair*	psPair;
	SKicadSchemBusTap*				psTap;
	CPortView*						pcEnd;
	size						i;
	size						uiTap;

	psTap = pcBus->GetTap(paiTaps->GetValue(0));
	AddTerminal(pcBus->GetSubnet(), &psTap->sBusPoint, pcSplitter->GetStartPortView(), uiItem, true);
	psSplitter = masSplitters.Add();
	psSplitter->pcSplitter = pcSplitter;
	psSplitter->iFacing = iPinFacing;
	psSplitter->iGap = KICAD_SCHEM_LOGICIM_SPLITTER_GAP;
	for (i = 0; i < paiTaps->NumElements(); i++)
	{
		if (HasTunnel(pcBus->GetTap(paiTaps->GetValue(i))->pcMember))
		{
			psSplitter->iGap = KICAD_SCHEM_LOGICIM_SPLITTER_TUNNEL_GAP;
		}
	}
	psSplitter->uiFirstPair = masSplitterPairs.NumElements();
	psSplitter->uiNumPairs = 0;
	for (i = 0; i < paiTaps->NumElements(); i++)
	{
		uiTap = paiOrder->GetValue(i);
		psTap = pcBus->GetTap(paiTaps->GetValue(uiTap));
		pcEnd = pcSplitter->GetEndPortView(paiFans->GetValue(i));
		AddTerminal(psTap->pcMember, &psTap->sWirePoint, pcEnd, uiItem);
		if ((iPinFacing != -1) && (papcPins->GetPtr(uiTap) != NULL) && (pcEnd != NULL))
		{
			psPair = masSplitterPairs.Add();
			psPair->pcPin = papcPins->GetPtr(uiTap);
			psPair->pcEnd = pcEnd;
			psSplitter->uiNumPairs++;
			masItems.Get(uiItem)->bPlacedSplitter = true;
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
void CKicadSchemLogicimConverter::AddTunnels(void)
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
				pcTunnel->Init(0, 0, GetKicadRotation(ROT_East, pcLabel->GetAngle()), pcNet->GetName(), false, mpcFonts);
				uiItem = AddItem(pcTunnel, KSLIK_Tunnel, pcLabel->GetPosition(), pcTunnel->GetPortView((size)0));
				AddTerminal(pcSubnet, pcLabel->GetPosition(), pcTunnel->GetPortView((size)0), uiItem);
				muiTunnels++;
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
CKicadSchemBus* CKicadSchemLogicimConverter::GetMemberBus(CKicadSchemSubnet* pcSubnet)
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
bool CKicadSchemLogicimConverter::IsJoinedByBus(CKicadSchemNet* pcNet)
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
