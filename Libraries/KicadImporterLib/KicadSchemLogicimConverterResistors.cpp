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
#include "BaseLib/Logger.h"
#include "BaseLib/StringHelper.h"
#include "MicroprocessorLib/Voltage.h"
#include "MicroprocessorLib/FamilyVoltageConfigurationStore.h"
#include "LogicimLib/TunnelView.h"
#include "KicadSchemSupplyNames.h"
#include "KicadSchemLogicPins.h"
#include "KicadSchemLogicimConverter.h"
#include "KicadSchemLogicimConverterInternal.h"


//////////////////////////////////////////////////////////////////////////
//
// A series resistor (neither end on a supply; a pull is PullsPlan.txt's)
// is a chip select or bus line leaving through a termination, the same
// signal on both sides: it joins its nets into one, unless R x C is
// KICAD_SCHEM_LOGICIM_SLOW_RC or more, an RC filter, which is kept apart
// (masSlowRCs) unless filters are joined too (SetJoinFilters, for the
// decode).  C is the capacitance on the side it charges (see
// GetLoadSide, GetNetCapacitance).  One whose value can't be read is
// joined, warned of.  A joined group of nets is named by one of them
// (NameNetGroups) and the resistor's pins get its tunnels, so it's drawn
// and wired as one net.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::JoinSeriesResistors(void)
{
	CArrayKicadSchemPassiveElement	asResistors;
	CArrayKicadSchemPassiveElement	asJoined;
	SKicadSchemPassiveElement*		psResistor;
	SKicadSchemLogicimRC*			psRC;
	size							i;
	int								iLoad;
	float64							fCapacitance;
	CChars							sz;

	for (i = 0; i < mcModel.NumItems(); i++)
	{
		if ((mcModel.GetItem(i)->pcSymbol != NULL) && !mcSymbolItems.HasKey(&mcModel.GetItem(i)->pcSymbol))
		{
			mcSymbolItems.Put(&mcModel.GetItem(i)->pcSymbol, &i);
		}
	}
	GetKicadSchemPassiveElements(mpcFile, mpcNetlist, false, &masCapacitors);
	asResistors.Init();
	asJoined.Init();
	GetKicadSchemPassiveElements(mpcFile, mpcNetlist, true, &asResistors);
	for (i = 0; i < asResistors.NumElements(); i++)
	{
		psResistor = asResistors.Get(i);
		if ((psResistor->apcNets[0] == NULL) || (psResistor->apcNets[1] == NULL) || (psResistor->apcNets[0] == psResistor->apcNets[1]) || IsSupplyNet(psResistor->apcNets[0]) || IsSupplyNet(psResistor->apcNets[1]))
		{
			continue;
		}
		if (!IsVoltage(psResistor->fValue))
		{
			gcLogger.Warning2(__METHOD__, " Series resistor [", psResistor->pcSymbol->GetReference(), "]'s value [", psResistor->pcSymbol->GetValue(), "] can't be read: it joins its nets.", NULL);
			msCounts.uiUnreadResistors++;
		}
		else
		{
			iLoad = GetLoadSide(psResistor);
			fCapacitance = GetNetCapacitance(psResistor->apcNets[iLoad]);
			if (!mbJoinFilters && (psResistor->fValue * fCapacitance >= KICAD_SCHEM_LOGICIM_SLOW_RC))
			{
				psRC = masSlowRCs.Add();
				psRC->sResistor = *psResistor;
				psRC->iLoad = iLoad;
				psRC->fCapacitance = fCapacitance;
				sz.Init();
				sz.Append((int64)(psResistor->fValue * fCapacitance * 1e9 + 0.5));
				gcLogger.Warning2(__METHOD__, " Series resistor [", psResistor->pcSymbol->GetReference(), "] makes an RC filter of [", sz.Text(), "ns]: its nets are left apart.", NULL);
				sz.Kill();
				msCounts.uiSlowRCs++;
				continue;
			}
		}
		mcModel.JoinNets(psResistor->apcNets[0], psResistor->apcNets[1]);
		asJoined.Add(psResistor);
		msCounts.uiSeriesResistors++;
	}

	NameNetGroups(&asJoined);
	for (i = 0; i < asJoined.NumElements(); i++)
	{
		AddResistorTunnels(asJoined.Get(i));
	}
	asJoined.Kill();
	asResistors.Kill();
	mcSymbolItems.ReInit();
}


