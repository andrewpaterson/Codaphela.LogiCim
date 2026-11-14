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
#include <stdio.h>
#include <stdlib.h>
#include "BaseLib/Logger.h"
#include "BaseLib/FileUtil.h"
#include "StandardLib/Objects.h"
#include "StandardLib/ObjectReader.h"
#include "StandardLib/ObjectWriter.h"
#include "SupportLib/ColourARGB32.h"
#include "SupportLib/ImageColour.h"
#include "SupportLib/ImageColourConverter.h"
#include "SupportLib/ImageReader.h"
#include "SupportLib/TrueTypeFont.h"
#include "SupportLib/TrueTypeFontFactory.h"
#include "WindowLib/JewelColours.h"
#include "WindowLib/JewelTheme.h"
#include "WindowLib/ClearCanvasDraw.h"
#include "WindowLib/FillContainer.h"
#include "WindowLib/RoundedBoxCanvasDraw.h"
#include "LogicimComponentChooser.h"
#include "WindowLib/FileChooser.h"
#include "LogicimLib/ComponentProperties.h"
#include "MicroprocessorLib/LongTime.h"
#include "LogicimWindowContent.h"
#include "LogicimWindowContentInternal.h"


//////////////////////////////////////////////////////////////////////////
//
// A Place.Subcircuit.<type name> command for each of the project's
// circuits, with the keys the project keeps with it (see
// CCircuitView::SetPlaceKeys), as the circuits change: after a load, an
// undo, a new subcircuit.  A key another command already has there is
// dropped, from the project too, without saying.  Kept by the project,
// not the keymap, and given in the component chooser, not listed there.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::FollowSubcircuitCommands(void)
{
	CCircuitViews*		pcCircuits;
	Ptr<CCommand>		pCommand;
	size				i;
	bool				bChanged;

	if (mpLogicimCircuit.IsNull() || (mpLogicimCircuit->GetEditor()->GetCircuits() == NULL))
	{
		return;
	}
	pcCircuits = mpLogicimCircuit->GetEditor()->GetCircuits();
	bChanged = false;

	//The commands of circuits that have gone.
	for (i = mpCommands->NumCommands(); i > 0; i--)
	{
		pCommand = mpCommands->GetCommand(i - 1);
		if ((strncmp(pCommand->GetName(), LOGICIM_PLACE_SUBCIRCUIT, strlen(LOGICIM_PLACE_SUBCIRCUIT)) == 0) && (pcCircuits->Get(&pCommand->GetName()[strlen(LOGICIM_PLACE_SUBCIRCUIT)]) == NULL))
		{
			mpCommands->Remove(pCommand->GetName(), mpWindow->GetComponentInput());
			bChanged = true;
		}
	}

	for (i = 0; i < pcCircuits->NumCircuits(); i++)
	{
		bChanged |= FollowSubcircuitCommand(pcCircuits->Get(i));
	}

	if (bChanged && (mpWindow->GetComponentInput() != NULL))
	{
		mpCommands->BindKeys(mpWindow->GetComponentInput());
	}
}


