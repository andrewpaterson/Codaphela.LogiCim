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
// No circuits and nothing simulated.  New components face East, as
// Logicim's (whose West is East here).
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::Init(CFamilyVoltageConfigurationStore* pcFamilies, Ptr<CCircuitFonts> pFonts)
{
	PreInit();

	mpcCircuits = NewMalloc<CCircuitViews>();
	mpcCircuits->Init();
	mpcCircuit = NULL;
	mpcViewPath = NULL;
	mpcFamilies = pcFamilies;
	mpFonts = pFonts;
	mcSelection.Init();
	mcUndo.Init();
	mpcEdit = NULL;
	mpcNextUnit = NULL;
	meCreationRotation = ROT_East;
	mbCreationMirrored = false;
	ClearHover();
	masLeads.Init();
	mapcFollowing.Init();
	masAnchors.Init();
	mbDetachedMove = false;
	mbLeadsLifted = false;
	mapcLiftedLeads.Init();
	maiLiftedLeadIndices.Init();
	mapcLeadRoutes.Init();
	msPointer.Init(0, 0);
	mbPointer = false;
	mbControl = false;
	mbShift = false;

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::Free(void)
{
	SafeKill(mpcNextUnit);
	KillProject(true);
	mcUndo.Kill();
	mapcLeadRoutes.Kill();
	maiLiftedLeadIndices.Kill();
	mapcLiftedLeads.Kill();
	masAnchors.Kill();
	mapcFollowing.Kill();
	masLeads.Kill();
	mcSelection.Kill();
	mpcFamilies = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::Class(void)
{
	U_Pointer(mpcCircuits);
	U_Pointer(mpcCircuit);
	U_Pointer(mpcViewPath);
	M_Pointer(mpSimulation);
	U_Pointer(mpcFamilies);
	M_Pointer(mpFonts);
	U_Unknown(CCircuitSelection, mcSelection);
	U_Unknown(CUndoStack, mcUndo);
	U_Pointer(mpcEdit);
	U_Pointer(mpcNextUnit);
	U_Enum(meCreationRotation);
	U_Bool(mbCreationMirrored);
	U_Pointer(mpcHover);
	U_Bool(mbHoverConnection);
	U_2Int32(msHoverConnection);
	U_Pointer(mpcHoverTrace);
	U_Unknown(CArrayMoveLead, masLeads);
	U_Unknown(CArrayTraceViewPtr, mapcFollowing);
	U_Unknown(CArrayMoveAnchor, masAnchors);
	U_Bool(mbDetachedMove);
	U_Bool(mbLeadsLifted);
	U_Unknown(CArrayTraceViewPtr, mapcLiftedLeads);
	U_Unknown(CArrayInt, maiLiftedLeadIndices);
	U_Unknown(CArrayTraceViewPtr, mapcLeadRoutes);
	U_Data(SFloat64Vec2, msPointer);
	U_Bool(mbPointer);
	U_Bool(mbControl);
	U_Bool(mbShift);
}


//////////////////////////////////////////////////////////////////////////
//
// An edit in progress is dropped as it is, not discarded.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::KillProject(bool bFreeing)
{
	KillEdit();
	KillLeads();
	masAnchors.ReInit();
	mcSelection.Clear();
	ClearHover();
	KillSimulation(bFreeing);
	if (mpcCircuits != NULL)
	{
		mpcCircuits->Kill();
		free(mpcCircuits);
		mpcCircuits = NULL;
	}
	mpcCircuit = NULL;
	mpcViewPath = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The views forget the simulation, then letting go of it frees it.  When
// the editor is being freed the simulation is let go of with the editor's
// other pointers.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::KillSimulation(bool bFreeing)
{
	if (mpSimulation.IsNotNull())
	{
		if (GetTopCircuit() != NULL)
		{
			GetTopCircuit()->DisconnectSimulation(mpcCircuits->GetTopPath(GetTopCircuit()), &mpSimulation);
		}
		if (!bFreeing)
		{
			mpSimulation = NULL;
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A new, empty circuit (see CCircuitViews::Add).
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CCircuitEditor::AddCircuit(const char* szTypeName)
{
	return mpcCircuits->Add(szTypeName);
}


//////////////////////////////////////////////////////////////////////////
//
// Edits the circuit of the type and simulates it, in a new simulation
// started afresh; the last one is killed.  Starting to edit a circuit can
// be undone; simulating the same one again, as a reset does, can't.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::Simulate(const char* szTypeName)
{
	CCircuitView*	pcCircuit;
	CCircuitView*	pcPrevious;

	pcCircuit = mpcCircuits->Get(szTypeName);
	if (pcCircuit == NULL)
	{
		return gcLogger.Error2(__METHOD__, " There is no circuit [", szTypeName, "] to simulate.", NULL);
	}

	DiscardEdit();
	mcSelection.Clear();
	KillSimulation();
	pcPrevious = GetTopCircuit();
	mpcViewPath = NULL;
	mpcCircuit = pcCircuit;
	mpSimulation = OMalloc<CCircuitSimulation>(szTypeName);
	UpdateHover();
	if (!mpcCircuit->CreateSimulation(GetViewPath(), &mpSimulation, mpcFamilies, true))
	{
		return false;
	}
	if ((mcUndo.NumSnapshots() == 0) || (pcCircuit != pcPrevious))
	{
		PushUndo(false);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's mousePressed for button one: a press during an edit (a
// placement) ends it, otherwise it starts one.  A press on a component
// that isn't selected, without shift (which adds to the selection), moves
// just that component if it's dragged; on its port it's a wire pull, as a
// hovered connection is.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::ButtonPressed(SFloat64Vec2 sPosition)
{
	CStartEditInComponent*	pcStartEdit;
	CStartEditInPort*		pcStartPort;
	CSelectionEdit*			pcSelectionEdit;
	CComponentView*			pcPressed;
	SInt32Vec2				sPoint;

	msPointer = sPosition;
	mbPointer = true;
	if (mpcCircuit == NULL)
	{
		return;
	}

	UpdateHover();
	if (mpcEdit != NULL)
	{
		DoneEdit(sPosition);
	}
	else if (mcSelection.IsOn(sPosition))
	{
		pcStartEdit = NewMalloc<CStartEditInComponent>();
		pcStartEdit->Init(NULL);
		StartEdit(pcStartEdit, sPosition);
	}
	else if (mbHoverConnection)
	{
		pcStartPort = NewMalloc<CStartEditInPort>();
		pcStartPort->Init();
		StartEdit(pcStartPort, sPosition);
	}
	else
	{
		sPoint.Init(RoundToGrid(sPosition.x), RoundToGrid(sPosition.y));
		pcPressed = mbShift ? NULL : mpcCircuit->GetComponentAt(sPosition);
		if ((pcPressed != NULL) && (mpcCircuit->GetPortViewAt(sPoint) == NULL))
		{
			pcStartEdit = NewMalloc<CStartEditInComponent>();
			pcStartEdit->Init(pcPressed);
			StartEdit(pcStartEdit, sPosition);
		}
		else
		{
			pcSelectionEdit = NewMalloc<CSelectionEdit>();
			pcSelectionEdit->Init();
			StartEdit(pcSelectionEdit, sPosition);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::ButtonReleased(SFloat64Vec2 sPosition)
{
	CComponentView*		pcNextUnit;

	msPointer = sPosition;
	mbPointer = true;
	DoneEdit(sPosition);

	//A unit placed by the press is followed by placing the part's next unit under the pointer, once the button is up
	//(the release would otherwise place it straight away).
	if (mpcNextUnit != NULL)
	{
		pcNextUnit = mpcNextUnit;
		mpcNextUnit = NULL;
		if (mpcEdit == NULL)
		{
			pcNextUnit->SetPosition(RoundToGrid(msPointer.x), RoundToGrid(msPointer.y));
			StartPlace(pcNextUnit);
		}
		else
		{
			SafeKill(pcNextUnit);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::PointerMoved(SFloat64Vec2 sPosition)
{
	msPointer = sPosition;
	mbPointer = true;
	if (mpcEdit != NULL)
	{
		mpcEdit->Move(sPosition);
	}
	UpdateHover();
}


//////////////////////////////////////////////////////////////////////////
//
// A change to the keys changes the selection being dragged, as Logicim's
// mouseModifierKeys.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::SetModifiers(bool bControl, bool bShift)
{
	if ((bControl != mbControl) || (bShift != mbShift))
	{
		mbControl = bControl;
		mbShift = bShift;
		if (mbPointer)
		{
			PointerMoved(msPointer);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit, the component under the pointer, the edit and the
// selection, as Logicim's paint.  The painter must be for the editor's
// path and simulation.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::Paint(CCircuitPainter* pcPainter)
{
	if (mpcCircuit == NULL)
	{
		return;
	}

	mpcCircuit->Paint(pcPainter);
	if (mbHoverConnection)
	{
		pcPainter->ClearTransform();
		pcPainter->Circle(msHoverConnection.x, msHoverConnection.y, pcPainter->GetViewport()->GetCircleRadius() * 5 / pcPainter->GetViewport()->GetPixelsPerGrid(), false, true, 1, pcPainter->GetColours()->msPortHover, pcPainter->GetColours()->msPortHover);
	}
	else if (mpcHover != NULL)
	{
		PaintSelectionMarkers(pcPainter, mpcHover, pcPainter->GetColours()->msViewHover);
	}
	if (mpcEdit != NULL)
	{
		mpcEdit->Paint(pcPainter);
		PaintAnchors(pcPainter);
	}
	mcSelection.Paint(pcPainter);
	pcPainter->ClearTransform();
}


//////////////////////////////////////////////////////////////////////////
//
// The stretches of wire that will join the moved wires back to where they
// were, drawn as disconnected wires, as a pulled wire is.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::PaintAnchors(CCircuitPainter* pcPainter)
{
	CArrayTraceLine		asLines;
	size				i;
	STraceLine*			psLine;
	ARGB32				sColour;

	if (masAnchors.NumElements() == 0)
	{
		return;
	}
	asLines.Init();
	for (i = 0; i < masAnchors.NumElements(); i++)
	{
		GetAnchorLines(masAnchors.Get(i), &asLines);
	}
	pcPainter->ClearTransform();
	sColour = pcPainter->GetColours()->msDisconnectedTrace;
	for (i = 0; i < asLines.NumElements(); i++)
	{
		psLine = asLines.Get(i);
		pcPainter->Line(psLine->sStart.x, psLine->sStart.y, psLine->sEnd.x, psLine->sEnd.y, SHAPE_LINE_WIDTH, sColour);
	}
	asLines.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The simulation doesn't run during an edit, as Logicim's canRun.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::CanRun(void)
{
	return mpcEdit == NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::IsEditing(void) { return mpcEdit != NULL; }
CCircuitViews* CCircuitEditor::GetCircuits(void) { return mpcCircuits; }
CCircuitView* CCircuitEditor::GetCircuit(void) { return mpcCircuit; }
CCircuitSimulation* CCircuitEditor::GetSimulation(void) { return &mpSimulation; }
CCircuitFonts* CCircuitEditor::GetFonts(void) { return &mpFonts; }
CUndoStack* CCircuitEditor::GetUndoStack(void) { return &mcUndo; }
CCircuitSelection* CCircuitEditor::GetSelection(void) { return &mcSelection; }
CComponentView* CCircuitEditor::GetHover(void) { return mpcHover; }
CTraceView* CCircuitEditor::GetHoverTrace(void) { return mpcHoverTrace; }


//////////////////////////////////////////////////////////////////////////
//
// False if no connection is hovered.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::GetHoverConnection(SInt32Vec2* psConnection)
{
	*psConnection = msHoverConnection;
	return mbHoverConnection;
}
ERotation CCircuitEditor::GetCreationRotation(void) { return meCreationRotation; }
bool CCircuitEditor::IsCreationMirrored(void) { return mbCreationMirrored; }
bool CCircuitEditor::IsControl(void) { return mbControl; }
bool CCircuitEditor::IsShift(void) { return mbShift; }


//////////////////////////////////////////////////////////////////////////
//
// The component of the edited circuit at the point (grid units), NULL if
// none.
//
//////////////////////////////////////////////////////////////////////////
CComponentView* CCircuitEditor::GetComponentAt(SFloat64Vec2 sPosition)
{
	if (mpcCircuit == NULL)
	{
		return NULL;
	}
	return mpcCircuit->GetComponentAt(sPosition);
}


//////////////////////////////////////////////////////////////////////////
//
// Only the component selected, as a right click on it does.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::SelectOnly(CComponentView* pcComponent)
{
	if ((mcSelection.NumComponents() == 1) && (mcSelection.NumTraces() == 0) && mcSelection.Contains(pcComponent))
	{
		return;
	}
	DiscardEdit();
	mcSelection.Clear();
	if (pcComponent != NULL)
	{
		mcSelection.Add(pcComponent);
	}
	UpdateHover();
	PushUndo(true);
}


//////////////////////////////////////////////////////////////////////////
//
// Selects the views a rectangle picks, from the selection before it by the
// selection mode, as Logicim's calculateSelection.  Returns whether the
// selection changed.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::SelectInRectangle(SFloat64Vec2 sStart, SFloat64Vec2 sEnd, CCircuitSelection* pcPrevious)
{
	CArrayComponentViewPtr	apcComponents;
	CArrayTraceViewPtr		apcTraces;

	apcComponents.Init();
	apcTraces.Init();
	mpcCircuit->GetViewsInRectangle(sStart, sEnd, &apcComponents, &apcTraces);
	mcSelection.Set(&apcComponents, &apcTraces, pcPrevious, CalculateSelectionMode(mbControl, mbShift));
	apcTraces.Kill();
	apcComponents.Kill();
	return !mcSelection.Equals(pcPrevious);
}


//////////////////////////////////////////////////////////////////////////
//
// The edit owns the stateful edit.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::StartEdit(CStatefulEdit* pcEdit, SFloat64Vec2 sStart)
{
	mpcEdit = (CCircuitEdit*)malloc(sizeof(CCircuitEdit));
	mpcEdit->Init(sStart, pcEdit, this);
	UpdateHover();
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's editDone.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::DoneEdit(SFloat64Vec2 sPosition)
{
	if (mpcEdit != NULL)
	{
		mpcEdit->Done(sPosition);
		KillEdit();
	}
	UpdateHover();
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's discardEdit.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::DiscardEdit(void)
{
	if (mpcEdit != NULL)
	{
		mpcEdit->Discard();
		KillEdit();
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::KillEdit(void)
{
	if (mpcEdit != NULL)
	{
		mpcEdit->Kill();
		free(mpcEdit);
		mpcEdit = NULL;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The component under the pointer and the connection or point on a wire,
// as Logicim's calculateHighlightedPort; none during an edit or while
// anything is selected.  A port on the grid point nearest the pointer is
// hovered, with its component, from any side; ports are on the edge of
// their component's box, or outside it, so the box alone would only find
// them from inside.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::UpdateHover(void)
{
	SInt32Vec2	sPoint;
	CPortView*	pcPort;

	ClearHover();
	if ((mpcEdit == NULL) && mcSelection.IsEmpty() && mbPointer && (mpcCircuit != NULL))
	{
		sPoint.Init(RoundToGrid(msPointer.x), RoundToGrid(msPointer.y));
		pcPort = mpcCircuit->GetPortViewAt(sPoint);
		if (pcPort != NULL)
		{
			mpcHover = pcPort->GetOwner();
			mbHoverConnection = true;
			msHoverConnection = sPoint;
			return;
		}

		mpcHover = mpcCircuit->GetComponentAt(msPointer);
		if ((mpcHover != NULL) && mpcCircuit->IsConnectionAt(sPoint))
		{
			mbHoverConnection = true;
			msHoverConnection = sPoint;
		}
		else
		{
			mpcHoverTrace = mpcCircuit->GetTraceAt(sPoint);
			if (mpcHoverTrace != NULL)
			{
				mbHoverConnection = true;
				msHoverConnection = sPoint;
				if (mpcCircuit->IsConnectionAt(sPoint))
				{
					mpcHoverTrace = NULL;
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::ClearHover(void)
{
	mpcHover = NULL;
	mbHoverConnection = false;
	msHoverConnection.Init(0, 0);
	mpcHoverTrace = NULL;
}
