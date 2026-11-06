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
// A splitter for each run of the bus's taps on one side of it, and a tunnel at each of
// its labels if its name is on another bus too.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddBus(CKicadSchemBus* pcBus, size uiBus)
{
	CArrayInt				aiTaps;
	CArrayInt				aiUsed;
	CArrayInt				aiRun;
	size					i;
	size					j;
	size					uiMember;
	size					uiNamed;
	SKicadSchemBusTap*		psTap;
	SKicadSchemBusTap*		psOther;
	CKicadSchemLabel*		pcLabel;
	CTunnelView*			pcTunnel;
	size					uiItem;
	bool					bBefore;
	CArrayTemplatePtr<CPortView>	apcGrouped;
	CArrayInt				aiGrouped;
	CPortView*				pcPin;
	CPortView*				pcOtherPin;

	if (pcBus->NumMembers() == 0)
	{
		return;
	}

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
	if (uiNamed > 1)
	{
		for (i = 0; i < pcBus->NumLabels(); i++)
		{
			pcLabel = pcBus->GetLabel(i);
			pcTunnel = NewMalloc<CTunnelView>();
			pcTunnel->Init(0, 0, GetKicadRotation(ROT_East, pcLabel->GetAngle()), pcBus->GetName(), false, mpcFonts);
			uiItem = AddItem(pcTunnel, pcLabel->GetPosition(), pcTunnel->GetPortView((size)0));
			AddTerminal(pcBus->GetSubnet(), pcLabel->GetPosition(), pcTunnel->GetPortView((size)0), uiItem, true);
			muiTunnels++;
		}
	}

	//The taps, sorted by side, then where the bus is across, then along it.
	aiTaps.Init();
	aiUsed.Init();
	for (i = 0; i < pcBus->NumTaps(); i++)
	{
		psTap = pcBus->GetTap(i);
		uiMember = pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName());
		if (uiMember == ARRAY_ELEMENT_NOT_FOUND)
		{
			continue;
		}
		for (j = 0; j < aiTaps.NumElements(); j++)
		{
			psOther = pcBus->GetTap(aiTaps.GetValue(j));
			if (psTap->bVertical != psOther->bVertical)
			{
				bBefore = psTap->bVertical;
			}
			else if (GetKicadSchemTapSide(psTap) != GetKicadSchemTapSide(psOther))
			{
				bBefore = GetKicadSchemTapSide(psTap) < GetKicadSchemTapSide(psOther);
			}
			else if (fabs(GetKicadSchemTapAcross(psTap) - GetKicadSchemTapAcross(psOther)) > KICAD_SCHEM_POINT_TOLERANCE)
			{
				bBefore = GetKicadSchemTapAcross(psTap) < GetKicadSchemTapAcross(psOther);
			}
			else
			{
				bBefore = GetKicadSchemTapAlong(psTap) < GetKicadSchemTapAlong(psOther);
			}
			if (bBefore)
			{
				break;
			}
		}
		aiTaps.InsertAt((int)i, j);
	}

	//The taps wired to a component's pins, one splitter for each component and way its pins face (one end for each
	//bit, so a member twice starts another), beside it.
	aiRun.Init();
	apcGrouped.Init();
	for (i = 0; i < aiTaps.NumElements(); i++)
	{
		pcPin = GetTapPin(pcBus->GetTap(aiTaps.GetValue(i)));
		apcGrouped.Add(pcPin);
	}
	for (i = 0; i < aiTaps.NumElements(); i++)
	{
		pcPin = apcGrouped.GetPtr(i);
		if (pcPin == NULL)
		{
			continue;
		}
		aiRun.ReInit();
		aiUsed.ReInit();
		for (j = i; j < aiTaps.NumElements(); j++)
		{
			pcOtherPin = apcGrouped.GetPtr(j);
			if ((pcOtherPin == NULL) || (pcOtherPin->GetOwner() != pcPin->GetOwner()) || (CTraceAutoConnector::GetPortFacing(pcOtherPin) != CTraceAutoConnector::GetPortFacing(pcPin)))
			{
				continue;
			}
			psTap = pcBus->GetTap(aiTaps.GetValue(j));
			uiMember = pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName());
			if (aiUsed.Find((int)uiMember) != ARRAY_ELEMENT_NOT_FOUND)
			{
				continue;
			}
			aiRun.Add(aiTaps.GetValue(j));
			aiUsed.Add((int)uiMember);
			apcGrouped.SetPtr(j, NULL);
		}
		psOther = pcBus->GetTap(aiRun.GetValue(0));
		AddBusSplitter(pcBus, &aiRun, psOther->bVertical, GetKicadSchemTapSide(psOther));
	}

	//The rest in runs: the same side of the same piece of the bus, each tap no more than two entries' spacing from the
	//last, and no member twice.
	aiGrouped.Init();
	for (i = 0; i < aiTaps.NumElements(); i++)
	{
		if (GetTapPin(pcBus->GetTap(aiTaps.GetValue(i))) == NULL)
		{
			aiGrouped.Add(aiTaps.GetValue(i));
		}
	}
	aiRun.ReInit();
	aiUsed.ReInit();
	for (i = 0; i < aiGrouped.NumElements(); i++)
	{
		psTap = pcBus->GetTap(aiGrouped.GetValue(i));
		if (aiRun.NumElements() > 0)
		{
			psOther = pcBus->GetTap(aiRun.GetValue(aiRun.NumElements() - 1));
			if ((psTap->bVertical != psOther->bVertical) || (GetKicadSchemTapSide(psTap) != GetKicadSchemTapSide(psOther)) ||
				(fabs(GetKicadSchemTapAcross(psTap) - GetKicadSchemTapAcross(psOther)) > KICAD_SCHEM_POINT_TOLERANCE) || (GetKicadSchemTapAlong(psTap) - GetKicadSchemTapAlong(psOther) > 2 * KICAD_SCHEM_LOGICIM_GRID + KICAD_SCHEM_POINT_TOLERANCE) ||
				(aiUsed.Find((int)pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName())) != ARRAY_ELEMENT_NOT_FOUND))
			{
				AddBusSplitter(pcBus, &aiRun, psOther->bVertical, GetKicadSchemTapSide(psOther));
				aiRun.ReInit();
				aiUsed.ReInit();
			}
		}
		aiRun.Add(aiGrouped.GetValue(i));
		aiUsed.Add((int)pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName()));
	}
	if (aiRun.NumElements() > 0)
	{
		psOther = pcBus->GetTap(aiRun.GetValue(aiRun.NumElements() - 1));
		AddBusSplitter(pcBus, &aiRun, psOther->bVertical, GetKicadSchemTapSide(psOther));
	}
	aiGrouped.Kill();
	apcGrouped.Kill();
	aiRun.Kill();
	aiUsed.Kill();
	aiTaps.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The first component pin the tap's member is wired to (not a tunnel's,
