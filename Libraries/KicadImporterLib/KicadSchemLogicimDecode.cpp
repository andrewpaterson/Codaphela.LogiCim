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
#include <stdio.h>
#include <stdlib.h>
#include "BaseLib/Logger.h"
#include "BaseLib/FileUtil.h"
#include "BaseLib/StringHelper.h"
#include "StandardLib/Objects.h"
#include "MicroprocessorLib/Pin.h"
#include "MicroprocessorLib/PowerOutPort.h"
#include "MicroprocessorLib/Trace.h"
#include "MicroprocessorLib/Voltage.h"
#include "LogicimLib/TunnelView.h"
#include "LogicimLib/CircuitFonts.h"
#include "KicadSchemSupplyNames.h"
#include "KicadSchemLogicimDecode.h"


//The address bits looked up at once when an address is turned into the combination of address lines it drives.
#define KICAD_SCHEM_DECODE_LOOKUP_BITS	12

//The widest address decoded: every address is walked, so this many bits is 256M addresses a mode.
#define KICAD_SCHEM_DECODE_MAX_WIDTH	28


//////////////////////////////////////////////////////////////////////////
//
// Whether the symbol is a resistor or a resistor network ("R1", "RN5"),
// by its reference's letters.
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemResistor(CKicadSchemSymbol* pcSymbol)
{
	const char*	szReference;
	size		i;

	szReference = pcSymbol->GetReference();
	if ((szReference == NULL) || ((szReference[0] != 'R') && (szReference[0] != 'r')))
	{
		return false;
	}
	i = ((szReference[1] == 'N') || (szReference[1] == 'n')) ? 2 : 1;
	return (szReference[i] >= '0') && (szReference[i] <= '9');
}


