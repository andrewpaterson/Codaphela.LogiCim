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
// A ground or positive supply with its port on the power symbol's pin.
// Power flags are left out.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddPower(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	size					i;
	CKicadSchemLibPin*		pcPin;
	char*					szName;
	CGroundView*			pcGround;
	CPositivePowerView*		pcPositive;
	CComponentView*			pcView;
	SFloat64Vec2			sPosition;
	size					uiItem;

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
	if ((pcPin == NULL) || StrEmpty(szName) || (StringCompare(szName, "PWR_FLAG") == 0) || (strstr(pcLibSymbol->GetName(), "PWR_FLAG") != NULL))
	{
		msCounts.uiSkipped++;
		return true;
	}

	if (IsGroundName(szName))
	{
		pcGround = NewMalloc<CGroundView>();
		pcGround->Init(0, 0, GetKicadSchemRotation(ROT_North, pcSymbol->GetAngle()), szName);
		pcView = pcGround;
	}
	else
	{
		pcPositive = NewMalloc<CPositivePowerView>();
		pcPositive->Init(0, 0, GetKicadSchemRotation(ROT_South, pcSymbol->GetAngle()), szName, GetSupplyVoltage(szName));
		pcView = pcPositive;
	}

	mpcFile->GetPinPosition(&sPosition, pcSymbol, pcPin);
	uiItem = mcModel.AddItem(pcView, KSLIK_Supply, &sPosition, pcView->GetPortView((size)0), pcSymbol);
	AddPinTerminal(pcSymbol, pcPin, pcView->GetPortView((size)0), uiItem);
	msCounts.uiPowers++;
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the pin's name, ignoring case, ends with szEnd and is either
// szEnd alone, starts with a V ("VOUT"), or has a voltage ("3.3VOut").
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemRegulatorPinName(const char* szName, const char* szEnd)
{
	size	uiLength;
	size	uiEnd;
	size	i;

	uiLength = StrLen(szName);
	uiEnd = StrLen(szEnd);
	if ((uiLength < uiEnd) || (StringInsensitiveCompare(&szName[uiLength - uiEnd], szEnd) != 0))
	{
		return false;
	}
	if ((uiLength == uiEnd) || (szName[0] == 'V') || (szName[0] == 'v'))
	{
		return true;
	}
	for (i = 0; i < uiLength - uiEnd; i++)
	{
		if ((szName[i] >= '0') && (szName[i] <= '9'))
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the text has a digit, so it can say a voltage.
//
//////////////////////////////////////////////////////////////////////////
static bool HasKicadSchemVoltage(const char* szText)
{
	size	i;

	for (i = 0; (szText != NULL) && (szText[i] != '\0'); i++)
	{
		if ((szText[i] >= '0') && (szText[i] <= '9'))
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// A voltage regulator (an input pin "VIN", "6VIn"; an output pin "VOUT",
// "3.3VOut"; and a ground) isn't drawn.  Its output's net, on all its
// output pins if it has more than one (a tab), is remembered with the
// voltage the output pin's name says ("3.3VOut") or, if it doesn't say,
// its net's name ("+3V3"), for ApplyRegulatedVoltages.  False (and
// nothing added) if it isn't a regulator or neither says its voltage.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddRegulator(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	size					i;
	CKicadSchemLibPin*		pcPin;
	CKicadSchemLibPin*		pcInput;
	CKicadSchemLibPin*		pcOutput;
	bool					bGround;
	CKicadSchemSubnet*		pcSubnet;
	CKicadSchemNet*			pcNet;
	char*					szNetName;
	const char*				szVoltage;
	float32					fVoltage;
	SKicadSchemLogicimRegulated*	psRegulated;

	pcInput = NULL;
	pcOutput = NULL;
	bGround = false;
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
		else if (IsGroundName(pcPin->GetName()))
		{
			bGround = true;
		}
	}
	if ((pcInput == NULL) || (pcOutput == NULL) || !bGround)
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
	fVoltage = GetSupplyVoltage(szVoltage);

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
			}
		}
	}
	msCounts.uiRegulators++;
	return true;
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
	CKicadSchemLibSymbol*	pcLibSymbol;
	CKicadSchemLibPin*		pcPin;
	CKicadSchemSubnet*		pcSubnet;
	size					i;
	float32					fVoltage;

	pcLibSymbol = mpcFile->GetLibSymbol(pcSymbol);
	if (pcLibSymbol == NULL)
	{
		return 0;
	}
	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (!pcSymbol->IsUnitPin(pcPin) || IsGroundName(pcPin->GetName()))
		{
			continue;
		}
		if ((pcPin->GetType() != KSPT_PowerIn) && !IsKicadSchemPowerPinName(pcPin->GetName()))
		{
			continue;
		}
		pcSubnet = mpcNetlist->GetPinSubnet(pcSymbol, pcPin);
		if ((pcSubnet == NULL) || (pcSubnet->GetNet() == NULL))
		{
			continue;
		}
		fVoltage = GetNetRegulatedVoltage(pcSubnet->GetNet());
		if (fVoltage > 0)
		{
			return fVoltage;
		}
	}
	return 0;
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
