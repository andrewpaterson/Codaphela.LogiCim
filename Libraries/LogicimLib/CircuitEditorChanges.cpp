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
// A new component of the element name (see NewPlacedComponentView) under
// the pointer, following it until the button is pressed, as Logicim's
// startPlaceComponent.  Fails if the pointer hasn't been over the circuit.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::StartPlaceComponent(const char* szXMLName)
{
	CComponentView*		pcComponent;

	if (!mbPointer || (mpcCircuit == NULL))
	{
		return false;
	}

	DiscardEdit();
	pcComponent = NewPlacedComponentView(szXMLName, RoundToGrid(msPointer.x), RoundToGrid(msPointer.y), meCreationRotation, mpcCircuit, &mpFonts);
	if (pcComponent == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Cannot place a [", szXMLName, "].", NULL);
	}
	return StartPlace(pcComponent);
}


//////////////////////////////////////////////////////////////////////////
//
// An instance of the subcircuit, as Logicim's startPlaceSubcircuit.  A
// circuit can't hold an instance of itself, or of a subcircuit that holds
// it.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::StartPlaceSubcircuit(const char* szTypeName)
{
	CCircuitView*		pcSubcircuit;
	CComponentView*		pcComponent;

	if (!mbPointer || (mpcCircuit == NULL))
	{
		return false;
	}

	pcSubcircuit = mpcCircuits->Get(szTypeName);
	if (pcSubcircuit == NULL)
	{
		return gcLogger.Error2(__METHOD__, " There is no subcircuit [", szTypeName, "].", NULL);
	}
	if ((pcSubcircuit == mpcCircuit) || CircuitUses(pcSubcircuit, mpcCircuit))
	{
		return gcLogger.Error2(__METHOD__, " Circuit [", mpcCircuit->GetTypeName(), "] cannot hold an instance of [", szTypeName, "], which would hold it.", NULL);
	}

	DiscardEdit();
	pcComponent = NewPlacedSubcircuitInstanceView(pcSubcircuit, RoundToGrid(msPointer.x), RoundToGrid(msPointer.y), meCreationRotation, &mpFonts);
	return StartPlace(pcComponent);
}


//////////////////////////////////////////////////////////////////////////
//
// The selection, or else the component under the pointer, follows the
// pointer without the button held, as a placed component does, until a
// click drops it.  Escape puts it back.  Ctrl keeps it connected, as a
// Ctrl+drag does.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::StartMove(void)
{
	return StartMove(mbControl);
}


