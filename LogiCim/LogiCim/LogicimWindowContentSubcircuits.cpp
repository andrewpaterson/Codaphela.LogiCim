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
// The project's circuits, in order, the one being edited selected.  The
// rows are only made again when the circuits have changed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ListSubcircuits(void)
{
	CCircuitViews*	pcCircuits;
	CCircuitView*	pcEdited;
	size			i;
	bool			bSame;
	int32			iSelected;

	if (mpSubcircuits.IsNull() || (mpLogicimCircuit.IsNull()))
	{
		return;
	}
	pcCircuits = mpLogicimCircuit->GetEditor()->GetCircuits();
	pcEdited = mpLogicimCircuit->GetEditor()->GetCircuit();
	if (pcCircuits == NULL)
	{
		return;
	}

	bSame = (maSubcircuitLabels.Size() == pcCircuits->NumCircuits());
	for (i = 0; bSame && (i < pcCircuits->NumCircuits()); i++)
	{
		bSame = StringCompare(maSubcircuitLabels.Get(i)->GetText(), pcCircuits->Get(i)->GetTypeName()) == 0;
	}

	mbListingSubcircuits = true;
	if (!bSame)
	{
		mpSubcircuits->RemoveAllComponents();
		maSubcircuitLabels.RemoveAll();
		for (i = 0; i < pcCircuits->NumCircuits(); i++)
		{
			AddListItem(mpSubcircuits, &maSubcircuitLabels, pcCircuits->Get(i)->GetTypeName());
		}
	}
	iSelected = -1;
	for (i = 0; i < pcCircuits->NumCircuits(); i++)
	{
		if (pcCircuits->Get(i) == pcEdited)
		{
			iSelected = (int32)i;
		}
	}
	if (mpSubcircuits->GetSelected() != iSelected)
	{
		mpSubcircuits->Select(iSelected);
	}
	mbListingSubcircuits = false;
}


//////////////////////////////////////////////////////////////////////////
//
// The subcircuit chosen in the list is edited, with a simulation of its
// own (see CCircuitEditor::Simulate), and zoomed to fit.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::SubcircuitSelected(void)
{
	CCircuitEditor*		pcEditor;
	char*				szName;

	if (mbListingSubcircuits)
	{
		return;
	}
	pcEditor = mpLogicimCircuit->GetEditor();
	szName = GetListItemText(mpSubcircuits, &maSubcircuitLabels);
	if (!StrEmpty(szName) && ((pcEditor->GetCircuit() == NULL) || (StringCompare(pcEditor->GetCircuit()->GetTypeName(), szName) != 0)))
	{
		if (pcEditor->IsInInstance() && (StringCompare(pcEditor->GetTopCircuit()->GetTypeName(), szName) == 0))
		{
			//Back out to the circuit simulated, keeping its simulation.
			pcEditor->LeaveInstances();
		}
		else
		{
			pcEditor->Simulate(szName);
		}
		ZoomToFit(false);
	}
	UpdateInfo();
	UpdateEditMenu();
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
	UpdateEditMenu();
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
	UpdateEditMenu();
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
	mpLeaveItem = NULL;
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
		mpLeaveItem = mpCanvasMenu->AddItem("Leave Sub-circuit");
	}
	UpdateEditMenu();
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
	else if (pcItem == &mpLeaveItem)
	{
		LeaveSubcircuit();
	}
}
