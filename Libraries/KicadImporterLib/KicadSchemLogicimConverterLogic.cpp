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
	size					j;
	SKicadSchemLogicPin*	psPin;
	SKicadSchemPointMean	sCentre;
	SKicadSchemPointMean	sInputs;
	SKicadSchemPointMean	sOutputs;

	psLayout->aiInputs.Init();
	psLayout->iOutput = -1;
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
				psLayout->iOutput = (int)i;
			}
		}
		else
		{
			for (j = 0; j < psLayout->aiInputs.NumElements(); j++)
			{
				if (strcmp(psPin->szLetters.Text(), psPins->GetPin(psLayout->aiInputs.GetValue(j))->szLetters.Text()) < 0)
				{
					break;
				}
			}
			psLayout->aiInputs.InsertAt((int)i, j);
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
void CKicadSchemLogicimConverter::CollectLogicPins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, SKicadSchemLogicPins* psPins)
{
	size					i;
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
		mpcFile->GetPinPosition(&sPosition, pcSymbol, pcPin);
		psPins->AddPin(pcPin, szIndex.Text(), szLetters.Text(), bNegated, &sPosition);
		szLetters.Kill();
		szIndex.Kill();
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
	size					i;
	size					uiGroup;
	bool					bValid;
	int						iOutput;
	int						iInputs;
	const char*				szPortName;
	bool					bHasCP;
	bool					bHasD;
	bool					bHasQ;
	SKicadSchemLogicPin*	psPin;

	bValid = psPins->aszIndices.NumElements() > 0;
	for (uiGroup = 0; bValid && (uiGroup < psPins->aszIndices.NumElements()); uiGroup++)
	{
		iOutput = 0;
		iInputs = 0;
		bHasCP = false;
		bHasD = false;
		bHasQ = false;
		for (i = 0; i < psPins->NumPins(); i++)
		{
			psPin = psPins->GetPin(i);
			if (psPin->uiGroup != uiGroup)
			{
				continue;
			}
			if (eFunction == KSLF_DFlipFlop)
			{
				szPortName = GetKicadSchemFlipFlopPortName(&psPin->szLetters, psPin->bNegated);
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
			else if (psPin->szLetters.Equals("Y"))
			{
				iOutput++;
			}
			else if (psPin->szLetters.Length() == 1)
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
	SKicadSchemLogicPin*	psPin;

	GetKicadSchemLogicLayout(psPins, uiGroup, eFunction, &sLayout);
	eRotation = GetKicadSchemFacing(&sLayout.sInputs, &psPins->GetPin(sLayout.iOutput)->sPosition);
	GetKicadSchemLogicPartName(&szName, pcSymbol, pcLibSymbol, psPins->aszIndices.Get(uiGroup));
	pcView = NewKicadSchemLogicPartView(eFunction, eRotation, szName.Text(), szFamily, (int)sLayout.aiInputs.NumElements());
	szName.Kill();

	uiItem = mcModel.AddItem(pcView, KSLIK_Part, &sLayout.sCentre, NULL, pcSymbol);
	if (eFunction == KSLF_DFlipFlop)
	{
		for (i = 0; i < psPins->NumPins(); i++)
		{
			psPin = psPins->GetPin(i);
			if (psPin->uiGroup == uiGroup)
			{
				szPortName = GetKicadSchemFlipFlopPortName(&psPin->szLetters, psPin->bNegated);
				AddPinTerminal(pcSymbol, psPin->pcPin, pcView->GetPortView(szPortName), uiItem);
			}
		}
	}
	else
	{
		for (i = 0; i < sLayout.aiInputs.NumElements(); i++)
		{
			AddPinTerminal(pcSymbol, psPins->GetPin(sLayout.aiInputs.GetValue(i))->pcPin, pcView->GetPortView(i), uiItem);
		}
		AddPinTerminal(pcSymbol, psPins->GetPin(sLayout.iOutput)->pcPin, pcView->GetPortView(sLayout.aiInputs.NumElements()), uiItem);
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

	GetKicadSchemLogicLayout(psPins, ARRAY_ELEMENT_NOT_FOUND, eFunction, &sLayout);
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
	uiItem = mcModel.AddItem(pcUnit, KSLIK_Part, &psLayout->sCentre, NULL, pcSymbol);
	for (i = 0; i < psLayout->aiInputs.NumElements(); i++)
	{
		szPortName.Init();
		CPins::CalculatePortName(&szPortName, "Input", i, psLayout->aiInputs.NumElements());
		AddPinTerminal(pcSymbol, psPins->GetPin(psLayout->aiInputs.GetValue(i))->pcPin, pcUnit->GetPortView(szPortName.Text()), uiItem);
		szPortName.Kill();
	}
	AddPinTerminal(pcSymbol, psPins->GetPin(psLayout->iOutput)->pcPin, pcUnit->GetPortView("Output"), uiItem);
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
	SKicadSchemLogicPin*	psPin;

	pcPackage = NewMalloc<CLogic74PackageView>();
	pcPackage->Init(0, 0, eRotation, pcSymbol->GetReference(), szFamily, false, szPartType, KICAD_SCHEM_LOGICIM_PIN_SPACING);
	uiItem = mcModel.AddItem(pcPackage, KSLIK_Part, &psLayout->sCentre, NULL, pcSymbol);
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
			AddPinTerminal(pcSymbol, psPin->pcPin, pcPortView, uiItem);
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
		uiItem = mcModel.AddItem(pcView, KSLIK_Part, &sCentre, NULL, pcSymbol);
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