//////////////////////////////////////////////////////////////////////////
//
// pcCircuit's command, made if it has none, with the project's keys for
// it.  True if either changed.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::FollowSubcircuitCommand(CCircuitView* pcCircuit)
{
	Ptr<CCommand>		pCommand;
	CCommandKeyList		cKeys;
	CCommandKeyList		cKept;
	CChars				szName;
	CChars				szText;
	size				i;
	size				j;
	bool				bChanged;
	bool				bTaken;

	bChanged = false;
	pCommand = FindLogicimPlaceCommand(&mpCommands, pcCircuit->GetTypeName(), true);
	if (pCommand.IsNull())
	{
		szName.Init(LOGICIM_PLACE_SUBCIRCUIT);
		szName.Append(pcCircuit->GetTypeName());
		szText.Init("Add ");
		szText.Append(pcCircuit->GetTypeName());
		pCommand = mpCommands->Add(szName.Text(), szText.Text(), "", Ptr<CImageCel>(), this, &CLogicimWindowContent::PlaceSubcircuitCommand);
		pCommand->SetContext(LOGICIM_CIRCUIT_CONTEXT);
		pCommand->AllowContext(COMPONENT_INPUT_GLOBAL_CONTEXT);
		pCommand->SetKeptByKeymap(false);
		pCommand->SetListedInKeymap(false);
		szText.Kill();
		szName.Kill();
		bChanged = true;
	}

	cKeys.Init();
	ReadPlaceKeys(pcCircuit->GetPlaceKeys(), &cKeys);
	if (!pCommand->GetKeyList()->HasSameKeys(&cKeys))
	{
		cKept.Init();
		for (i = 0; i < cKeys.NumKeys(); i++)
		{
			bTaken = false;
			for (j = 0; !bTaken && (j < mpCommands->NumCommands()); j++)
			{
				bTaken = (mpCommands->GetCommand(j) != pCommand) && mpCommands->GetCommand(j)->GetKeyList()->Contains(cKeys.GetContext(i), cKeys.GetKeys(i));
			}
			if (!bTaken && pCommand->IsContextAllowed(cKeys.GetContext(i)))
			{
				cKept.Add(cKeys.GetContext(i), cKeys.GetKeys(i));
			}
		}
		pCommand->SetKeys(&cKept);
		if (cKept.NumKeys() != cKeys.NumKeys())
		{
			szText.Init();
			WritePlaceKeys(&cKept, &szText);
			pcCircuit->SetPlaceKeys(szText.Text());
			szText.Kill();
		}
		cKept.Kill();
		bChanged = true;
	}
	cKeys.Kill();
	return bChanged;
}


//////////////////////////////////////////////////////////////////////////
//
// The subcircuits' commands' keys, as the component chooser (or a key
// moved in Settings' Keymap) left them, go into the project as a change that can be undone.
// True if any had changed.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::KeepSubcircuitKeys(void)
{
	CCircuitViews*	pcCircuits;
	CCircuitView*	pcCircuit;
	Ptr<CCommand>	pCommand;
	CChars			szText;
	size			i;
	bool			bChanged;

	if (mpLogicimCircuit.IsNull() || (mpLogicimCircuit->GetEditor()->GetCircuits() == NULL))
	{
		return false;
	}
	pcCircuits = mpLogicimCircuit->GetEditor()->GetCircuits();
	bChanged = false;
	for (i = 0; i < pcCircuits->NumCircuits(); i++)
	{
		pcCircuit = pcCircuits->Get(i);
		pCommand = FindLogicimPlaceCommand(&mpCommands, pcCircuit->GetTypeName(), true);
		if (pCommand.IsNotNull())
		{
			szText.Init();
			WritePlaceKeys(pCommand->GetKeyList(), &szText);
			if (!szText.Equals(pcCircuit->GetPlaceKeys()))
			{
				pcCircuit->SetPlaceKeys(szText.Text());
				bChanged = true;
			}
			szText.Kill();
		}
	}
	if (bChanged)
	{
		mpLogicimCircuit->GetEditor()->PushUndo(false);
		UpdateInfo();
	}
	return bChanged;
}


//////////////////////////////////////////////////////////////////////////
//
// Place.Subcircuit.<type name>: an instance of it, as Add Component would
// place it.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::PlaceSubcircuitCommand(CCommand* pcCommand)
{
	PlaceComponent(&pcCommand->GetName()[strlen(LOGICIM_PLACE_SUBCIRCUIT)], true);
}


