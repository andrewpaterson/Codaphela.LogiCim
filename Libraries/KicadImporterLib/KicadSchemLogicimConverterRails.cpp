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
#include "BaseLib/StringHelper.h"
#include "MicroprocessorLib/LongTime.h"
#include "MicroprocessorLib/ResetSupervisor.h"
#include "LogicimLib/PowerSourceView.h"
#include "LogicimLib/ResetSupervisorView.h"
#include "LogicimLib/AnalogComparatorView.h"
#include "LogicimLib/ComponentPlacement.h"
#include "KicadSchemSupplyNames.h"
#include "KicadSchemLogicimConverter.h"


//////////////////////////////////////////////////////////////////////////
//
// The nets of the symbol's power pins (its power inputs, or pins named
// as power but not "NC"), the ground ones (bGround) or the others, added
// to papcNets.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::GetPowerPinNets(CKicadSchemSymbol* pcSymbol, bool bGround, CArrayTemplatePtr<CKicadSchemNet>* papcNets)
{
	CKicadSchemLibSymbol*	pcLibSymbol;
	CKicadSchemLibPin*		pcPin;
	CKicadSchemSubnet*		pcSubnet;
	size					i;

	pcLibSymbol = mpcFile->GetLibSymbol(pcSymbol);
	if (pcLibSymbol == NULL)
	{
		return;
	}
	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (!pcSymbol->IsUnitPin(pcPin) || (IsKicadSchemGroundName(pcPin->GetName()) != bGround))
		{
			continue;
		}
		if (((pcPin->GetType() != KSPT_PowerIn) && !IsKicadSchemPowerPinName(pcPin->GetName())) || IsKicadSchemNoConnectName(pcPin->GetName()))
		{
			continue;
		}
		pcSubnet = mpcNetlist->GetPinSubnet(pcSymbol, pcPin);
		if ((pcSubnet != NULL) && (pcSubnet->GetNet() != NULL))
		{
			papcNets->Add(pcSubnet->GetNet());
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The net of the first of the part's power pins, ground (bGround) or not,
// in any of its units (each a symbol with its reference).  NULL if it
// has none.
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemNet* CKicadSchemLogicimConverter::GetPowerPinNet(CKicadSchemSymbol* pcSymbol, bool bGround)
{
	CArrayTemplatePtr<CKicadSchemNet>	apcNets;
	CKicadSchemSymbol*					pcUnit;
	CKicadSchemNet*						pcNet;
	size								i;

	apcNets.Init();
	for (i = 0; (i < mpcFile->NumSymbols()) && (apcNets.NumElements() == 0); i++)
	{
		pcUnit = mpcFile->GetSymbol(i);
		if ((pcUnit == pcSymbol) || (StringCompare(pcUnit->GetReference(), pcSymbol->GetReference()) == 0))
		{
			GetPowerPinNets(pcUnit, bGround, &apcNets);
		}
	}
	pcNet = apcNets.SafeGetPtr(0);
	apcNets.Kill();
	return pcNet;
}


//////////////////////////////////////////////////////////////////////////
//
// The power rails: the nets the parts' power pins are on and the nets of
// the regulators' inputs and outputs.  Their subnets count as drawn
// (IsSubnetDrawn), so their supplies are made, as the hidden power pins
// on them need.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::FindRailNets(void)
{
	size						i;
	SKicadSchemLogicimItem*		psItem;
	CIntegratedCircuitView*		pcIntegratedCircuit;
	CKicadSchemNet*				apcNets[2];
	size						j;

	for (i = 0; i < mcModel.NumItems(); i++)
	{
		psItem = mcModel.GetItem(i);
		pcIntegratedCircuit = psItem->pcView->GetIntegratedCircuitView();
		if ((psItem->pcSymbol == NULL) || (pcIntegratedCircuit == NULL) || pcIntegratedCircuit->IsExplicitPowerPorts())
		{
			continue;
		}
		apcNets[0] = GetPowerPinNet(psItem->pcSymbol, false);
		apcNets[1] = GetPowerPinNet(psItem->pcSymbol, true);
		for (j = 0; j < 2; j++)
		{
			if ((apcNets[j] != NULL) && !IsRailNet(apcNets[j]))
			{
				mapcRailNets.Add(apcNets[j]);
			}
		}
	}
	muiPartRailNets = mapcRailNets.NumElements();
	for (i = 0; i < masRegulated.NumElements(); i++)
	{
		apcNets[0] = masRegulated.Get(i)->pcNet;
		apcNets[1] = masRegulated.Get(i)->pcFeed;
		for (j = 0; j < 2; j++)
		{
			if ((apcNets[j] != NULL) && !IsRailNet(apcNets[j]))
			{
				mapcRailNets.Add(apcNets[j]);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsRailNet(CKicadSchemNet* pcNet)
{
	return (pcNet != NULL) && (mapcRailNets.Find(pcNet) != ARRAY_ELEMENT_NOT_FOUND);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a part's power pin is on the net, not only a regulator's.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsPartRailNet(CKicadSchemNet* pcNet)
{
	size	uiIndex;

	uiIndex = (pcNet != NULL) ? mapcRailNets.Find(pcNet) : ARRAY_ELEMENT_NOT_FOUND;
	return (uiIndex != ARRAY_ELEMENT_NOT_FOUND) && (uiIndex < muiPartRailNets);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a supply is on the net: a power symbol's, a label's or a
// regulator's (see AddRegulatorSupplies).
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::HasNetSupply(CKicadSchemNet* pcNet)
{
	return (pcNet != NULL) && (HasSupply(pcNet) || (GetNetRegulatedVoltage(pcNet) > 0));
}


//////////////////////////////////////////////////////////////////////////
//
// Each regulator a supply on its output's rail, at its symbol, wired to
// what's drawn on the output, unless a power symbol already supplies it.
// A regulator with outputs on more than one net (a tab on its own) is one
// supply, on the net a part's power pin is on if one is.  It's fed from
// its input's rail if a supply is on that, with its part's dropout and
// rise time (see GetKicadSchemRegulatorPart); otherwise it rises on its
// own.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddRegulatorSupplies(void)
{
	size							i;
	size							j;
	SKicadSchemLogicimRegulated*	psRegulated;
	SKicadSchemLogicimRegulated*	psOther;
	CPositivePowerView*				pcView;
	SKicadSchemRegulatorPart*		psPart;
	size							uiItem;
	bool							bDone;

	for (i = 0; i < masRegulated.NumElements(); i++)
	{
		psRegulated = masRegulated.Get(i);
		bDone = false;
		for (j = 0; (j < masRegulated.NumElements()) && !bDone; j++)
		{
			psOther = masRegulated.Get(j);
			if ((j != i) && (psOther->pcSymbol == psRegulated->pcSymbol))
			{
				//The other output is the one: earlier, or the one a part is powered from when this one isn't.
				bDone = IsPartRailNet(psOther->pcNet) ? !IsPartRailNet(psRegulated->pcNet) || (j < i) : !IsPartRailNet(psRegulated->pcNet) && (j < i);
			}
		}
		if (bDone || HasSupply(psRegulated->pcNet))
		{
			continue;
		}
		psPart = GetKicadSchemRegulatorPart(psRegulated->pcSymbol->GetValue());
		pcView = (CPositivePowerView*)NewSupply(psRegulated->pcSymbol->GetReference(), psRegulated->fVoltage, psRegulated->pcSymbol->GetAngle(), ROT_North, ROT_South);
		pcView->SetRiseTime(NanosecondsToTime(psPart->iRiseNanoseconds));
		pcView->SetRail(psRegulated->pcNet->GetName());
		if ((psRegulated->pcFeed != NULL) && (psRegulated->pcFeed != psRegulated->pcNet) && HasNetSupply(psRegulated->pcFeed))
		{
			pcView->SetFeed(psPart->fDropout);
			pcView->SetFeedRail(psRegulated->pcFeed->GetName());
		}
		uiItem = mcModel.AddItem(pcView, KSLIK_Part, psRegulated->pcSymbol->GetPosition(), NULL, psRegulated->pcSymbol);
		AddPinTerminal(psRegulated->pcSymbol, psRegulated->pcOutput, pcView->GetPortView((size)0), uiItem);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each part whose VCC and GND nets have supplies on them is on those nets
// as power rails, its power pins hidden and joined to them by name; one
// whose nets don't keeps its implied power.  The supplies on the rails
// are on them too, so they join every part on the rail wherever they're
// drawn.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::ApplyPowerRails(void)
{
	size						i;
	SKicadSchemLogicimItem*		psItem;
	CIntegratedCircuitView*		pcIntegratedCircuit;
	CKicadSchemNet*				pcVcc;
	CKicadSchemNet*				pcGnd;
	size						uiTerminal;
	CKicadSchemNet*				pcNet;

	for (i = 0; i < mcModel.NumItems(); i++)
	{
		psItem = mcModel.GetItem(i);
		pcIntegratedCircuit = psItem->pcView->GetIntegratedCircuitView();
		if ((psItem->pcSymbol != NULL) && (pcIntegratedCircuit != NULL) && !pcIntegratedCircuit->IsExplicitPowerPorts())
		{
			pcVcc = GetPowerPinNet(psItem->pcSymbol, false);
			pcGnd = GetPowerPinNet(psItem->pcSymbol, true);
			if (HasNetSupply(pcVcc) && HasNetSupply(pcGnd))
			{
				pcIntegratedCircuit->SetPowerRails(pcVcc->GetName(), pcGnd->GetName());
			}
		}
		else if (psItem->eKind == KSLIK_Supply)
		{
			uiTerminal = mcModel.FirstItemTerminal(i);
			pcNet = (uiTerminal != ARRAY_ELEMENT_NOT_FOUND) ? mcModel.GetTerminal(uiTerminal)->pcSubnet->GetNet() : NULL;
			if (IsRailNet(pcNet))
			{
				((CPowerSourceView*)psItem->pcView)->SetRail(pcNet->GetName());
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A DS1813 (its value says) as a reset supervisor with its trip voltage,
// its RST pin its reset.  False if the symbol isn't one.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddSupervisor(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	float32					fTripVoltage;
	CResetSupervisorView*	pcView;
	size					uiItem;

	fTripVoltage = GetKicadSchemSupervisorTripVoltage(pcSymbol->GetValue());
	if (fTripVoltage == 0.0f)
	{
		return false;
	}
	pcView = NewMalloc<CResetSupervisorView>();
	pcView->Init(0, 0, ROT_East, pcSymbol->GetReference(), PLACED_FAMILY, false, fTripVoltage, NanosecondsToTime((int64)RESET_SUPERVISOR_RESET_TIME));
	uiItem = mcModel.AddItem(pcView, KSLIK_Part, pcSymbol->GetPosition(), NULL, pcSymbol);
	AddNamedPinTerminals(pcSymbol, pcLibSymbol, pcView, uiItem);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// A comparator on the list (IsKicadSchemComparatorPart, a MAX9010) as an
// analog comparator, its IN+, IN- and OUT pins its ports.  False if the
// symbol isn't one.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddAnalogComparator(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	CAnalogComparatorView*	pcView;
	size					uiItem;

	if (!IsKicadSchemComparatorPart(pcSymbol->GetValue()))
	{
		return false;
	}
	pcView = NewMalloc<CAnalogComparatorView>();
	pcView->Init(0, 0, ROT_East, pcSymbol->GetReference(), PLACED_FAMILY, false);
	uiItem = mcModel.AddItem(pcView, KSLIK_Part, pcSymbol->GetPosition(), NULL, pcSymbol);
	AddNamedPinTerminals(pcSymbol, pcLibSymbol, pcView, uiItem);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Each of the symbol's unit's pins whose name, active low marks aside
// ("~{RST}" is RST), is one of the view's ports, as a terminal of it.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddNamedPinTerminals(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, CComponentView* pcView, size uiItem)
{
	size				i;
	CKicadSchemLibPin*	pcPin;
	CChars				szName;
	CPortView*			pcPortView;

	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (!pcSymbol->IsUnitPin(pcPin))
		{
			continue;
		}
		szName.Init(pcPin->GetName());
		szName.Replace("~{", "");
		szName.Replace("}", "");
		pcPortView = pcView->GetPortView(szName.Text());
		if (pcPortView != NULL)
		{
			AddPinTerminal(pcSymbol, pcPin, pcPortView, uiItem);
		}
		szName.Kill();
	}
}

