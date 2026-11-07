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
// Whether the symbol is a connector (a "J"), as a board's edge connector
// is.
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemConnector(CKicadSchemSymbol* pcSymbol)
{
	char*	szReference;

	szReference = pcSymbol->GetReference();
	return (szReference != NULL) && ((szReference[0] == 'J') || (szReference[0] == 'j')) && (szReference[1] >= '0') && (szReference[1] <= '9');
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit pin's name for the connector's pin on the net: the net's
// ("D0") or, on a net that's only the pin's, the connector's pin's
// ("J1-5").  False, and szName empty, on a supply's net.
//
//////////////////////////////////////////////////////////////////////////
static bool GetKicadSchemBoardPinName(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CKicadSchemNet* pcNet, CChars* szName)
{
	if (pcNet->GetNameSource() == KSNNS_Pin)
	{
		szName->Append(pcSymbol->GetReference());
		szName->Append('-');
		szName->Append(pcPin->GetNumber());
		return true;
	}

	szName->Append((pcNet->GetName()[0] == '/') ? &pcNet->GetName()[1] : pcNet->GetName());
	if (IsKicadSchemPowerNetName(szName->Text()))
	{
		szName->Clear();
		return false;
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Where the connector's pin is wired, psOuter, and the way it faces out
// of the connector's body, returned.
//
//////////////////////////////////////////////////////////////////////////
ERotation CKicadSchemLogicimConverter::GetBoardPinFacing(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, SFloat64Vec2* psOuter)
{
	SFloat64Vec2		sInner;
	SFloat64Vec2		sLibrary;
	int					iAngle;

	//The pin points into the connector's body from where it's wired.  Library Y is up, so KiCad's anticlockwise quarter
	//turns step as the directions turning right do on the grid, whose Y is down.
	mpcFile->GetPinPosition(psOuter, pcSymbol, pcPin);
	iAngle = GetKicadSchemQuarterTurns(pcPin->GetAngle());
	sLibrary = *pcPin->GetPosition() + SFloat64Vec2(gaiDirectionX[iAngle], gaiDirectionY[iAngle]) * pcPin->GetLength();
	pcSymbol->GetSheetPosition(&sInner, &sLibrary);
	return GetKicadSchemFacing(&sInner, psOuter);
}


//////////////////////////////////////////////////////////////////////////
//
// The board's record of the connector, with a place for each of its
// unit's pins.  NULL if the sheet isn't a board's.
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemBoardConnector* CKicadSchemLogicimConverter::AddBoardConnector(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	size	i;
	size	uiNumPins;

	if (mpcBoard == NULL)
	{
		return NULL;
	}

	uiNumPins = 0;
	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		if (pcSymbol->IsUnitPin(pcLibSymbol->GetPin(i)))
		{
			uiNumPins++;
		}
	}
	return mpcBoard->AddConnector(pcSymbol->GetReference(), uiNumPins);
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit's pin for one of the connector's pins (see AddBoardPins),
// made unless another of the connector's pins on the net made it.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddBoardPin(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CKicadSchemBoardConnector* pcConnector)
{
	CKicadSchemSubnet*		pcSubnet;
	CKicadSchemNet*			pcNet;
	CChars					szName;
	size					uiItem;

	pcSubnet = mpcNetlist->GetPinSubnet(pcSymbol, pcPin);
	pcNet = (pcSubnet != NULL) ? pcSubnet->GetNet() : NULL;
	if ((pcNet == NULL) || StrEmpty(pcNet->GetName()))
	{
		return;
	}

	szName.Init();
	if (GetKicadSchemBoardPinName(pcSymbol, pcPin, pcNet, &szName))
	{
		uiItem = mcModel.FindBoardPin(szName.Text());
		if (uiItem != ARRAY_ELEMENT_NOT_FOUND)
		{
			JoinBoardPin(pcSymbol, pcPin, pcConnector, uiItem);
		}
		else
		{
			AddBoardPinView(pcSymbol, pcPin, pcConnector, szName.Text());
		}
	}
	szName.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The connector's pin on the circuit's pin another of its pins on the
// net made (item uiItem).  It's wired to it if it's on the same part of
// the net; another part is already joined through its tunnels.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::JoinBoardPin(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CKicadSchemBoardConnector* pcConnector, size uiItem)
{
	CPinView*	pcPinView;
	size		uiTerminal;

	pcPinView = (CPinView*)mcModel.GetItem(uiItem)->pcView;
	if (pcConnector != NULL)
	{
		pcConnector->AddPin(pcPin->GetNumber(), pcPinView);
	}
	uiTerminal = mcModel.FindPortTerminal(pcPinView->GetPortView((size)0));
	if ((uiTerminal != ARRAY_ELEMENT_NOT_FOUND) && (mcModel.GetTerminal(uiTerminal)->pcSubnet == mpcNetlist->GetPinSubnet(pcSymbol, pcPin)))
	{
		AddPinTerminal(pcSymbol, pcPin, pcPinView->GetPortView((size)0), uiItem);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A circuit pin named szName for the connector's pin: where the
// connector's is, facing out of its body, and shown on that side of an
// instance in the order of the pin numbers.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddBoardPinView(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CKicadSchemBoardConnector* pcConnector, const char* szName)
{
	CPinView*		pcPinView;
	SFloat64Vec2	sOuter;
	ERotation		eRotation;
	size			uiItem;

	eRotation = GetBoardPinFacing(pcSymbol, pcPin, &sOuter);
	pcPinView = NewMalloc<CPinView>();
	pcPinView->Init(0, 0, eRotation, szName, "LVC", false, 1, RADIX_Binary, mpcFonts);
	pcPinView->SetSubcircuitAppearance(RotationToPinAlignment(eRotation), PAN_Negative, atoi(pcPin->GetNumber()), false, false);
	//Placed as a part is (its port is its position), so a label KiCad has on the connector's pin is attached to it.
	uiItem = mcModel.AddItem(pcPinView, KSLIK_BoardPin, &sOuter, NULL, pcSymbol);
	msCounts.uiBoardPins++;
	if (pcConnector != NULL)
	{
		pcConnector->AddPin(pcPin->GetNumber(), pcPinView);
	}
	AddPinTerminal(pcSymbol, pcPin, pcPinView->GetPortView((size)0), uiItem);
}


//////////////////////////////////////////////////////////////////////////
//
// A connector (a board's edge connector, "J1") is how the sheet joins the
// rest of the computer, so each of its pins on a net is a pin of the
// circuit, as a subcircuit: named as its net ("D0") or, on a net that's
// only the pin's, as the connector's pin ("J1-5").  A pin on a supply's
// net (see IsKicadSchemPowerNetName) is left out: the supplies are
// already there.  Another of the connector's pins on the same part of the
// net (the same subnet) is wired to the same pin; one on another part of
// it isn't, as that part is joined to the rest by its labels' tunnels.
//
// The circuit's pin is where the connector's is, facing out away from
// the connector's body, and is shown on that side of an instance, in the
// order of the connector's pin numbers.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddBoardPins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	size						i;
	CKicadSchemLibPin*			pcPin;
	CKicadSchemBoardConnector*	pcConnector;

	if (!IsKicadSchemConnector(pcSymbol))
	{
		return false;
	}

	pcConnector = AddBoardConnector(pcSymbol, pcLibSymbol);
	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (pcSymbol->IsUnitPin(pcPin) && (pcPin->GetType() != KSPT_NoConnect))
		{
			AddBoardPin(pcSymbol, pcPin, pcConnector);
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// An instance of the subcircuit for the symbol, named as its reference
// with its value, turned as KiCad turns the symbol (a mirrored symbol
// isn't mirrored), and its item, puiItem.
//
//////////////////////////////////////////////////////////////////////////
CSubcircuitInstanceView* CKicadSchemLogicimConverter::AddSubcircuitInstance(CKicadSchemSymbol* pcSymbol, CCircuitView* pcSubcircuit, size* puiItem)
{
	CSubcircuitInstanceView*	pcInstance;
	char*						szValue;

	szValue = pcSymbol->GetValue();
	pcInstance = NewMalloc<CSubcircuitInstanceView>();
	pcInstance->Init(0, 0, GetKicadSchemRotation(ROT_East, pcSymbol->GetAngle()), pcSymbol->GetReference(), (szValue != NULL) ? szValue : "", pcSubcircuit, mpcFonts, KICAD_SCHEM_LOGICIM_PIN_SPACING);
	*puiItem = mcModel.AddItem(pcInstance, KSLIK_Subcircuit, pcSymbol->GetPosition(), NULL, pcSymbol);
	return pcInstance;
}


//////////////////////////////////////////////////////////////////////////
//
// An instance of the placeholder subcircuit for the symbol's unit.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddPlaceholder(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	CCircuitView*							pcPlaceholder;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	CArrayChars								aszNames;
	CSubcircuitInstanceView*				pcInstance;
	SSubcircuitPinView*						psPin;
	size									uiItem;
	size									i;

	pcPlaceholder = GetPlaceholderCircuit(pcSymbol, pcLibSymbol);
	if (pcPlaceholder == NULL)
	{
		return false;
	}

	pcInstance = AddSubcircuitInstance(pcSymbol, pcPlaceholder, &uiItem);

	apcPins.Init();
	aszNames.Init();
	GetPlaceholderPins(pcSymbol, pcLibSymbol, &apcPins, &aszNames);
	for (i = 0; i < apcPins.NumElements(); i++)
	{
		psPin = pcInstance->GetPin(aszNames.GetText(i));
		if (psPin != NULL)
		{
			AddPinTerminal(pcSymbol, apcPins.GetPtr(i), psPin->pcPortView, uiItem);
		}
	}
	aszNames.Kill();
	apcPins.Kill();
	msCounts.uiPlaceholders++;
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Where the library symbol's pin i of papcPins (its shown pins) goes on an
// instance's sides: the side it's on, returned in peAlignment, and the
// returned weight, its order along that side: down the left and right
// sides (library Y is up) and right to left along the top and bottom.
//
//////////////////////////////////////////////////////////////////////////
static int32 GetKicadSchemLibPinPlace(CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, size i, EPinAlignment* peAlignment)
{
	size				j;
	CKicadSchemLibPin*	pcPin;
	CKicadSchemLibPin*	pcOther;
	int					iAngle;
	int					iOtherAngle;
	int32				iWeight;
	bool				bBefore;

	//A pin points into the body, so it faces out the other way: a pin pointing right (0) is on the left.
	pcPin = papcPins->GetPtr(i);
	iAngle = GetKicadSchemQuarterTurns(pcPin->GetAngle());
	*peAlignment = RotationToPinAlignment(OppositeRotation(GetKicadSchemRotation(ROT_East, pcPin->GetAngle())));

	iWeight = 0;
	for (j = 0; j < papcPins->NumElements(); j++)
	{
		pcOther = papcPins->GetPtr(j);
		iOtherAngle = GetKicadSchemQuarterTurns(pcOther->GetAngle());
		if ((j == i) || (iOtherAngle != iAngle))
		{
			continue;
		}
		if ((iAngle == 0) || (iAngle == 2))
		{
			bBefore = (pcOther->GetPosition()->y > pcPin->GetPosition()->y) || ((pcOther->GetPosition()->y == pcPin->GetPosition()->y) && (j < i));
		}
		else
		{
			bBefore = (pcOther->GetPosition()->x > pcPin->GetPosition()->x) || ((pcOther->GetPosition()->x == pcPin->GetPosition()->x) && (j < i));
		}
		if (bBefore)
		{
			iWeight++;
		}
	}
	return iWeight;
}


//////////////////////////////////////////////////////////////////////////
//
// The schematic of the board a symbol stands for: its Schematic property
// (see KICAD_SCHEM_BOARD_SCHEMATIC_PROPERTY), or else its library
// symbol's.  NULL if neither has one.
//
//////////////////////////////////////////////////////////////////////////
static char* GetKicadSchemBoardSchematic(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	char*	szSchematic;

	szSchematic = pcSymbol->GetProperties()->GetValue(KICAD_SCHEM_BOARD_SCHEMATIC_PROPERTY);
	if (StrEmpty(szSchematic))
	{
		szSchematic = pcLibSymbol->GetProperties()->GetValue(KICAD_SCHEM_BOARD_SCHEMATIC_PROPERTY);
	}
	return StrEmpty(szSchematic) ? NULL : szSchematic;
}


//////////////////////////////////////////////////////////////////////////
//
// A board drawn as a symbol (one with a Schematic property, see
// GetKicadSchemBoardSchematic): an instance of the board's circuit, made
// from its own schematic by the board source.  The symbol's pins are
// joined to the circuit's pins of the board's connector with the same
// numbers (see JoinBoardPins).  False, to make it a placeholder, if
// there's no board source or the board can't be made.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddBoard(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	char*									szSchematic;
	CKicadSchemBoard*						pcBoard;
	CKicadSchemBoardConnector*				pcConnector;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	CArrayChars								aszNames;
	CSubcircuitInstanceView*				pcInstance;
	size									uiItem;

	szSchematic = GetKicadSchemBoardSchematic(pcSymbol, pcLibSymbol);
	if ((mpcBoardSource == NULL) || (szSchematic == NULL))
	{
		return false;
	}
	pcBoard = mpcBoardSource->GetBoard(szSchematic, mpcFile->GetFileName());
	if (pcBoard == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Board [", pcSymbol->GetReference(), "]'s schematic [", szSchematic, "] couldn't be made, it's a placeholder.", NULL);
	}

	apcPins.Init();
	aszNames.Init();
	GetPlaceholderPins(pcSymbol, pcLibSymbol, &apcPins, &aszNames);
	pcConnector = pcBoard->FindConnector(apcPins.NumElements());
	if (pcConnector != NULL)
	{
		if (!pcBoard->IsLaidOut())
		{
			LayOutBoardPins(&apcPins, pcConnector);
			pcBoard->SetLaidOut();
		}
		pcInstance = AddSubcircuitInstance(pcSymbol, pcBoard->GetCircuit(), &uiItem);
		JoinBoardPins(pcSymbol, &apcPins, pcConnector, pcInstance, uiItem);
		msCounts.uiBoards++;
	}
	else
	{
		gcLogger.Error2(__METHOD__, " Board [", pcSymbol->GetReference(), "]'s schematic [", szSchematic, "] has no connector, it's a placeholder.", NULL);
	}
	aszNames.Kill();
	apcPins.Kill();
	return pcConnector != NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The symbol's pins joined to the instance's pins for the circuit's pins
// of the board's connector with the same numbers.  A pin on a supply or
// on nothing on the board has no circuit pin and isn't joined, and two of
// the symbol's pins on one circuit pin (one net on the board) are joined
// to it once.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::JoinBoardPins(CKicadSchemSymbol* pcSymbol, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CKicadSchemBoardConnector* pcConnector, CSubcircuitInstanceView* pcInstance, size uiItem)
{
	CArrayTemplatePtr<CPortView>	apcJoined;
	SSubcircuitPinView*				psPin;
	CPinView*						pcPinView;
	size							i;

	apcJoined.Init();
	for (i = 0; i < papcPins->NumElements(); i++)
	{
		pcPinView = pcConnector->GetPin(papcPins->GetPtr(i)->GetNumber());
		psPin = (pcPinView != NULL) ? pcInstance->GetPin(pcPinView) : NULL;
		if ((psPin != NULL) && (apcJoined.Find(psPin->pcPortView) == ARRAY_ELEMENT_NOT_FOUND))
		{
			apcJoined.Add(psPin->pcPortView);
			AddPinTerminal(pcSymbol, papcPins->GetPtr(i), psPin->pcPortView, uiItem);
		}
	}
	apcJoined.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Shows the circuit pin on an instance's side eAlignment, iWeight along
// it, inverted and clocked as KiCad draws the library symbol's pin.
//
//////////////////////////////////////////////////////////////////////////
static void SetKicadSchemPinAppearance(CPinView* pcPinView, CKicadSchemLibPin* pcPin, EPinAlignment eAlignment, int32 iWeight)
{
	pcPinView->SetSubcircuitAppearance(eAlignment, PAN_Negative, iWeight, strstr(pcPin->GetGraphicStyle(), "inverted") != NULL, strstr(pcPin->GetGraphicStyle(), "clock") != NULL);
}


//////////////////////////////////////////////////////////////////////////
//
// The placeholder's name: the library symbol's, without its library, and
// the unit's letter if it has more than one.
//
//////////////////////////////////////////////////////////////////////////
static void GetKicadSchemPlaceholderName(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, CChars* pszTypeName)
{
	char*	szLibName;
	char*	szColon;

	szLibName = pcLibSymbol->GetName();
	szColon = strrchr(szLibName, ':');
	pszTypeName->Init((szColon != NULL) ? szColon + 1 : szLibName);
	if (pcLibSymbol->NumUnits() > 1)
	{
		pszTypeName->Append('_');
		pszTypeName->Append((char)('A' + pcSymbol->GetUnit() - 1));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Shows the board's circuit pins on an instance as the symbol shows its
// pins: on the same sides, in the same order, inverted and clocked as
// they are.  A circuit pin that isn't one of the symbol's (a pin of
// another connector) stays as its own connector placed it.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::LayOutBoardPins(CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CKicadSchemBoardConnector* pcConnector)
{
	size				i;
	CKicadSchemLibPin*	pcPin;
	CPinView*			pcPinView;
	EPinAlignment		eAlignment;
	int32				iWeight;
	CArrayTemplatePtr<CPinView>	apcLaidOut;

	apcLaidOut.Init();
	for (i = 0; i < papcPins->NumElements(); i++)
	{
		pcPin = papcPins->GetPtr(i);
		pcPinView = pcConnector->GetPin(pcPin->GetNumber());
		if ((pcPinView == NULL) || (apcLaidOut.Find(pcPinView) != ARRAY_ELEMENT_NOT_FOUND))
		{
			continue;
		}
		apcLaidOut.Add(pcPinView);
		iWeight = GetKicadSchemLibPinPlace(papcPins, i, &eAlignment);
		SetKicadSchemPinAppearance(pcPinView, pcPin, eAlignment, iWeight);
	}
	apcLaidOut.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The placeholder for the library symbol's unit, made the first time
// (see GetKicadSchemPlaceholderName).  Its pins are on the sides the
// library symbol's are, in the same order along them.
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CKicadSchemLogicimConverter::GetPlaceholderCircuit(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	CChars									szTypeName;
	CCircuitView*							pcPlaceholder;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	CArrayChars								aszNames;
	size									i;
	CKicadSchemLibPin*						pcPin;
	EPinAlignment							eAlignment;
	int32									iWeight;
	CPinView*								pcPinView;
	SInt32Bounds							sBox;

	GetKicadSchemPlaceholderName(pcSymbol, pcLibSymbol, &szTypeName);
	pcPlaceholder = mpcCircuits->Get(szTypeName.Text());
	if (pcPlaceholder != NULL)
	{
		szTypeName.Kill();
		return pcPlaceholder;
	}
	pcPlaceholder = mpcCircuits->Add(szTypeName.Text());
	szTypeName.Kill();
	if (pcPlaceholder == NULL)
	{
		return NULL;
	}

	apcPins.Init();
	aszNames.Init();
	GetPlaceholderPins(pcSymbol, pcLibSymbol, &apcPins, &aszNames);
	sBox.Init(SInt32Vec2(-2, -1), SInt32Vec2(0, 1));
	for (i = 0; i < apcPins.NumElements(); i++)
	{
		pcPin = apcPins.GetPtr(i);
		iWeight = GetKicadSchemLibPinPlace(&apcPins, i, &eAlignment);
		pcPinView = NewMalloc<CPinView>();
		pcPinView->Init(0, (int32)(3 * i), ROT_East, aszNames.GetText(i), "LVC", false, 1, RADIX_Hexadecimal, &sBox);
		SetKicadSchemPinAppearance(pcPinView, pcPin, eAlignment, iWeight);
		pcPlaceholder->Add(pcPinView);
	}
	aszNames.Kill();
	apcPins.Kill();
	return pcPlaceholder;
}


//////////////////////////////////////////////////////////////////////////
//
// The unit's shown pins and the names their placeholder pins have: the
// pin's name, or its number if it has none, made unique by its number.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::GetPlaceholderPins(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, CArrayChars* paszNames)
{
	size				i;
	CKicadSchemLibPin*	pcPin;
	CChars				szName;
	char*				szPinName;

	for (i = 0; i < pcLibSymbol->NumPins(); i++)
	{
		pcPin = pcLibSymbol->GetPin(i);
		if (!pcSymbol->IsUnitPin(pcPin) || pcPin->IsHidden())
		{
			continue;
		}
		szPinName = pcPin->GetName();
		if (StrEmpty(szPinName) || (StringCompare(szPinName, "~") == 0))
		{
			szName.Init(pcPin->GetNumber());
		}
		else
		{
			szName.Init(szPinName);
		}
		if (paszNames->Contains(szName.Text()))
		{
			szName.Append('_');
			szName.Append(pcPin->GetNumber());
		}
		papcPins->Add(pcPin);
		paszNames->Add(szName.Text());
		szName.Kill();
	}
}
