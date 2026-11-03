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
#include "LogicimLib/CircuitFile.h"
#include "KicadImporterLib/KicadSchemLogicimConverter.h"
#include "LogicimCircuit.h"


#define LOGICIM_MAIN_CIRCUIT		"Main"


//////////////////////////////////////////////////////////////////////////
//
// Kill must be called even when this fails.  The circuit's text is
// coloured in eColourOrder, the canvas's.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Init(const char* szPropagationTimes, const char* szFontDirectory, EColourOrder eColourOrder)
{
	Ptr<CFontFamily>	pFamily;
	bool				bFonts;

	mcColours.InitDark();
	mcFamilies.Init();
	mcEditor.Init(&mcFamilies, &mcFonts);

	//Nothing else holds the font families, so they are rooted.
	bFonts = LoadCircuitFonts(&mcFonts, szFontDirectory, eColourOrder);
	pFamily = mcFonts.GetSansSerif();
	if (pFamily.IsNotNull())
	{
		ORoot()->Add(pFamily);
	}
	pFamily = mcFonts.GetMonospaced();
	if (pFamily.IsNotNull())
	{
		ORoot()->Add(pFamily);
	}
	if (!bFonts || !mcFamilies.Load(szPropagationTimes))
	{
		return false;
	}

	mcEditor.AddCircuit(LOGICIM_MAIN_CIRCUIT);
	return CreateSimulation();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimCircuit::Kill(void)
{
	mcEditor.Kill();
	mcFamilies.Kill();
	mcColours.Kill();
	RemoveFontFamily(mcFonts.GetSansSerif());
	RemoveFontFamily(mcFonts.GetMonospaced());
	mcFonts.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimCircuit::RemoveFontFamily(Ptr<CFontFamily> pFamily)
{
	if (pFamily.IsNotNull())
	{
		ORoot()->Remove(pFamily);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Simulates the circuit being edited afresh.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::CreateSimulation(void)
{
	return mcEditor.Simulate(mcEditor.GetCircuit() != NULL ? mcEditor.GetCircuit()->GetTypeName() : LOGICIM_MAIN_CIRCUIT);
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
	if ((mcEditor.GetSimulation() != NULL) && mcEditor.CanRun())
	{
		return mcEditor.GetSimulation()->RunToTime(iTimeForward);
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Save(const char* szFilename)
{
	return mcEditor.Save(szFilename);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Load(const char* szFilename)
{
	return mcEditor.Load(szFilename);
}


//////////////////////////////////////////////////////////////////////////
//
// Converts the KiCad schematic to a circuit named as its file ("Counter
// 16bit"), with the placeholder subcircuits made for its parts, and loads
// them as a project is loaded so the import can be undone.  If it can't
// be imported the project is left as it was.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::ImportKicadSchematic(const char* szFilename)
{
	CKicadSchemFile					cFile;
	CKicadSchemNetlist				cNetlist;
	CCircuitViews					cCircuits;
	CKicadSchemLogicimConverter		cConverter;
	CXMLWriter						cWriter;
	CMarkup							cMarkup;
	CMarkupTag*						pcRoot;
	CChars							szName;
	const char*						szStart;
	const char*						sz;
	bool							bResult;

	szStart = szFilename;
	for (sz = szFilename; *sz != '\0'; sz++)
	{
		if ((*sz == '/') || (*sz == '\\'))
		{
			szStart = sz + 1;
		}
	}
	szName.Init(szStart);
	if (szName.EndsWith(".kicad_sch"))
	{
		szName.RemoveFromEnd(10);
	}

	cFile.Init();
	bResult = cFile.Load((char*)szFilename);
	cNetlist.Init(&cFile);
	cCircuits.Init();
	cConverter.Init(&cFile, &cNetlist, &cCircuits, &mcFonts);
	bResult = bResult && cNetlist.Build() && cConverter.Convert(szName.Text());
	if (bResult)
	{
		cWriter.Init();
		SaveCircuitsXML(&cCircuits, cConverter.GetCircuit(), NULL, &cWriter);
		cMarkup.Init();
		pcRoot = ReadCircuitTextRoot(&cMarkup, cWriter.GetText(), szFilename, "Circuits");
		bResult = (pcRoot != NULL) && mcEditor.LoadXML(pcRoot, szFilename);
		cMarkup.Kill();
		cWriter.Kill();
		if (bResult)
		{
			mcEditor.PushUndo(false);
		}
	}

	cConverter.Kill();
	cCircuits.Kill();
	cNetlist.Kill();
	cFile.Kill();
	szName.Kill();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CCircuitEditor* CLogicimCircuit::GetEditor(void) { return &mcEditor; }
CCircuitColours* CLogicimCircuit::GetColours(void) { return &mcColours; }
CCircuitFonts* CLogicimCircuit::GetFonts(void) { return &mcFonts; }
int64 CLogicimCircuit::GetTime(void) { return (mcEditor.GetSimulation() != NULL) ? mcEditor.GetSimulation()->GetTime() : -1; }