//////////////////////////////////////////////////////////////////////////
//
// Starts moving the selection, or else the hovered component, about the
// pointer.  Fails if there's nothing to move or an edit is in progress.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::StartMove(bool bConnected)
{
	CMoveComponentsEdit*	pcMove;

	if (!mbPointer || (mpcCircuit == NULL) || (mpcEdit != NULL))
	{
		return false;
	}

	pcMove = NewMoveEdit(bConnected);
	if (pcMove == NULL)
	{
		return false;
	}
	StartEdit(pcMove, msPointer);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// A move of the selection (left selected when done), or else of the
// hovered component (left unselected), as every move of existing views is
// made: dragged, from the keyboard, or turned and flipped.  NULL if
// there's neither.  The hover is only kept while nothing is selected.
//
//////////////////////////////////////////////////////////////////////////
CMoveComponentsEdit* CCircuitEditor::NewMoveEdit(bool bConnected)
{
	CMoveComponentsEdit*	pcMove;

	if (!mcSelection.IsEmpty())
	{
		pcMove = NewMalloc<CMoveComponentsEdit>();
		pcMove->Init(mcSelection.GetComponents(), mcSelection.GetTraces(), false, bConnected);
		return pcMove;
	}
	else if (mpcHover != NULL)
	{
		pcMove = NewMalloc<CMoveComponentsEdit>();
		pcMove->Init(mpcHover, false, bConnected);
		return pcMove;
	}
	else
	{
		return NULL;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The component is in the circuit from now, but not simulated until it's
// placed.  It's turned as new components are and is mirrored if they are.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::StartPlace(CComponentView* pcComponent)
{
	CMoveComponentsEdit*	pcMove;

	mcSelection.Clear();
	pcComponent->SetMirrored(mbCreationMirrored);
	mpcCircuit->Add(pcComponent);
	pcMove = NewMalloc<CMoveComponentsEdit>();
	pcMove->Init(pcComponent, true, true);
	StartEdit(pcMove, msPointer);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Turns what's being moved or placed, or the component under the pointer,
// or the selection, a quarter clockwise about the pointer, as Logicim's
// rotateRight.  Components placed afterwards are turned too, which is
// done first so the undo snapshot the turn saves has it.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::RotateRight(void)
{
	meCreationRotation = ::RotateRight(meCreationRotation);
	TransformComponents(ET_RotateRight);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::RotateLeft(void)
{
	meCreationRotation = ::RotateLeft(meCreationRotation);
	TransformComponents(ET_RotateLeft);
}


//////////////////////////////////////////////////////////////////////////
//
// Mirrors what's being moved or placed, or the component under the
// pointer, or the selection, left to right on the screen about the
// pointer.  Components placed afterwards are flipped too.  Logicim had no
// flipping.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::FlipHorizontally(void)
{
	TransformOrientation(&meCreationRotation, &mbCreationMirrored, true, 2);
	TransformComponents(ET_FlipHorizontally);
}


//////////////////////////////////////////////////////////////////////////
//
// As FlipHorizontally, top to bottom.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::FlipVertically(void)
{
	TransformOrientation(&meCreationRotation, &mbCreationMirrored, true, 0);
	TransformComponents(ET_FlipVertically);
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's rotateMoveComponents: the edit in progress is turned or
// flipped, or else a connected move (see NewMoveEdit) is started, turned
// or flipped and done.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::TransformComponents(EEditTransform eTransform)
{
	bool	bStarted;

	bStarted = false;
	if (mpcEdit == NULL)
	{
		if (!StartMove(true))
		{
			return;
		}
		bStarted = true;
	}

	switch (eTransform)
	{
	case ET_RotateRight:
		mpcEdit->Rotate(true);
		break;
	case ET_RotateLeft:
		mpcEdit->Rotate(false);
		break;
	case ET_FlipHorizontally:
		mpcEdit->Flip(true);
		break;
	case ET_FlipVertically:
		mpcEdit->Flip(false);
		break;
	}

	if (bStarted)
	{
		DoneEdit(msPointer);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Deletes the selection or, with nothing selected, what's under the
// pointer, as Logicim's editActionDeleteComponentIfPossible: the wire a
// hovered point is on; the wires ending at a hovered connection, or the
// hovered component if none do.  Fails if there's nothing to delete or an
// edit is in progress.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::Delete(void)
{
	CArrayComponentViewPtr	apcComponents;
	CArrayTraceViewPtr		apcTraces;
	size					i;

	if (mpcEdit != NULL)
	{
		return false;
	}

	apcComponents.Init();
	apcTraces.Init();
	if (mcSelection.IsEmpty())
	{
		if (mpcHoverTrace != NULL)
		{
			apcTraces.Add(mpcHoverTrace);
		}
		else if (mbHoverConnection)
		{
			GetTracesEndingAt(msHoverConnection, &apcTraces);
		}
		if ((apcTraces.NumElements() == 0) && (mpcHover != NULL))
		{
			apcComponents.Add(mpcHover);
		}
	}
	else
	{
		for (i = 0; i < mcSelection.NumComponents(); i++)
		{
			apcComponents.Add(mcSelection.GetComponent(i));
		}
		for (i = 0; i < mcSelection.NumTraces(); i++)
		{
			apcTraces.Add(mcSelection.GetTrace(i));
		}
	}

	if ((apcComponents.NumElements() == 0) && (apcTraces.NumElements() == 0))
	{
		apcTraces.Kill();
		apcComponents.Kill();
		return false;
	}

	DeleteViews(&apcComponents, &apcTraces);
	apcTraces.Kill();
	apcComponents.Kill();
	UpdateHover();
	PushUndo(false);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Escape: drops the edit (taking out what was being placed) and the
// selection, as Logicim's stopSimulatorEdit.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::StopEdit(void)
{
	DiscardEdit();
	mcSelection.Clear();
	UpdateHover();
}


//////////////////////////////////////////////////////////////////////////
//
// The component whose properties are edited, as Logicim's getComponent:
// the hovered one, or else the only selected one.  NULL if there's neither
// or an edit is in progress.
//
//////////////////////////////////////////////////////////////////////////
CComponentView* CCircuitEditor::GetPropertiesComponent(void)
{
	if (mpcEdit != NULL)
	{
		return NULL;
	}
	if (mpcHover != NULL)
	{
		return mpcHover;
	}
	if (mcSelection.NumTraces() == 0)
	{
		return mcSelection.GetSingleComponent();
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The component's properties, with their current values, added to the
// (initialised) properties.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::GetProperties(CComponentView* pcComponent, CComponentProperties* pcProperties)
{
	SComponentPropertiesContext	sContext;

	InitPropertiesContext(&sContext);
	pcComponent->GetProperties(pcProperties, &sContext);
}


//////////////////////////////////////////////////////////////////////////
//
// Replaces the component with a new one of the same kind made from the
// properties (see CComponentView::InitFromProperties), turned to the
// rotation and mirrored or not, as Logicim's PropertyEditEvent.  The old
// component is taken out of every simulation and freed; the new one is
// made and started in them, its wires merged and split, and the
// simulations rewired.  The new one is selected if the old one was.
// Returns the new component, or NULL (leaving the old one) if it can't be
// made, or an instance would be of a subcircuit that holds the circuit
// being edited.
//
//////////////////////////////////////////////////////////////////////////
CComponentView* CCircuitEditor::SetProperties(CComponentView* pcComponent, CComponentProperties* pcProperties, ERotation eRotation, bool bMirrored)
{
	CComponentView*				pcNew;
	CArrayComponentViewPtr		apcComponents;
	CArrayTraceViewPtr			apcSeeds;
	CArrayTraceLine				asNoLines;
	bool						bSelected;
	size						i;

	if ((mpcEdit != NULL) || (mpcCircuit == NULL))
	{
		return NULL;
	}
	pcNew = NewFromProperties(pcComponent, pcProperties, eRotation, bMirrored);
	if (pcNew == NULL)
	{
		return NULL;
	}

	ClearHover();
	DestroyInSimulations(pcComponent);
	bSelected = mcSelection.Contains(pcComponent);
	mcSelection.Remove(pcComponent);
	mpcCircuit->Replace(pcComponent, pcNew);
	SafeKill(pcComponent);
	if (bSelected)
	{
		mcSelection.Add(pcNew);
	}

	apcComponents.Init();
	apcComponents.Add(pcNew);
	CreateComponents(&apcComponents);
	apcSeeds.Init();
	AddPortTraces(&apcComponents, &apcSeeds);
	apcComponents.Kill();
	if (apcSeeds.NumElements() > 0)
	{
		for (i = 0; i < apcSeeds.NumElements(); i++)
		{
			mcSelection.Remove(apcSeeds.GetPtr(i));
		}
		asNoLines.Init();
		mpcCircuit->ReplaceWires(&asNoLines, &apcSeeds, NULL);
		asNoLines.Kill();
	}
	apcSeeds.Kill();

	Rewire();
	UpdateHover();
	PushUndo(false);
	return pcNew;
}


//////////////////////////////////////////////////////////////////////////
//
// A new component of the same kind made from the properties, where the
// component is, turned and mirrored.  NULL (with an error for an
// instance) if it can't be made, or would be an instance of a subcircuit
// that holds the circuit being edited.
//
//////////////////////////////////////////////////////////////////////////
CComponentView* CCircuitEditor::NewFromProperties(CComponentView* pcComponent, CComponentProperties* pcProperties, ERotation eRotation, bool bMirrored)
{
	SComponentPropertiesContext	sContext;
	CComponentView*				pcNew;
	CCircuitInstanceView*		pcInstanceView;
	CCircuitView*				pcSubcircuit;

	InitPropertiesContext(&sContext);
	pcNew = NewComponentView(pcComponent->GetXMLName());
	if ((pcNew == NULL) || !pcNew->InitFromProperties(pcComponent->GetPosition().x, pcComponent->GetPosition().y, eRotation, pcProperties, &sContext))
	{
		if (pcNew != NULL)
		{
			free(pcNew);
		}
		return NULL;
	}
	pcNew->InitAfterProperties(pcProperties);
	pcNew->SetMirrored(bMirrored);
	pcInstanceView = pcNew->GetCircuitInstanceView();
	if (pcInstanceView != NULL)
	{
		pcSubcircuit = pcInstanceView->GetInstanceCircuitView();
		if ((pcSubcircuit == mpcCircuit) || CircuitUses(pcSubcircuit, mpcCircuit))
		{
			SafeKill(pcNew);
			gcLogger.Error2(__METHOD__, " Circuit [", mpcCircuit->GetTypeName(), "] cannot hold an instance of [", pcSubcircuit->GetTypeName(), "], which would hold it.", NULL);
			return NULL;
		}
	}
	return pcNew;
}


//////////////////////////////////////////////////////////////////////////
//
// Takes the component out of the simulations of every path the circuit
// being edited is on, and an instance's paths through it.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::DestroyInSimulations(CComponentView* pcComponent)
{
	CArrayViewPathPtr		apcViewPaths;
	CViewPath*				pcViewPath;
	CCircuitInstanceView*	pcInstanceView;
	size					i;
	size					j;

	apcViewPaths.Init();
	GetEditedPaths(&apcViewPaths);
	for (i = 0; i < apcViewPaths.NumElements(); i++)
	{
		pcViewPath = apcViewPaths.GetPtr(i);
		for (j = 0; j < pcViewPath->NumCircuitSimulations(); j++)
		{
			mpcCircuit->DestroyComponent(pcComponent, pcViewPath, pcViewPath->GetCircuitSimulation(j));
		}
	}
	apcViewPaths.Kill();
	pcInstanceView = pcComponent->GetCircuitInstanceView();
	if (pcInstanceView != NULL)
	{
		mpcCircuits->GetViewPaths()->RemovePathsThrough(pcInstanceView);
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::InitPropertiesContext(SComponentPropertiesContext* psContext)
{
	psContext->pcFamilies = mpcFamilies;
	psContext->pcFonts = &mpFonts;
	psContext->pcCircuits = mpcCircuits;
}


//////////////////////////////////////////////////////////////////////////
//
// Takes the views' components out of every simulation, then the views out
// of the circuit (with the paths through deleted instances) and frees
// them.  The wires that touched deleted wires are merged and split again
// (as Logicim's deleteTraceViews) and the simulations rewired.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::DeleteViews(CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcTraces)
{
	CArrayViewPathPtr		apcViewPaths;
	size					i;
	size					j;
	size					k;
	CViewPath*				pcViewPath;
	CCircuitSimulation*		pcSimulation;
	CComponentView*			pcComponent;
	CCircuitInstanceView*	pcInstanceView;
	CTraceView*				pcTrace;
	CTraceView*				pcOther;
	CArrayTraceViewPtr		apcTouching;
	CArrayTraceLine			asNoLines;

	ClearHover();
	apcTouching.Init();
	for (i = 0; i < papcTraces->NumElements(); i++)
	{
		pcTrace = papcTraces->GetPtr(i);
		for (j = 0; j < mpcCircuit->NumTraces(); j++)
		{
			pcOther = mpcCircuit->GetTrace(j);
			if (pcOther->Touches(pcTrace) && (papcTraces->Find(pcOther) == ARRAY_ELEMENT_NOT_FOUND) && (apcTouching.Find(pcOther) == ARRAY_ELEMENT_NOT_FOUND))
			{
				apcTouching.Add(pcOther);
			}
		}
	}

	apcViewPaths.Init();
	GetEditedPaths(&apcViewPaths);
	for (i = 0; i < apcViewPaths.NumElements(); i++)
	{
		pcViewPath = apcViewPaths.GetPtr(i);
		for (j = 0; j < pcViewPath->NumCircuitSimulations(); j++)
		{
			pcSimulation = pcViewPath->GetCircuitSimulation(j);
			for (k = 0; k < papcComponents->NumElements(); k++)
			{
				mpcCircuit->DestroyComponent(papcComponents->GetPtr(k), pcViewPath, pcSimulation);
			}
			for (k = 0; k < papcTraces->NumElements(); k++)
			{
				papcTraces->GetPtr(k)->ClearTraces(pcViewPath, pcSimulation);
			}
		}
	}
	apcViewPaths.Kill();

	for (i = 0; i < papcComponents->NumElements(); i++)
	{
		pcComponent = papcComponents->GetPtr(i);
		mcSelection.Remove(pcComponent);
		pcInstanceView = pcComponent->GetCircuitInstanceView();
		if (pcInstanceView != NULL)
		{
			mpcCircuits->GetViewPaths()->RemovePathsThrough(pcInstanceView);
		}
		mpcCircuit->Remove(pcComponent);
		SafeKill(pcComponent);
	}
	for (i = 0; i < papcTraces->NumElements(); i++)
	{
		pcTrace = papcTraces->GetPtr(i);
		mcSelection.Remove(pcTrace);
		mpcCircuit->RemoveTrace(pcTrace);
		SafeKill(pcTrace);
	}

	if (apcTouching.NumElements() > 0)
	{
		for (i = 0; i < apcTouching.NumElements(); i++)
		{
			mcSelection.Remove(apcTouching.GetPtr(i));
		}
		asNoLines.Init();
		mpcCircuit->ReplaceWires(&asNoLines, &apcTouching, NULL);
		asNoLines.Kill();
	}
	apcTouching.Kill();

	Rewire();
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the circuit holds an instance of the subcircuit, or of a
// subcircuit that does, all the way down.
//
//////////////////////////////////////////////////////////////////////////
bool CircuitUses(CCircuitView* pcCircuit, CCircuitView* pcSubcircuit)
{
	size					i;
	CCircuitInstanceView*	pcInstanceView;
	CCircuitView*			pcInstanceCircuit;

	for (i = 0; i < pcCircuit->NumComponents(); i++)
	{
		pcInstanceView = pcCircuit->GetComponent(i)->GetCircuitInstanceView();
		if (pcInstanceView != NULL)
		{
			pcInstanceCircuit = pcInstanceView->GetInstanceCircuitView();
			if ((pcInstanceCircuit == pcSubcircuit) || ((pcInstanceCircuit != NULL) && CircuitUses(pcInstanceCircuit, pcSubcircuit)))
			{
				return true;
			}
		}
	}
	return false;
}
