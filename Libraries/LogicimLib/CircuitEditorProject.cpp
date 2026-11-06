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
// The component's index in the circuit, ARRAY_ELEMENT_NOT_FOUND if it
// isn't in it.
//
//////////////////////////////////////////////////////////////////////////
size FindCircuitComponentIndex(CCircuitView* pcCircuit, CComponentView* pcComponent)
{
	size	i;

	for (i = 0; i < pcCircuit->NumComponents(); i++)
	{
		if (pcCircuit->GetComponent(i) == pcComponent)
		{
			return i;
		}
	}
	return ARRAY_ELEMENT_NOT_FOUND;
}


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
			pcWriter->Attribute("component", (int64)FindComponentIndex(mcSelection.GetComponent(i)));
			pcWriter->End();
		}
		for (i = 0; i < mcSelection.NumTraces(); i++)
		{
			pcWriter->Begin("Selected");
			pcWriter->Attribute("trace", (int64)FindTraceIndex(mcSelection.GetTrace(i)));
			pcWriter->End();
		}
	}
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
	CMarkupTag*					pcEditorTag;
	CMarkupTag*					pcTag;
	STagIterator				sIter;
	CCircuitViews*				pcCircuits;
	CCircuitView*				pcCircuit;
	Ptr<CCircuitSimulation>		pSimulation;
	ERotation					eRotation;
	bool						bMirrored;
	char*						szCircuit;
	char*						szRotation;
	char*						szMirrored;
	char*						szPath;
	CCircuitView*				pcTop;
	CArrayTemplatePtr<CCircuitInstanceView>	apcPath;
	CViewPath*					pcViewPath;
	CArrayTemplate<int64>		aiComponents;
	CArrayTemplate<int64>		aiTraces;
	int64						iIndex;
	size						i;
	bool						bResult;

	pcCircuits = NewMalloc<CCircuitViews>();
	pcCircuits->Init();
	bResult = pcCircuits->LoadXML(pcRoot->GetTag("Circuits"));

	eRotation = ROT_East;
	bMirrored = false;
	szCircuit = NULL;
	szPath = NULL;
	apcPath.Init();
	aiComponents.Init();
	aiTraces.Init();
	pcEditorTag = pcRoot->GetTag("Editor");
	if (pcEditorTag != NULL)
	{
		szCircuit = pcEditorTag->GetStringAttribute("circuit");
		szPath = pcEditorTag->GetStringAttribute("path");
		szRotation = pcEditorTag->GetStringAttribute("rotation");
		if (szRotation != NULL)
		{
			eRotation = StringToRotation(szRotation);
			if (eRotation == ROT_Cannot)
			{
				bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has an unknown rotation [", szRotation, "].", NULL);
			}
		}
		szMirrored = pcEditorTag->GetStringAttribute("mirrored");
		if ((szMirrored != NULL) && !pcEditorTag->GetBoolAttribute("mirrored", &bMirrored))
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has a mirrored that isn't true or false [", szMirrored, "].", NULL);
		}

		pcTag = pcEditorTag->GetTag("Selected", &sIter);
		while (pcTag != NULL)
		{
			if (pcTag->GetInt64Attribute("component", &iIndex))
			{
				aiComponents.Add(&iIndex);
			}
			else if (pcTag->GetInt64Attribute("trace", &iIndex))
			{
				aiTraces.Add(&iIndex);
			}
			else
			{
				bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has a [Selected] that is neither a component nor a trace.", NULL);
			}
			pcTag = pcEditorTag->GetNextTag(&sIter);
		}
	}

	pcCircuit = NULL;
	if (bResult)
	{
		pcCircuit = (szCircuit != NULL) ? pcCircuits->Get(szCircuit) : pcCircuits->Get((size)0);
		if (pcCircuit == NULL)
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has no circuit to edit.", NULL);
		}
	}

	//The path into the instance entered: each instance's index in the circuit before it.
	pcTop = pcCircuit;
	if (bResult && !StrEmpty(szPath))
	{
		pcCircuit = FindInstancePath(pcTop, szPath, &apcPath);
		if (pcCircuit == NULL)
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has a path [", szPath, "] that isn't a path of instances.", NULL);
		}
	}

	if (bResult)
	{
		for (i = 0; i < aiComponents.NumElements(); i++)
		{
			iIndex = *aiComponents.Get(i);
			if ((iIndex < 0) || ((size)iIndex >= pcCircuit->NumComponents()))
			{
				bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] selects component [", LongToString(iIndex), "], which isn't in the circuit.", NULL);
			}
		}
		for (i = 0; i < aiTraces.NumElements(); i++)
		{
			iIndex = *aiTraces.Get(i);
			if ((iIndex < 0) || ((size)iIndex >= pcCircuit->NumTraces()))
			{
				bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] selects trace [", LongToString(iIndex), "], which isn't in the circuit.", NULL);
			}
		}
	}

	if (bResult)
	{
		pSimulation = OMalloc<CCircuitSimulation>(pcTop->GetTypeName());
		bResult = LoadCircuitFileSimulation(pcRoot, pcTop, pcCircuits->GetTopPath(pcTop), &pSimulation, mpcFamilies);
	}

	pcViewPath = NULL;
	if (bResult && (apcPath.NumElements() > 0))
	{
		pcViewPath = pcCircuits->GetTopPath(pcTop);
		for (i = 0; (i < apcPath.NumElements()) && (pcViewPath != NULL); i++)
		{
			pcViewPath = pcViewPath->GetChild(apcPath.GetPtr(i));
		}
		if (pcViewPath == NULL)
		{
			bResult = gcLogger.Error2(__METHOD__, " [", szSource, "] has a path [", szPath, "] that isn't simulated.", NULL);
		}
	}

	if (!bResult)
	{
		if (pSimulation.IsNotNull())
		{
			pcTop->DisconnectSimulation(pcCircuits->GetTopPath(pcTop), &pSimulation);
			pSimulation = NULL;
		}
		pcCircuits->Kill();
		free(pcCircuits);
		apcPath.Kill();
		aiTraces.Kill();
		aiComponents.Kill();
		return false;
	}

	KillProject();
	mpcCircuits = pcCircuits;
	mpcCircuit = pcCircuit;
	mpcViewPath = pcViewPath;
	mpSimulation = pSimulation;
	apcPath.Kill();
	meCreationRotation = eRotation;
	mbCreationMirrored = bMirrored;
	for (i = 0; i < aiComponents.NumElements(); i++)
	{
		mcSelection.Add(mpcCircuit->GetComponent((size)*aiComponents.Get(i)));
	}
	for (i = 0; i < aiTraces.NumElements(); i++)
	{
		mcSelection.Add(mpcCircuit->GetTrace((size)*aiTraces.Get(i)));
	}
	aiTraces.Kill();
	aiComponents.Kill();
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
// selection; bStep (Ctrl+Shift+Z) goes back one change, whatever it was.
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
// Ctrl+Y goes forward to after the next change that wasn't only to the
// selection, or to the last change if they all were; bStep (Ctrl+Shift+Y)
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
// one.  A part's next unit waiting to be placed is dropped.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::LoadUndo(size uiIndex)
{
	CMarkup		cMarkup;
	CMarkupTag*	pcRoot;
	bool		bResult;

	if (uiIndex == ARRAY_ELEMENT_NOT_FOUND)
	{
		return false;
	}

	DiscardEdit();
	SafeKill(mpcNextUnit);

	cMarkup.Init();
	pcRoot = ReadCircuitTextRoot(&cMarkup, mcUndo.Get(uiIndex), "Undo", "Circuits");
	bResult = (pcRoot != NULL) && LoadXML(pcRoot, "Undo");
	cMarkup.Kill();
	if (bResult)
	{
		mcUndo.MoveTo(uiIndex);
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// The component's index in the circuit being edited.
//
//////////////////////////////////////////////////////////////////////////
size CCircuitEditor::FindComponentIndex(CComponentView* pcComponent)
{
	size	i;

	for (i = 0; i < mpcCircuit->NumComponents(); i++)
	{
		if (mpcCircuit->GetComponent(i) == pcComponent)
		{
			return i;
		}
	}
	return ARRAY_ELEMENT_NOT_FOUND;
}


//////////////////////////////////////////////////////////////////////////
//
// The wire's index in the circuit being edited.
//
//////////////////////////////////////////////////////////////////////////
size CCircuitEditor::FindTraceIndex(CTraceView* pcTrace)
{
	size	i;

	for (i = 0; i < mpcCircuit->NumTraces(); i++)
	{
		if (mpcCircuit->GetTrace(i) == pcTrace)
		{
			return i;
		}
	}
	return ARRAY_ELEMENT_NOT_FOUND;
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
		psz->Append((int64)FindCircuitComponentIndex(pcCircuit, pcInstance));
		pcCircuit = mpcViewPath->Get(i)->GetInstanceCircuitView();
	}
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