//////////////////////////////////////////////////////////////////////////
//
// A supply's net: a power symbol's or a supply label's name, or a power
// rail (see FindRailNets).
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsSupplyNet(CKicadSchemNet* pcNet)
{
	return IsKicadSchemPowerNetName(pcNet->GetName()) || IsKicadSchemSupplyLabel(pcNet->GetName()) || IsRailNet(pcNet);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a part made from a symbol drives the net: one of its pins on it
// is an output.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsNetDriven(CKicadSchemNet* pcNet)
{
	size					i;
	size					j;
	CKicadSchemSubnet*		pcSubnet;
	SKicadSchemNetPin*		psPin;

	for (i = 0; i < pcNet->NumSubnets(); i++)
	{
		pcSubnet = pcNet->GetSubnet(i);
		for (j = 0; j < pcSubnet->NumPins(); j++)
		{
			psPin = pcSubnet->GetPin(j);
			if (mcSymbolItems.HasKey(&psPin->pcSymbol) && IsKicadSchemOutputPinType(psPin->pcPin->GetType()))
			{
				return true;
			}
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether one of the net's pins is a connector's (a board pin), where it
// leaves the board.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::HasNetConnector(CKicadSchemNet* pcNet)
{
	size					i;
	size					j;
	CKicadSchemSubnet*		pcSubnet;
	size*					puiItem;

	for (i = 0; i < pcNet->NumSubnets(); i++)
	{
		pcSubnet = pcNet->GetSubnet(i);
		for (j = 0; j < pcSubnet->NumPins(); j++)
		{
			puiItem = mcSymbolItems.Get(&pcSubnet->GetPin(j)->pcSymbol);
			if ((puiItem != NULL) && (mcModel.GetItem(*puiItem)->eKind == KSLIK_BoardPin))
			{
				return true;
			}
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The side a series resistor charges: the one its driver isn't on, or if
// that can't be told (both or neither driven) the one with the more
// capacitance.
//
//////////////////////////////////////////////////////////////////////////
int CKicadSchemLogicimConverter::GetLoadSide(SKicadSchemPassiveElement* psResistor)
{
	bool	bDriven0;
	bool	bDriven1;

	bDriven0 = IsNetDriven(psResistor->apcNets[0]);
	bDriven1 = IsNetDriven(psResistor->apcNets[1]);
	if (bDriven0 != bDriven1)
	{
		return bDriven0 ? 1 : 0;
	}
	return (GetNetCapacitance(psResistor->apcNets[1]) > GetNetCapacitance(psResistor->apcNets[0])) ? 1 : 0;
}


//////////////////////////////////////////////////////////////////////////
//
// The capacitance on a net, in farads: the capacitors drawn from it to a
// supply, or where there are none an estimate of what's on it: its parts'
// pins that aren't outputs or power, each at its family's input
// capacitance, its wiring and its connector pins (see the
// KICAD_SCHEM_LOGICIM capacitances).
//
//////////////////////////////////////////////////////////////////////////
float64 CKicadSchemLogicimConverter::GetNetCapacitance(CKicadSchemNet* pcNet)
{
	size						i;
	size						j;
	SKicadSchemPassiveElement*	psCapacitor;
	CKicadSchemSubnet*			pcSubnet;
	float64						fCapacitance;

	fCapacitance = 0;
	for (i = 0; i < masCapacitors.NumElements(); i++)
	{
		psCapacitor = masCapacitors.Get(i);
		for (j = 0; j < 2; j++)
		{
			if ((psCapacitor->apcNets[j] == pcNet) && (psCapacitor->apcNets[1 - j] != NULL) && IsSupplyNet(psCapacitor->apcNets[1 - j]) && IsVoltage(psCapacitor->fValue))
			{
				fCapacitance += psCapacitor->fValue;
			}
		}
	}
	if (fCapacitance > 0)
	{
		return fCapacitance;
	}

	fCapacitance = KICAD_SCHEM_LOGICIM_WIRING_CAPACITANCE;
	for (i = 0; i < pcNet->NumSubnets(); i++)
	{
		pcSubnet = pcNet->GetSubnet(i);
		for (j = 0; j < pcSubnet->NumPins(); j++)
		{
			fCapacitance += GetPinCapacitance(pcSubnet->GetPin(j));
		}
	}
	return fCapacitance;
}


//////////////////////////////////////////////////////////////////////////
//
// What a pin adds to its net's capacitance: a connector pin the
// backplane's, an input of a part its family's, a placeholder's pin the
// default input's; an output, a power pin or a part left out nothing.
//
//////////////////////////////////////////////////////////////////////////
float64 CKicadSchemLogicimConverter::GetPinCapacitance(SKicadSchemNetPin* psPin)
{
	size*						puiItem;
	SKicadSchemLogicimItem*		psItem;
	CIntegratedCircuitView*		pcIntegratedCircuit;
	CFamilyVoltageConfiguration*	pcFamily;
	float64						fCapacitance;
	EKicadSchemPinType			eType;

	puiItem = mcSymbolItems.Get(&psPin->pcSymbol);
	eType = psPin->pcPin->GetType();
	if ((puiItem == NULL) || IsKicadSchemOutputPinType(eType) || (eType == KSPT_PowerIn) || (eType == KSPT_PowerOut))
	{
		return 0;
	}
	psItem = mcModel.GetItem(*puiItem);
	if (psItem->eKind == KSLIK_BoardPin)
	{
		return KICAD_SCHEM_LOGICIM_CONNECTOR_CAPACITANCE;
	}
	pcIntegratedCircuit = psItem->pcView->GetIntegratedCircuitView();
	if ((pcIntegratedCircuit != NULL) && (mpcFamilies != NULL))
	{
		pcFamily = mpcFamilies->Get(pcIntegratedCircuit->GetFamily());
		fCapacitance = (pcFamily != NULL) ? pcFamily->GetInputCapacitance() : NoVoltage();
		if (IsVoltage(fCapacitance))
		{
			return fCapacitance * 1e-12;
		}
	}
	return KICAD_SCHEM_LOGICIM_INPUT_CAPACITANCE;
}


//////////////////////////////////////////////////////////////////////////
//
// Each group of joined nets named by one of them: one a label names, on
// the connector's side if more than one is, else the driven side's.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::NameNetGroups(CArrayKicadSchemPassiveElement* pasJoined)
{
	size				i;
	size				j;
	CKicadSchemNet*		pcNet;
	CKicadSchemNet*		pcNaming;
	int					iScore;
	int					iNamingScore;

	for (i = 0; i < pasJoined->NumElements(); i++)
	{
		pcNaming = mcModel.GetGroupNet(pasJoined->Get(i)->apcNets[0]);
		if (pcNaming != mcModel.GetGroup(pcNaming))
		{
			continue;
		}
		iNamingScore = -1;
		for (j = 0; j < mpcNetlist->NumNets(); j++)
		{
			pcNet = mpcNetlist->GetNet(j);
			if (mcModel.GetGroup(pcNet) != mcModel.GetGroup(pasJoined->Get(i)->apcNets[0]))
			{
				continue;
			}
			iScore = ((pcNet->GetNameSource() != KSNNS_Pin) ? 4 : 0) + (HasNetConnector(pcNet) ? 2 : 0) + (IsNetDriven(pcNet) ? 1 : 0);
			if (iScore > iNamingScore)
			{
				iNamingScore = iScore;
				pcNaming = pcNet;
			}
		}
		mcModel.SetGroupNet(pcNaming, pcNaming);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A tunnel named as the resistor's group at each of its pins, facing away
// from the resistor, so each side's wires reach the other's by name.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddResistorTunnels(SKicadSchemPassiveElement* psResistor)
{
	size				i;
	SFloat64Vec2		sPosition;
	CTunnelView*		pcTunnel;
	size				uiItem;

	for (i = 0; i < 2; i++)
	{
		mpcFile->GetPinPosition(&sPosition, psResistor->pcSymbol, psResistor->apcPins[i]);
		pcTunnel = NewMalloc<CTunnelView>();
		pcTunnel->Init(0, 0, GetKicadSchemFacing(psResistor->pcSymbol->GetPosition(), &sPosition), mcModel.GetNetName(psResistor->apcNets[i]), false, mpcFonts);
		uiItem = mcModel.AddItem(pcTunnel, KSLIK_Tunnel, &sPosition, pcTunnel->GetPortView((size)0), NULL);
		mcModel.AddTerminal(mpcNetlist->GetPinSubnet(psResistor->pcSymbol, psResistor->apcPins[i]), &sPosition, pcTunnel->GetPortView((size)0), uiItem);
		msCounts.uiTunnels++;
	}
}