// supply's or splitter's), NULL if none.
//
//////////////////////////////////////////////////////////////////////////
CPortView* CKicadSchemLogicimConverter::GetTapPin(SKicadSchemBusTap* psTap)
{
	size							i;
	SKicadSchemLogicimTerminal*		psTerminal;
	SKicadSchemLogicimItem*			psItem;

	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		psTerminal = masTerminals.Get(i);
		if ((psTerminal->pcSubnet != psTap->pcMember) || psTerminal->bBus)
		{
			continue;
		}
		psItem = masItems.Get(psTerminal->uiItem);
		if ((psItem->pcPort == NULL) && (StringCompare(psItem->pcView->GetType(), "Splitter") != 0))
		{
			return psTerminal->pcPortView;
		}
	}
	return NULL;
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
	size							i;
	size							j;
	size							k;
	SKicadSchemBusTap*				psTap;
	SKicadSchemLogicimTerminal*		psTerminal;
	SKicadSchemLogicimItem*			psItem;
	int								aiFacings[4];
	int								iFacing;
	CArrayTemplatePtr<CComponentView>	apcOwners;
	CArrayInt						aiOwnerPins;
	size							uiOwner;
	CComponentView*					pcMost;
	int								iMost;
	CPortView*						pcPin;

	for (i = 0; i < 4; i++)
	{
		aiFacings[i] = 0;
	}
	apcOwners.Init();
	aiOwnerPins.Init();
	for (k = 0; k < 2; k++)
	{
		for (i = 0; i < paiTaps->NumElements(); i++)
		{
			psTap = pcBus->GetTap(paiTaps->GetValue(i));
			for (j = 0; j < masTerminals.NumElements(); j++)
			{
				psTerminal = masTerminals.Get(j);
				if ((psTerminal->pcSubnet != psTap->pcMember) || psTerminal->bBus)
				{
					continue;
				}
				psItem = masItems.Get(psTerminal->uiItem);
				if ((psItem->pcPort != NULL) || (StringCompare(psItem->pcView->GetType(), "Splitter") == 0))
				{
					continue;
				}
				if (k == 0)
				{
					aiFacings[CTraceAutoConnector::GetPortFacing(psTerminal->pcPortView)]++;
				}
				else if (CTraceAutoConnector::GetPortFacing(psTerminal->pcPortView) == iFacing)
				{
					uiOwner = apcOwners.Find(psItem->pcView);
					if (uiOwner == ARRAY_ELEMENT_NOT_FOUND)
					{
						apcOwners.Add(psItem->pcView);
						aiOwnerPins.Add(1);
					}
					else
					{
						aiOwnerPins.SetValue(uiOwner, aiOwnerPins.GetValue(uiOwner) + 1);
					}
				}
			}
		}
		if (k == 0)
		{
			iFacing = -1;
			for (i = 0; i < 4; i++)
			{
				if ((aiFacings[i] > 0) && ((iFacing == -1) || (aiFacings[i] > aiFacings[iFacing])))
				{
					iFacing = (int)i;
				}
			}
			if (iFacing == -1)
			{
				break;
			}
		}
	}

	pcMost = NULL;
	iMost = 0;
	for (i = 0; i < apcOwners.NumElements(); i++)
	{
		if (aiOwnerPins.GetValue(i) > iMost)
		{
			iMost = aiOwnerPins.GetValue(i);
			pcMost = apcOwners.GetPtr(i);
		}
	}

	for (i = 0; i < paiTaps->NumElements(); i++)
	{
		psTap = pcBus->GetTap(paiTaps->GetValue(i));
		pcPin = NULL;
		for (j = 0; (j < masTerminals.NumElements()) && (pcMost != NULL); j++)
		{
			psTerminal = masTerminals.Get(j);
			if ((psTerminal->pcSubnet == psTap->pcMember) && !psTerminal->bBus && (psTerminal->pcPortView->GetOwner() == pcMost) && (CTraceAutoConnector::GetPortFacing(psTerminal->pcPortView) == iFacing))
			{
				pcPin = psTerminal->pcPortView;
				break;
			}
		}
		papcPins->Add(pcPin);
	}
	aiOwnerPins.Kill();
	apcOwners.Kill();
	return (pcMost != NULL) ? iFacing : -1;
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
	ERotation				eRotation;
	CSplitterView*			pcSplitter;
	CArrayInt				aiFans;
	CArrayInt				aiAlongs;
	int*					aiSplitIndices;
	int32					iBitWidth;
	int32					iFanOut;
	int32					iFan;
	int32					iRestFan;
	int32					iAlong;
	size					i;
	size					j;
	size					uiTaps;
	size					uiItem;
	SKicadSchemBusTap*		psTap;
	SKicadSchemPoint		sCentre;
	CPortView*				pcEnd;
	CArrayTemplatePtr<CPortView>	apcPins;
	CArrayInt				aiOrder;
	int						iPinFacing;
	bool					bAlongY;
	SKicadSchemLogicimSplitter*	psSplitter;
	int32					iKey;
	int32					iOtherKey;
	size					uiTap;
	int32					iSpacing;
	CPortView*				pcSplitterPin;

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
	for (i = 0; i < uiTaps; i++)
	{
		j = aiOrder.NumElements();
		if ((iPinFacing != -1) && (apcPins.GetPtr(i) != NULL))
		{
			iKey = bAlongY ? apcPins.GetPtr(i)->GetGridPosition().y : apcPins.GetPtr(i)->GetGridPosition().x;
			for (j = 0; j < aiOrder.NumElements(); j++)
			{
				pcEnd = apcPins.GetPtr(aiOrder.GetValue(j));
				if (pcEnd == NULL)
				{
					break;
				}
				iOtherKey = bAlongY ? pcEnd->GetGridPosition().y : pcEnd->GetGridPosition().x;
				if (iKey < iOtherKey)
				{
					break;
				}
			}
		}
		aiOrder.InsertAt((int)i, j);
	}

	//Ends as far apart as the pins they're lined up with: two grid units for an instance's pins, one for a 74 part's.
	iSpacing = KICAD_SCHEM_LOGICIM_SPLITTER_SPACING;
	for (i = 1; (i < uiTaps) && (iPinFacing != -1); i++)
	{
		pcEnd = apcPins.GetPtr(aiOrder.GetValue(i - 1));
		pcSplitterPin = apcPins.GetPtr(aiOrder.GetValue(i));
		if ((pcEnd != NULL) && (pcSplitterPin != NULL))
		{
			iKey = bAlongY ? pcSplitterPin->GetGridPosition().y - pcEnd->GetGridPosition().y : pcSplitterPin->GetGridPosition().x - pcEnd->GetGridPosition().x;
			if ((iKey > 0) && (iKey < iSpacing))
			{
				iSpacing = iKey;
			}
		}
	}

	//The rotation that has the ends facing the members, and where each end is along the bus.
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
				iAlong = 0;
				if (pcEnd != NULL)
				{
					iAlong = bAlongY ? pcEnd->GetGridPosition().y : pcEnd->GetGridPosition().x;
				}
				aiAlongs.Add(iAlong);
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

	//The ends in order along the bus.
	aiFans.Init();
	for (iFan = 0; iFan < iFanOut; iFan++)
	{
		for (j = 0; j < aiFans.NumElements(); j++)
		{
			if (aiAlongs.GetValue(iFan) < aiAlongs.GetValue(aiFans.GetValue(j)))
			{
				break;
			}
		}
		aiFans.InsertAt(iFan, j);
	}

	aiSplitIndices = (int*)malloc(sizeof(int) * (iBitWidth + 1));
	iRestFan = ((size)iFanOut > uiTaps) ? aiFans.GetValue(uiTaps) : 0;
	for (i = 0; i < (size)iBitWidth; i++)
	{
		aiSplitIndices[i] = iRestFan;
	}
	sCentre.Init();
	for (i = 0; i < uiTaps; i++)
	{
		psTap = pcBus->GetTap(paiTaps->GetValue(aiOrder.GetValue(i)));
		aiSplitIndices[pcBus->GetMemberIndex(psTap->pcMember->GetNet()->GetName())] = aiFans.GetValue(i);
		sCentre.fX += psTap->sBusPoint.fX;
		sCentre.fY += psTap->sBusPoint.fY;
	}
	sCentre.fX /= (float64)uiTaps;
	sCentre.fY /= (float64)uiTaps;

	pcSplitter = NewMalloc<CSplitterView>();
	pcSplitter->Init(0, 0, eRotation, "", iBitWidth, iFanOut, iSpacing, SA_LeftHanded, 0, aiSplitIndices);
	free(aiSplitIndices);
	uiItem = AddItem(pcSplitter, &sCentre, NULL);

	psTap = pcBus->GetTap(paiTaps->GetValue(0));
	AddTerminal(pcBus->GetSubnet(), &psTap->sBusPoint, pcSplitter->GetStartPortView(), uiItem, true);
	psSplitter = masSplitters.Add();
	psSplitter->pcSplitter = pcSplitter;
	psSplitter->iFacing = iPinFacing;
	psSplitter->iGap = KICAD_SCHEM_LOGICIM_SPLITTER_GAP;
	for (i = 0; i < uiTaps; i++)
	{
		psTap = pcBus->GetTap(paiTaps->GetValue(i));
		for (j = 0; j < masTerminals.NumElements(); j++)
		{
			if ((masTerminals.Get(j)->pcSubnet == psTap->pcMember) && (StringCompare(masItems.Get(masTerminals.Get(j)->uiItem)->pcView->GetType(), "Tunnel") == 0))
			{
				psSplitter->iGap = KICAD_SCHEM_LOGICIM_SPLITTER_TUNNEL_GAP;
			}
		}
	}
	psSplitter->uiFirstPair = mapcSplitterPins.NumElements();
	psSplitter->uiNumPairs = 0;
	for (i = 0; i < uiTaps; i++)
	{
		uiTap = aiOrder.GetValue(i);
		psTap = pcBus->GetTap(paiTaps->GetValue(uiTap));
		pcEnd = pcSplitter->GetEndPortView(aiFans.GetValue(i));
		AddTerminal(psTap->pcMember, &psTap->sWirePoint, pcEnd, uiItem);
		if ((iPinFacing != -1) && (apcPins.GetPtr(uiTap) != NULL) && (pcEnd != NULL))
		{
			mapcSplitterPins.Add(apcPins.GetPtr(uiTap));
			mapcSplitterEnds.Add(pcEnd);
			psSplitter->uiNumPairs++;
		}
	}
	aiOrder.Kill();
	apcPins.Kill();
	aiFans.Kill();
	aiAlongs.Kill();
	muiSplitters++;
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
				uiItem = AddItem(pcTunnel, pcLabel->GetPosition(), pcTunnel->GetPortView((size)0));
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
