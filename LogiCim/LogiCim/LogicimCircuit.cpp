/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela LogiCim

Codaphela LogiCim is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela LogiCim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Codaphela LogiCim.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include "BaseLib/XMLWriter.h"
#include "BaseLib/Markup.h"
#include "StandardLib/Objects.h"
#include "StandardLib/ObjectReader.h"
#include "StandardLib/ObjectWriter.h"
#include "LogicimLib/CircuitFile.h"
#include "KicadImporterLib/KicadSchemBoards.h"
#include "LogicimCircuit.h"


#define LOGICIM_MAIN_CIRCUIT		"Main"


//////////////////////////////////////////////////////////////////////////
//
// Setup makes the editor; see it.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimCircuit::Init(void)
{
	PreInit();

	mcColours.InitDark();
	mcFamilies.Init();

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimCircuit::Class(void)
{
	U_Unknown(CFamilyVoltageConfigurationStore, mcFamilies);
	M_Pointer(mpEditor);
	U_Unknown(CCircuitColours, mcColours);
}


//////////////////////////////////////////////////////////////////////////
//
// The editor is freed with this.  It only points to the families, it
// doesn't use them as it's freed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimCircuit::Free(void)
{
	mcFamilies.Kill();
	mcColours.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// A project is saved as a circuits file, see Save(const char*).
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Save(CObjectWriter* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Load(CObjectReader* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The families, the fonts and the editor with an empty "Main" circuit
// being simulated.  Free still frees whatever was made when this fails.
// The circuit's text is coloured in eColourOrder, the canvas's.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Setup(const char* szPropagationTimes, const char* szFontDirectory, EColourOrder eColourOrder)
{
	Ptr<CCircuitFonts>	pFonts;

	pFonts = LoadCircuitFonts(szFontDirectory, eColourOrder);
	mpEditor = OMalloc<CCircuitEditor>(&mcFamilies, pFonts);
	if (!pFonts->HasFamilies() || !mcFamilies.Load(szPropagationTimes))
	{
		return false;
	}

	mpEditor->AddCircuit(LOGICIM_MAIN_CIRCUIT);
	return CreateSimulation();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// Simulates the circuit being edited afresh.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::CreateSimulation(void)
{
	return mpEditor->Simulate(mpEditor->GetCircuit() != NULL ? mpEditor->GetCircuit()->GetTypeName() : LOGICIM_MAIN_CIRCUIT);
}


//////////////////////////////////////////////////////////////////////////
//
// Starts the simulation again from nothing.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Reset(void)
{
	return CreateSimulation();
}


//////////////////////////////////////////////////////////////////////////
//
// iTimeForward is in Logicim's time units (1/1024 ns).  Nothing runs
// while an edit is in progress.  Returns true if any events happened, so
// what the circuit shows may have changed.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Run(int64 iTimeForward)
{
	if ((mpEditor->GetSimulation() != NULL) && mpEditor->CanRun())
	{
		return mpEditor->GetSimulation()->RunToTime(iTimeForward);
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Save(const char* szFilename)
{
	return mpEditor->Save(szFilename);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Load(const char* szFilename)
{
	return mpEditor->Load(szFilename);
}


//////////////////////////////////////////////////////////////////////////
//
// Converts the KiCad schematic to a circuit named as its file ("Audio
// Board"), with a circuit for each board drawn on it (a symbol with a
// Schematic property, found through KiCad's path variables), and the
// boards drawn on those, and the placeholder subcircuits made for its
// parts, and loads them as a project is loaded so the import can be
// undone.  If it can't be imported the project is left as it was.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::ImportKicadSchematic(const char* szFilename)
{
	CKicadPathVariables			cVariables;
	CCircuitViews				cCircuits;
	CKicadSchemBoardImporter	cImporter;
	CCircuitView*				pcTop;
	CXMLWriter					cWriter;
	CMarkup						cMarkup;
	CMarkupTag*					pcRoot;
	bool						bResult;

	cVariables.Init();
	cVariables.LoadKicadCommon();
	cCircuits.Init();
	cImporter.Init(&cCircuits, mpEditor->GetFonts(), &cVariables);
	pcTop = cImporter.Import(szFilename);
	bResult = pcTop != NULL;
	if (bResult)
	{
		cWriter.Init();
		SaveCircuitsXML(&cCircuits, pcTop, NULL, &cWriter);
		cMarkup.Init();
		pcRoot = ReadCircuitTextRoot(&cMarkup, cWriter.GetText(), szFilename, "Circuits");
		bResult = (pcRoot != NULL) && mpEditor->LoadXML(pcRoot, szFilename);
		cMarkup.Kill();
		cWriter.Kill();
		if (bResult)
		{
			mpEditor->PushUndo(false);
		}
	}

	cImporter.Kill();
	cCircuits.Kill();
	cVariables.Kill();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CCircuitEditor* CLogicimCircuit::GetEditor(void) { return &mpEditor; }
CCircuitColours* CLogicimCircuit::GetColours(void) { return &mcColours; }
CCircuitFonts* CLogicimCircuit::GetFonts(void) { return mpEditor->GetFonts(); }
int64 CLogicimCircuit::GetTime(void) { return (mpEditor->GetSimulation() != NULL) ? mpEditor->GetSimulation()->GetTime() : -1; }
