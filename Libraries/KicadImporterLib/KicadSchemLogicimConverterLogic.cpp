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


//Pin names on logic parts that are their power.
static const char* gaszKicadSchemPowerPinNames[] = { "GND", "VCC", "VDD", "VSS", "VEE", "3V", "5V", "3V3", "V+", "V-", "NC", "N.C." };


//////////////////////////////////////////////////////////////////////////
//
// The parts of a pin's name: the gate number it starts with and the
// letters after it, and whether it's negated (~{...}).
//
//////////////////////////////////////////////////////////////////////////
static void SplitKicadSchemLogicPinName(const char* szName, CChars* pszIndex, CChars* pszLetters, bool* pbNegated)
{
	CChars	szName2;
	size	i;

	szName2.Init(szName);
	*pbNegated = false;
	if (szName2.StartsWith("~{"))
	{
		*pbNegated = true;
		szName2.RemoveFromStart(2);
		if (szName2.EndsWith("}"))
		{
			szName2.RemoveFromEnd(1);
		}
	}
	else if (szName2.StartsWith("~") && (szName2.Length() > 1))
	{
		*pbNegated = true;
		szName2.RemoveFromStart(1);
	}

	for (i = 0; i < szName2.Length(); i++)
	{
		if ((szName2.GetChar(i) < '0') || (szName2.GetChar(i) > '9'))
		{
			break;
		}
	}
	pszIndex->Init(szName2.Text(), 0, i);
	pszLetters->Init(szName2.Text(), i);
	pszLetters->UpperCase();
	szName2.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemPowerPinName(const char* szName)
{
	size	i;

	for (i = 0; i < sizeof(gaszKicadSchemPowerPinNames) / sizeof(const char*); i++)
	{
		if (StringInsensitiveCompare(szName, gaszKicadSchemPowerPinNames[i]) == 0)
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The pin types a gate's output can have.
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemOutputPinType(EKicadSchemPinType eType)
{
	return (eType == KSPT_Output) || (eType == KSPT_TriState) || (eType == KSPT_OpenCollector) || (eType == KSPT_OpenEmitter);
}


//////////////////////////////////////////////////////////////////////////
//
// A gate (or buffer) of one bit inputs at the origin, with implied power.
//
//////////////////////////////////////////////////////////////////////////
template<class M>
static CComponentView* NewKicadSchemGateView(ERotation eRotation, const char* szName, const char* szFamily, int iInputCount)
{
	M*	pcGate;

	pcGate = NewMalloc<M>();
	pcGate->Init(0, 0, eRotation, szName, szFamily, false, iInputCount, 1);
	return pcGate;
}


//////////////////////////////////////////////////////////////////////////
//
// A gate of one bit inputs at the origin, with implied power, its inputs
// KICAD_SCHEM_LOGICIM_PIN_SPACING apart.
//
//////////////////////////////////////////////////////////////////////////
template<class M>
static CComponentView* NewKicadSchemSpacedGateView(ERotation eRotation, const char* szName, const char* szFamily, int iInputCount)
{
	M*	pcGate;

	pcGate = NewMalloc<M>();
	pcGate->Init(0, 0, eRotation, szName, szFamily, false, iInputCount, 1, KICAD_SCHEM_LOGICIM_PIN_SPACING);
	return pcGate;
}


//Which Logicim flip flop port a KiCad flip flop pin is, by its letters.
struct SKicadSchemFlipFlopPin
{
	const char*		szLetters;
	bool			bNegated;
	const char*		szPortName;
};


static SKicadSchemFlipFlopPin gasKicadSchemFlipFlopPins[] =
{
	{ "CP", false, "CP" }, { "CLK", false, "CP" }, { "C", false, "CP" },
	{ "SD", true, "SD" }, { "S", true, "SD" }, { "PRE", true, "SD" }, { "SET", true, "SD" },
	{ "D", false, "D" },
	{ "RD", true, "RD" }, { "R", true, "RD" }, { "CLR", true, "RD" }, { "RESET", true, "RD" },
	{ "Q", false, "Q" },
	{ "Q", true, "QB" },
};


//////////////////////////////////////////////////////////////////////////
//
// The Logicim flip flop port for the pin, or NULL.
//
//////////////////////////////////////////////////////////////////////////
static const char* GetKicadSchemFlipFlopPortName(CChars* pszLetters, bool bNegated)
{
	size	i;

	for (i = 0; i < sizeof(gasKicadSchemFlipFlopPins) / sizeof(SKicadSchemFlipFlopPin); i++)
	{
		if (pszLetters->Equals(gasKicadSchemFlipFlopPins[i].szLetters))
		{
			//Set and reset are active low however their pins are drawn.
			if ((gasKicadSchemFlipFlopPins[i].szPortName[0] == 'Q') && (gasKicadSchemFlipFlopPins[i].bNegated != bNegated))
			{
				continue;
			}
			return gasKicadSchemFlipFlopPins[i].szPortName;
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemLogicPins::Init(void)
{
	aszIndices.Init();
	apcPins.Init();
	aiGroups.Init();
	aszLetters.Init();
	aiNegated.Init();
	asPositions.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemLogicPins::Kill(void)
{
	asPositions.Kill();
	aiNegated.Kill();
	aszLetters.Kill();
	aiGroups.Kill();
	apcPins.Kill();
	aszIndices.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the pin is a gate's output (Y) or one of a flip flop's (Q or
// QB).
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemLogicOutput(SKicadSchemLogicPins* psPins, size uiPin, EKicadSchemLogicFunction eFunction)
{
	if (eFunction == KSLF_DFlipFlop)
	{
		return GetKicadSchemFlipFlopPortName(psPins->aszLetters.Get(uiPin), psPins->aiNegated.GetValue(uiPin) != 0)[0] == 'Q';
	}
	return psPins->aszLetters.Get(uiPin)->Equals("Y");
}


//////////////////////////////////////////////////////////////////////////
//
// The layout of the pins of gate number iGroup (all the pins if -1): the
// inputs in letter order, the output (a gate's Y, a flip flop's Q) and
// the centres of all of them, of the inputs and of the outputs.  The
// layout's inputs must be killed.
//
//////////////////////////////////////////////////////////////////////////
static void GetKicadSchemLogicLayout(SKicadSchemLogicPins* psPins, int iGroup, EKicadSchemLogicFunction eFunction, SKicadSchemLogicLayout* psLayout)
{
	size					i;
	size					j;
	SFloat64Vec2*			psPosition;
	SKicadSchemPointMean	sCentre;
	SKicadSchemPointMean	sInputs;
	SKicadSchemPointMean	sOutputs;

	psLayout->aiInputs.Init();
	psLayout->iOutput = -1;
	sCentre.Init();
	sInputs.Init();
	sOutputs.Init();
	for (i = 0; i < psPins->apcPins.NumElements(); i++)
	{
		if ((iGroup != -1) && (psPins->aiGroups.GetValue(i) != iGroup))
		{
			continue;
		}
		psPosition = psPins->asPositions.Get(i);
		sCentre.Add(psPosition);
		if (IsKicadSchemLogicOutput(psPins, i, eFunction))
		{
			sOutputs.Add(psPosition);
			if ((eFunction != KSLF_DFlipFlop) || (StringCompare(GetKicadSchemFlipFlopPortName(psPins->aszLetters.Get(i), psPins->aiNegated.GetValue(i) != 0), "Q") == 0))
			{
				psLayout->iOutput = (int)i;
			}
		}
		else
		{
			for (j = 0; j < psLayout->aiInputs.NumElements(); j++)
			{
				if (strcmp(psPins->aszLetters.GetText(i), psPins->aszLetters.GetText(psLayout->aiInputs.GetValue(j))) < 0)
				{
					break;
				}
			}
			psLayout->aiInputs.InsertAt((int)i, j);
			sInputs.Add(psPosition);
		}
	}
	sCentre.Get(&psLayout->sCentre);
	sInputs.Get(&psLayout->sInputs);
	sOutputs.Get(&psLayout->sOutputs);
}


//////////////////////////////////////////////////////////////////////////
//
// The unit's pins (not its power pins), each split into its gate number
// and letters (see SplitKicadSchemLogicPinName), with where it is.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::CollectLogicPins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, SKicadSchemLogicPins* psPins)
{
	size					i;
	size					uiGroup;
	CKicadSchemLibPin*		pcPin;
	CChars					szIndex;
	CChars					szLetters;
	bool					bNegated;
	SFloat64Vec2			sPosition;

	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (!pcSymbol->IsUnitPin(pcPin) || (pcPin->GetType() == KSPT_PowerIn) || IsKicadSchemPowerPinName(pcPin->GetName()))
		{
			continue;
		}
		SplitKicadSchemLogicPinName(pcPin->GetName(), &szIndex, &szLetters, &bNegated);
		uiGroup = psPins->aszIndices.GetIndex(szIndex.Text());
		if (uiGroup == ARRAY_ELEMENT_NOT_FOUND)
		{
			psPins->aszIndices.Add(szIndex.Text());
			uiGroup = psPins->aszIndices.NumElements() - 1;
		}
		psPins->apcPins.Add(pcPin);
		psPins->aiGroups.Add((int)uiGroup);
		psPins->aszLetters.Add(szLetters.Text());
		psPins->aiNegated.Add(bNegated ? 1 : 0);
		mpcFile->GetPinPosition(&sPosition, pcSymbol, pcPin);
		psPins->asPositions.Add(&sPosition);
		szLetters.Kill();
		szIndex.Kill();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Gives a gate's unnamed pins (KiCad's own 74xx symbols name their gates'
// pins "~") letters: the inputs A up in pin number order, and the output
// Y.  Nothing changes unless every pin is unnamed.
//
//////////////////////////////////////////////////////////////////////////
static void NameUnnamedKicadSchemLogicPins(SKicadSchemLogicPins* psPins)
{
	size					i;
	size					j;
	size					k;
	CKicadSchemLibPin*		pcPin;
	CKicadSchemLibPin*		pcOther;
	CChars					szLetters;

	if (psPins->apcPins.NumElements() == 0)
	{
		return;
	}
	for (i = 0; i < psPins->apcPins.NumElements(); i++)
	{
		if (!psPins->aszLetters.Get(i)->Empty() && !psPins->aszLetters.Get(i)->Equals("~"))
		{
			return;
		}
	}

	for (i = 0; i < psPins->apcPins.NumElements(); i++)
	{
		pcPin = psPins->apcPins.GetPtr(i);
		if (IsKicadSchemOutputPinType(pcPin->GetType()))
		{
			psPins->aszLetters.Get(i)->Set("Y");
			continue;
		}
		k = 0;
		for (j = 0; j < psPins->apcPins.NumElements(); j++)
		{
			pcOther = psPins->apcPins.GetPtr(j);
			if ((j != i) && !IsKicadSchemOutputPinType(pcOther->GetType()) &&
				((atoi(pcOther->GetNumber()) < atoi(pcPin->GetNumber())) || ((atoi(pcOther->GetNumber()) == atoi(pcPin->GetNumber())) && (j < i))))
			{
				k++;
			}
		}
		szLetters.Init();
		szLetters.Append((char)('A' + k));
		psPins->aszLetters.Get(i)->Set(szLetters.Text());
		szLetters.Kill();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Whether every gate number has an output and its inputs (a flip flop a
// clock, data and output), named as a logic part's pins are.
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemLogicPartValid(SKicadSchemLogicPins* psPins, EKicadSchemLogicFunction eFunction)
{
	size			i;
	size			uiGroup;
	bool			bValid;
	int				iOutput;
	int				iInputs;
	const char*		szPortName;
	bool			bHasCP;
	bool			bHasD;
	bool			bHasQ;

	bValid = psPins->aszIndices.NumElements() > 0;
	for (uiGroup = 0; bValid && (uiGroup < psPins->aszIndices.NumElements()); uiGroup++)
	{
		iOutput = 0;
		iInputs = 0;
		bHasCP = false;
		bHasD = false;
		bHasQ = false;
		for (i = 0; i < psPins->apcPins.NumElements(); i++)
		{
			if (psPins->aiGroups.GetValue(i) != (int)uiGroup)
			{
				continue;
			}
			if (eFunction == KSLF_DFlipFlop)
			{
				szPortName = GetKicadSchemFlipFlopPortName(psPins->aszLetters.Get(i), psPins->aiNegated.GetValue(i) != 0);
				if (szPortName == NULL)
				{
					bValid = false;
				}
				else
				{
					bHasCP |= StringCompare(szPortName, "CP") == 0;
					bHasD |= StringCompare(szPortName, "D") == 0;
					bHasQ |= StringCompare(szPortName, "Q") == 0;
				}
			}
			else if (psPins->aszLetters.Get(i)->Equals("Y"))
			{
				iOutput++;
			}
			else if (psPins->aszLetters.Get(i)->Length() == 1)
			{
				iInputs++;
			}
			else
			{
				bValid = false;
			}
		}
		if (eFunction == KSLF_DFlipFlop)
		{
			bValid &= bHasCP && bHasD && bHasQ;
		}
		else if ((eFunction == KSLF_Inverter) || (eFunction == KSLF_Buffer))
		{
			bValid &= (iOutput == 1) && (iInputs == 1);
		}
		else
		{
			bValid &= (iOutput == 1) && (iInputs >= 2);
		}
	}
	return bValid;
}


//////////////////////////////////////////////////////////////////////////
//
// The name of the part for a gate number: the symbol's reference and the
// gate's letter, as KiCad's U1A (the unit's letter when the gates are
// units).
//
//////////////////////////////////////////////////////////////////////////
static void GetKicadSchemLogicPartName(CChars* pszName, CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, CChars* pszIndex)
{
	int		iIndex;

	pszName->Init(pcSymbol->GetReference());
	if (pszIndex->Length() > 0)
	{
		iIndex = atoi(pszIndex->Text());
		if ((iIndex >= 1) && (iIndex <= 26))
		{
			pszName->Append((char)('A' + iIndex - 1));
		}
		else
		{
			pszName->Append(pszIndex->Text());
		}
	}
	else if (pcLibSymbol->NumUnits() > 1)
	{
		pszName->Append((char)('A' + pcSymbol->GetUnit() - 1));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The generic gate (or flip flop) view of the logic function.
//
//////////////////////////////////////////////////////////////////////////
static CComponentView* NewKicadSchemLogicPartView(EKicadSchemLogicFunction eFunction, ERotation eRotation, const char* szName, const char* szFamily, int iInputs)
{
	CDTypeFlipFlopView*		pcFlipFlop;

	switch (eFunction)
	{
	case KSLF_And:
		return NewKicadSchemSpacedGateView<CAndGateView>(eRotation, szName, szFamily, iInputs);
	case KSLF_Nand:
		return NewKicadSchemSpacedGateView<CNandGateView>(eRotation, szName, szFamily, iInputs);
	case KSLF_Or:
		return NewKicadSchemSpacedGateView<COrGateView>(eRotation, szName, szFamily, iInputs);
	case KSLF_Nor:
		return NewKicadSchemSpacedGateView<CNorGateView>(eRotation, szName, szFamily, iInputs);
	case KSLF_Xor:
		return NewKicadSchemSpacedGateView<CXorGateView>(eRotation, szName, szFamily, iInputs);
	case KSLF_Xnor:
		return NewKicadSchemSpacedGateView<CXnorGateView>(eRotation, szName, szFamily, iInputs);
	case KSLF_Buffer:
		return NewKicadSchemGateView<CBufferView>(eRotation, szName, szFamily, 1);
	case KSLF_Inverter:
		return NewKicadSchemGateView<CInverterView>(eRotation, szName, szFamily, 1);
	default:
		pcFlipFlop = NewMalloc<CDTypeFlipFlopView>();
		pcFlipFlop->Init(0, 0, eRotation, szName, szFamily, false);
		return pcFlipFlop;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A gate or flip flop for each gate number of the unit's pins.  False
// (and nothing added) if the pins aren't named as a logic part's are.
// A real 74 series gate part is its package, or the unit of it KiCad
// draws (see AddLogic74Part).
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddLogicParts(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, EKicadSchemLogicFunction eFunction, const char* szFamily, const char* szPartType)
{
	SKicadSchemLogicPins	sPins;
	size					uiGroup;
	bool					bValid;
	bool					bPackaged;

	sPins.Init();
	CollectLogicPins(pcSymbol, pcLibSymbol, &sPins);
	if (eFunction != KSLF_DFlipFlop)
	{
		NameUnnamedKicadSchemLogicPins(&sPins);
	}
	bValid = IsKicadSchemLogicPartValid(&sPins, eFunction);

	bPackaged = false;
	if (bValid && (eFunction != KSLF_DFlipFlop))
	{
		bPackaged = AddLogic74Part(pcSymbol, pcLibSymbol, eFunction, &sPins, szFamily, szPartType);
	}
	for (uiGroup = 0; bValid && !bPackaged && (uiGroup < sPins.aszIndices.NumElements()); uiGroup++)
	{
		AddLogicPart(pcSymbol, pcLibSymbol, eFunction, &sPins, uiGroup, szFamily);
	}
	sPins.Kill();
	return bValid;
}


//////////////////////////////////////////////////////////////////////////
//
// The gate or flip flop for one gate number, facing from its inputs to
// its output, each pin on its port: a gate's inputs' port views come
// first and then the output's.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddLogicPart(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, EKicadSchemLogicFunction eFunction, SKicadSchemLogicPins* psPins, size uiGroup, const char* szFamily)
{
	SKicadSchemLogicLayout	sLayout;
	CChars					szName;
	ERotation				eRotation;
	CComponentView*			pcView;
	size					uiItem;
	size					i;
	const char*				szPortName;

	GetKicadSchemLogicLayout(psPins, (int)uiGroup, eFunction, &sLayout);
	eRotation = GetKicadSchemFacing(&sLayout.sInputs, psPins->asPositions.Get(sLayout.iOutput));
	GetKicadSchemLogicPartName(&szName, pcSymbol, pcLibSymbol, psPins->aszIndices.Get(uiGroup));
	pcView = NewKicadSchemLogicPartView(eFunction, eRotation, szName.Text(), szFamily, (int)sLayout.aiInputs.NumElements());
	szName.Kill();

	uiItem = AddItem(pcView, KSLIK_Part, &sLayout.sCentre, NULL);
	if (eFunction == KSLF_DFlipFlop)
	{
		for (i = 0; i < psPins->apcPins.NumElements(); i++)
		{
			if (psPins->aiGroups.GetValue(i) == (int)uiGroup)
			{
				szPortName = GetKicadSchemFlipFlopPortName(psPins->aszLetters.Get(i), psPins->aiNegated.GetValue(i) != 0);
				AddPinTerminal(pcSymbol, psPins->apcPins.GetPtr(i), pcView->GetPortView(szPortName), uiItem);
			}
		}
	}
	else
	{
		for (i = 0; i < sLayout.aiInputs.NumElements(); i++)
		{
			AddPinTerminal(pcSymbol, psPins->apcPins.GetPtr(sLayout.aiInputs.GetValue(i)), pcView->GetPortView(i), uiItem);
		}
		AddPinTerminal(pcSymbol, psPins->apcPins.GetPtr(sLayout.iOutput), pcView->GetPortView(sLayout.aiInputs.NumElements()), uiItem);
	}
	sLayout.aiInputs.Kill();
	msCounts.uiGates++;
}


//////////////////////////////////////////////////////////////////////////
//
// The generic gate a KiCad logic function is.
//
//////////////////////////////////////////////////////////////////////////
static bool GetKicadSchemGateFunction(EKicadSchemLogicFunction eFunction, EGateFunction* peGateFunction)
{
	switch (eFunction)
	{
	case KSLF_And:			*peGateFunction = GF_And;		return true;
	case KSLF_Nand:			*peGateFunction = GF_Nand;		return true;
	case KSLF_Or:			*peGateFunction = GF_Or;		return true;
	case KSLF_Nor:			*peGateFunction = GF_Nor;		return true;
	case KSLF_Xor:			*peGateFunction = GF_Xor;		return true;
	case KSLF_Xnor:			*peGateFunction = GF_Xnor;		return true;
	case KSLF_Buffer:		*peGateFunction = GF_Buffer;	return true;
	case KSLF_Inverter:		*peGateFunction = GF_Inverter;	return true;
	default:				return false;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The row (sType) of the real 74 series gate part, if its gates are the
// logic function's and every gate has the row's inputs and a gate number
// the part has.
//
//////////////////////////////////////////////////////////////////////////
static bool FindKicadSchemLogic74Gates(SLogic74PartType* psType, EKicadSchemLogicFunction eFunction, SKicadSchemLogicPins* psPins, const char* szPartType)
{
	EGateFunction	eGateFunction;
	size			i;
	size			j;
	int				iInputs;
	int				iIndex;

	if (!FindLogic74Part(psType, szPartType) || !GetKicadSchemGateFunction(eFunction, &eGateFunction))
	{
		return false;
	}
	if ((psType->psPart->eKind != L74K_Gates) || (psType->psPart->eGateFunction != eGateFunction))
	{
		return false;
	}

	for (i = 0; i < psPins->aszIndices.NumElements(); i++)
	{
		iInputs = 0;
		for (j = 0; j < psPins->apcPins.NumElements(); j++)
		{
			if ((psPins->aiGroups.GetValue(j) == (int)i) && !psPins->aszLetters.Get(j)->Equals("Y"))
			{
				iInputs++;
			}
		}
		if (iInputs != psType->psPart->iInputCount)
		{
			return false;
		}
		iIndex = psPins->aszIndices.Get(i)->Empty() ? 1 : atoi(psPins->aszIndices.GetText(i));
		if ((iIndex < 1) || (iIndex > psType->iUnitCount))
		{
			return false;
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The real 74 series part (see SLogic74Part) the symbol's unit is of, as
// KiCad draws it, facing from its inputs to its outputs.  A unit of a
// part drawn as units ("74LVC00_Multi") is a unit view of the part, named
// as the part; a single gate part ("1G08") is a unit view too.  A part
// drawn whole is a package.  False (and nothing added) if there's no row
// for the part or its pins aren't its row's gates.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddLogic74Part(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, EKicadSchemLogicFunction eFunction, SKicadSchemLogicPins* psPins, const char* szFamily, const char* szPartType)
{
	SLogic74PartType		sType;
	SKicadSchemLogicLayout	sLayout;
	ERotation				eRotation;
	bool					bUnit;

	if (!FindKicadSchemLogic74Gates(&sType, eFunction, psPins, szPartType))
	{
		return false;
	}
	bUnit = (pcLibSymbol->NumUnits() > 1) || (sType.iUnitCount == 1);
	if (bUnit && (psPins->aszIndices.NumElements() != 1))
	{
		return false;
	}

	GetKicadSchemLogicLayout(psPins, -1, eFunction, &sLayout);
	eRotation = GetKicadSchemFacing(&sLayout.sInputs, &sLayout.sOutputs);
	if (bUnit)
	{
		AddLogic74Unit(pcSymbol, (pcLibSymbol->NumUnits() > 1) ? pcSymbol->GetUnit() - 1 : 0, psPins, &sLayout, eRotation, szFamily, szPartType);
	}
	else
	{
		AddLogic74Package(pcSymbol, psPins, &sLayout, eRotation, szFamily, szPartType);
	}
	sLayout.aiInputs.Kill();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Unit iUnit of the part: its inputs in letter order, then its output, as
// the generic gate's.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddLogic74Unit(CKicadSchemSymbol* pcSymbol, int iUnit, SKicadSchemLogicPins* psPins, SKicadSchemLogicLayout* psLayout, ERotation eRotation, const char* szFamily, const char* szPartType)
{
	CLogic74UnitView*	pcUnit;
	size				uiItem;
	size				i;
	CChars				szPortName;

	pcUnit = NewMalloc<CLogic74UnitView>();
	pcUnit->Init(0, 0, eRotation, pcSymbol->GetReference(), szFamily, false, szPartType, iUnit, KICAD_SCHEM_LOGICIM_PIN_SPACING);
	uiItem = AddItem(pcUnit, KSLIK_Part, &psLayout->sCentre, NULL);
	for (i = 0; i < psLayout->aiInputs.NumElements(); i++)
	{
		szPortName.Init();
		CPins::CalculatePortName(&szPortName, "Input", i, psLayout->aiInputs.NumElements());
		AddPinTerminal(pcSymbol, psPins->apcPins.GetPtr(psLayout->aiInputs.GetValue(i)), pcUnit->GetPortView(szPortName.Text()), uiItem);
		szPortName.Kill();
	}
	AddPinTerminal(pcSymbol, psPins->apcPins.GetPtr(psLayout->iOutput), pcUnit->GetPortView("Output"), uiItem);
	msCounts.uiGates++;
}


//////////////////////////////////////////////////////////////////////////
//
// The whole part, each pin on the port of its name ("2B").
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddLogic74Package(CKicadSchemSymbol* pcSymbol, SKicadSchemLogicPins* psPins, SKicadSchemLogicLayout* psLayout, ERotation eRotation, const char* szFamily, const char* szPartType)
{
	CLogic74PackageView*	pcPackage;
	size					uiItem;
	size					i;
	CChars					szPortName;
	CChars*					pszIndex;
	CPortView*				pcPortView;

	pcPackage = NewMalloc<CLogic74PackageView>();
	pcPackage->Init(0, 0, eRotation, pcSymbol->GetReference(), szFamily, false, szPartType, KICAD_SCHEM_LOGICIM_PIN_SPACING);
	uiItem = AddItem(pcPackage, KSLIK_Part, &psLayout->sCentre, NULL);
	for (i = 0; i < psPins->apcPins.NumElements(); i++)
	{
		pszIndex = psPins->aszIndices.Get(psPins->aiGroups.GetValue(i));
		szPortName.Init();
		szPortName.Append(pszIndex->Empty() ? "1" : pszIndex->Text());
		szPortName.Append(psPins->aszLetters.Get(i));
		pcPortView = pcPackage->GetPortView(szPortName.Text());
		szPortName.Kill();
		if (pcPortView != NULL)
		{
			AddPinTerminal(pcSymbol, psPins->apcPins.GetPtr(i), pcPortView, uiItem);
		}
	}
	msCounts.uiGates += psPins->aszIndices.NumElements();
}


//////////////////////////////////////////////////////////////////////////
//
// A KiCad pin's name as the label of the port it's on (see
// CLogicBlockView): its overbars ("~{OE}", "1~{OE}") taken out, and
// pbNegated whether it had any; a sigma ("Σ1", a sum) is an S.
//
//////////////////////////////////////////////////////////////////////////
static void GetKicadSchemPinLabel(const char* szName, CChars* pszLabel, bool* pbNegated)
{
	size	i;

	*pbNegated = false;
	for (i = 0; (szName != NULL) && (szName[i] != '\0'); i++)
	{
		if ((szName[i] == '~') && (szName[i + 1] == '{'))
		{
			*pbNegated = true;
			i++;
		}
		else if (szName[i] == '}')
		{
		}
		else if (((unsigned char)szName[i] == 0xCE) && ((unsigned char)szName[i + 1] == 0xA3))
		{
			pszLabel->Append('S');
			i++;
		}
		else
		{
			pszLabel->Append(szName[i]);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The other names a pin's label goes by: a 74F283's carries ("C0", "C4")
// are a 74LVC283's "CIN" and "COUT", and a bus switch's channel letter
// after the name ("I0A", "YA", "EA") is the channel's number before it
// ("1I0", "1Y", "1E").  False if there's no other name.
//
//////////////////////////////////////////////////////////////////////////
static bool GetKicadSchemPinLabelAlias(const char* szLabel, CChars* pszAlias)
{
	size	uiLength;
	char	cChannel;
	CChars	szName;

	if (StringCompare(szLabel, "C0") == 0)
	{
		pszAlias->Set("CIN");
		return true;
	}
	if (StringCompare(szLabel, "C4") == 0)
	{
		pszAlias->Set("COUT");
		return true;
	}
	uiLength = StrLen(szLabel);
	if ((uiLength >= 2) && ((szLabel[0] == 'I') || (szLabel[0] == 'Y') || (szLabel[0] == 'E')))
	{
		cChannel = szLabel[uiLength - 1];
		if ((cChannel >= 'A') && (cChannel <= 'D'))
		{
			szName.Init(szLabel, 0, uiLength - 1);
			pszAlias->Kill();
			pszAlias->Init();
			pszAlias->Append((int32)(cChannel - 'A' + 1));
			pszAlias->Append(szName.Text());
			szName.Kill();
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The view's port labelled szLabel (as it reads: an active low port's
// label is overlined) and inverted or not, or the only one labelled
// szLabel.  A unit's labels have no flip flop number ("1CP" is
// "CP").
//
//////////////////////////////////////////////////////////////////////////
static CPortView* FindKicadSchemLabelledPort(CComponentView* pcView, const char* szLabel, bool bNegated, bool bUnit)
{
	size		i;
	CPortView*	pcPortView;
	CPortView*	pcOnly;
	size		uiLabelled;

	if (bUnit)
	{
		while ((*szLabel >= '0') && (*szLabel <= '9'))
		{
			szLabel++;
		}
	}
	pcOnly = NULL;
	uiLabelled = 0;
	for (i = 0; i < pcView->NumPortViews(); i++)
	{
		pcPortView = pcView->GetPortView(i);
		if (CircuitPlainTextEquals(pcPortView->GetText(), szLabel))
		{
			if (pcPortView->IsInverting() == bNegated)
			{
				return pcPortView;
			}
			pcOnly = pcPortView;
			uiLabelled++;
		}
	}
	return (uiLabelled == 1) ? pcOnly : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The view's port for each of the unit's logic pins (not its power or
// unconnected pins), in the order of the pins.  False if a pin has none.
//
//////////////////////////////////////////////////////////////////////////
static bool MapKicadSchemLogic74Pins(CComponentView* pcView, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayPortViewPtr* papcPorts, bool bUnit)
{
	size		i;
	CChars		szLabel;
	CChars		szAlias;
	bool		bNegated;
	CPortView*	pcPortView;

	papcPorts->Kill();
	papcPorts->Init();
	for (i = 0; i < papcPins->NumElements(); i++)
	{
		szLabel.Init();
		GetKicadSchemPinLabel(papcPins->GetPtr(i)->GetName(), &szLabel, &bNegated);
		pcPortView = FindKicadSchemLabelledPort(pcView, szLabel.Text(), bNegated, bUnit);
		if (pcPortView == NULL)
		{
			szAlias.Init();
			if (GetKicadSchemPinLabelAlias(szLabel.Text(), &szAlias))
			{
				pcPortView = FindKicadSchemLabelledPort(pcView, szAlias.Text(), bNegated, bUnit);
			}
			szAlias.Kill();
		}
		szLabel.Kill();
		if (pcPortView == NULL)
		{
			return false;
		}
		papcPorts->Add(pcPortView);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// A package (or unit) view of the part turned eRotation.
//
//////////////////////////////////////////////////////////////////////////
static CComponentView* NewKicadSchemLogic74View(CKicadSchemSymbol* pcSymbol, ERotation eRotation, const char* szFamily, const char* szPartType, bool bUnit, int iUnit)
{
	CLogic74PackageView*	pcPackage;
	CLogic74UnitView*		pcUnit;

	if (bUnit)
	{
		pcUnit = NewMalloc<CLogic74UnitView>();
		pcUnit->Init(0, 0, eRotation, pcSymbol->GetReference(), szFamily, false, szPartType, iUnit, KICAD_SCHEM_LOGICIM_PIN_SPACING);
		return pcUnit;
	}
	pcPackage = NewMalloc<CLogic74PackageView>();
	pcPackage->Init(0, 0, eRotation, pcSymbol->GetReference(), szFamily, false, szPartType, KICAD_SCHEM_LOGICIM_PIN_SPACING);
	return pcPackage;
}


//////////////////////////////////////////////////////////////////////////
//
// The way to turn a 74 part so the pins KiCad has at pasPositions on the
// left of it are on its left: papcPorts are the pins' ports on the part
// unturned.  East if its pins aren't on both sides.
//
//////////////////////////////////////////////////////////////////////////
static ERotation GetKicadSchemLogic74Rotation(CArrayFloat64Vec2* pasPositions, CArrayPortViewPtr* papcPorts)
{
	SKicadSchemPointMean	sLeftMean;
	SKicadSchemPointMean	sRightMean;
	SFloat64Vec2			sLeft;
	SFloat64Vec2			sRight;
	size					i;

	sLeftMean.Init();
	sRightMean.Init();
	for (i = 0; i < papcPorts->NumElements(); i++)
	{
		if (papcPorts->GetPtr(i)->GetGridPosition().x < 0)
		{
			sLeftMean.Add(pasPositions->Get(i));
		}
		else
		{
			sRightMean.Add(pasPositions->Get(i));
		}
	}
	if ((sLeftMean.uiCount == 0) || (sRightMean.uiCount == 0))
	{
		return ROT_East;
	}
	sLeftMean.Get(&sLeft);
	sRightMean.Get(&sRight);
	return GetKicadSchemFacing(&sLeft, &sRight);
}


//////////////////////////////////////////////////////////////////////////
//
// A real 74 series part that isn't gates (see SLogic74Part): a package of
// the whole part, or a unit of a part KiCad draws as units (a 74LVC74's
// flip flop).  Each KiCad pin is on the port its name labels (see
// GetKicadSchemPinLabel).  The view is turned so the pins KiCad has on
// the left of the part are on its left.  False (and nothing added) if a
// logic pin has no port.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddLogic74Block(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, const char* szFamily, const char* szPartType)
{
	SLogic74PartType						sType;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	CArrayPortViewPtr						apcPorts;
	CArrayFloat64Vec2						asPositions;
	SKicadSchemPointMean					sMean;
	SFloat64Vec2							sCentre;
	bool									bUnit;
	int										iUnit;
	size									i;
	CComponentView*							pcView;
	bool									bMapped;
	size									uiItem;
	ERotation								eRotation;

	if (!FindLogic74Part(&sType, szPartType) || (sType.psPart->eKind != L74K_Block))
	{
		return false;
	}
	bUnit = (pcLibSymbol->NumUnits() > 1) && (sType.iUnitCount > 1);
	iUnit = bUnit ? pcSymbol->GetUnit() - 1 : 0;

	apcPins.Init();
	asPositions.Init();
	apcPorts.Init();
	CollectLogic74Pins(pcSymbol, pcLibSymbol, &apcPins, &asPositions);
	bMapped = false;
	eRotation = ROT_East;
	if (apcPins.NumElements() > 0)
	{
		//Which side of the part's middle each pin's port is on, unturned, gives the way KiCad turned it.
		pcView = NewKicadSchemLogic74View(pcSymbol, ROT_East, szFamily, szPartType, bUnit, iUnit);
		bMapped = MapKicadSchemLogic74Pins(pcView, &apcPins, &apcPorts, bUnit);
		if (bMapped)
		{
			eRotation = GetKicadSchemLogic74Rotation(&asPositions, &apcPorts);
		}
		SafeKill(pcView);
	}

	if (bMapped)
	{
		pcView = NewKicadSchemLogic74View(pcSymbol, eRotation, szFamily, szPartType, bUnit, iUnit);
		MapKicadSchemLogic74Pins(pcView, &apcPins, &apcPorts, bUnit);
		sMean.Init();
		for (i = 0; i < asPositions.NumElements(); i++)
		{
			sMean.Add(asPositions.Get(i));
		}
		sMean.Get(&sCentre);
		uiItem = AddItem(pcView, KSLIK_Part, &sCentre, NULL);
		for (i = 0; i < apcPins.NumElements(); i++)
		{
			AddPinTerminal(pcSymbol, apcPins.GetPtr(i), apcPorts.GetPtr(i), uiItem);
		}
		msCounts.uiBlocks++;
	}

	apcPorts.Kill();
	asPositions.Kill();
	apcPins.Kill();
	return bMapped;
}


//////////////////////////////////////////////////////////////////////////
//
// The unit's logic pins (not its power or unconnected pins) and where
// KiCad has them on the sheet.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::CollectLogic74Pins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayFloat64Vec2* pasPositions)
{
	size				i;
	CKicadSchemLibPin*	pcPin;
	SFloat64Vec2		sPosition;

	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (!pcSymbol->IsUnitPin(pcPin) || (pcPin->GetType() == KSPT_PowerIn) || (pcPin->GetType() == KSPT_NoConnect) || IsKicadSchemPowerPinName(pcPin->GetName()))
		{
			continue;
		}
		papcPins->Add(pcPin);
		mpcFile->GetPinPosition(&sPosition, pcSymbol, pcPin);
		pasPositions->Add(&sPosition);
	}
}
