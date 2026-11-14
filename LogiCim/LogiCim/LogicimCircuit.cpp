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
	masProjects.Init();

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
	U_Unknown(CArrayLogicimProjectStart, masProjects);
}


//////////////////////////////////////////////////////////////////////////
//
// The editor is freed with this.  It only points to the families, it
// doesn't use them as it's freed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimCircuit::Free(void)
{
	KillProjects();
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
	if (!CreateSimulation())
	{
		return false;
	}
	StartProject("");
	return true;
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
// szFilename is the project's file from now on, if it was saved, and it
// has no changes.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Save(const char* szFilename)
{
	SLogicimProjectStart*	psProject;
	bool					bResult;

	bResult = mpEditor->Save(szFilename);
	psProject = GetProject();
	if (bResult && (psProject != NULL))
	{
		psProject->szFilename.Set(szFilename);
		psProject->uiSavedId = GetChangeId();
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// szFilename is the project's file from now on, if it was loaded.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::Load(const char* szFilename)
{
	bool	bResult;

	bResult = mpEditor->Load(szFilename);
	if (bResult)
	{
		StartProject(szFilename);
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// Converts the KiCad schematic to a circuit named as its file ("Audio
// Board"), with a circuit for each board drawn on it (a symbol with a
// Schematic property, found through KiCad's path variables), and the
// boards drawn on those, and the placeholder subcircuits made for its
// parts, and loads them as a project is loaded so the import can be
// undone.  pcProgress (NULL for none) is told each board's steps.  The
// project's file is the schematic's name as an .xml file
// beside it ("Audio Board.xml"), not yet written.  If it can't be
// imported the project is left as it was.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::ImportKicadSchematic(const char* szFilename, CKicadSchemImportProgress* pcProgress)
{
	CKicadPathVariables			cVariables;
	CCircuitViews				cCircuits;
	CKicadSchemBoardImporter	cImporter;
	CCircuitView*				pcTop;
	bool						bResult;
	CChars						szProject;

	cVariables.Init();
	cVariables.LoadKicadCommon();
	cCircuits.Init();
	cImporter.Init(&cCircuits, mpEditor->GetFonts(), &cVariables);
	cImporter.SetProgress(pcProgress);
	pcTop = cImporter.Import(szFilename);
	bResult = (pcTop != NULL) && LoadCircuits(&cCircuits, pcTop, szFilename);
	if (bResult)
	{
		//As imported it has no changes: it can be imported again.
		szProject.Init(szFilename);
		if (szProject.EndsWith(".kicad_sch"))
		{
			szProject.RemoveFromEnd(10);
		}
		szProject.Append(".xml");
		StartProject(szProject.Text());
		szProject.Kill();
	}

	cImporter.Kill();
	cCircuits.Kill();
	cVariables.Kill();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// Replaces the project with a new one, an empty "Main" circuit being
// simulated as LogiCim starts with, which hasn't a file yet.  It's loaded
// as a project is loaded, so it can be undone back to the old one.  If
// it can't be made the project is left as it was.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::NewProject(void)
{
	CCircuitViews	cCircuits;
	CCircuitView*	pcTop;
	bool			bResult;

	cCircuits.Init();
	pcTop = cCircuits.Add(LOGICIM_MAIN_CIRCUIT);
	bResult = (pcTop != NULL) && LoadCircuits(&cCircuits, pcTop, "New Project");
	if (bResult)
	{
		StartProject("");
	}
	cCircuits.Kill();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// The circuits, with pcTop simulated, in place of the project's, loaded
// as a project is loaded so it can be undone.  szSource names them in
// errors.  If they can't be loaded the project is left as it was.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::LoadCircuits(CCircuitViews* pcCircuits, CCircuitView* pcTop, const char* szSource)
{
	CXMLWriter	cWriter;
	CMarkup		cMarkup;
	CMarkupTag*	pcRoot;
	bool		bResult;

	cWriter.Init();
	SaveCircuitsXML(pcCircuits, pcTop, NULL, &cWriter);
	cMarkup.Init();
	pcRoot = ReadCircuitTextRoot(&cMarkup, cWriter.GetText(), szSource, "Circuits");
	bResult = (pcRoot != NULL) && mpEditor->LoadXML(pcRoot, szSource);
	cMarkup.Kill();
	cWriter.Kill();
	if (bResult)
	{
		mpEditor->PushUndo(false);
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CCircuitEditor* CLogicimCircuit::GetEditor(void) { return &mpEditor; }
CCircuitColours* CLogicimCircuit::GetColours(void) { return &mcColours; }
CCircuitFonts* CLogicimCircuit::GetFonts(void) { return mpEditor->GetFonts(); }


//////////////////////////////////////////////////////////////////////////
//
// The project the current undo snapshot is of: the start nearest before
// it in the undo stack.  One whose start was dropped off the bottom of
// the stack is the latest that started before the oldest snapshot kept.
// NULL before Setup.
//
//////////////////////////////////////////////////////////////////////////
SLogicimProjectStart* CLogicimCircuit::GetProject(void)
{
	CUndoStack*				pcUndo;
	SLogicimProjectStart*	psProject;
	SLogicimProjectStart*	psBefore;
	size					uiIndex;
	size					uiId;
	size					i;

	if (mpEditor.IsNull())
	{
		return NULL;
	}
	pcUndo = mpEditor->GetUndoStack();
	if (pcUndo->NumSnapshots() == 0)
	{
		return NULL;
	}
	for (uiIndex = pcUndo->GetCurrent() + 1; uiIndex > 0; uiIndex--)
	{
		uiId = pcUndo->GetId(uiIndex - 1);
		for (i = 0; i < masProjects.NumElements(); i++)
		{
			psProject = masProjects.Get(i);
			if (psProject->uiId == uiId)
			{
				return psProject;
			}
		}
	}

	uiId = pcUndo->GetId(0);
	psBefore = NULL;
	for (i = 0; i < masProjects.NumElements(); i++)
	{
		psProject = masProjects.Get(i);
		if ((psProject->uiId < uiId) && ((psBefore == NULL) || (psProject->uiId > psBefore->uiId)))
		{
			psBefore = psProject;
		}
	}
	return psBefore;
}


//////////////////////////////////////////////////////////////////////////
//
// The undo snapshot of the project's last change: the current one, or the
// one before the changes only to the selection that came after it.
// MAX_SIZE if there are none.
//
//////////////////////////////////////////////////////////////////////////
size CLogicimCircuit::GetChangeId(void)
{
	CUndoStack*	pcUndo;
	size		uiIndex;

	pcUndo = mpEditor->GetUndoStack();
	if (pcUndo->NumSnapshots() == 0)
	{
		return MAX_SIZE;
	}
	uiIndex = pcUndo->GetCurrent();
	while ((uiIndex > 0) && pcUndo->IsSelectionOnly(uiIndex))
	{
		uiIndex--;
	}
	return pcUndo->GetId(uiIndex);
}


//////////////////////////////////////////////////////////////////////////
//
// A project starts at the current undo snapshot, with szFilename (empty
// for none) and no changes.  Starts no longer in the undo stack (undone,
// then pushed over) are forgotten, as are those that fell off its bottom
// but the latest of them, whose snapshots may still be there.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimCircuit::StartProject(const char* szFilename)
{
	CUndoStack*				pcUndo;
	SLogicimProjectStart*	psProject;
	size					uiOldest;
	size					uiLatestBefore;
	size					i;

	pcUndo = mpEditor->GetUndoStack();
	uiOldest = pcUndo->GetId(0);
	uiLatestBefore = MAX_SIZE;
	for (i = 0; i < masProjects.NumElements(); i++)
	{
		psProject = masProjects.Get(i);
		if ((psProject->uiId < uiOldest) && ((uiLatestBefore == MAX_SIZE) || (psProject->uiId > uiLatestBefore)))
		{
			uiLatestBefore = psProject->uiId;
		}
	}
	for (i = masProjects.NumElements(); i > 0; i--)
	{
		psProject = masProjects.Get(i - 1);
		if ((psProject->uiId < uiOldest) ? (psProject->uiId != uiLatestBefore) : (pcUndo->FindId(psProject->uiId) == ARRAY_ELEMENT_NOT_FOUND))
		{
			psProject->szFilename.Kill();
			masProjects.RemoveAt(i - 1);
		}
	}

	psProject = masProjects.Add();
	psProject->uiId = pcUndo->GetCurrentId();
	psProject->uiSavedId = GetChangeId();
	psProject->szFilename.Init(szFilename);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimCircuit::KillProjects(void)
{
	size	i;

	for (i = 0; i < masProjects.NumElements(); i++)
	{
		masProjects.Get(i)->szFilename.Kill();
	}
	masProjects.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The file of the project being edited, empty if it hasn't one.  It
// follows undo and redo back and forth over a project's start.
//
//////////////////////////////////////////////////////////////////////////
char* CLogicimCircuit::GetFilename(void)
{
	SLogicimProjectStart*	psProject;

	psProject = GetProject();
	return (psProject != NULL) ? psProject->szFilename.Text() : (char*)"";
}


//////////////////////////////////////////////////////////////////////////
//
// The project has changed since it was saved, loaded, imported or made:
// its last change isn't the one it was saved in.  Undoing back to that
// one leaves it unchanged again.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimCircuit::IsModified(void)
{
	SLogicimProjectStart*	psProject;

	psProject = GetProject();
	return (psProject != NULL) && (GetChangeId() != psProject->uiSavedId);
}
int64 CLogicimCircuit::GetTime(void) { return (mpEditor->GetSimulation() != NULL) ? mpEditor->GetSimulation()->GetTime() : -1; }
