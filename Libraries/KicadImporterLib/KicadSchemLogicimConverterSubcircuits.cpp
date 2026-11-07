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
// Whether the net is a supply's: a ground ("GND"), a supply pin's name
// ("VCC") or a voltage ("+3V3", "3.3V", "5V").
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemPowerNetName(const char* szName)
{
	size	i;
	bool	bVolts;

	if (CKicadSchemLogicimConverter::IsGroundName(szName) || IsKicadSchemPowerPinName(szName) || (szName[0] == '+') || (szName[0] == '-'))
	{
		return true;
	}
	if ((szName[0] < '0') || (szName[0] > '9'))
	{
		return false;
	}
	bVolts = false;
	for (i = 0; szName[i] != '\0'; i++)
	{
		if ((szName[i] == 'V') || (szName[i] == 'v'))
		{
			bVolts = true;
		}
		else if (((szName[i] < '0') || (szName[i] > '9')) && (szName[i] != '.'))
		{
			return false;
		}
	}
	return bVolts;
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
// The side of an instance a circuit pin facing eRotation is shown on.
//
//////////////////////////////////////////////////////////////////////////
static EPinAlignment GetKicadSchemBoardPinAlignment(ERotation eRotation)
{
	switch (eRotation)
	{
	case ROT_West:	return PA_Left;
	case ROT_North:	return PA_Top;
	case ROT_South:	return PA_Bottom;
	default:		return PA_Right;
	}
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
	int					aiDX[4] = { 1, 0, -1, 0 };
	int					aiDY[4] = { 0, 1, 0, -1 };

	//The pin points into the connector's body from where it's wired (library Y is up).
	mpcFile->GetPinPosition(psOuter, pcSymbol, pcPin);
	iAngle = GetKicadSchemQuarterTurns(pcPin->GetAngle());
	sLibrary = *pcPin->GetPosition() + SFloat64Vec2(aiDX[iAngle], aiDY[iAngle]) * pcPin->GetLength();
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
	CPinView*				pcPinView;
	SFloat64Vec2			sOuter;
	ERotation				eRotation;
	size					uiItem;
	size					uiTerminal;
	char*					szNumber;

	pcSubnet = mpcNetlist->GetPinSubnet(pcSymbol, pcPin);
	pcNet = (pcSubnet != NULL) ? pcSubnet->GetNet() : NULL;
	if ((pcNet == NULL) || StrEmpty(pcNet->GetName()))
	{
		return;
	}

	szName.Init();
	if (!GetKicadSchemBoardPinName(pcSymbol, pcPin, pcNet, &szName))
	{
		szName.Kill();
		return;
	}

	szNumber = pcPin->GetNumber();
	uiItem = mcModel.FindBoardPin(szName.Text());
	if (uiItem != ARRAY_ELEMENT_NOT_FOUND)
	{
		szName.Kill();
		pcPinView = (CPinView*)mcModel.GetItem(uiItem)->pcView;
		if (pcConnector != NULL)
		{
			pcConnector->AddPin(szNumber, pcPinView);
		}

		//Another part of the net (joined by its labels) is already joined through its tunnels.
		uiTerminal = mcModel.FindPortTerminal(pcPinView->GetPortView((size)0));
		if ((uiTerminal != ARRAY_ELEMENT_NOT_FOUND) && (mcModel.GetTerminal(uiTerminal)->pcSubnet == pcSubnet))
		{
			AddPinTerminal(pcSymbol, pcPin, pcPinView->GetPortView((size)0), uiItem);
		}
		return;
	}

	eRotation = GetBoardPinFacing(pcSymbol, pcPin, &sOuter);
	pcPinView = NewMalloc<CPinView>();
	pcPinView->Init(0, 0, eRotation, szName.Text(), "LVC", false, 1, RADIX_Binary, mpcFonts);
	szName.Kill();
	pcPinView->SetSubcircuitAppearance(GetKicadSchemBoardPinAlignment(eRotation), PAN_Negative, atoi(szNumber), false, false);
	//Placed as a part is (its port is its position), so a label KiCad has on the connector's pin is attached to it.
	uiItem = AddItem(pcPinView, KSLIK_BoardPin, &sOuter, NULL);
	msCounts.uiBoardPins++;
	if (pcConnector != NULL)
	{
		pcConnector->AddPin(szNumber, pcPinView);
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
// An instance of the placeholder subcircuit for the symbol's unit, turned
// as KiCad turns the symbol (a mirrored symbol isn't mirrored).
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
	char*									szValue;

	pcPlaceholder = GetPlaceholderCircuit(pcSymbol, pcLibSymbol);
	if (pcPlaceholder == NULL)
	{
		return false;
	}

	szValue = pcSymbol->GetValue();
	pcInstance = NewMalloc<CSubcircuitInstanceView>();
	pcInstance->Init(0, 0, GetKicadRotation(ROT_East, pcSymbol->GetAngle()), pcSymbol->GetReference(), (szValue != NULL) ? szValue : "", pcPlaceholder, mpcFonts, KICAD_SCHEM_LOGICIM_PIN_SPACING);
	uiItem = AddItem(pcInstance, KSLIK_Subcircuit, pcSymbol->GetPosition(), NULL);

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

	//A pin points into the body, so a pin pointing right (0) is on the left.
	pcPin = papcPins->GetPtr(i);
	iAngle = GetKicadSchemQuarterTurns(pcPin->GetAngle());
	switch (iAngle)
	{
	case 1:		*peAlignment = PA_Bottom;	break;
	case 2:		*peAlignment = PA_Right;	break;
	case 3:		*peAlignment = PA_Top;		break;
	default:	*peAlignment = PA_Left;		break;
	}

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
// A board drawn as a symbol (one with a Schematic property, see
// KICAD_SCHEM_BOARD_SCHEMATIC_PROPERTY): an instance of the board's
// circuit, made from its own schematic by the board source.  The
// symbol's pins are joined to the circuit's pins of the board's connector
// with the same numbers; a pin on a supply or on nothing on the board has
// no circuit pin and isn't joined.  False, to make it a placeholder, if
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
	CArrayTemplatePtr<CPortView>			apcJoined;
	CSubcircuitInstanceView*				pcInstance;
	SSubcircuitPinView*						psPin;
	CPinView*								pcPinView;
	size									uiItem;
	size									i;
	char*									szValue;

	if (mpcBoardSource == NULL)
	{
		return false;
	}
	szSchematic = pcSymbol->GetProperties()->GetValue(KICAD_SCHEM_BOARD_SCHEMATIC_PROPERTY);
	if (StrEmpty(szSchematic))
	{
		szSchematic = pcLibSymbol->GetProperties()->GetValue(KICAD_SCHEM_BOARD_SCHEMATIC_PROPERTY);
	}
	if (StrEmpty(szSchematic))
	{
		return false;
	}

	pcBoard = mpcBoardSource->GetBoard(szSchematic, mpcFile->GetFileName());
	if (pcBoard == NULL)
	{
		gcLogger.Error2(__METHOD__, " Board [", pcSymbol->GetReference(), "]'s schematic [", szSchematic, "] couldn't be made, it's a placeholder.", NULL);
		return false;
	}

	apcPins.Init();
	aszNames.Init();
	GetPlaceholderPins(pcSymbol, pcLibSymbol, &apcPins, &aszNames);
	pcConnector = pcBoard->FindConnector(apcPins.NumElements());
	if (pcConnector == NULL)
	{
		gcLogger.Error2(__METHOD__, " Board [", pcSymbol->GetReference(), "]'s schematic [", szSchematic, "] has no connector, it's a placeholder.", NULL);
		aszNames.Kill();
		apcPins.Kill();
		return false;
	}
	if (!pcBoard->IsLaidOut())
	{
		LayOutBoardPins(&apcPins, pcConnector);
		pcBoard->SetLaidOut();
	}

	szValue = pcSymbol->GetValue();
	pcInstance = NewMalloc<CSubcircuitInstanceView>();
	pcInstance->Init(0, 0, GetKicadRotation(ROT_East, pcSymbol->GetAngle()), pcSymbol->GetReference(), (szValue != NULL) ? szValue : "", pcBoard->GetCircuit(), mpcFonts, KICAD_SCHEM_LOGICIM_PIN_SPACING);
	uiItem = AddItem(pcInstance, KSLIK_Subcircuit, pcSymbol->GetPosition(), NULL);

	//Two of the symbol's pins on one circuit pin (one net on the board) are joined to it once.
	apcJoined.Init();
	for (i = 0; i < apcPins.NumElements(); i++)
	{
		pcPinView = pcConnector->GetPin(apcPins.GetPtr(i)->GetNumber());
		psPin = (pcPinView != NULL) ? pcInstance->GetPin(pcPinView) : NULL;
		if ((psPin != NULL) && (apcJoined.Find(psPin->pcPortView) == ARRAY_ELEMENT_NOT_FOUND))
		{
			apcJoined.Add(psPin->pcPortView);
			AddPinTerminal(pcSymbol, apcPins.GetPtr(i), psPin->pcPortView, uiItem);
		}
	}
	apcJoined.Kill();
	aszNames.Kill();
	apcPins.Kill();
	msCounts.uiBoards++;
	return true;
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
		pcPinView->SetSubcircuitAppearance(eAlignment, PAN_Negative, iWeight, strstr(pcPin->GetGraphicStyle(), "inverted") != NULL, strstr(pcPin->GetGraphicStyle(), "clock") != NULL);
	}
	apcLaidOut.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The placeholder for the library symbol's unit, made the first time.
// Its pins are on the sides the library symbol's are, in the same order
// along them.  It's named as the library symbol, without its library, and
// the unit's letter if it has more than one.
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CKicadSchemLogicimConverter::GetPlaceholderCircuit(CKicadSchemSymbol* pcSymbol, CKicadSchemLibSymbol* pcLibSymbol)
{
	CChars									szTypeName;
	char*									szLibName;
	char*									szColon;
	CCircuitView*							pcPlaceholder;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	CArrayChars								aszNames;
	size									i;
	CKicadSchemLibPin*						pcPin;
	EPinAlignment							eAlignment;
	int32									iWeight;
	CPinView*								pcPinView;
	SInt32Bounds							sBox;

	szLibName = pcLibSymbol->GetName();
	szColon = strrchr(szLibName, ':');
	szTypeName.Init((szColon != NULL) ? szColon + 1 : szLibName);
	if (pcLibSymbol->NumUnits() > 1)
	{
		szTypeName.Append('_');
		szTypeName.Append((char)('A' + pcSymbol->GetUnit() - 1));
	}

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
		pcPinView->SetSubcircuitAppearance(eAlignment, PAN_Negative, iWeight, strstr(pcPin->GetGraphicStyle(), "inverted") != NULL, strstr(pcPin->GetGraphicStyle(), "clock") != NULL);
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
