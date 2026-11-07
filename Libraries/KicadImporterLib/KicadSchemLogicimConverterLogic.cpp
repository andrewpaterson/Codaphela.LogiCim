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
#include "KicadSchemLogicPins.h"


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


//////////////////////////////////////////////////////////////////////////
//
// The symbol's part, not parsed yet.
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemLogicPart::Init(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	this->pcSymbol = pcSymbol;
	this->pcLibSymbol = pcLibSymbol;
	eFunction = KSLF_None;
	szFamily.Init();
	szPartType.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemLogicPart::Kill(void)
{
	szPartType.Kill();
	szFamily.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the library symbol's name, or else the symbol's value, is a
// real 74 series part ("74LVC74"), its family and part type kept.
//
//////////////////////////////////////////////////////////////////////////
bool SKicadSchemLogicPart::Parse74Part(void)
{
	szFamily.Clear();
	szPartType.Clear();
	eFunction = KSLF_None;
	return ParseKicadSchemLogic74Part(pcLibSymbol->GetName(), &szFamily, &szPartType) || ParseKicadSchemLogic74Part(pcSymbol->GetValue(), &szFamily, &szPartType);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the library symbol's name, or else the symbol's value, is a
// logic function's part ("74HC00", "SN74LVC1G08"), its function, family
// and part type kept.
//
//////////////////////////////////////////////////////////////////////////
bool SKicadSchemLogicPart::ParseFunction(void)
{
	szFamily.Clear();
	szPartType.Clear();
	eFunction = ParseKicadSchemLogicFunction(pcLibSymbol->GetName(), &szFamily, &szPartType);
	if (eFunction == KSLF_None)
	{
		eFunction = ParseKicadSchemLogicFunction(pcSymbol->GetValue(), &szFamily, &szPartType);
	}
	return eFunction != KSLF_None;
}


//Orders a part's pins, by index, by their letters.
struct SKicadSchemLetterOrder
{
	SKicadSchemLogicPins*	psPins;

	bool	IsBefore(size uiPin, size uiOther);
};


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool SKicadSchemLetterOrder::IsBefore(size uiPin, size uiOther)
{
	return strcmp(psPins->GetPin(uiPin)->szLetters.Text(), psPins->GetPin(uiOther)->szLetters.Text()) < 0;
}


//////////////////////////////////////////////////////////////////////////
//
// The layout of the pins of gate number uiGroup (all the pins if
// ARRAY_ELEMENT_NOT_FOUND): the
// inputs in letter order, the output (a gate's Y, a flip flop's Q) and
// the centres of all of them, of the inputs and of the outputs.  The
// layout's inputs must be killed.
//
//////////////////////////////////////////////////////////////////////////
static void GetKicadSchemLogicLayout(SKicadSchemLogicPins* psPins, size uiGroup, EKicadSchemLogicFunction eFunction, SKicadSchemLogicLayout* psLayout)
{
	size					i;
	SKicadSchemLogicPin*	psPin;
	SKicadSchemPointMean	sCentre;
	SKicadSchemPointMean	sInputs;
	SKicadSchemPointMean	sOutputs;
	SKicadSchemLetterOrder	sOrder;

	sOrder.psPins = psPins;
	psLayout->auiInputs.Init();
	psLayout->uiOutput = ARRAY_ELEMENT_NOT_FOUND;
	sCentre.Init();
	sInputs.Init();
	sOutputs.Init();
	for (i = 0; i < psPins->NumPins(); i++)
	{
		psPin = psPins->GetPin(i);
		if ((uiGroup != ARRAY_ELEMENT_NOT_FOUND) && (psPin->uiGroup != uiGroup))
		{
			continue;
		}
		sCentre.Add(&psPin->sPosition);
		if (IsKicadSchemLogicOutput(psPin, eFunction))
		{
			sOutputs.Add(&psPin->sPosition);
			if ((eFunction != KSLF_DFlipFlop) || (StringCompare(GetKicadSchemFlipFlopPortName(&psPin->szLetters, psPin->bNegated), "Q") == 0))
			{
				psLayout->uiOutput = i;
			}
		}
		else
		{
			psLayout->auiInputs.InsertInOrder(i, &sOrder);
			sInputs.Add(&psPin->sPosition);
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
void CKicadSchemLogicimConverter::CollectLogicPins(SKicadSchemLogicPart* psPart, SKicadSchemLogicPins* psPins)
{
	size					i;
	CKicadSchemLibPin*		pcPin;
	CChars					szIndex;
	CChars					szLetters;
	bool					bNegated;
	SFloat64Vec2			sPosition;

	for (i = 0; i < psPart->pcLibSymbol->NumPins(); i++)
	{
		pcPin = psPart->pcLibSymbol->GetPin(i);
		if (!psPart->pcSymbol->IsUnitPin(pcPin) || (pcPin->GetType() == KSPT_PowerIn) || IsKicadSchemPowerPinName(pcPin->GetName()))
		{
			continue;
		}
		SplitKicadSchemLogicPinName(pcPin->GetName(), &szIndex, &szLetters, &bNegated);
		mpcFile->GetPinPosition(&sPosition, psPart->pcSymbol, pcPin);
		psPins->AddPin(pcPin, szIndex.Text(), szLetters.Text(), bNegated, &sPosition);
		szLetters.Kill();
		szIndex.Kill();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Whether flip flop number uiGroup's pins are all flip flop ports, among
// them a clock, data and an output.
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemFlipFlopValid(SKicadSchemLogicPins* psPins, size uiGroup)
{
	size					i;
	const char*				szPortName;
	bool					bHasCP;
	bool					bHasD;
	bool					bHasQ;
	SKicadSchemLogicPin*	psPin;

	bHasCP = false;
	bHasD = false;
	bHasQ = false;
	for (i = 0; i < psPins->NumPins(); i++)
	{
		psPin = psPins->GetPin(i);
		if (psPin->uiGroup == uiGroup)
		{
			szPortName = GetKicadSchemFlipFlopPortName(&psPin->szLetters, psPin->bNegated);
			if (szPortName == NULL)
			{
				return false;
			}
			bHasCP |= StringCompare(szPortName, "CP") == 0;
			bHasD |= StringCompare(szPortName, "D") == 0;
			bHasQ |= StringCompare(szPortName, "Q") == 0;
		}
	}
	return bHasCP && bHasD && bHasQ;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether gate number uiGroup has an output (Y) and its inputs (one
// letter each): one input for an inverter or buffer, two or more for any
// other gate.
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemLogicGateValid(SKicadSchemLogicPins* psPins, size uiGroup, EKicadSchemLogicFunction eFunction)
{
	size					i;
	int						iOutput;
	int						iInputs;
	SKicadSchemLogicPin*	psPin;

	if (eFunction == KSLF_DFlipFlop)
	{
		return IsKicadSchemFlipFlopValid(psPins, uiGroup);
	}

	iOutput = 0;
	iInputs = 0;
	for (i = 0; i < psPins->NumPins(); i++)
	{
		psPin = psPins->GetPin(i);
		if (psPin->uiGroup != uiGroup)
		{
			continue;
		}
		if (psPin->szLetters.Equals("Y"))
		{
			iOutput++;
		}
		else if (psPin->szLetters.Length() == 1)
		{
			iInputs++;
		}
		else
		{
			return false;
		}
	}
	if ((eFunction == KSLF_Inverter) || (eFunction == KSLF_Buffer))
	{
		return (iOutput == 1) && (iInputs == 1);
	}
	return (iOutput == 1) && (iInputs >= 2);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the part has gates and every one is valid (see
// IsKicadSchemLogicGateValid).
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemLogicPartValid(SKicadSchemLogicPins* psPins, EKicadSchemLogicFunction eFunction)
{
	size	uiGroup;

	if (psPins->aszIndices.NumElements() == 0)
	{
		return false;
	}
	for (uiGroup = 0; uiGroup < psPins->aszIndices.NumElements(); uiGroup++)
	{
		if (!IsKicadSchemLogicGateValid(psPins, uiGroup, eFunction))
		{
			return false;
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The name of the part for a gate number: the symbol's reference and the
// gate's letter, as KiCad's U1A (the unit's letter when the gates are
// units).
//
//////////////////////////////////////////////////////////////////////////
static void GetKicadSchemLogicPartName(CChars* pszName, SKicadSchemLogicPart* psPart, CChars* pszIndex)
{
	int		iIndex;

	pszName->Init(psPart->pcSymbol->GetReference());
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
	else if (psPart->pcLibSymbol->NumUnits() > 1)
	{
		pszName->Append((char)('A' + psPart->pcSymbol->GetUnit() - 1));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The generic gate (or flip flop) view of the logic function.
//
//////////////////////////////////////////////////////////////////////////
static CComponentView* NewKicadSchemLogicPartView(SKicadSchemLogicPart* psPart, ERotation eRotation, const char* szName, int iInputs)
{
	CDTypeFlipFlopView*		pcFlipFlop;

	switch (psPart->eFunction)
	{
	case KSLF_And:
		return NewKicadSchemSpacedGateView<CAndGateView>(eRotation, szName, psPart->szFamily.Text(), iInputs);
	case KSLF_Nand:
		return NewKicadSchemSpacedGateView<CNandGateView>(eRotation, szName, psPart->szFamily.Text(), iInputs);
	case KSLF_Or:
		return NewKicadSchemSpacedGateView<COrGateView>(eRotation, szName, psPart->szFamily.Text(), iInputs);
	case KSLF_Nor:
		return NewKicadSchemSpacedGateView<CNorGateView>(eRotation, szName, psPart->szFamily.Text(), iInputs);
	case KSLF_Xor:
		return NewKicadSchemSpacedGateView<CXorGateView>(eRotation, szName, psPart->szFamily.Text(), iInputs);
	case KSLF_Xnor:
		return NewKicadSchemSpacedGateView<CXnorGateView>(eRotation, szName, psPart->szFamily.Text(), iInputs);
	case KSLF_Buffer:
		return NewKicadSchemGateView<CBufferView>(eRotation, szName, psPart->szFamily.Text(), 1);
	case KSLF_Inverter:
		return NewKicadSchemGateView<CInverterView>(eRotation, szName, psPart->szFamily.Text(), 1);
	default:
		pcFlipFlop = NewMalloc<CDTypeFlipFlopView>();
		pcFlipFlop->Init(0, 0, eRotation, szName, psPart->szFamily.Text(), false);
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
bool CKicadSchemLogicimConverter::AddLogicParts(SKicadSchemLogicPart* psPart)
{
	SKicadSchemLogicPins	sPins;
	size					uiGroup;
	bool					bValid;
	bool					bPackaged;

	sPins.Init();
	CollectLogicPins(psPart, &sPins);
	if (psPart->eFunction != KSLF_DFlipFlop)
	{
		NameUnnamedKicadSchemLogicPins(&sPins);
	}
	bValid = IsKicadSchemLogicPartValid(&sPins, psPart->eFunction);

	bPackaged = false;
	if (bValid && (psPart->eFunction != KSLF_DFlipFlop))
	{
		bPackaged = AddLogic74Part(psPart, &sPins);
	}
	for (uiGroup = 0; bValid && !bPackaged && (uiGroup < sPins.aszIndices.NumElements()); uiGroup++)
	{
		AddLogicPart(psPart, &sPins, uiGroup);
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
void CKicadSchemLogicimConverter::AddLogicPart(SKicadSchemLogicPart* psPart, SKicadSchemLogicPins* psPins, size uiGroup)
{
	SKicadSchemLogicLayout	sLayout;
	CChars					szName;
	ERotation				eRotation;
	CComponentView*			pcView;
	size					uiItem;
	size					i;
	const char*				szPortName;
	SKicadSchemLogicPin*	psPin;

	GetKicadSchemLogicLayout(psPins, uiGroup, psPart->eFunction, &sLayout);
	eRotation = GetKicadSchemFacing(&sLayout.sInputs, &psPins->GetPin(sLayout.uiOutput)->sPosition);
	GetKicadSchemLogicPartName(&szName, psPart, psPins->aszIndices.Get(uiGroup));
	pcView = NewKicadSchemLogicPartView(psPart, eRotation, szName.Text(), (int)sLayout.auiInputs.NumElements());
	szName.Kill();

	uiItem = mcModel.AddItem(pcView, KSLIK_Part, &sLayout.sCentre, NULL, psPart->pcSymbol);
	if (psPart->eFunction == KSLF_DFlipFlop)
	{
		for (i = 0; i < psPins->NumPins(); i++)
		{
			psPin = psPins->GetPin(i);
			if (psPin->uiGroup == uiGroup)
			{
				szPortName = GetKicadSchemFlipFlopPortName(&psPin->szLetters, psPin->bNegated);
				AddPinTerminal(psPart->pcSymbol, psPin->pcPin, pcView->GetPortView(szPortName), uiItem);
			}
		}
	}
	else
	{
		for (i = 0; i < sLayout.auiInputs.NumElements(); i++)
		{
			AddPinTerminal(psPart->pcSymbol, psPins->GetPin(sLayout.auiInputs.GetValue(i))->pcPin, pcView->GetPortView(i), uiItem);
		}
		AddPinTerminal(psPart->pcSymbol, psPins->GetPin(sLayout.uiOutput)->pcPin, pcView->GetPortView(sLayout.auiInputs.NumElements()), uiItem);
	}
	sLayout.auiInputs.Kill();
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
static bool FindKicadSchemLogic74Gates(SLogic74PartType* psType, SKicadSchemLogicPart* psPart, SKicadSchemLogicPins* psPins)
{
	EGateFunction	eGateFunction;
	size			i;
	size			j;
	int				iInputs;
	int				iIndex;

	if (!FindLogic74Part(psType, psPart->szPartType.Text()) || !GetKicadSchemGateFunction(psPart->eFunction, &eGateFunction))
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
		for (j = 0; j < psPins->NumPins(); j++)
		{
			if ((psPins->GetPin(j)->uiGroup == i) && !psPins->GetPin(j)->szLetters.Equals("Y"))
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
bool CKicadSchemLogicimConverter::AddLogic74Part(SKicadSchemLogicPart* psPart, SKicadSchemLogicPins* psPins)
{
	SLogic74PartType		sType;
	SKicadSchemLogicLayout	sLayout;
	ERotation				eRotation;
	bool					bUnit;

	if (!FindKicadSchemLogic74Gates(&sType, psPart, psPins))
	{
		return false;
	}
	bUnit = (psPart->pcLibSymbol->NumUnits() > 1) || (sType.iUnitCount == 1);
	if (bUnit && (psPins->aszIndices.NumElements() != 1))
	{
		return false;
	}

	GetKicadSchemLogicLayout(psPins, ARRAY_ELEMENT_NOT_FOUND, psPart->eFunction, &sLayout);
	eRotation = GetKicadSchemFacing(&sLayout.sInputs, &sLayout.sOutputs);
	if (bUnit)
	{
		AddLogic74Unit(psPart, (psPart->pcLibSymbol->NumUnits() > 1) ? psPart->pcSymbol->GetUnit() - 1 : 0, psPins, &sLayout, eRotation);
	}
	else
	{
		AddLogic74Package(psPart, psPins, &sLayout, eRotation);
	}
	sLayout.auiInputs.Kill();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// A package (or unit) view of the part turned eRotation.
//
//////////////////////////////////////////////////////////////////////////
static CComponentView* NewKicadSchemLogic74View(SKicadSchemLogicPart* psPart, ERotation eRotation, bool bUnit, int iUnit)
{
	CLogic74PackageView*	pcPackage;
	CLogic74UnitView*		pcUnit;

	if (bUnit)
	{
		pcUnit = NewMalloc<CLogic74UnitView>();
		pcUnit->Init(0, 0, eRotation, psPart->pcSymbol->GetReference(), psPart->szFamily.Text(), false, psPart->szPartType.Text(), iUnit, KICAD_SCHEM_LOGICIM_PIN_SPACING);
		return pcUnit;
	}
	pcPackage = NewMalloc<CLogic74PackageView>();
	pcPackage->Init(0, 0, eRotation, psPart->pcSymbol->GetReference(), psPart->szFamily.Text(), false, psPart->szPartType.Text(), KICAD_SCHEM_LOGICIM_PIN_SPACING);
	return pcPackage;
}


//////////////////////////////////////////////////////////////////////////
//
// Unit iUnit of the part: its inputs in letter order, then its output, as
// the generic gate's.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddLogic74Unit(SKicadSchemLogicPart* psPart, int iUnit, SKicadSchemLogicPins* psPins, SKicadSchemLogicLayout* psLayout, ERotation eRotation)
{
	CComponentView*		pcUnit;
	size				uiItem;
	size				i;
	CChars				szPortName;

	pcUnit = NewKicadSchemLogic74View(psPart, eRotation, true, iUnit);
	uiItem = mcModel.AddItem(pcUnit, KSLIK_Part, &psLayout->sCentre, NULL, psPart->pcSymbol);
	for (i = 0; i < psLayout->auiInputs.NumElements(); i++)
	{
		szPortName.Init();
		CPins::CalculatePortName(&szPortName, "Input", i, psLayout->auiInputs.NumElements());
		AddPinTerminal(psPart->pcSymbol, psPins->GetPin(psLayout->auiInputs.GetValue(i))->pcPin, pcUnit->GetPortView(szPortName.Text()), uiItem);
		szPortName.Kill();
	}
	AddPinTerminal(psPart->pcSymbol, psPins->GetPin(psLayout->uiOutput)->pcPin, pcUnit->GetPortView("Output"), uiItem);
	msCounts.uiGates++;
}


//////////////////////////////////////////////////////////////////////////
//
// The whole part, each pin on the port of its name ("2B").
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddLogic74Package(SKicadSchemLogicPart* psPart, SKicadSchemLogicPins* psPins, SKicadSchemLogicLayout* psLayout, ERotation eRotation)
{
	CComponentView*			pcPackage;
	size					uiItem;
	size					i;
	CChars					szPortName;
	CChars*					pszIndex;
	CPortView*				pcPortView;
	SKicadSchemLogicPin*	psPin;

	pcPackage = NewKicadSchemLogic74View(psPart, eRotation, false, 0);
	uiItem = mcModel.AddItem(pcPackage, KSLIK_Part, &psLayout->sCentre, NULL, psPart->pcSymbol);
	for (i = 0; i < psPins->NumPins(); i++)
	{
		psPin = psPins->GetPin(i);
		pszIndex = psPins->aszIndices.Get(psPin->uiGroup);
		szPortName.Init();
		szPortName.Append(pszIndex->Empty() ? "1" : pszIndex->Text());
		szPortName.Append(&psPin->szLetters);
		pcPortView = pcPackage->GetPortView(szPortName.Text());
		szPortName.Kill();
		if (pcPortView != NULL)
		{
			AddPinTerminal(psPart->pcSymbol, psPin->pcPin, pcPortView, uiItem);
		}
	}
	msCounts.uiGates += psPins->aszIndices.NumElements();
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
// The way KiCad turned the part, from which side of the part's middle
// each pin's port is on, unturned (see GetKicadSchemLogic74Rotation).
// False if a logic pin has no port, or there are no logic pins.
//
//////////////////////////////////////////////////////////////////////////
static bool GetKicadSchemLogic74Turn(SKicadSchemLogicPart* psPart, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayFloat64Vec2* pasPositions, bool bUnit, int iUnit, ERotation* peRotation)
{
	CComponentView*		pcView;
	CArrayPortViewPtr	apcPorts;
	bool				bMapped;

	if (papcPins->NumElements() == 0)
	{
		return false;
	}
	pcView = NewKicadSchemLogic74View(psPart, ROT_East, bUnit, iUnit);
	apcPorts.Init();
	bMapped = MapKicadSchemLogic74Pins(pcView, papcPins, &apcPorts, bUnit);
	if (bMapped)
	{
		*peRotation = GetKicadSchemLogic74Rotation(pasPositions, &apcPorts);
	}
	apcPorts.Kill();
	SafeKill(pcView);
	return bMapped;
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
bool CKicadSchemLogicimConverter::AddLogic74Block(SKicadSchemLogicPart* psPart)
{
	SLogic74PartType						sType;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	CArrayFloat64Vec2						asPositions;
	bool									bUnit;
	int										iUnit;
	bool									bMapped;
	ERotation								eRotation;

	if (!FindLogic74Part(&sType, psPart->szPartType.Text()) || (sType.psPart->eKind != L74K_Block))
	{
		return false;
	}
	bUnit = (psPart->pcLibSymbol->NumUnits() > 1) && (sType.iUnitCount > 1);
	iUnit = bUnit ? psPart->pcSymbol->GetUnit() - 1 : 0;

	apcPins.Init();
	asPositions.Init();
	CollectLogic74Pins(psPart, &apcPins, &asPositions);
	bMapped = GetKicadSchemLogic74Turn(psPart, &apcPins, &asPositions, bUnit, iUnit, &eRotation);
	if (bMapped)
	{
		AddLogic74BlockView(psPart, &apcPins, &asPositions, NewKicadSchemLogic74View(psPart, eRotation, bUnit, iUnit), bUnit);
	}
	asPositions.Kill();
	apcPins.Kill();
	return bMapped;
}


//////////////////////////////////////////////////////////////////////////
//
// The block's view where the middle of its pins is, each pin on its port.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddLogic74BlockView(SKicadSchemLogicPart* psPart, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayFloat64Vec2* pasPositions, CComponentView* pcView, bool bUnit)
{
	CArrayPortViewPtr		apcPorts;
	SKicadSchemPointMean	sMean;
	SFloat64Vec2			sCentre;
	size					i;
	size					uiItem;

	apcPorts.Init();
	MapKicadSchemLogic74Pins(pcView, papcPins, &apcPorts, bUnit);
	sMean.Init();
	for (i = 0; i < pasPositions->NumElements(); i++)
	{
		sMean.Add(pasPositions->Get(i));
	}
	sMean.Get(&sCentre);
	uiItem = mcModel.AddItem(pcView, KSLIK_Part, &sCentre, NULL, psPart->pcSymbol);
	for (i = 0; i < papcPins->NumElements(); i++)
	{
		AddPinTerminal(psPart->pcSymbol, papcPins->GetPtr(i), apcPorts.GetPtr(i), uiItem);
	}
	apcPorts.Kill();
	msCounts.uiBlocks++;
}


//////////////////////////////////////////////////////////////////////////
//
// The unit's logic pins (not its power or unconnected pins) and where
// KiCad has them on the sheet.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::CollectLogic74Pins(SKicadSchemLogicPart* psPart, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayFloat64Vec2* pasPositions)
{
	size				i;
	CKicadSchemLibPin*	pcPin;
	SFloat64Vec2		sPosition;

	for (i = 0; i < psPart->pcLibSymbol->NumPins(); i++)
	{
		pcPin = psPart->pcLibSymbol->GetPin(i);
		if (!psPart->pcSymbol->IsUnitPin(pcPin) || (pcPin->GetType() == KSPT_PowerIn) || (pcPin->GetType() == KSPT_NoConnect) || IsKicadSchemPowerPinName(pcPin->GetName()))
		{
			continue;
		}
		papcPins->Add(pcPin);
		mpcFile->GetPinPosition(&sPosition, psPart->pcSymbol, pcPin);
		pasPositions->Add(&sPosition);
	}
}
