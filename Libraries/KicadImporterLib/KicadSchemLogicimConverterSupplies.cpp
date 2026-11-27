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
#include "MicroprocessorLib/LongTime.h"
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
// Every regulator's output nets (AddRegulator), before any other symbol
// is made, so a power symbol knows if it's on a regulator's output.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::FindRegulators(void)
{
	size					i;
	CKicadSchemSymbol*		pcSymbol;
	CKicadSchemLibSymbol*	pcLibSymbol;

	for (i = 0; i < mpcFile->NumSymbols(); i++)
	{
		pcSymbol = mpcFile->GetSymbol(i);
		pcLibSymbol = mpcFile->GetLibSymbol(pcSymbol);
		if ((pcLibSymbol != NULL) && !pcLibSymbol->IsPower() && !IsKicadSchemSkippedSymbol(pcSymbol) && AddRegulator(pcSymbol, pcLibSymbol))
		{
			mapcRegulators.Add(pcSymbol);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A ground or positive supply with its port on the power symbol's pin, or
// a tunnel of its net's name if a regulator drives the net: the power
// symbol only names the regulator's output.  Power flags are left out.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddPower(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	size					i;
	CKicadSchemLibPin*		pcPin;
	const char*				szName;
	CComponentView*			pcView;
	SFloat64Vec2			sPosition;
	size					uiItem;
	CKicadSchemSubnet*		pcSubnet;
	CKicadSchemNet*			pcNet;
	CTunnelView*			pcTunnel;

	pcPin = NULL;
	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		if (pcSymbol->IsUnitPin(pcLibSymbol->GetPin(i)))
		{
			pcPin = pcLibSymbol->GetPin(i);
			break;
		}
	}
	szName = (pcPin != NULL) ? CKicadSchemNetlist::GetGlobalPowerName(pcSymbol, pcLibSymbol, pcPin) : NULL;
	if ((pcPin == NULL) || StrEmpty(szName) || IsKicadSchemPowerFlag(szName, pcLibSymbol))
	{
		msCounts.uiSkipped++;
		return true;
	}

	mpcFile->GetPinPosition(&sPosition, pcSymbol, pcPin);
	pcSubnet = mpcNetlist->GetPinSubnet(pcSymbol, pcPin);
	pcNet = (pcSubnet != NULL) ? pcSubnet->GetNet() : NULL;
	if ((pcNet != NULL) && (GetNetRegulatedVoltage(pcNet) > 0))
	{
		pcTunnel = NewMalloc<CTunnelView>();
		pcTunnel->Init(0, 0, GetKicadSchemRotation(ROT_North, pcSymbol->GetAngle()), pcNet->GetName(), false, mpcFonts);
		uiItem = mcModel.AddItem(pcTunnel, KSLIK_Tunnel, &sPosition, pcTunnel->GetPortView((size)0), pcSymbol);
		AddPinTerminal(pcSymbol, pcPin, pcTunnel->GetPortView((size)0), uiItem);
		msCounts.uiTunnels++;
		return true;
	}

	pcView = NewSupply(szName, IsKicadSchemGroundName(szName) ? 0.0f : GetKicadSchemSupplyVoltage(szName), pcSymbol->GetAngle(), ROT_North, ROT_South);
	uiItem = mcModel.AddItem(pcView, KSLIK_Supply, &sPosition, pcView->GetPortView((size)0), pcSymbol);
	AddPinTerminal(pcSymbol, pcPin, pcView->GetPortView((size)0), uiItem);
	msCounts.uiPowers++;
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// A supply for each net a label names as one ("GND", "3.3V"; see
// IsKicadSchemSupplyLabel) and no power symbol or regulator supplies, as
// a power symbol is: the HCP65's schematics name their supplies with labels.
// It's one for the net, beside the first of its labels on a part of it
// that's drawn, where the label's text is, and wired to that part; the
// labels' tunnels join the rest.  A supply in place of each label's
// tunnel, as a power symbol on each pin is, left too little room between
// the pins tied to it to route the pins beside them (2026-11-26).
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddSupplyLabels(void)
{
	size				i;
	CKicadSchemNet*		pcNet;
	CKicadSchemSubnet*	pcSubnet;
	CKicadSchemLabel*	pcLabel;
	CComponentView*		pcView;
	SFloat64Vec2		sPosition;
	float64				fAngle;
	size				uiItem;

	for (i = 0; i < mpcNetlist->NumNets(); i++)
	{
		pcNet = mpcNetlist->GetNet(i);
		pcLabel = (HasSupply(pcNet) || (GetNetRegulatedVoltage(pcNet) > 0)) ? NULL : FindSupplyLabel(pcNet, &pcSubnet);
		if (pcLabel == NULL)
		{
			continue;
		}

		//Off the label's wire, so it isn't attached to the pin the label (and its tunnel) may be on.
		fAngle = pcLabel->GetAngle() * M_PI / 180.0;
		sPosition = *pcLabel->GetPosition() + SFloat64Vec2(cos(fAngle), -sin(fAngle)) * (KICAD_SCHEM_LOGICIM_GRID * 2);
		pcView = NewSupply(pcLabel->GetText(), GetKicadSchemSupplyLabelVoltage(pcLabel->GetText()), pcLabel->GetAngle(), ROT_West, ROT_West);
		uiItem = mcModel.AddItem(pcView, KSLIK_Supply, &sPosition, pcView->GetPortView((size)0), NULL);
		mcModel.AddTerminal(pcSubnet, &sPosition, pcView->GetPortView((size)0), uiItem);
		msCounts.uiPowers++;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a power symbol already supplies the net.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::HasSupply(CKicadSchemNet* pcNet)
{
	size	j;
	size	k;

	for (j = 0; j < pcNet->NumSubnets(); j++)
	{
		for (k = mcModel.FirstSubnetTerminal(pcNet->GetSubnet(j)); k != ARRAY_ELEMENT_NOT_FOUND; k = mcModel.NextSubnetTerminal(k))
		{
			if (mcModel.GetItem(mcModel.GetTerminal(k)->uiItem)->eKind == KSLIK_Supply)
			{
				return true;
			}
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The net's first label named as a supply that isn't negative on a part
// of it that's drawn, or of a power rail, and that part (*ppcSubnet).
// NULL if it has none: nothing drawn needs its supply.
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemLabel* CKicadSchemLogicimConverter::FindSupplyLabel(CKicadSchemNet* pcNet, CKicadSchemSubnet** ppcSubnet)
{
	size				j;
	size				k;
	CKicadSchemSubnet*	pcSubnet;
	CKicadSchemLabel*	pcLabel;

	for (j = 0; j < pcNet->NumSubnets(); j++)
	{
		pcSubnet = pcNet->GetSubnet(j);
		if (!IsSubnetDrawn(pcSubnet) && !IsRailNet(pcNet))
		{
			continue;
		}
		for (k = 0; k < pcSubnet->NumLabels(); k++)
		{
			pcLabel = pcSubnet->GetLabel(k);
			//LogiCim's supplies are grounds and positive supplies, so a negative one (the Audio DAC's -5V, for its op
			//amps) stays an ordinary net.
			if (!mpcNetlist->IsBusLabel(pcLabel) && IsKicadSchemSupplyLabel(pcLabel->GetText()) && (GetKicadSchemSupplyLabelVoltage(pcLabel->GetText()) >= 0.0f))
			{
				*ppcSubnet = pcSubnet;
				return pcLabel;
			}
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// A ground for 0V, else a positive supply rising over the import's rise
// time, turned by fAngle from the way each faces upright (where the
// symbol or label has its body).
//
//////////////////////////////////////////////////////////////////////////
CComponentView* CKicadSchemLogicimConverter::NewSupply(const char* szName, float32 fVoltage, float64 fAngle, ERotation eGroundUpright, ERotation ePositiveUpright)
{
	CGroundView*			pcGround;
	CPositivePowerView*		pcPositive;

	if (fVoltage == 0.0f)
	{
		pcGround = NewMalloc<CGroundView>();
		pcGround->Init(0, 0, GetKicadSchemRotation(eGroundUpright, fAngle), szName);
		return pcGround;
	}
	pcPositive = NewMalloc<CPositivePowerView>();
	pcPositive->Init(0, 0, GetKicadSchemRotation(ePositiveUpright, fAngle), szName, fVoltage);
	pcPositive->SetRiseTime(NanosecondsToTime((int64)KICAD_SCHEM_LOGICIM_RISE_TIME));
	return pcPositive;
}


//////////////////////////////////////////////////////////////////////////
//
// The output pin of a voltage regulator: one with an input pin ("VIN",
// "6VIn", *ppcInput), an output pin ("VOUT", "3.3VOut") and a ground (the
// first, *ppcGround).  NULL if the symbol's unit isn't a regulator.
//
//////////////////////////////////////////////////////////////////////////
static CKicadSchemLibPin* GetKicadSchemRegulatorOutput(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, CKicadSchemLibPin** ppcInput, CKicadSchemLibPin** ppcGround)
{
	size					i;
	CKicadSchemLibPin*		pcPin;
	CKicadSchemLibPin*		pcInput;
	CKicadSchemLibPin*		pcOutput;
	CKicadSchemLibPin*		pcGround;

	pcInput = NULL;
	pcOutput = NULL;
	pcGround = NULL;
	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (!pcSymbol->IsUnitPin(pcPin))
		{
			continue;
		}
		if (IsKicadSchemRegulatorPinName(pcPin->GetName(), "OUT"))
		{
			pcOutput = pcPin;
		}
		else if (IsKicadSchemRegulatorPinName(pcPin->GetName(), "IN"))
		{
			pcInput = pcPin;
		}
		else if (IsKicadSchemGroundName(pcPin->GetName()) && (pcGround == NULL))
		{
			pcGround = pcPin;
		}
	}
	*ppcInput = pcInput;
	*ppcGround = pcGround;
	return ((pcInput != NULL) && (pcGround != NULL)) ? pcOutput : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// A voltage regulator (see GetKicadSchemRegulatorOutput), drawn later
// (AddRegulatorSupplies).  Its output's net, on all its output pins if it
// has more than one (a tab), is remembered with the voltage the output pin's name says
// ("3.3VOut") or, if it doesn't say, its net's name ("+3V3"), for
// ApplyRegulatedVoltages.  False (and nothing added) if it isn't a
// regulator or neither says its voltage.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddRegulator(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	CKicadSchemLibPin*		pcOutput;
	CKicadSchemLibPin*		pcInput;
	CKicadSchemLibPin*		pcGround;
	CKicadSchemSubnet*		pcSubnet;
	const char*				szNetName;
	const char*				szVoltage;

	pcOutput = GetKicadSchemRegulatorOutput(pcSymbol, pcLibSymbol, &pcInput, &pcGround);
	if (pcOutput == NULL)
	{
		return false;
	}

	pcSubnet = mpcNetlist->GetPinSubnet(pcSymbol, pcOutput);
	szNetName = ((pcSubnet != NULL) && (pcSubnet->GetNet() != NULL)) ? pcSubnet->GetNet()->GetName() : NULL;
	if (HasKicadSchemVoltage(pcOutput->GetName()))
	{
		szVoltage = pcOutput->GetName();
	}
	else if (HasKicadSchemVoltage(szNetName))
	{
		szVoltage = szNetName;
	}
	else
	{
		return false;
	}
	AddRegulatedNets(pcSymbol, pcLibSymbol, GetKicadSchemSupplyVoltage(szVoltage), pcInput, pcGround);
	msCounts.uiRegulators++;
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The nets of the regulator's output pins driven at fVoltage from its
// input pin's net, grounded by its ground pin's, unless another regulator
// drives them.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddRegulatedNets(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, float32 fVoltage, CKicadSchemLibPin* pcInput, CKicadSchemLibPin* pcGroundPin)
{
	size							i;
	CKicadSchemLibPin*				pcPin;
	CKicadSchemSubnet*				pcSubnet;
	CKicadSchemNet*					pcNet;
	SKicadSchemLogicimRegulated*	psRegulated;
	CKicadSchemNet*					pcFeed;
	CKicadSchemNet*					pcGround;

	pcSubnet = mpcNetlist->GetPinSubnet(pcSymbol, pcInput);
	pcFeed = (pcSubnet != NULL) ? pcSubnet->GetNet() : NULL;
	pcSubnet = mpcNetlist->GetPinSubnet(pcSymbol, pcGroundPin);
	pcGround = (pcSubnet != NULL) ? pcSubnet->GetNet() : NULL;
	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (pcSymbol->IsUnitPin(pcPin) && IsKicadSchemRegulatorPinName(pcPin->GetName(), "OUT"))
		{
			pcSubnet = mpcNetlist->GetPinSubnet(pcSymbol, pcPin);
			pcNet = (pcSubnet != NULL) ? pcSubnet->GetNet() : NULL;
			if ((pcNet != NULL) && (GetNetRegulatedVoltage(pcNet) == 0))
			{
				psRegulated = masRegulated.Add();
				psRegulated->pcNet = pcNet;
				psRegulated->fVoltage = fVoltage;
				psRegulated->pcSymbol = pcSymbol;
				psRegulated->pcOutput = pcPin;
				psRegulated->pcInput = pcInput;
				psRegulated->pcFeed = pcFeed;
				psRegulated->pcGroundPin = pcGroundPin;
				psRegulated->pcGround = pcGround;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The voltage of the regulator whose output drives the net.  0 if none
// does.
//
//////////////////////////////////////////////////////////////////////////
float32 CKicadSchemLogicimConverter::GetNetRegulatedVoltage(CKicadSchemNet* pcNet)
{
	size	i;

	for (i = 0; i < masRegulated.NumElements(); i++)
	{
		if (masRegulated.Get(i)->pcNet == pcNet)
		{
			return masRegulated.Get(i)->fVoltage;
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
//
// The voltage of the regulator whose output the symbol's power pins (its
// power inputs that aren't ground) are on, if any of them are on one.  0
// if none are.
//
//////////////////////////////////////////////////////////////////////////
float32 CKicadSchemLogicimConverter::GetRegulatedVoltage(CKicadSchemSymbol* pcSymbol)
{
	CArrayTemplatePtr<CKicadSchemNet>	apcNets;
	size								i;
	float32								fVoltage;

	apcNets.Init();
	GetPowerPinNets(pcSymbol, false, &apcNets);
	fVoltage = 0;
	for (i = 0; (i < apcNets.NumElements()) && (fVoltage == 0); i++)
	{
		fVoltage = GetNetRegulatedVoltage(apcNets.GetPtr(i));
	}
	apcNets.Kill();
	return fVoltage;
}


//////////////////////////////////////////////////////////////////////////
//
// Each IC made for a symbol whose power pins a regulator's output reaches
// is powered at the regulator's voltage (its implied Vcc).  A part drawn
// as units is one part: if any of its units' power pins are on the
// output, all its units are.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::ApplyRegulatedVoltages(void)
{
	size						i;
	size						j;
	SKicadSchemLogicimItem*		psItem;
	CIntegratedCircuitView*		pcIntegratedCircuit;
	CKicadSchemSymbol*			pcSymbol;
	float32						fVoltage;

	if (masRegulated.NumElements() == 0)
	{
		return;
	}
	for (i = 0; i < mcModel.NumItems(); i++)
	{
		psItem = mcModel.GetItem(i);
		pcIntegratedCircuit = psItem->pcView->GetIntegratedCircuitView();
		if ((psItem->pcSymbol == NULL) || (pcIntegratedCircuit == NULL))
		{
			continue;
		}
		fVoltage = 0;
		for (j = 0; (j < mpcFile->NumSymbols()) && (fVoltage == 0); j++)
		{
			pcSymbol = mpcFile->GetSymbol(j);
			if ((pcSymbol == psItem->pcSymbol) || (StringCompare(pcSymbol->GetReference(), psItem->pcSymbol->GetReference()) == 0))
			{
				fVoltage = GetRegulatedVoltage(pcSymbol);
			}
		}
		if (fVoltage > 0)
		{
			pcIntegratedCircuit->SetImpliedVcc(fVoltage);
		}
	}
}
