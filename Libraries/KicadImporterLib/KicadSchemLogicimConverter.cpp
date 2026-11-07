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


//The rotations, turning right from East, to try something in each of.
ERotation gaeKicadSchemLogicimRotations[4] = { ROT_East, ROT_South, ROT_West, ROT_North };


//Reference prefixes and library names of the symbols that are left out.
static const char* gaszKicadSchemSkippedReferences[] = { "R", "RN", "C", "CN", "L", "FB", "D", "LED", "H", "MH", "TP", "NT", "FID" };
static const char* gaszKicadSchemSkippedLibraryNames[] = { "MountingHole", "NetTie", "TestPoint", "PWR_FLAG", "Fiducial" };


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::Init(CKicadSchemFile* pcFile, CKicadSchemNetlist* pcNetlist, CCircuitViews* pcCircuits, CCircuitFonts* pcFonts)
{
	mpcFile = pcFile;
	mpcNetlist = pcNetlist;
	mpcCircuits = pcCircuits;
	mpcFonts = pcFonts;
	mcModel.Init();
	masAlignments.Init();
	masRegulated.Init();
	mpcBoard = NULL;
	mpcBoardSource = NULL;
	mfSpread = 1.0;
	msCounts.Init();
	mcBuses.Init(pcNetlist, pcFonts, &mcModel, &msCounts);
}


