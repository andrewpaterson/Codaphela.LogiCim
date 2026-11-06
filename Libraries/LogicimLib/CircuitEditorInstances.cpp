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
// The top path of the circuit being edited; NULL if there's none.
//
//////////////////////////////////////////////////////////////////////////
CViewPath* CCircuitEditor::GetViewPath(void)
{
	if (mpcCircuit == NULL)
	{
		return NULL;
	}
	if (mpcViewPath != NULL)
	{
		return mpcViewPath;
	}
	return mpcCircuits->GetTopPath(mpcCircuit);
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit simulated: the edited one at the top, otherwise the one the
// path into the instance entered starts at.
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CCircuitEditor::GetTopCircuit(void)
{
	if (mpcViewPath != NULL)
	{
		return mpcViewPath->GetTop()->GetCircuitView();
	}
	return mpcCircuit;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::IsInInstance(void)
{
	return mpcViewPath != NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Edits the circuit of a subcircuit instance in the edited circuit, as
// it's simulated inside that instance (KiCad's entering a sheet): its
// views are drawn with the instance's voltages, and edits to it are made
// to every instance of it.  False if the view isn't an instance.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::EnterInstance(CComponentView* pcInstance)
{
	CCircuitInstanceView*	pcInstanceView;
	CViewPath*				pcChild;

	pcInstanceView = (pcInstance != NULL) ? pcInstance->GetCircuitInstanceView() : NULL;
	if ((pcInstanceView == NULL) || (pcInstanceView->GetInstanceCircuitView() == NULL) || (mpcCircuit == NULL))
	{
		return false;
	}
	pcChild = GetViewPath()->GetChild(pcInstanceView);
	if (pcChild == NULL)
	{
		return false;
	}

	DiscardEdit();
	mcSelection.Clear();
	mpcViewPath = pcChild;
	mpcCircuit = pcInstanceView->GetInstanceCircuitView();
	UpdateHover();
	PushUndo(true);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Back to the circuit the instance entered is in, the instance selected.
// False at the top.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::LeaveInstance(void)
{
	CViewPath*			pcPrevious;
	CComponentView*		pcInstance;

	if (mpcViewPath == NULL)
	{
		return false;
	}
	DiscardEdit();
	mcSelection.Clear();
	pcInstance = (CComponentView*)(CSubcircuitInstanceView*)mpcViewPath->GetLast();
	pcPrevious = mpcViewPath->GetPrevious();
	mpcViewPath = ((pcPrevious == NULL) || pcPrevious->IsTop()) ? NULL : pcPrevious;
	mpcCircuit = (pcPrevious != NULL) ? pcPrevious->GetCircuitView() : GetTopCircuit();
	if ((pcInstance != NULL) && (mpcCircuit->FindComponentIndex(pcInstance) != ARRAY_ELEMENT_NOT_FOUND))
	{
		mcSelection.Add(pcInstance);
	}
	UpdateHover();
	PushUndo(true);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Back to the circuit simulated.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::LeaveInstances(void)
{
	while (LeaveInstance())
	{
	}
}