//////////////////////////////////////////////////////////////////////////
//
// The net a symbol's pin is on, NULL for none.
//
//////////////////////////////////////////////////////////////////////////
static CKicadSchemNet* GetKicadSchemPinNet(CKicadSchemNetlist* pcNetlist, CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin)
{
	CKicadSchemSubnet*	pcSubnet;

	pcSubnet = pcNetlist->GetPinSubnet(pcSymbol, pcPin);
	return (pcSubnet != NULL) ? pcSubnet->GetNet() : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The address line a board pin's name is ("A12" is 12), -1 if it isn't
// one.
//
//////////////////////////////////////////////////////////////////////////
static int GetKicadSchemAddressBit(const char* szName)
{
	size	i;

	if ((szName[0] != 'A') || (szName[1] == '\0'))
	{
		return -1;
	}
	for (i = 1; szName[i] != '\0'; i++)
	{
		if ((szName[i] < '0') || (szName[i] > '9'))
		{
			return -1;
		}
	}
	return atoi(&szName[1]);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a pin of this type drives its net.
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemDrivingPin(EKicadSchemPinType eType)
{
	return (eType == KSPT_Output) || (eType == KSPT_TriState) || (eType == KSPT_OpenCollector) || (eType == KSPT_OpenEmitter);
}


//////////////////////////////////////////////////////////////////////////
//
// pcFonts are the converter's; pcFamilies give the parts their voltages.
// Neither is held.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::Init(CKicadSchemFile* pcFile, CKicadSchemNetlist* pcNetlist, CCircuitFonts* pcFonts, CFamilyVoltageConfigurationStore* pcFamilies)
{
	mpcFile = pcFile;
	mpcNetlist = pcNetlist;
	mpcFonts = pcFonts;
	mpcFamilies = pcFamilies;
	mcCircuits.Init();
	mcConverter.Init(pcFile, pcNetlist, &mcCircuits, pcFonts);
	mcPaths.Init();
	mpcPath = NULL;
	mpSimulation = NULL;
	mcNetGroups.Init();
	masAddresses.Init();
	masModes.Init();
	masOutputs.Init();
	miAddressWidth = 0;
}


//////////////////////////////////////////////////////////////////////////
//
// The views go before the simulation, as the simulation's timeline must
// outlive the components they made.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::Kill(void)
{
	size	i;

	for (i = 0; i < masOutputs.NumElements(); i++)
	{
		masOutputs.Get(i)->aiRanges.Kill();
		masOutputs.Get(i)->acStates.Kill();
	}
	masOutputs.Kill();
	masModes.Kill();
	masAddresses.Kill();
	mcNetGroups.Kill();
	mcConverter.Kill();
	mcCircuits.Kill();
	if (mpSimulation.IsNotNull())
	{
		ORoot()->Remove(mpSimulation);
		mpSimulation = NULL;
	}
	mcPaths.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The sheet's circuit (named szTypeName) built, wired by its netlist and
// simulated, and its ranges written to pszDest (see the class).  False,
// with the error logged, if it can't be or an output doesn't settle.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimDecode::Decode(const char* szTypeName, CChars* pszDest)
{
	size	uiMode;
	size	uiModes;

	if (!Build(szTypeName))
	{
		return false;
	}
	JoinSeriesResistors();
	AddNetTunnels();
	if (!Simulate())
	{
		return false;
	}
	DriveSupplies();
	if (!FindPins())
	{
		return false;
	}

	uiModes = (size)1 << masModes.NumElements();
	for (uiMode = 0; uiMode < uiModes; uiMode++)
	{
		if (!DecodeMode(uiMode, pszDest))
		{
			return false;
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The converter's parts, without placing or routing.  A bus isn't
// followed (its splitters' bus ends would need tunnels as wide as it).
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimDecode::Build(const char* szTypeName)
{
	SKicadSchemLogicimCounts*	psCounts;
	CChars						szCount;

	if (!mcConverter.Build(szTypeName))
	{
		return gcLogger.Error2(__METHOD__, " Could not build the circuit of [", szTypeName, "].", NULL);
	}
	psCounts = mcConverter.GetCounts();
	if (psCounts->uiSplitters > 0)
	{
		return gcLogger.Error2(__METHOD__, " [", szTypeName, "] has buses, which the decode doesn't follow.", NULL);
	}
	if (psCounts->uiPlaceholders > 0)
	{
		//A placeholder has no logic, so an output it drives never settles and the decode fails there.
		szCount.Init();
		szCount.Append((int64)psCounts->uiPlaceholders);
		gcLogger.Warning2(__METHOD__, " [", szTypeName, "] has ", szCount.Text(), " parts with no logic (placeholders).", NULL);
		szCount.Kill();
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The nets a resistor joins, unless one is a supply's: a chip select
// leaving a board through a series resistor is the same signal on both
// sides, but a pull up isn't its supply.  The pins of one resistor in a
// network have the same name ("R1" on pins 1 and 8).
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::JoinSeriesResistors(void)
{
	size					i;
	size					j;
	size					k;
	CKicadSchemSymbol*		pcSymbol;
	CKicadSchemLibSymbol*	pcLibSymbol;
	CKicadSchemLibPin*		pcPin;
	CKicadSchemLibPin*		pcOther;
	CKicadSchemNet*			pcNet;
	CKicadSchemNet*			pcOtherNet;

	mcNetGroups.AddNum(mpcNetlist->NumNets());
	for (i = 0; i < mpcFile->NumSymbols(); i++)
	{
		pcSymbol = mpcFile->GetSymbol(i);
		pcLibSymbol = mpcFile->GetLibSymbol(pcSymbol);
		if ((pcLibSymbol == NULL) || !IsKicadSchemResistor(pcSymbol))
		{
			continue;
		}
		for (j = 0; j < pcLibSymbol->NumPins(); j++)
		{
			pcPin = pcLibSymbol->GetPin(j);
			pcNet = GetKicadSchemPinNet(mpcNetlist, pcSymbol, pcPin);
			if (!pcSymbol->IsUnitPin(pcPin) || (pcNet == NULL) || IsKicadSchemPowerNetName(pcNet->GetName()))
			{
				continue;
			}
			for (k = j + 1; k < pcLibSymbol->NumPins(); k++)
			{
				pcOther = pcLibSymbol->GetPin(k);
				if (!pcSymbol->IsUnitPin(pcOther) || (StringCompare(pcPin->GetName(), pcOther->GetName()) != 0))
				{
					continue;
				}
				pcOtherNet = GetKicadSchemPinNet(mpcNetlist, pcSymbol, pcOther);
				if ((pcOtherNet != NULL) && !IsKicadSchemPowerNetName(pcOtherNet->GetName()))
				{
					mcNetGroups.Join(GetNetGroup(pcNet), GetNetGroup(pcOtherNet));
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Every component moved apart, and a tunnel named as its net's group on
// each port the converter made for the net, so the wiring joins the
// ports of a net by name (see CCircuitWiring) where it would have joined
// them by the wires LayOut routes.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::AddNetTunnels(void)
{
	CCircuitView*					pcCircuit;
	CKicadSchemLogicimModel*		pcModel;
	SKicadSchemLogicimTerminal*		psTerminal;
	CKicadSchemNet*					pcNet;
	CTunnelView*					pcTunnel;
	SInt32Vec2						sPosition;
	size							i;
	size							uiComponents;

	pcCircuit = mcConverter.GetCircuit();
	uiComponents = pcCircuit->NumComponents();
	for (i = 0; i < uiComponents; i++)
	{
		pcCircuit->GetComponent(i)->SetPosition((int32)(i * KICAD_SCHEM_DECODE_SPACING), 0);
	}

	pcModel = mcConverter.GetModel();
	for (i = 0; i < pcModel->NumTerminals(); i++)
	{
		psTerminal = pcModel->GetTerminal(i);
		pcNet = (psTerminal->pcSubnet != NULL) ? psTerminal->pcSubnet->GetNet() : NULL;
		if (pcNet == NULL)
		{
			continue;
		}
		sPosition = psTerminal->pcPortView->GetGridPosition();
		pcTunnel = NewMalloc<CTunnelView>();
		pcTunnel->Init(sPosition.x, sPosition.y, ROT_East, GetGroupNet(GetNetGroup(pcNet))->GetName(), false, mpcFonts);
		pcCircuit->Add(pcTunnel);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit's simulation, started.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimDecode::Simulate(void)
{
	CCircuitView*	pcCircuit;

	pcCircuit = mcConverter.GetCircuit();
	mpSimulation = OMalloc<CCircuitSimulation>(pcCircuit->GetTypeName());
	ORoot()->Add(mpSimulation);
	mpcPath = mcPaths.AddTop(pcCircuit);
	if (!pcCircuit->CreateSimulation(mpcPath, &mpSimulation, mpcFamilies))
	{
		return gcLogger.Error2(__METHOD__, " Could not simulate [", pcCircuit->GetTypeName(), "].", NULL);
	}
	mpSimulation->GetTimeline()->Run();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// A supply on each net named as one ("GND", "3.3V"), at its voltage: a
// board's inputs tied high or low by a label, with no power symbol, are
// driven by nothing the converter makes.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::DriveSupplies(void)
{
	CKicadSchemLogicimModel*		pcModel;
	SKicadSchemLogicimTerminal*		psTerminal;
	CKicadSchemNet*					pcNet;
	CTrace*							pcTrace;
	CArrayInt						aiSupplied;
	CPowerSource*					pcSupply;
	float32							fVoltage;
	size							i;
	int								iGroup;

	aiSupplied.Init();
	pcModel = mcConverter.GetModel();
	for (i = 0; i < pcModel->NumTerminals(); i++)
	{
		psTerminal = pcModel->GetTerminal(i);
		pcNet = (psTerminal->pcSubnet != NULL) ? psTerminal->pcSubnet->GetNet() : NULL;
		if ((pcNet == NULL) || !IsKicadSchemPowerNetName(pcNet->GetName()))
		{
			continue;
		}
		iGroup = (int)GetNetGroup(pcNet);
		pcTrace = GetPortTrace(psTerminal->pcPortView);
		if ((pcTrace == NULL) || (aiSupplied.Find(iGroup) != ARRAY_ELEMENT_NOT_FOUND))
		{
			continue;
		}
		fVoltage = IsKicadSchemGroundName(pcNet->GetName()) ? 0.0f : GetKicadSchemSupplyVoltage(pcNet->GetName());
		pcSupply = &OMalloc<CPowerSource>(mpSimulation->GetSubcircuitTopSimulation(), pcNet->GetName(), fVoltage);
		pcSupply->GetPowerOutPort()->ConnectLive(pcTrace, mpSimulation->GetTimeline());
		aiSupplied.Add(iGroup);
	}
	aiSupplied.Kill();
	mpSimulation->GetTimeline()->Run();
}


//////////////////////////////////////////////////////////////////////////
//
// The board's pins: an output where a part drives its net (through
// series resistors), an input where only inputs are on it.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimDecode::FindPins(void)
{
	CKicadSchemLogicimModel*	pcModel;
	SKicadSchemLogicimItem*		psItem;
	SKicadSchemLogicimTerminal*	psTerminal;
	SKicadSchemDecodeOutput*	psOutput;
	SKicadSchemNetPin*			psPin;
	CKicadSchemNet*				pcNet;
	CKicadSchemSubnet*			pcSubnet;
	CArrayInt					aiDriven;
	CArrayInt					aiRead;
	CPinView*					pcPinView;
	size						i;
	size						j;
	size						k;
	size						uiGroup;
	size						uiTerminal;

	aiDriven.Init();
	aiRead.Init();
	for (i = 0; i < mpcNetlist->NumNets(); i++)
	{
		aiDriven.Add(0);
		aiRead.Add(0);
	}
	for (i = 0; i < mpcNetlist->NumNets(); i++)
	{
		pcNet = mpcNetlist->GetNet(i);
		uiGroup = mcNetGroups.Find(i);
		for (j = 0; j < pcNet->NumSubnets(); j++)
		{
			pcSubnet = pcNet->GetSubnet(j);
			for (k = 0; k < pcSubnet->NumPins(); k++)
			{
				psPin = pcSubnet->GetPin(k);
				if (IsKicadSchemDrivingPin(psPin->pcPin->GetType()))
				{
					aiDriven.SetValue(uiGroup, 1);
				}
				else if (psPin->pcPin->GetType() == KSPT_Input)
				{
					aiRead.SetValue(uiGroup, 1);
				}
			}
		}
	}

	pcModel = mcConverter.GetModel();
	for (i = 0; i < pcModel->NumItems(); i++)
	{
		psItem = pcModel->GetItem(i);
		uiTerminal = pcModel->FirstItemTerminal(i);
		if ((psItem->eKind != KSLIK_BoardPin) || (uiTerminal == ARRAY_ELEMENT_NOT_FOUND))
		{
			continue;
		}
		psTerminal = pcModel->GetTerminal(uiTerminal);
		pcPinView = (CPinView*)psItem->pcView;
		uiGroup = GetNetGroup(psTerminal->pcSubnet->GetNet());
		if (aiDriven.GetValue(uiGroup))
		{
			psOutput = masOutputs.Add();
			psOutput->pcPinView = pcPinView;
			psOutput->bActiveLow = pcPinView->GetName()[0] == '~';
			psOutput->acStates.Init();
			psOutput->aiRanges.Init();
			psOutput->iStart = -1;
		}
		else if (aiRead.GetValue(uiGroup))
		{
			AddInput(pcPinView);
		}
	}
	aiRead.Kill();
	aiDriven.Kill();

	if (masAddresses.NumElements() == 0)
	{
		return gcLogger.Error2(__METHOD__, " The board has no address lines (inputs named A0, A1...).", NULL);
	}
	if (miAddressWidth > KICAD_SCHEM_DECODE_MAX_WIDTH)
	{
		return gcLogger.Error2(__METHOD__, " The board's address lines are too wide to walk every address.", NULL);
	}
	mpSimulation->GetTimeline()->Run();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// A board pin driven low to start with, by supplies at ground and at the
// pin's Vcc.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::AddInput(CPinView* pcPinView)
{
	SKicadSchemDecodeInput*		psInput;
	CPin*						pcPin;
	CTrace*						pcVcc;
	float32						fVcc;
	int							iBit;

	iBit = GetKicadSchemAddressBit(pcPinView->GetName());
	psInput = (iBit >= 0) ? masAddresses.Add() : masModes.Add();
	if (iBit >= miAddressWidth)
	{
		miAddressWidth = iBit + 1;
	}

	pcPin = (CPin*)pcPinView->GetComponent(mpcPath, &mpSimulation);
	pcVcc = pcPin->GetVoltageCommon()->GetTrace();
	fVcc = (pcVcc != NULL) ? pcVcc->GetVoltage(mpSimulation->GetTime()) : NoVoltage();

	psInput->pcPinView = pcPinView;
	psInput->iBit = iBit;
	psInput->pcLow = &OMalloc<CPowerSource>(mpSimulation->GetSubcircuitTopSimulation(), pcPinView->GetName(), 0.0f);
	psInput->pcHigh = &OMalloc<CPowerSource>(mpSimulation->GetSubcircuitTopSimulation(), pcPinView->GetName(), fVcc);
	psInput->bHigh = false;
	psInput->pcLow->GetPowerOutPort()->ConnectLive(GetPortTrace(pcPinView->GetPortView((size)0)), mpSimulation->GetTimeline());
}


//////////////////////////////////////////////////////////////////////////
//
// Every combination of the address lines in the mode (bit k of uiMode is
// mode pin k's level), each output read once it settles, and the ranges
// they're active over written.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimDecode::DecodeMode(size uiMode, CChars* pszDest)
{
	size						uiCombinations;
	size						i;
	size						j;
	size						uiCombination;
	SKicadSchemDecodeOutput*	psOutput;
	char						cState;
	CChars						szWhere;

	SetInputs(&masModes, uiMode);
	uiCombinations = (size)1 << masAddresses.NumElements();
	for (j = 0; j < masOutputs.NumElements(); j++)
	{
		masOutputs.Get(j)->acStates.Resize(uiCombinations);
	}

	for (i = 0; i < uiCombinations; i++)
	{
		//A Gray code: one address line changes at each step.
		uiCombination = i ^ (i >> 1);
		SetInputs(&masAddresses, uiCombination);
		mpSimulation->GetTimeline()->Run();
		for (j = 0; j < masOutputs.NumElements(); j++)
		{
			psOutput = masOutputs.Get(j);
			cState = ReadOutput(psOutput);
			if (cState == KSDS_Unsettled)
			{
				szWhere.Init();
				AppendMode(uiMode, &szWhere);
				szWhere.Append(" with address lines ");
				szWhere.Append((int32)uiCombination, 2);
				gcLogger.Error2(__METHOD__, " [", psOutput->pcPinView->GetName(), "] is neither high nor low in mode [", szWhere.Text(), "].", NULL);
				szWhere.Kill();
				return false;
			}
			psOutput->acStates.SetValue(uiCombination, cState);
		}
	}
	WriteRanges(uiMode, pszDest);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Input k high if bit k of uiValue is: its other supply connected in
// place of the one driving it.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::SetInputs(CArrayKicadSchemDecodeInput* pasInputs, size uiValue)
{
	size						k;
	SKicadSchemDecodeInput*		psInput;
	bool						bHigh;
	CPowerOutPort*				pcOld;
	CPowerOutPort*				pcNew;
	CTrace*						pcTrace;

	for (k = 0; k < pasInputs->NumElements(); k++)
	{
		psInput = pasInputs->Get(k);
		bHigh = ((uiValue >> k) & 1) != 0;
		if (bHigh == psInput->bHigh)
		{
			continue;
		}
		pcOld = psInput->bHigh ? psInput->pcHigh->GetPowerOutPort() : psInput->pcLow->GetPowerOutPort();
		pcNew = bHigh ? psInput->pcHigh->GetPowerOutPort() : psInput->pcLow->GetPowerOutPort();
		pcTrace = pcOld->GetTrace();
		pcOld->Disconnect(mpSimulation->GetTimeline());
		pcNew->ConnectLive(pcTrace, mpSimulation->GetTimeline());
		psInput->bHigh = bHigh;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the output is at its active level, as its pin reads its trace.
//
//////////////////////////////////////////////////////////////////////////
char CKicadSchemLogicimDecode::ReadOutput(SKicadSchemDecodeOutput* psOutput)
{
	CPinView*		pcPinView;
	CPort*			pcPort;
	STraceValue		sValue;

	pcPinView = psOutput->pcPinView;
	pcPort = pcPinView->GetPortView((size)0)->GetPort(mpcPath, &mpSimulation, 0);
	sValue = pcPinView->ReadPortValue(pcPinView->GetComponent(mpcPath, &mpSimulation), pcPort, mpSimulation->GetTime());
	if (sValue.IsHigh())
	{
		return psOutput->bActiveLow ? KSDS_Inactive : KSDS_Active;
	}
	if (sValue.IsLow())
	{
		return psOutput->bActiveLow ? KSDS_Active : KSDS_Inactive;
	}
	return KSDS_Unsettled;
}




//////////////////////////////////////////////////////////////////////////
//
// Each output's ranges in the mode, found by walking every address (see
// FindRanges), written a line each (see the class).
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::WriteRanges(size uiMode, CChars* pszDest)
{
	CArrayInt					aiLow;
	CArrayInt					aiHigh;
	CArrayInt					aiRows;
	int							iLowBits;
	size						k;
	SKicadSchemDecodeOutput*	psOutput;
	CChars						szMode;

	iLowBits = (miAddressWidth < KICAD_SCHEM_DECODE_LOOKUP_BITS) ? miAddressWidth : KICAD_SCHEM_DECODE_LOOKUP_BITS;
	aiLow.Init();
	aiHigh.Init();
	aiRows.Init();
	MakeAddressLookups(&aiLow, &aiHigh, iLowBits);
	FindRows(&aiRows);
	FindRanges(&aiLow, &aiHigh, iLowBits, &aiRows);
	aiRows.Kill();
	aiHigh.Kill();
	aiLow.Kill();

	szMode.Init();
	AppendMode(uiMode, &szMode);
	for (k = 0; k < masOutputs.NumElements(); k++)
	{
		psOutput = masOutputs.Get(k);
		WriteOutputRanges(szMode.Text(), psOutput, pszDest);
		psOutput->aiRanges.Kill();
		psOutput->aiRanges.Init();
	}
	szMode.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The combination of address lines (bit k for address line k) that each
// value of an address's low iLowBits bits drives, and each value of the
// bits above them, so an address's combination is two lookups.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::MakeAddressLookups(CArrayInt* paiLow, CArrayInt* paiHigh, int iLowBits)
{
	size	i;
	size	k;
	int		iBit;

	for (i = 0; i < ((size)1 << iLowBits); i++)
	{
		paiLow->Add(0);
	}
	for (i = 0; i < ((size)1 << (miAddressWidth - iLowBits)); i++)
	{
		paiHigh->Add(0);
	}
	for (k = 0; k < masAddresses.NumElements(); k++)
	{
		iBit = masAddresses.Get(k)->iBit;
		if (iBit < iLowBits)
		{
			for (i = 0; i < paiLow->NumElements(); i++)
			{
				if (((i >> iBit) & 1) != 0)
				{
					paiLow->SetValue(i, paiLow->GetValue(i) | (1 << k));
				}
			}
		}
		else
		{
			for (i = 0; i < paiHigh->NumElements(); i++)
			{
				if (((i >> (iBit - iLowBits)) & 1) != 0)
				{
					paiHigh->SetValue(i, paiHigh->GetValue(i) | (1 << k));
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each combination's row: the first combination whose outputs are all in
// the same states.  A board has few rows, so walking the addresses mostly
// compares row numbers.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::FindRows(CArrayInt* paiRows)
{
	size	uiCombinations;
	size	uiOutputs;
	size	i;
	size	j;
	size	k;
	int		iRow;

	uiOutputs = masOutputs.NumElements();
	uiCombinations = (size)1 << masAddresses.NumElements();
	for (i = 0; i < uiCombinations; i++)
	{
		iRow = (int)i;
		for (j = 0; (j < i) && (iRow == (int)i); j++)
		{
			if (paiRows->GetValue(j) != (int)j)
			{
				continue;
			}
			for (k = 0; k < uiOutputs; k++)
			{
				if (masOutputs.Get(k)->acStates.GetValue(i) != masOutputs.Get(k)->acStates.GetValue(j))
				{
					break;
				}
			}
			if (k == uiOutputs)
			{
				iRow = (int)j;
			}
		}
		paiRows->Add(iRow);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Every address walked, past the last so the open ranges close: the
// address lines the board hasn't make no difference, so an address's
// outputs are those of the combination of the lines it has.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::FindRanges(CArrayInt* paiLow, CArrayInt* paiHigh, int iLowBits, CArrayInt* paiRows)
{
	int							iAddresses;
	int							iAddress;
	int							iLowMask;
	int							iCombination;
	int							iRow;
	int							iLastRow;
	size						k;
	SKicadSchemDecodeOutput*	psOutput;

	iAddresses = 1 << miAddressWidth;
	iLowMask = (1 << iLowBits) - 1;
	iLastRow = -1;
	for (iAddress = 0; iAddress <= iAddresses; iAddress++)
	{
		if (iAddress < iAddresses)
		{
			iCombination = paiLow->GetValue(iAddress & iLowMask) | paiHigh->GetValue(iAddress >> iLowBits);
			iRow = paiRows->GetValue(iCombination);
		}
		else
		{
			iCombination = 0;
			iRow = -1;
		}
		if (iRow == iLastRow)
		{
			continue;
		}
		for (k = 0; k < masOutputs.NumElements(); k++)
		{
			psOutput = masOutputs.Get(k);
			SetActive(psOutput, iAddress, (iRow >= 0) && (psOutput->acStates.GetValue(iCombination) == KSDS_Active));
		}
		iLastRow = iRow;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The output is active from iAddress or not: a range starts or ends.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::SetActive(SKicadSchemDecodeOutput* psOutput, int iAddress, bool bActive)
{
	if (bActive && (psOutput->iStart < 0))
	{
		psOutput->iStart = iAddress;
	}
	else if (!bActive && (psOutput->iStart >= 0))
	{
		psOutput->aiRanges.Add(psOutput->iStart);
		psOutput->aiRanges.Add(iAddress - 1);
		psOutput->iStart = -1;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A line for each of the output's ranges, or for each run of ranges as
// long as each other at the same distance apart (a register repeating
// through the board's undecoded address lines).
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::WriteOutputRanges(const char* szMode, SKicadSchemDecodeOutput* psOutput, CChars* pszDest)
{
	CArrayInt*	paiRanges;
	size		uiRanges;
	size		i;
	size		j;
	int			iFirst;
	int			iLength;
	int			iEvery;

	paiRanges = &psOutput->aiRanges;
	uiRanges = paiRanges->NumElements() / 2;
	for (i = 0; i < uiRanges; i = j)
	{
		iFirst = paiRanges->GetValue(i * 2);
		iLength = paiRanges->GetValue(i * 2 + 1) - iFirst;
		iEvery = (i + 1 < uiRanges) ? paiRanges->GetValue(i * 2 + 2) - iFirst : 0;
		for (j = i + 1; j < uiRanges; j++)
		{
			if ((paiRanges->GetValue(j * 2 + 1) - paiRanges->GetValue(j * 2) != iLength) || (paiRanges->GetValue(j * 2) - paiRanges->GetValue(j * 2 - 2) != iEvery))
			{
				break;
			}
		}

		pszDest->Append(szMode);
		pszDest->Append('\t');
		pszDest->Append(psOutput->pcPinView->GetName());
		pszDest->Append('\t');
		AppendAddress(iFirst, pszDest);
		pszDest->Append('\t');
		AppendAddress(iFirst + iLength, pszDest);
		if (j > i + 1)
		{
			pszDest->Append('\t');
			AppendAddress(iEvery, pszDest);
			pszDest->Append('\t');
			AppendAddress(paiRanges->GetValue(j * 2 - 1), pszDest);
		}
		pszDest->AppendNewLine();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each mode pin's name and level: "Kernal Mode=1,RAM Swap=0".
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::AppendMode(size uiMode, CChars* psz)
{
	size	k;

	for (k = 0; k < masModes.NumElements(); k++)
	{
		if (k > 0)
		{
			psz->Append(',');
		}
		psz->Append(masModes.Get(k)->pcPinView->GetName());
		psz->Append(((uiMode >> k) & 1) ? "=1" : "=0");
	}
}


//////////////////////////////////////////////////////////////////////////
//
// $ and hexadecimal, as wide as the address lines: "$10000F".
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimDecode::AppendAddress(int iAddress, CChars* psz)
{
	char	sz[16];

	snprintf(sz, sizeof(sz), "$%0*X", (miAddressWidth + 3) / 4, iAddress);
	psz->Append(sz);
}


//////////////////////////////////////////////////////////////////////////
//
// The net's index in the netlist.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimDecode::FindNet(CKicadSchemNet* pcNet)
{
	size	i;

	for (i = 0; i < mpcNetlist->NumNets(); i++)
	{
		if (mpcNetlist->GetNet(i) == pcNet)
		{
			return i;
		}
	}
	return ARRAY_ELEMENT_NOT_FOUND;
}


//////////////////////////////////////////////////////////////////////////
//
// The group of nets joined by series resistors the net is in: the index
// of the group's first net.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimDecode::GetNetGroup(CKicadSchemNet* pcNet)
{
	return mcNetGroups.Find(FindNet(pcNet));
}


//////////////////////////////////////////////////////////////////////////
//
// The net that names a group: its tunnels are named as it.
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemNet* CKicadSchemLogicimDecode::GetGroupNet(size uiGroup)
{
	return mpcNetlist->GetNet(uiGroup);
}


//////////////////////////////////////////////////////////////////////////
//
// The trace a port view's first bit is on in the simulation, NULL if its
// view made no component (a tunnel).
//
//////////////////////////////////////////////////////////////////////////
CTrace* CKicadSchemLogicimDecode::GetPortTrace(CPortView* pcPortView)
{
	CPort*	pcPort;

	pcPort = pcPortView->GetPort(mpcPath, &mpSimulation, 0);
	return (pcPort != NULL) ? pcPort->GetTrace() : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The decode of the schematic's sheet (see CKicadSchemLogicimDecode),
// with the parts' families from szPropagationTimes and the fonts the
// converter measures with from szFontDirectory, appended to pszDest.
// The objects must be initialised (ObjectsInit).  False, with the error
// logged, if it can't be decoded.
//
//////////////////////////////////////////////////////////////////////////
bool DecodeKicadSchemFile(const char* szSchematic, const char* szPropagationTimes, const char* szFontDirectory, CChars* pszDest)
{
	CKicadSchemFile						cFile;
	CKicadSchemNetlist					cNetlist;
	CFamilyVoltageConfigurationStore	cFamilies;
	Ptr<CCircuitFonts>					pFonts;
	CKicadSchemLogicimDecode			cDecode;
	CFileUtil							cFileUtil;
	CChars								szTypeName;
	bool								bResult;

	pFonts = LoadCircuitFonts(szFontDirectory, CCO_Unknown);
	ORoot()->Add(pFonts);
	cFamilies.Init();
	cFile.Init();
	cNetlist.Init(&cFile);
	szTypeName.Init(szSchematic);
	cFileUtil.RemovePath(&szTypeName);
	cFileUtil.RemoveExtension(&szTypeName);

	bResult = pFonts->HasFamilies() && cFamilies.Load(szPropagationTimes);
	if (!bResult)
	{
		gcLogger.Error2(__METHOD__, " Could not load the fonts in [", szFontDirectory, "] or the families in [", szPropagationTimes, "].", NULL);
	}
	else
	{
		bResult = cFile.Load((char*)szSchematic) && cNetlist.Build();
		if (!bResult)
		{
			gcLogger.Error2(__METHOD__, " Could not read [", szSchematic, "].", NULL);
		}
	}
	if (bResult)
	{
		cDecode.Init(&cFile, &cNetlist, &pFonts, &cFamilies);
		bResult = cDecode.Decode(szTypeName.Text(), pszDest);
		cDecode.Kill();
	}

	szTypeName.Kill();
	cNetlist.Kill();
	cFile.Kill();
	cFamilies.Kill();
	ORoot()->Remove(pFonts);
	pFonts = NULL;
	return bResult;
}