//////////////////////////////////////////////////////////////////////////
//
// The circuits keep what was made.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::Kill(void)
{
	mcBuses.Kill();
	masRegulated.Kill();
	masAlignments.Kill();
	mcModel.Kill();
	mpcBoardSource = NULL;
	mpcBoard = NULL;
	mpcFonts = NULL;
	mpcCircuits = NULL;
	mpcNetlist = NULL;
	mpcFile = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// pcBoard records the pins of the sheet's connectors, so the sheet can be
// drawn as a symbol on another board.  A symbol with a Schematic property
// (another board drawn on this one) is an instance of the board
// pcBoardSource makes from it.  Either may be NULL.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::SetBoards(CKicadSchemBoard* pcBoard, CKicadSchemBoardSource* pcBoardSource)
{
	mpcBoard = pcBoard;
	mpcBoardSource = pcBoardSource;
}


//////////////////////////////////////////////////////////////////////////
//
// Makes the circuit szTypeName in the circuits from the sheet.  The
// netlist must be built.  Fails if there is already a circuit of the type.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::Convert(const char* szTypeName)
{
	size			i;
	size			uiFirstUnconnected;
	float64			fWorse;
	float64			fSpread;
	bool			bRouted;
	CCircuitView*	pcCircuit;

	pcCircuit = mpcCircuits->Add(szTypeName);
	if (pcCircuit == NULL)
	{
		return false;
	}
	mcModel.SetCircuit(pcCircuit);

	for (i = 0; i < mpcFile->NumSymbols(); i++)
	{
		if (!AddSymbol(mpcFile->GetSymbol(i)))
		{
			return false;
		}
	}
	ApplyRegulatedVoltages();
	mcBuses.AddTunnels();
	mcBuses.AddBuses();
	RemoveUnconnectedItems();
	AttachItems();
	MeasureImportBoxes();
	AddConnections();
	FindAlignments();

	//Wired spread out, then as close as it wires as well: the least spread if that does, otherwise halfway between
	//the closest that does and the widest that doesn't, a few times.  The circuit is left wired at the chosen spread.
	Place(KICAD_SCHEM_LOGICIM_SPREAD_MAXIMUM);
	uiFirstUnconnected = Route();
	mfSpread = KICAD_SCHEM_LOGICIM_SPREAD_MAXIMUM;
	Place(KICAD_SCHEM_LOGICIM_SPREAD_MINIMUM);
	msCounts.uiUnconnected = Route();
	bRouted = true;
	if (msCounts.uiUnconnected <= uiFirstUnconnected)
	{
		mfSpread = KICAD_SCHEM_LOGICIM_SPREAD_MINIMUM;
	}
	else
	{
		fWorse = KICAD_SCHEM_LOGICIM_SPREAD_MINIMUM;
		bRouted = false;
		for (i = 0; i < KICAD_SCHEM_LOGICIM_SPREAD_HALVINGS; i++)
		{
			fSpread = (mfSpread + fWorse) / 2;
			Place(fSpread);
			msCounts.uiUnconnected = Route();
			if (msCounts.uiUnconnected > uiFirstUnconnected)
			{
				fWorse = fSpread;
				bRouted = false;
			}
			else
			{
				mfSpread = fSpread;
				bRouted = true;
			}
		}
	}
	if (!bRouted)
	{
		Place(mfSpread);
		msCounts.uiUnconnected = Route();
	}
	Explode();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::AddSymbol(CKicadSchemSymbol* pcSymbol)
{
	CKicadSchemLibSymbol*		pcLibSymbol;
	EKicadSchemLogicFunction	eFunction;
	CChars						szFamily;
	CChars						szPartType;
	bool						bAdded;

	pcLibSymbol = mpcFile->GetLibSymbol(pcSymbol);
	if (pcLibSymbol == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Symbol [", pcSymbol->GetReference(), "] has no library symbol.", NULL);
	}
	if (pcLibSymbol->IsPower())
	{
		return AddPower(pcSymbol, pcLibSymbol);
	}
	if (IsSkippedSymbol(pcSymbol))
	{
		msCounts.uiSkipped++;
		return true;
	}
	if (AddRegulator(pcSymbol, pcLibSymbol) || AddBoardPins(pcSymbol, pcLibSymbol) || AddBoard(pcSymbol, pcLibSymbol))
	{
		return true;
	}

	//A real 74 series part that isn't gates.
	szFamily.Init();
	szPartType.Init();
	if (ParseKicadSchemLogic74Part(pcLibSymbol->GetName(), &szFamily, &szPartType) || ParseKicadSchemLogic74Part(pcSymbol->GetValue(), &szFamily, &szPartType))
	{
		if (AddLogic74Block(pcSymbol, pcLibSymbol, szFamily.Text(), szPartType.Text()))
		{
			szPartType.Kill();
			szFamily.Kill();
			return true;
		}
	}
	szPartType.Kill();
	szFamily.Kill();

	szFamily.Init();
	szPartType.Init();
	eFunction = ParseKicadSchemLogicFunction(pcLibSymbol->GetName(), &szFamily, &szPartType);
	if (eFunction == KSLF_None)
	{
		eFunction = ParseKicadSchemLogicFunction(pcSymbol->GetValue(), &szFamily, &szPartType);
	}
	bAdded = false;
	if (eFunction != KSLF_None)
	{
		bAdded = AddLogicParts(pcSymbol, pcLibSymbol, eFunction, szFamily.Text(), szPartType.Text());
	}
	szPartType.Kill();
	szFamily.Kill();
	if (bAdded)
	{
		return true;
	}
	return AddPlaceholder(pcSymbol, pcLibSymbol);
}


//////////////////////////////////////////////////////////////////////////
//
// No points yet.
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemPointMean::Init(void)
{
	sSum.Zero();
	uiCount = 0;
}


//////////////////////////////////////////////////////////////////////////
//
// One more point.
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemPointMean::Add(SFloat64Vec2* psPoint)
{
	sSum += *psPoint;
	uiCount++;
}


//////////////////////////////////////////////////////////////////////////
//
// The middle of the points, the origin if there are none.
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemPointMean::Get(SFloat64Vec2* psMean)
{
	if (uiCount == 0)
	{
		psMean->Zero();
		return;
	}
	*psMean = sSum / (float64)uiCount;
}


//////////////////////////////////////////////////////////////////////////
//
// The way from the inputs to the output.
//
//////////////////////////////////////////////////////////////////////////
ERotation GetKicadSchemFacing(SFloat64Vec2* psFrom, SFloat64Vec2* psTo)
{
	float64		fDX;
	float64		fDY;

	fDX = psTo->x - psFrom->x;
	fDY = psTo->y - psFrom->y;
	if (fabs(fDX) >= fabs(fDY))
	{
		return (fDX >= 0) ? ROT_East : ROT_West;
	}
	return (fDY > 0) ? ROT_South : ROT_North;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimConverter::AddPinTerminal(CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CPortView* pcPortView, size uiItem)
{
	SFloat64Vec2		sPosition;

	mpcFile->GetPinPosition(&sPosition, pcSymbol, pcPin);
	mcModel.AddTerminal(mpcNetlist->GetPinSubnet(pcSymbol, pcPin), &sPosition, pcPortView, uiItem);
}


//////////////////////////////////////////////////////////////////////////
//
// Passive parts and mechanical symbols, by their reference's letters or
// their library symbol.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsSkippedSymbol(CKicadSchemSymbol* pcSymbol)
{
	CChars	szPrefix;
	char*	szReference;
	size	i;
	bool	bSkipped;

	for (i = 0; i < sizeof(gaszKicadSchemSkippedLibraryNames) / sizeof(const char*); i++)
	{
		if ((strstr(pcSymbol->GetLibId(), gaszKicadSchemSkippedLibraryNames[i]) != NULL) || (strstr(pcSymbol->GetLibSymbolName(), gaszKicadSchemSkippedLibraryNames[i]) != NULL))
		{
			return true;
		}
	}

	szReference = pcSymbol->GetReference();
	if (szReference == NULL)
	{
		return false;
	}
	for (i = 0; ((szReference[i] >= 'A') && (szReference[i] <= 'Z')) || ((szReference[i] >= 'a') && (szReference[i] <= 'z')); i++)
	{
	}
	szPrefix.Init(szReference, 0, i);
	szPrefix.UpperCase();
	bSkipped = false;
	for (i = 0; i < sizeof(gaszKicadSchemSkippedReferences) / sizeof(const char*); i++)
	{
		if (szPrefix.Equals(gaszKicadSchemSkippedReferences[i]))
		{
			bSkipped = true;
			break;
		}
	}
	szPrefix.Kill();
	return bSkipped;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimConverter::IsGroundName(const char* szName)
{
	CChars	szUpper;
	bool	bGround;

	szUpper.Init(szName);
	szUpper.UpperCase();
	bGround = szUpper.StartsWith("GND") || szUpper.StartsWith("VSS") || szUpper.StartsWith("AGND") || szUpper.StartsWith("DGND") || szUpper.StartsWith("EARTH") || szUpper.Equals("0V");
	szUpper.Kill();
	return bGround;
}


//////////////////////////////////////////////////////////////////////////
//
// The voltage in a supply's name: "+5V" is 5, "3.3V" and "+3V3" are 3.3
// and "12VOut" is 12.  A name without one (VCC) is 5.
//
//////////////////////////////////////////////////////////////////////////
float32 CKicadSchemLogicimConverter::GetSupplyVoltage(const char* szName)
{
	size		i;
	float64		fVoltage;
	float64		fScale;

	for (i = 0; (szName[i] != '\0') && ((szName[i] < '0') || (szName[i] > '9')); i++)
	{
	}
	if (szName[i] == '\0')
	{
		return 5.0f;
	}

	fVoltage = 0;
	for (; (szName[i] >= '0') && (szName[i] <= '9'); i++)
	{
		fVoltage = fVoltage * 10 + (szName[i] - '0');
	}
	if ((szName[i] == '.') || (((szName[i] == 'V') || (szName[i] == 'v')) && (szName[i + 1] >= '0') && (szName[i + 1] <= '9')))
	{
		i++;
		fScale = 0.1;
		for (; (szName[i] >= '0') && (szName[i] <= '9'); i++)
		{
			fVoltage += fScale * (szName[i] - '0');
			fScale /= 10;
		}
	}
	if (fVoltage <= 0)
	{
		return 5.0f;
	}
	return (float32)fVoltage;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CKicadSchemLogicimConverter::GetCircuit(void) { return mcModel.GetCircuit(); }
float64 CKicadSchemLogicimConverter::GetSpread(void) { return mfSpread; }
SKicadSchemLogicimCounts* CKicadSchemLogicimConverter::GetCounts(void) { return &msCounts; }
CKicadSchemLogicimModel* CKicadSchemLogicimConverter::GetModel(void) { return &mcModel; }
