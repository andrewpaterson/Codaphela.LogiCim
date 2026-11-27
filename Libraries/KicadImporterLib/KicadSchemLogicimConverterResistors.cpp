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
#include "MicroprocessorLib/RCFilterPins.h"
#include "MicroprocessorLib/Pull.h"
#include "LogicimLib/TunnelView.h"
#include "KicadSchemSupplyNames.h"
#include "KicadSchemLogicPins.h"
#include "KicadSchemLogicimConverter.h"
#include "KicadSchemLogicimConverterInternal.h"


//////////////////////////////////////////////////////////////////////////
//
// A resistor with one end on a supply is a pull (AddPulls).  A series
// resistor (neither end on a supply, nor a bussed network's) is a chip
// select or bus line leaving through a termination, the same
// signal on both sides: it joins its nets into one, unless R x C is
// KICAD_SCHEM_LOGICIM_SLOW_RC or more, an RC filter, which becomes an RC
// element (masRCFilters, AddRCFilters) unless filters are joined too
// (SetJoinFilters, for the decode).  C is the capacitance on the side it
// charges (see GetLoadSide, GetNetCapacitance).  One whose value can't be
// read is joined, warned of.  A joined group of nets is named by one of
// them (NameNetGroups) and the resistor's pins get its tunnels, so it's
// drawn and wired as one net.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::JoinSeriesResistors(void)
{
	CArrayKicadSchemPassiveElement	asResistors;
	CArrayKicadSchemPassiveElement	asJoined;
	CArrayKicadSchemPassiveElement	asPulls;
	SKicadSchemPassiveElement*		psResistor;
	size							i;
	int								iLoad;
	float64							fCapacitance;
	bool							bSupply0;
	bool							bSupply1;

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
	asPulls.Init();
	GetKicadSchemPassiveElements(mpcFile, mpcNetlist, true, &asResistors);
	for (i = 0; i < asResistors.NumElements(); i++)
	{
		psResistor = asResistors.Get(i);
		if ((psResistor->apcNets[0] == NULL) || (psResistor->apcNets[1] == NULL) || (psResistor->apcNets[0] == psResistor->apcNets[1]))
		{
			continue;
		}
		bSupply0 = IsSupplyNet(psResistor->apcNets[0]);
		bSupply1 = IsSupplyNet(psResistor->apcNets[1]);
		if (bSupply0 != bSupply1)
		{
			KeepPull(&asPulls, psResistor, bSupply0 ? 0 : 1);
			continue;
		}
		if (bSupply0 || psResistor->bBussed)
		{
			continue;
		}
		if (!IsVoltage((float32)psResistor->fValue))
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
				AddSlowRC(psResistor, iLoad, fCapacitance);
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
	AddRCFilters();
	AddPulls(&asPulls);
	asPulls.Kill();
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
			if ((psCapacitor->apcNets[j] == pcNet) && (psCapacitor->apcNets[1 - j] != NULL) && IsSupplyNet(psCapacitor->apcNets[1 - j]) && IsVoltage((float32)psCapacitor->fValue))
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
		if (IsVoltage((float32)fCapacitance))
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


//////////////////////////////////////////////////////////////////////////
//
// Keeps an RC filter's resistor; one in parallel with a resistor already
// kept, between the same two nets, is the same filter, its resistance
// the two in parallel.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddSlowRC(SKicadSchemPassiveElement* psResistor, int iLoad, float64 fCapacitance)
{
	size					i;
	SKicadSchemLogicimRC*	psRC;
	float64					fOther;

	for (i = 0; i < masRCFilters.NumElements(); i++)
	{
		psRC = masRCFilters.Get(i);
		if (((psRC->sResistor.apcNets[0] == psResistor->apcNets[0]) && (psRC->sResistor.apcNets[1] == psResistor->apcNets[1])) ||
			((psRC->sResistor.apcNets[0] == psResistor->apcNets[1]) && (psRC->sResistor.apcNets[1] == psResistor->apcNets[0])))
		{
			fOther = psRC->sResistor.fValue;
			psRC->sResistor.fValue = (fOther * psResistor->fValue) / (fOther + psResistor->fValue);
			return;
		}
	}
	psRC = masRCFilters.Add();
	psRC->sResistor = *psResistor;
	psRC->iLoad = iLoad;
	psRC->fCapacitance = fCapacitance;
	psRC->eDiode = RCFD_None;
}


//////////////////////////////////////////////////////////////////////////
//
// Each RC filter as an RC element where its resistor is: its input on the
// resistor's driving side, its output on the side it charges, a diode
// across the resistor (GetRCFilterDiode) its fast way.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddRCFilters(void)
{
	CArrayKicadSchemPassiveElement	asDiodes;
	SKicadSchemLogicimRC*			psRC;
	SKicadSchemPassiveElement*		psResistor;
	CRCFilterView*					pcView;
	size							uiItem;
	size							i;
	CChars							szName;

	asDiodes.Init();
	GetKicadSchemDiodeElements(mpcFile, mpcNetlist, &asDiodes);
	for (i = 0; i < masRCFilters.NumElements(); i++)
	{
		psRC = masRCFilters.Get(i);
		psResistor = &psRC->sResistor;
		psRC->eDiode = GetRCFilterDiode(psRC, &asDiodes);
		pcView = NewMalloc<CRCFilterView>();
		szName.Init();
		GetKicadSchemPassiveElementName(&szName, psResistor);
		pcView->Init(0, 0, ROT_East, szName.Text(), (float32)psResistor->fValue, (float32)psRC->fCapacitance, psRC->eDiode);
		szName.Kill();
		uiItem = mcModel.AddItem(pcView, KSLIK_Part, psResistor->pcSymbol->GetPosition(), NULL, psResistor->pcSymbol);
		AddPinTerminal(psResistor->pcSymbol, psResistor->apcPins[1 - psRC->iLoad], pcView->GetPortView(RC_FILTER_PIN_IN), uiItem);
		AddPinTerminal(psResistor->pcSymbol, psResistor->apcPins[psRC->iLoad], pcView->GetPortView(RC_FILTER_PIN_OUT), uiItem);
		msCounts.uiRCFilters++;
	}
	asDiodes.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// A diode across the filter's resistor: its anode on the input, it
// charges the capacitor quickly (rising); on the output, it discharges it
// quickly (falling).
//
//////////////////////////////////////////////////////////////////////////
ERCFilterDiode CKicadSchemLogicimConverter::GetRCFilterDiode(SKicadSchemLogicimRC* psRC, CArrayKicadSchemPassiveElement* pasDiodes)
{
	size						i;
	SKicadSchemPassiveElement*	psDiode;
	CKicadSchemNet*				pcIn;
	CKicadSchemNet*				pcOut;

	pcIn = psRC->sResistor.apcNets[1 - psRC->iLoad];
	pcOut = psRC->sResistor.apcNets[psRC->iLoad];
	for (i = 0; i < pasDiodes->NumElements(); i++)
	{
		psDiode = pasDiodes->Get(i);
		if ((psDiode->apcNets[0] == pcIn) && (psDiode->apcNets[1] == pcOut))
		{
			return RCFD_Rising;
		}
		if ((psDiode->apcNets[0] == pcOut) && (psDiode->apcNets[1] == pcIn))
		{
			return RCFD_Falling;
		}
	}
	return RCFD_None;
}


//////////////////////////////////////////////////////////////////////////
//
// A resistor from a supply (apcNets[iSupply]) to another net, kept to be
// made a pull (AddPulls) once the series resistors have joined their nets,
// its supply's end second.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::KeepPull(CArrayKicadSchemPassiveElement* pasPulls, SKicadSchemPassiveElement* psResistor, int iSupply)
{
	SKicadSchemPassiveElement*	psPull;

	psPull = pasPulls->Add();
	*psPull = *psResistor;
	if (iSupply == 0)
	{
		psPull->apcPins[0] = psResistor->apcPins[1];
		psPull->apcPins[1] = psResistor->apcPins[0];
		psPull->apcNets[0] = psResistor->apcNets[1];
		psPull->apcNets[1] = psResistor->apcNets[0];
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each resistor from a supply (apcNets[1]) to a net something made reads,
// or a connector takes off the board, as a pull: up to the supply's
// voltage, or down for a ground, on the supply's rail if it is one, else
// with a supply of its own.  It carries its net's capacitance, all the
// nets series resistors joined it to (GetGroupCapacitance), for the curve
// its net takes when it's let go.  One whose value can't be read is 10k,
// warned of.  To a net nothing reads (an LED's) it's left out.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddPulls(CArrayKicadSchemPassiveElement* pasPulls)
{
	size	i;

	for (i = 0; i < pasPulls->NumElements(); i++)
	{
		AddPull(pasPulls->Get(i));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The pull of a resistor from its supply, apcNets[1].
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddPull(SKicadSchemPassiveElement* psResistor)
{
	CKicadSchemNet*	pcSignal;
	CKicadSchemNet*	pcSupply;
	float32			fResistance;
	float32			fVoltage;
	CPullView*		pcView;
	size			uiItem;
	CChars			szName;

	pcSignal = psResistor->apcNets[0];
	pcSupply = psResistor->apcNets[1];
	if (!IsNetRead(pcSignal))
	{
		msCounts.uiLeftOutPulls++;
		return;
	}
	fResistance = (float32)psResistor->fValue;
	if (!IsVoltage(fResistance))
	{
		gcLogger.Warning2(__METHOD__, " Pull [", psResistor->pcSymbol->GetReference(), "]'s value [", psResistor->pcSymbol->GetValue(), "] can't be read: it's 10k.", NULL);
		msCounts.uiUnreadResistors++;
		fResistance = 10000.0f;
	}
	fVoltage = GetSupplyNetVoltage(pcSupply);

	pcView = NewMalloc<CPullView>();
	szName.Init();
	GetKicadSchemPassiveElementName(&szName, psResistor);
	pcView->Init(0, 0, ROT_East, szName.Text(), fResistance, (fVoltage > 0.0f) ? PD_Up : PD_Down, fVoltage);
	szName.Kill();
	if (IsRailNet(pcSupply))
	{
		pcView->SetRail(pcSupply->GetName());
	}
	pcView->SetCapacitance((float32)GetGroupCapacitance(pcSignal));
	uiItem = mcModel.AddItem(pcView, KSLIK_Part, psResistor->pcSymbol->GetPosition(), NULL, psResistor->pcSymbol);
	AddPinTerminal(psResistor->pcSymbol, psResistor->apcPins[0], pcView->GetPortView(PULL_PIN_SIGNAL), uiItem);
	msCounts.uiPulls++;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a pin of logic made from the sheet (a part, a board drawn on it,
// a connector's board pin) is on the net, other than a power pin.  A
// placeholder's (an analog part's, a socket's) isn't logic.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsNetRead(CKicadSchemNet* pcNet)
{
	size					i;
	size					j;
	CKicadSchemSubnet*		pcSubnet;
	SKicadSchemNetPin*		psPin;
	EKicadSchemPinType		eType;
	size*					puiItem;

	for (i = 0; i < pcNet->NumSubnets(); i++)
	{
		pcSubnet = pcNet->GetSubnet(i);
		for (j = 0; j < pcSubnet->NumPins(); j++)
		{
			psPin = pcSubnet->GetPin(j);
			eType = psPin->pcPin->GetType();
			puiItem = mcSymbolItems.Get(&psPin->pcSymbol);
			if ((puiItem == NULL) || (eType == KSPT_PowerIn) || (eType == KSPT_PowerOut))
			{
				continue;
			}
			if ((mcModel.GetItem(*puiItem)->eKind != KSLIK_Subcircuit) || (GetKicadSchemBoardSchematic(psPin->pcSymbol, mpcFile->GetLibSymbol(psPin->pcSymbol)) != NULL))
			{
				return true;
			}
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// A supply net's voltage, from its label or power symbol's name ("3.3V",
// "+5V", "GND"), or the regulator that drives it.
//
//////////////////////////////////////////////////////////////////////////
float32 CKicadSchemLogicimConverter::GetSupplyNetVoltage(CKicadSchemNet* pcNet)
{
	if (IsKicadSchemSupplyLabel(pcNet->GetName()))
	{
		return GetKicadSchemSupplyLabelVoltage(pcNet->GetName());
	}
	if (IsKicadSchemPowerNetName(pcNet->GetName()))
	{
		return GetKicadSchemSupplyVoltage(pcNet->GetName());
	}
	return GetNetRegulatedVoltage(pcNet);
}


//////////////////////////////////////////////////////////////////////////
//
// The capacitance of the net and every net series resistors joined it to.
//
//////////////////////////////////////////////////////////////////////////
float64 CKicadSchemLogicimConverter::GetGroupCapacitance(CKicadSchemNet* pcNet)
{
	size				i;
	CKicadSchemNet*		pcGroup;
	CKicadSchemNet*		pcOther;
	float64				fCapacitance;

	pcGroup = mcModel.GetGroup(pcNet);
	fCapacitance = 0;
	for (i = 0; i < mpcNetlist->NumNets(); i++)
	{
		pcOther = mpcNetlist->GetNet(i);
		if (mcModel.GetGroup(pcOther) == pcGroup)
		{
			fCapacitance += GetNetCapacitance(pcOther);
		}
	}
	return fCapacitance;
}