//////////////////////////////////////////////////////////////////////////
//
// A project's keys for placing a subcircuit (see CCircuitView::
// SetPlaceKeys): "<context>:<keys>" separated by semicolons.  Keys
// without a context are over the circuit.
//
//////////////////////////////////////////////////////////////////////////
void ReadPlaceKeys(const char* szText, CCommandKeyList* pcKeys)
{
	CArrayChars	aszKeys;
	CChars		szContext;
	char*		sz;
	char*		szColon;
	size		i;

	aszKeys.Init();
	aszKeys.Split(StrEmpty(szText) ? "" : szText, ';');
	for (i = 0; i < aszKeys.NumElements(); i++)
	{
		sz = aszKeys.GetText(i);
		if (StrEmpty(sz))
		{
			continue;
		}
		szColon = strchr(sz, ':');
		szContext.Init();
		if (szColon != NULL)
		{
			szContext.Append(sz, szColon - sz);
		}
		else
		{
			szContext.Append(LOGICIM_CIRCUIT_CONTEXT);
		}
		pcKeys->Add(szContext.Text(), (szColon != NULL) ? &szColon[1] : sz);
		szContext.Kill();
	}
	aszKeys.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// As ReadPlaceKeys reads them, each with its context.
//
//////////////////////////////////////////////////////////////////////////
void WritePlaceKeys(CCommandKeyList* pcKeys, CChars* pszText)
{
	size	i;

	for (i = 0; i < pcKeys->NumKeys(); i++)
	{
		if (i > 0)
		{
			pszText->Append(";");
		}
		pszText->Append(pcKeys->GetContext(i));
		pszText->Append(":");
		pszText->Append(pcKeys->GetKeys(i));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit chosen in the Subcircuits tree is edited at its top, fitted to the
// canvas: simulated on its own (see CCircuitEditor::Simulate), unless
// it's the circuit simulated, which keeps its simulation.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::SubcircuitSelected(void)
{
	CCircuitEditor*		pcEditor;
	char*				szName;
	SLogicimPlace		sPlace;
	int32				iNode;

	if (mbListingSubcircuits)
	{
		return;
	}
	pcEditor = mpLogicimCircuit->GetEditor();
	iNode = mpSubcircuitTree->GetSelectedNode();
	szName = ((iNode != TREE_NO_NODE) && (pcEditor->GetCircuits() != NULL)) ? pcEditor->GetCircuits()->Get((size)mpSubcircuitTree->GetNodeData(iNode))->GetTypeName() : NULL;
	if (!StrEmpty(szName) && ((pcEditor->GetCircuit() == NULL) || (StringCompare(pcEditor->GetCircuit()->GetTypeName(), szName) != 0)))
	{
		sPlace.Init(szName, NULL);
		ShowPlace(&sPlace);
		sPlace.Kill();
	}
	UpdateInfo();
}


//////////////////////////////////////////////////////////////////////////
//
// Edits the subcircuit inside the instance as it's simulated there (see
// CCircuitEditor::EnterInstance), zoomed to fit.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::EnterSubcircuit(CComponentView* pcInstance)
{
	if (mpLogicimCircuit->GetEditor()->EnterInstance(pcInstance))
	{
		ZoomToFit(false);
	}
	UpdateInfo();
}


//////////////////////////////////////////////////////////////////////////
//
// Back to the circuit the instance entered is in, zoomed to fit.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::LeaveSubcircuit(void)
{
	if (mpLogicimCircuit->GetEditor()->LeaveInstance())
	{
		ZoomToFit(false);
	}
	UpdateInfo();
}


//////////////////////////////////////////////////////////////////////////
//
// Before the canvas's menu opens at a grid point: on a subcircuit
// instance it's selected and the menu enters it or edits its properties;
// elsewhere inside an instance it leaves it; otherwise the menu has no
// items and doesn't open.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::PrepareCanvasMenu(SFloat64Vec2 sGrid)
{
	CCircuitEditor*		pcEditor;
	CComponentView*		pcComponent;

	pcEditor = mpLogicimCircuit->GetEditor();
	mpCanvasMenu->RemoveAllItems();
	mpEnterItem = NULL;
	mpPropertiesItem = NULL;
	if (pcEditor->IsEditing())
	{
		return;
	}

	pcComponent = pcEditor->GetComponentAt(sGrid);
	if ((pcComponent != NULL) && (pcComponent->GetCircuitInstanceView() != NULL))
	{
		pcEditor->SelectOnly(pcComponent);
		mpEnterItem = mpCanvasMenu->AddItem("Enter Sub-circuit");
		mpPropertiesItem = mpCanvasMenu->AddItem("Properties");
	}
	else if (pcEditor->IsInInstance())
	{
		mpCanvasMenu->AddItem(mpCommands->Find("Circuit.LeaveSubcircuit"));
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::CanvasMenuItemClicked(CMenuItem* pcItem)
{
	CCircuitEditor*		pcEditor;

	pcEditor = mpLogicimCircuit->GetEditor();
	if (pcItem == &mpEnterItem)
	{
		EnterSubcircuit(pcEditor->GetSelection()->GetSingleComponent());
	}
	else if (pcItem == &mpPropertiesItem)
	{
		EditProperties();
	}
}
