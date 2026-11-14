/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela LogicimLib

Codaphela LogicimLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela LogicimLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela LogicimLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include <stdlib.h>
#include "BaseLib/ConstructorCall.h"
#include "BaseLib/PointerFunctions.h"
#include "BaseLib/Logger.h"
#include "BaseLib/XMLFile.h"
#include "BaseLib/XMLWriter.h"
#include "BaseLib/Markup.h"
#include "BaseLib/LogString.h"
#include "BaseLib/ArrayElementNotFound.h"
#include "StandardLib/Objects.h"
#include "MicroprocessorLib/CircuitSimulation.h"
#include "CircuitPainter.h"
#include "CircuitWiring.h"
#include "CircuitFile.h"
#include "SimulationStateXML.h"
#include "ComponentPlacement.h"
#include "ComponentViewFactory.h"
#include "MoveComponentsEdit.h"
#include "SelectionEdit.h"
#include "WirePullEdit.h"
#include "TraceAutoConnector.h"
#include "SubcircuitInstanceView.h"
#include "CircuitEditor.h"
#include "CircuitEditorInternal.h"


//////////////////////////////////////////////////////////////////////////
//
// A project is saved as a circuits file, see Save(const char*).
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::Save(CObjectWriter* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::Load(CObjectReader* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// An edit in progress is discarded first, so what's saved is what's
// simulated.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::Save(const char* szFilename)
{
	CXMLWriter	cWriter;
	bool		bResult;

	DiscardEdit();

	cWriter.Init();
	SaveXML(&cWriter);
	bResult = cWriter.Write(szFilename);
	cWriter.Kill();
	if (!bResult)
	{
		return gcLogger.Error2(__METHOD__, " Could not write [", szFilename, "].", NULL);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The whole project as Save writes it; the writer must be initialised and
// empty.  An edit in progress isn't discarded, so call it when there's
// none or it's done.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::SaveXML(CXMLWriter* pcWriter)
{
	size	i;
	CChars	szPath;

	pcWriter->Begin("Logicim");
	pcWriter->Attribute("version", (int32)CIRCUIT_FILE_VERSION);
	pcWriter->Begin("Editor");
	if (mpcCircuit != NULL)
	{
		pcWriter->Attribute("circuit", GetTopCircuit()->GetTypeName());
	}
	if (mpcViewPath != NULL)
	{
		szPath.Init();
		GetInstancePathText(&szPath);
		pcWriter->Attribute("path", szPath.Text());
		szPath.Kill();
	}
	pcWriter->Attribute("rotation", RotationToString(meCreationRotation));
	if (mbCreationMirrored)
	{
		pcWriter->Attribute("mirrored", true);
	}
	if (mpcCircuit != NULL)
	{
		for (i = 0; i < mcSelection.NumComponents(); i++)
		{
			pcWriter->Begin("Selected");
			pcWriter->Attribute("component", (int64)mpcCircuit->FindComponentIndex(mcSelection.GetComponent(i)));
			pcWriter->End();
		}
		for (i = 0; i < mcSelection.NumTraces(); i++)
		{
			pcWriter->Begin("Selected");
			pcWriter->Attribute("trace", (int64)mpcCircuit->FindTraceIndex(mcSelection.GetTrace(i)));
			pcWriter->End();
		}
	}
	SaveTabsXML(pcWriter);
	pcWriter->End();
	mpcCircuits->SaveXML(pcWriter);
	if (mpSimulation.IsNotNull())
	{
		SaveSimulationXML(pcWriter, GetTopCircuit(), mpcCircuits->GetTopPath(GetTopCircuit()), &mpSimulation);
	}
	pcWriter->End();
}


//////////////////////////////////////////////////////////////////////////
//
// The application's tabs in <Editor>: a <Tab> each, and which is active
// if there's more than one.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::SaveTabsXML(CXMLWriter* pcWriter)
{
	size			i;
	SCircuitPlace*	psTab;

	if (masTabs.NumElements() > 1)
	{
		pcWriter->Attribute("activeTab", (int64)muiActiveTab);
	}
	for (i = 0; i < masTabs.NumElements(); i++)
	{
		psTab = masTabs.Get(i);
		pcWriter->Begin("Tab");
		pcWriter->Attribute("circuit", psTab->szCircuit.Text());
		if (!psTab->szPath.Empty())
		{
			pcWriter->Attribute("path", psTab->szPath.Text());
		}
		pcWriter->Attribute("x", psTab->sPosition.x);
		pcWriter->Attribute("y", psTab->sPosition.y);
		pcWriter->Attribute("zoom", psTab->fZoom);
		pcWriter->End();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// An application's tabs (copies of pasTabs, uiActive the one active),
// saved and loaded with the project but not undone: undoing leaves them
// as they are.  Setting them isn't a change to the project.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::SetTabs(CArrayCircuitPlace* pasTabs, size uiActive)
{
	if (pasTabs != &masTabs)
	{
		CopyCircuitPlaces(&masTabs, pasTabs);
	}
	muiActiveTab = (uiActive < masTabs.NumElements()) ? uiActive : 0;
}


CArrayCircuitPlace* CCircuitEditor::GetTabs(void) { return &masTabs; }
size CCircuitEditor::GetActiveTab(void) { return muiActiveTab; }


//////////////////////////////////////////////////////////////////////////
//
// Replaces the project with the file's, as Logicim's loadFile, which is
// the first change that can be undone back to (see LoadXML).
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::Load(const char* szFilename)
{
	CXMLFile	cFile;
	CMarkupTag*	pcRoot;
	bool		bResult;

	cFile.Init();
	pcRoot = ReadCircuitFileRoot(&cFile, szFilename, "Circuits");
	bResult = (pcRoot != NULL) && LoadXML(pcRoot, szFilename);
	cFile.Kill();
	if (bResult)
	{
		PushUndo(false);
	}
	return bResult;
}


//What a project's <Editor> says: the circuit being edited and the path into it, the creation rotation and mirroring,
//the selection as indices of components and traces in the circuit, and the application's tabs.  The strings point
//into the tag.
struct SCircuitEditorXML
{
	char*					szCircuit;
	char*					szPath;
	ERotation				eRotation;
	bool					bMirrored;
	CArrayTemplate<int64>	aiComponents;
	CArrayTemplate<int64>	aiTraces;
	CArrayCircuitPlace		asTabs;
	int64					iActiveTab;

	void	Init(void);
	void	Kill(void);
};


//////////////////////////////////////////////////////////////////////////
//
// As it is with no <Editor>: the first circuit, facing East.
//
//////////////////////////////////////////////////////////////////////////
void SCircuitEditorXML::Init(void)
{
	szCircuit = NULL;
	szPath = NULL;
	eRotation = ROT_East;
	bMirrored = false;
	aiComponents.Init();
	aiTraces.Init();
	asTabs.Init();
	iActiveTab = 0;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SCircuitEditorXML::Kill(void)
{
	ClearCircuitPlaces(&asTabs);
	asTabs.Kill();
	aiTraces.Kill();
	aiComponents.Kill();
}


static bool LoadCircuitEditorTabXML(CMarkupTag* pcTabTag, const char* szSource, SCircuitPlace* psDest);


//////////////////////////////////////////////////////////////////////////
//
// Reads the <Editor> tag.  False (with an error naming szSource) if a
// value is wrong; the rest are still read.
//
//////////////////////////////////////////////////////////////////////////
static bool LoadCircuitEditorXML(CMarkupTag* pcEditorTag, const char* szSource, SCircuitEditorXML* psEditor)
{
	CMarkupTag*		pcTag;
	STagIterator	sIter;
	char*			szRotation;
	char*			szMirrored;
	int64			iIndex;
	bool			bResult;

	bResult = true;
	psEditor->szCircuit = pcEditorTag->GetStringAttribute("circuit");
	psEditor->szPath = pcEditorTag->GetStringAttribute("path");
	szRotation = pcEditorTag->GetStringAttribute("rotation");
	if (szRotation != NULL)
	{
		psEditor->eRotation = StringToRotation(szRotation);
		if (psEditor->eRotation == ROT_Cannot)
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has an unknown rotation [", szRotation, "].", NULL);
		}
	}
	szMirrored = pcEditorTag->GetStringAttribute("mirrored");
	if ((szMirrored != NULL) && !pcEditorTag->GetBoolAttribute("mirrored", &psEditor->bMirrored))
	{
		bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has a mirrored that isn't true or false [", szMirrored, "].", NULL);
	}

	pcTag = pcEditorTag->GetTag("Selected", &sIter);
	while (pcTag != NULL)
	{
		if (pcTag->GetInt64Attribute("component", &iIndex))
		{
			psEditor->aiComponents.Add(&iIndex);
		}
		else if (pcTag->GetInt64Attribute("trace", &iIndex))
		{
			psEditor->aiTraces.Add(&iIndex);
		}
		else
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has a [Selected] that is neither a component nor a trace.", NULL);
		}
		pcTag = pcEditorTag->GetNextTag(&sIter);
	}

	pcTag = pcEditorTag->GetTag("Tab", &sIter);
	while (pcTag != NULL)
	{
		bResult &= LoadCircuitEditorTabXML(pcTag, szSource, psEditor->asTabs.Add());
		pcTag = pcEditorTag->GetNextTag(&sIter);
	}
	if ((pcEditorTag->GetStringAttribute("activeTab") != NULL) && (!pcEditorTag->GetInt64Attribute("activeTab", &psEditor->iActiveTab) || (psEditor->iActiveTab < 0) || ((size)psEditor->iActiveTab >= psEditor->asTabs.NumElements())))
	{
		bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has an activeTab that isn't one of its tabs.", NULL);
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// A <Tab>, into psDest (not initialised): its circuit, the path into it
// (empty for its top), and the canvas's position and zoom (0 to fit).
// The circuit and path are found when the tab is shown, not here.
//
//////////////////////////////////////////////////////////////////////////
static bool LoadCircuitEditorTabXML(CMarkupTag* pcTabTag, const char* szSource, SCircuitPlace* psDest)
{
	float64	fX;
	float64	fY;

	psDest->Init(pcTabTag->GetStringAttribute("circuit"), pcTabTag->GetStringAttribute("path"));
	fX = 0;
	fY = 0;
	if (psDest->szCircuit.Empty() ||
		((pcTabTag->GetStringAttribute("x") != NULL) && !pcTabTag->GetFloat64Attribute("x", &fX)) ||
		((pcTabTag->GetStringAttribute("y") != NULL) && !pcTabTag->GetFloat64Attribute("y", &fY)) ||
		((pcTabTag->GetStringAttribute("zoom") != NULL) && (!pcTabTag->GetFloat64Attribute("zoom", &psDest->fZoom) || (psDest->fZoom < 0))))
	{
		return gcLogger.Error2(__METHOD__, " [", szSource, "] has a [Tab] without a circuit, or with a position or zoom that isn't a number.", NULL);
	}
	psDest->sPosition.Init(fX, fY);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether every component and trace selected is in the circuit.
//
//////////////////////////////////////////////////////////////////////////
static bool CheckCircuitEditorSelection(SCircuitEditorXML* psEditor, CCircuitView* pcCircuit, const char* szSource)
{
	size	i;
	int64	iIndex;
	bool	bResult;

	bResult = true;
	for (i = 0; i < psEditor->aiComponents.NumElements(); i++)
	{
		iIndex = *psEditor->aiComponents.Get(i);
		if ((iIndex < 0) || ((size)iIndex >= pcCircuit->NumComponents()))
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] selects component [", LongToString(iIndex), "], which isn't in the circuit.", NULL);
		}
	}
	for (i = 0; i < psEditor->aiTraces.NumElements(); i++)
	{
		iIndex = *psEditor->aiTraces.Get(i);
		if ((iIndex < 0) || ((size)iIndex >= pcCircuit->NumTraces()))
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] selects trace [", LongToString(iIndex), "], which isn't in the circuit.", NULL);
		}
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// The view path down the instances from the top circuit, NULL if one of
// them isn't simulated.
//
//////////////////////////////////////////////////////////////////////////
static CViewPath* GetCircuitEditorViewPath(CCircuitViews* pcCircuits, CCircuitView* pcTop, CArrayTemplatePtr<CCircuitInstanceView>* papcPath)
{
	CViewPath*	pcViewPath;
	size		i;

	pcViewPath = pcCircuits->GetTopPath(pcTop);
	for (i = 0; (i < papcPath->NumElements()) && (pcViewPath != NULL); i++)
	{
		pcViewPath = pcViewPath->GetChild(papcPath->GetPtr(i));
	}
	return pcViewPath;
}


//////////////////////////////////////////////////////////////////////////
//
// Replaces the project with the one under pcRoot, as Logicim's load: its
// circuits, the circuit being edited (the first if there's no <Editor>),
// the selection in it and its simulation as saved.  szSource names the
// document in errors.  If it can't be loaded the project is left as it
// was.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::LoadXML(CMarkupTag* pcRoot, const char* szSource)
{
	CMarkupTag*								pcEditorTag;
	SCircuitEditorXML						sEditor;
	CCircuitViews*							pcCircuits;
	CCircuitView*							pcCircuit;
	CCircuitView*							pcTop;
	Ptr<CCircuitSimulation>					pSimulation;
	CArrayTemplatePtr<CCircuitInstanceView>	apcPath;
	CViewPath*								pcViewPath;
	size									i;
	bool									bResult;

	pcCircuits = NewMalloc<CCircuitViews>();
	pcCircuits->Init();
	bResult = pcCircuits->LoadXML(pcRoot->GetTag("Circuits"));

	sEditor.Init();
	apcPath.Init();
	pcEditorTag = pcRoot->GetTag("Editor");
	if (pcEditorTag != NULL)
	{
		bResult &= LoadCircuitEditorXML(pcEditorTag, szSource, &sEditor);
	}

	pcCircuit = NULL;
	if (bResult)
	{
		pcCircuit = (sEditor.szCircuit != NULL) ? pcCircuits->Get(sEditor.szCircuit) : pcCircuits->Get((size)0);
		if (pcCircuit == NULL)
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has no circuit to edit.", NULL);
		}
	}

	//The path into the instance entered: each instance's index in the circuit before it.
	pcTop = pcCircuit;
	if (bResult && !StrEmpty(sEditor.szPath))
	{
		pcCircuit = FindInstancePath(pcTop, sEditor.szPath, &apcPath);
		if (pcCircuit == NULL)
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has a path [", sEditor.szPath, "] that isn't a path of instances.", NULL);
		}
	}
	if (bResult)
	{
		bResult = CheckCircuitEditorSelection(&sEditor, pcCircuit, szSource);
	}

	if (bResult)
	{
		pSimulation = OMalloc<CCircuitSimulation>(pcTop->GetTypeName());
		bResult = LoadCircuitFileSimulation(pcRoot, pcTop, pcCircuits->GetTopPath(pcTop), &pSimulation, mpcFamilies);
	}

	pcViewPath = NULL;
	if (bResult && (apcPath.NumElements() > 0))
	{
		pcViewPath = GetCircuitEditorViewPath(pcCircuits, pcTop, &apcPath);
		if (pcViewPath == NULL)
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has a path [", sEditor.szPath, "] that isn't simulated.", NULL);
		}
	}
	apcPath.Kill();

	if (!bResult)
	{
		if (pSimulation.IsNotNull())
		{
			pcTop->DisconnectSimulation(pcCircuits->GetTopPath(pcTop), &pSimulation);
			pSimulation = NULL;
		}
		pcCircuits->Kill();
		free(pcCircuits);
		sEditor.Kill();
		return false;
	}

	KillProject();
	mpcCircuits = pcCircuits;
	mpcCircuit = pcCircuit;
	mpcViewPath = pcViewPath;
	mpSimulation = pSimulation;
	meCreationRotation = sEditor.eRotation;
	mbCreationMirrored = sEditor.bMirrored;
	CopyCircuitPlaces(&masTabs, &sEditor.asTabs);
	muiActiveTab = (size)sEditor.iActiveTab;
	for (i = 0; i < sEditor.aiComponents.NumElements(); i++)
	{
		mcSelection.Add(mpcCircuit->GetComponent((size)*sEditor.aiComponents.Get(i)));
	}
	for (i = 0; i < sEditor.aiTraces.NumElements(); i++)
	{
		mcSelection.Add(mpcCircuit->GetTrace((size)*sEditor.aiTraces.Get(i)));
	}
	sEditor.Kill();
	UpdateHover();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Saves the project as it is now onto the undo stack, after a change.
// Nothing is saved until a circuit is being edited.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::PushUndo(bool bSelectionOnly)
{
	CXMLWriter	cWriter;

	if (mpcCircuit == NULL)
	{
		return;
	}

	cWriter.Init();
	SaveXML(&cWriter);
	mcUndo.Push(cWriter.GetText(), bSelectionOnly);
	cWriter.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Ctrl+Z goes back to before the last change that wasn't only to the
// selection; bStep (Ctrl+Alt+Z) goes back one change, whatever it was.
// An edit in progress is discarded first.  False if there's nothing to
// undo or the snapshot couldn't be loaded, when nothing changes.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::Undo(bool bStep)
{
	return LoadUndo(mcUndo.FindUndo(bStep));
}


//////////////////////////////////////////////////////////////////////////
//
// Ctrl+Shift+Z goes forward to after the next change that wasn't only to the
// selection, or to the last change if they all were; bStep (Ctrl+Alt+Shift+Z)
// goes forward one change.  As Undo otherwise.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::Redo(bool bStep)
{
	return LoadUndo(mcUndo.FindRedo(bStep));
}


//////////////////////////////////////////////////////////////////////////
//
// Not while an edit is in progress.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::CanUndo(bool bStep)
{
	return (mpcEdit == NULL) && (mcUndo.FindUndo(bStep) != ARRAY_ELEMENT_NOT_FOUND);
}


//////////////////////////////////////////////////////////////////////////
//
// Not while an edit is in progress.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::CanRedo(bool bStep)
{
	return (mpcEdit == NULL) && (mcUndo.FindRedo(bStep) != ARRAY_ELEMENT_NOT_FOUND);
}


//////////////////////////////////////////////////////////////////////////
//
// Loads the snapshot at uiIndex and makes it the undo stack's current
// one.  A part's next unit waiting to be placed is dropped; the tabs are
// kept as they are.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::LoadUndo(size uiIndex)
{
	CMarkup				cMarkup;
	CMarkupTag*			pcRoot;
	bool				bResult;
	CArrayCircuitPlace	asTabs;
	size				uiActiveTab;

	if (uiIndex == ARRAY_ELEMENT_NOT_FOUND)
	{
		return false;
	}

	DiscardEdit();
	SafeKill(mpcNextUnit);

	asTabs.Init();
	CopyCircuitPlaces(&asTabs, &masTabs);
	uiActiveTab = muiActiveTab;
	cMarkup.Init();
	pcRoot = ReadCircuitTextRoot(&cMarkup, mcUndo.Get(uiIndex), "Undo", "Circuits");
	bResult = (pcRoot != NULL) && LoadXML(pcRoot, "Undo");
	cMarkup.Kill();
	SetTabs(&asTabs, uiActiveTab);
	ClearCircuitPlaces(&asTabs);
	asTabs.Kill();
	if (bResult)
	{
		mcUndo.MoveTo(uiIndex);
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// The path into the instance entered, as each instance's index in the
// circuit before it ("12/3").
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::GetInstancePathText(CChars* psz)
{
	size				i;
	CCircuitView*		pcCircuit;
	CComponentView*		pcInstance;

	if (mpcViewPath == NULL)
	{
		return;
	}
	//The path's first view is the top circuit itself; the instances follow it.
	pcCircuit = mpcViewPath->GetTop()->GetCircuitView();
	for (i = 1; i < mpcViewPath->Length(); i++)
	{
		pcInstance = (CComponentView*)(CSubcircuitInstanceView*)mpcViewPath->Get(i);
		if (i > 1)
		{
			psz->Append('/');
		}
		psz->Append((int64)pcCircuit->FindComponentIndex(pcInstance));
		pcCircuit = mpcViewPath->Get(i)->GetInstanceCircuitView();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The simulation's view path at the end of szPath (empty for the top) in
// the circuit simulated, for drawing it as simulated there; NULL if
// there's no such path.
//
//////////////////////////////////////////////////////////////////////////
CViewPath* CCircuitEditor::GetInstanceViewPath(const char* szPath)
{
	CArrayTemplatePtr<CCircuitInstanceView>	apcPath;
	CCircuitView*							pcTop;
	CViewPath*								pcViewPath;

	pcTop = GetTopCircuit();
	if (pcTop == NULL)
	{
		return NULL;
	}
	apcPath.Init();
	pcViewPath = (StrEmpty(szPath) || (FindInstancePath(pcTop, szPath, &apcPath) != NULL)) ? GetCircuitEditorViewPath(mpcCircuits, pcTop, &apcPath) : NULL;
	apcPath.Kill();
	return pcViewPath;
}


//////////////////////////////////////////////////////////////////////////
//
// Edits the circuit at the end of szPath (as GetInstancePathText writes
// it, empty for the top) in the circuit simulated, as entering each
// instance on it would, but as one step to undo.  False, nothing
// changed, if there's no such path.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::EnterInstancePath(const char* szPath)
{
	CArrayTemplatePtr<CCircuitInstanceView>	apcPath;
	CCircuitView*							pcTop;
	CCircuitView*							pcCircuit;
	CViewPath*								pcViewPath;

	pcTop = GetTopCircuit();
	if (pcTop == NULL)
	{
		return false;
	}
	apcPath.Init();
	pcCircuit = StrEmpty(szPath) ? pcTop : FindInstancePath(pcTop, szPath, &apcPath);
	pcViewPath = (pcCircuit != NULL) ? GetCircuitEditorViewPath(mpcCircuits, pcTop, &apcPath) : NULL;
	apcPath.Kill();
	if (pcViewPath == NULL)
	{
		return false;
	}

	DiscardEdit();
	mcSelection.Clear();
	mpcViewPath = pcViewPath->IsTop() ? NULL : pcViewPath;
	mpcCircuit = pcCircuit;
	UpdateHover();
	PushUndo(true);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit at the end of the path of instance indices from pcTop, and
// the instances, NULL if the path doesn't follow instances.
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CCircuitEditor::FindInstancePath(CCircuitView* pcTop, const char* szPath, CArrayTemplatePtr<CCircuitInstanceView>* papcPath)
{
	CCircuitView*			pcCircuit;
	const char*				sz;
	char*					szEnd;
	long					lIndex;
	CComponentView*			pcComponent;
	CCircuitInstanceView*	pcInstanceView;

	pcCircuit = pcTop;
	sz = szPath;
	while (*sz != '\0')
	{
		lIndex = strtol(sz, &szEnd, 10);
		if ((szEnd == sz) || (lIndex < 0) || ((size)lIndex >= pcCircuit->NumComponents()))
		{
			return NULL;
		}
		pcComponent = pcCircuit->GetComponent((size)lIndex);
		pcInstanceView = pcComponent->GetCircuitInstanceView();
		if ((pcInstanceView == NULL) || (pcInstanceView->GetInstanceCircuitView() == NULL))
		{
			return NULL;
		}
		papcPath->Add(pcInstanceView);
		pcCircuit = pcInstanceView->GetInstanceCircuitView();
		sz = (*szEnd == '/') ? szEnd + 1 : szEnd;
		if ((*szEnd != '/') && (*szEnd != '\0'))
		{
			return NULL;
		}
	}
	return pcCircuit;
}
