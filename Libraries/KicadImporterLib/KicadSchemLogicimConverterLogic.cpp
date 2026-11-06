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
// A gate or flip flop for each gate number of the unit's pins.  False
// (and nothing added) if the pins aren't named as a logic part's are.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddLogicParts(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, EKicadSchemLogicFunction eFunction, const char* szFamily, const char* szPartType)
{
	CArrayChars							aszIndices;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	CArrayInt							aiGroups;
	CArrayChars							aszLetters;
	CArrayInt							aiNegated;
	CArrayKicadSchemPoint				asPositions;
	CArrayInt							aiInputs;
	size								i;
	size								j;
	size								k;
	size								uiGroup;
	CKicadSchemLibPin*					pcPin;
	CChars								szIndex;
	CChars								szLetters;
	CChars								szName;
	bool								bNegated;
	bool								bValid;
	int									iOutput;
	int									iInputs;
	SKicadSchemPoint					sPosition;
	SKicadSchemPoint					sCentre;
	SKicadSchemPoint					sInputs;
	ERotation							eRotation;
	CComponentView*						pcView;
	size								uiItem;
	const char*							szPortName;
	int									iIndex;
	bool								bHasCP;
	bool								bHasD;
	bool								bHasQ;
	bool								bUnnamed;
	bool								bPackaged;

	aszIndices.Init();
	apcPins.Init();
	aiGroups.Init();
	aszLetters.Init();
	aiNegated.Init();
	asPositions.Init();
	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (!pcPin->IsOn(pcSymbol->GetUnit(), pcSymbol->GetBodyStyle()) || (pcPin->GetType() == KSPT_PowerIn) || IsKicadSchemPowerPinName(pcPin->GetName()))
		{
			continue;
		}
		SplitKicadSchemLogicPinName(pcPin->GetName(), &szIndex, &szLetters, &bNegated);
		uiGroup = aszIndices.GetIndex(szIndex.Text());
		if (uiGroup == ARRAY_ELEMENT_NOT_FOUND)
		{
			aszIndices.Add(szIndex.Text());
			uiGroup = aszIndices.NumElements() - 1;
		}
		apcPins.Add(pcPin);
		aiGroups.Add((int)uiGroup);
		aszLetters.Add(szLetters.Text());
		aiNegated.Add(bNegated ? 1 : 0);
		mpcFile->GetPinPosition(&sPosition, pcSymbol, pcPin);
		asPositions.Add(&sPosition);
		szLetters.Kill();
		szIndex.Kill();
	}

	//Unnamed pins (KiCad's own 74xx symbols name their gates' pins "~") are the inputs, A up in pin number order, and
	//the output Y.
	bUnnamed = (eFunction != KSLF_DFlipFlop) && (apcPins.NumElements() > 0);
	for (i = 0; bUnnamed && (i < apcPins.NumElements()); i++)
	{
		bUnnamed = aszLetters.Get(i)->Empty() || aszLetters.Get(i)->Equals("~");
	}
	if (bUnnamed)
	{
		for (i = 0; i < apcPins.NumElements(); i++)
		{
			if (IsKicadSchemOutputPinType(apcPins.GetPtr(i)->GetType()))
			{
				aszLetters.Get(i)->Set("Y");
				continue;
			}
			k = 0;
			for (j = 0; j < apcPins.NumElements(); j++)
			{
				if ((j != i) && !IsKicadSchemOutputPinType(apcPins.GetPtr(j)->GetType()) &&
					((atoi(apcPins.GetPtr(j)->GetNumber()) < atoi(apcPins.GetPtr(i)->GetNumber())) || ((atoi(apcPins.GetPtr(j)->GetNumber()) == atoi(apcPins.GetPtr(i)->GetNumber())) && (j < i))))
				{
					k++;
				}
			}
			szLetters.Init();
			szLetters.Append((char)('A' + k));
			aszLetters.Get(i)->Set(szLetters.Text());
			szLetters.Kill();
		}
	}

	//Every gate needs an output and its inputs; a flip flop a clock, data and output.
	bValid = aszIndices.NumElements() > 0;
	for (uiGroup = 0; bValid && (uiGroup < aszIndices.NumElements()); uiGroup++)
	{
		iOutput = 0;
		iInputs = 0;
		bHasCP = false;
		bHasD = false;
		bHasQ = false;
		for (i = 0; i < apcPins.NumElements(); i++)
		{
			if (aiGroups.GetValue(i) != (int)uiGroup)
			{
				continue;
			}
			if (eFunction == KSLF_DFlipFlop)
			{
				szPortName = GetKicadSchemFlipFlopPortName(aszLetters.Get(i), aiNegated.GetValue(i) != 0);
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
			else if (aszLetters.Get(i)->Equals("Y"))
			{
				iOutput++;
			}
			else if (aszLetters.Get(i)->Length() == 1)
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

	//A real 74 series gate part is its package, or the unit of it KiCad draws.
	bPackaged = false;
	if (bValid && (eFunction != KSLF_DFlipFlop))
	{
		bPackaged = AddLogic74Part(pcSymbol, pcLibSymbol, eFunction, &aszIndices, &apcPins, &aiGroups, &aszLetters, &asPositions, szFamily, szPartType);
	}

	for (uiGroup = 0; bValid && !bPackaged && (uiGroup < aszIndices.NumElements()); uiGroup++)
	{
		//The inputs in letter order, then the output.
		aiInputs.Init();
		iOutput = -1;
		sCentre.Init();
		sInputs.Init();
		k = 0;
		for (i = 0; i < apcPins.NumElements(); i++)
		{
			if (aiGroups.GetValue(i) != (int)uiGroup)
			{
				continue;
			}
			sCentre.fX += asPositions.Get(i)->fX;
			sCentre.fY += asPositions.Get(i)->fY;
			k++;
			if ((eFunction != KSLF_DFlipFlop) && aszLetters.Get(i)->Equals("Y"))
			{
				iOutput = (int)i;
			}
			else if ((eFunction == KSLF_DFlipFlop) && (GetKicadSchemFlipFlopPortName(aszLetters.Get(i), aiNegated.GetValue(i) != 0)[0] == 'Q'))
			{
				if (StringCompare(GetKicadSchemFlipFlopPortName(aszLetters.Get(i), aiNegated.GetValue(i) != 0), "Q") == 0)
				{
					iOutput = (int)i;
				}
			}
			else
			{
				for (j = 0; j < aiInputs.NumElements(); j++)
				{
					if (strcmp(aszLetters.GetText(i), aszLetters.GetText(aiInputs.GetValue(j))) < 0)
					{
						break;
					}
				}
				aiInputs.InsertAt((int)i, j);
				sInputs.fX += asPositions.Get(i)->fX;
				sInputs.fY += asPositions.Get(i)->fY;
			}
		}
		sCentre.fX /= (float64)k;
		sCentre.fY /= (float64)k;
		sInputs.fX /= (float64)aiInputs.NumElements();
		sInputs.fY /= (float64)aiInputs.NumElements();
		eRotation = GetKicadSchemFacing(&sInputs, asPositions.Get(iOutput));

		szName.Init(pcSymbol->GetReference());
		if (aszIndices.Get(uiGroup)->Length() > 0)
		{
			iIndex = atoi(aszIndices.GetText(uiGroup));
			if ((iIndex >= 1) && (iIndex <= 26))
			{
				szName.Append((char)('A' + iIndex - 1));
			}
			else
			{
				szName.Append(aszIndices.GetText(uiGroup));
			}
		}
		else if (pcLibSymbol->NumUnits() > 1)
		{
			//A gate to a unit, as KiCad's U1A.
			szName.Append((char)('A' + pcSymbol->GetUnit() - 1));
		}

		iInputs = (int)aiInputs.NumElements();
		switch (eFunction)
		{
		case KSLF_And:
			pcView = NewKicadSchemSpacedGateView<CAndGateView>(eRotation, szName.Text(), szFamily, iInputs);
			break;
		case KSLF_Nand:
			pcView = NewKicadSchemSpacedGateView<CNandGateView>(eRotation, szName.Text(), szFamily, iInputs);
			break;
		case KSLF_Or:
			pcView = NewKicadSchemSpacedGateView<COrGateView>(eRotation, szName.Text(), szFamily, iInputs);
			break;
		case KSLF_Nor:
			pcView = NewKicadSchemSpacedGateView<CNorGateView>(eRotation, szName.Text(), szFamily, iInputs);
			break;
		case KSLF_Xor:
			pcView = NewKicadSchemSpacedGateView<CXorGateView>(eRotation, szName.Text(), szFamily, iInputs);
			break;
		case KSLF_Xnor:
			pcView = NewKicadSchemSpacedGateView<CXnorGateView>(eRotation, szName.Text(), szFamily, iInputs);
			break;
		case KSLF_Buffer:
			pcView = NewKicadSchemGateView<CBufferView>(eRotation, szName.Text(), szFamily, 1);
			break;
		case KSLF_Inverter:
			pcView = NewKicadSchemGateView<CInverterView>(eRotation, szName.Text(), szFamily, 1);
			break;
		default:
			pcView = NewMalloc<CDTypeFlipFlopView>();
			((CDTypeFlipFlopView*)pcView)->Init(0, 0, eRotation, szName.Text(), szFamily, false);
			break;
		}
		szName.Kill();

		uiItem = AddItem(pcView, KSLIK_Part, &sCentre, NULL);
		if (eFunction == KSLF_DFlipFlop)
		{
			for (i = 0; i < apcPins.NumElements(); i++)
			{
				if (aiGroups.GetValue(i) == (int)uiGroup)
				{
					szPortName = GetKicadSchemFlipFlopPortName(aszLetters.Get(i), aiNegated.GetValue(i) != 0);
					AddPinTerminal(pcSymbol, apcPins.GetPtr(i), pcView->GetPortView(szPortName), uiItem);
				}
			}
		}
		else
		{
			//The inputs' port views come first and then the output's.
			for (j = 0; j < aiInputs.NumElements(); j++)
			{
				AddPinTerminal(pcSymbol, apcPins.GetPtr(aiInputs.GetValue(j)), pcView->GetPortView(j), uiItem);
			}
			AddPinTerminal(pcSymbol, apcPins.GetPtr(iOutput), pcView->GetPortView(aiInputs.NumElements()), uiItem);
		}
		aiInputs.Kill();
		muiGates++;
	}

	asPositions.Kill();
	aiNegated.Kill();
	aszLetters.Kill();
	aiGroups.Kill();
	apcPins.Kill();
	aszIndices.Kill();
	return bValid;
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
// The real 74 series part (see SLogic74Part) the symbol's unit is of, as
// KiCad draws it.  A unit of a part drawn as units ("74LVC00_Multi") is a
// unit view of the part, named as the part; a single gate part ("1G08")
// is a unit view too.  A part drawn whole is a package, each pin on the
// port of its name ("2B").  False (and nothing added) if there's no row
// for the part or its pins aren't its row's gates.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddLogic74Part(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, EKicadSchemLogicFunction eFunction, CArrayChars* paszIndices, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayInt* paiGroups, CArrayChars* paszLetters, CArrayKicadSchemPoint* pasPositions, const char* szFamily, const char* szPartType)
{
	SLogic74PartType		sType;
	EGateFunction			eGateFunction;
	size					i;
	size					j;
	int						iInputs;
	int						iIndex;
	int						iUnit;
	SKicadSchemPoint		sCentre;
	SKicadSchemPoint		sInputs;
	SKicadSchemPoint		sOutputs;
	int						iOutputs;
	CArrayInt				aiInputs;
	int						iOutput;
	CLogic74PackageView*	pcPackage;
	CLogic74UnitView*		pcUnit;
	CComponentView*			pcView;
	size					uiItem;
	CChars					szPortName;
	CPortView*				pcPortView;
	bool					bUnit;

	if (!FindLogic74Part(&sType, szPartType) || !GetKicadSchemGateFunction(eFunction, &eGateFunction))
	{
		return false;
	}
	if ((sType.psPart->eKind != L74K_Gates) || (sType.psPart->eGateFunction != eGateFunction))
	{
		return false;
	}

	//Every gate has the row's inputs, and a gate number the part has.
	for (i = 0; i < paszIndices->NumElements(); i++)
	{
		iInputs = 0;
		for (j = 0; j < papcPins->NumElements(); j++)
		{
			if ((paiGroups->GetValue(j) == (int)i) && !paszLetters->Get(j)->Equals("Y"))
			{
				iInputs++;
			}
		}
		if (iInputs != sType.psPart->iInputCount)
		{
			return false;
		}
		iIndex = paszIndices->Get(i)->Empty() ? 1 : atoi(paszIndices->GetText(i));
		if ((iIndex < 1) || (iIndex > sType.iUnitCount))
		{
			return false;
		}
	}

	bUnit = (pcLibSymbol->NumUnits() > 1) || (sType.iUnitCount == 1);
	if (bUnit && (paszIndices->NumElements() != 1))
	{
		return false;
	}

	sCentre.Init();
	sInputs.Init();
	sOutputs.Init();
	iOutputs = 0;
	iOutput = -1;
	aiInputs.Init();
	for (i = 0; i < papcPins->NumElements(); i++)
	{
		sCentre.fX += pasPositions->Get(i)->fX;
		sCentre.fY += pasPositions->Get(i)->fY;
		if (paszLetters->Get(i)->Equals("Y"))
		{
			sOutputs.fX += pasPositions->Get(i)->fX;
			sOutputs.fY += pasPositions->Get(i)->fY;
			iOutputs++;
			iOutput = (int)i;
		}
		else
		{
			sInputs.fX += pasPositions->Get(i)->fX;
			sInputs.fY += pasPositions->Get(i)->fY;
			for (j = 0; j < aiInputs.NumElements(); j++)
			{
				if (strcmp(paszLetters->GetText(i), paszLetters->GetText(aiInputs.GetValue(j))) < 0)
				{
					break;
				}
			}
			aiInputs.InsertAt((int)i, j);
		}
	}
	sCentre.fX /= (float64)papcPins->NumElements();
	sCentre.fY /= (float64)papcPins->NumElements();
	sInputs.fX /= (float64)aiInputs.NumElements();
	sInputs.fY /= (float64)aiInputs.NumElements();
	sOutputs.fX /= (float64)iOutputs;
	sOutputs.fY /= (float64)iOutputs;

	if (bUnit)
	{
		//The unit's inputs in letter order, then its output, as the generic gate's.
		iUnit = (pcLibSymbol->NumUnits() > 1) ? pcSymbol->GetUnit() - 1 : 0;
		pcUnit = NewMalloc<CLogic74UnitView>();
		pcUnit->Init(0, 0, GetKicadSchemFacing(&sInputs, &sOutputs), pcSymbol->GetReference(), szFamily, false, szPartType, iUnit, KICAD_SCHEM_LOGICIM_PIN_SPACING);
		pcView = pcUnit;
		uiItem = AddItem(pcView, KSLIK_Part, &sCentre, NULL);
		for (j = 0; j < aiInputs.NumElements(); j++)
		{
			szPortName.Init();
			CPins::CalculatePortName(&szPortName, "Input", j, aiInputs.NumElements());
			AddPinTerminal(pcSymbol, papcPins->GetPtr(aiInputs.GetValue(j)), pcView->GetPortView(szPortName.Text()), uiItem);
			szPortName.Kill();
		}
		AddPinTerminal(pcSymbol, papcPins->GetPtr(iOutput), pcView->GetPortView("Output"), uiItem);
		muiGates++;
	}
	else
	{
		pcPackage = NewMalloc<CLogic74PackageView>();
		pcPackage->Init(0, 0, GetKicadSchemFacing(&sInputs, &sOutputs), pcSymbol->GetReference(), szFamily, false, szPartType, KICAD_SCHEM_LOGICIM_PIN_SPACING);
		pcView = pcPackage;
		uiItem = AddItem(pcView, KSLIK_Part, &sCentre, NULL);
		for (i = 0; i < papcPins->NumElements(); i++)
		{
			szPortName.Init();
			szPortName.Append(paszIndices->Get(paiGroups->GetValue(i))->Empty() ? "1" : paszIndices->GetText(paiGroups->GetValue(i)));
			szPortName.Append(paszLetters->Get(i));
			pcPortView = pcView->GetPortView(szPortName.Text());
			szPortName.Kill();
			if (pcPortView != NULL)
			{
				AddPinTerminal(pcSymbol, papcPins->GetPtr(i), pcPortView, uiItem);
			}
		}
		muiGates += paszIndices->NumElements();
	}
	aiInputs.Kill();
	return true;
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
	CArrayKicadSchemPoint					asPositions;
	CKicadSchemLibPin*						pcPin;
	SKicadSchemPoint						sPosition;
	SKicadSchemPoint						sCentre;
	SKicadSchemPoint						sLeft;
	SKicadSchemPoint						sRight;
	int										iLeft;
	int										iRight;
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
	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (!pcPin->IsOn(pcSymbol->GetUnit(), pcSymbol->GetBodyStyle()) || (pcPin->GetType() == KSPT_PowerIn) || (pcPin->GetType() == KSPT_NoConnect) || IsKicadSchemPowerPinName(pcPin->GetName()))
		{
			continue;
		}
		apcPins.Add(pcPin);
		mpcFile->GetPinPosition(&sPosition, pcSymbol, pcPin);
		asPositions.Add(&sPosition);
	}
	if (apcPins.NumElements() == 0)
	{
		asPositions.Kill();
		apcPins.Kill();
		return false;
	}

	//Which side of the part's middle each pin's port is on, unturned, gives the way KiCad turned it.
	apcPorts.Init();
	pcView = NewKicadSchemLogic74View(pcSymbol, ROT_East, szFamily, szPartType, bUnit, iUnit);
	bMapped = MapKicadSchemLogic74Pins(pcView, &apcPins, &apcPorts, bUnit);
	sCentre.Init();
	sLeft.Init();
	sRight.Init();
	iLeft = 0;
	iRight = 0;
	for (i = 0; bMapped && (i < apcPins.NumElements()); i++)
	{
		sCentre.fX += asPositions.Get(i)->fX;
		sCentre.fY += asPositions.Get(i)->fY;
		if (apcPorts.GetPtr(i)->GetGridPosition().x < 0)
		{
			sLeft.fX += asPositions.Get(i)->fX;
			sLeft.fY += asPositions.Get(i)->fY;
			iLeft++;
		}
		else
		{
			sRight.fX += asPositions.Get(i)->fX;
			sRight.fY += asPositions.Get(i)->fY;
			iRight++;
		}
	}
	SafeKill(pcView);
	if (!bMapped)
	{
		apcPorts.Kill();
		asPositions.Kill();
		apcPins.Kill();
		return false;
	}

	sCentre.fX /= (float64)apcPins.NumElements();
	sCentre.fY /= (float64)apcPins.NumElements();
	eRotation = ROT_East;
	if ((iLeft > 0) && (iRight > 0))
	{
		sLeft.fX /= (float64)iLeft;
		sLeft.fY /= (float64)iLeft;
		sRight.fX /= (float64)iRight;
		sRight.fY /= (float64)iRight;
		eRotation = GetKicadSchemFacing(&sLeft, &sRight);
	}

	pcView = NewKicadSchemLogic74View(pcSymbol, eRotation, szFamily, szPartType, bUnit, iUnit);
	MapKicadSchemLogic74Pins(pcView, &apcPins, &apcPorts, bUnit);
	uiItem = AddItem(pcView, KSLIK_Part, &sCentre, NULL);
	for (i = 0; i < apcPins.NumElements(); i++)
	{
		AddPinTerminal(pcSymbol, apcPins.GetPtr(i), apcPorts.GetPtr(i), uiItem);
	}
	muiBlocks++;

	apcPorts.Kill();
	asPositions.Kill();
	apcPins.Kill();
	return true;
}
