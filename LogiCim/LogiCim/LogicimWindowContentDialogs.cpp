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


//The project file chooser's Files of Type, Logicim's files first as its file chooser's filter is.
static SFileChooserType gasProjectFileTypes[] =
{
	{ "Logicim Projects (*.xml)", "xml" },
	{ "All Files", NULL },
};


//The KiCad schematic chooser's Files of Type.
static SFileChooserType gasKicadFileTypes[] =
{
	{ "KiCad Schematics (*.kicad_sch)", "kicad_sch" },
	{ "All Files", NULL },
};


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowDialog(ELogicimDialog eDialog)
{
	if (eDialog == LDG_Properties)
	{
		ShowPropertiesDialog();
	}
	else if (eDialog == LDG_NewSubcircuit)
	{
		ShowNewSubcircuitDialog();
	}
	else if (eDialog == LDG_NewSimulation)
	{
		ShowNewSimulationDialog();
	}
	else if (eDialog == LDG_SelectZoom)
	{
		ShowSelectZoomDialog();
	}
	else if (eDialog == LDG_ChooseComponent)
	{
		ShowComponentChooser();
	}
	else if (eDialog == LDG_LoadProject)
	{
		ShowProjectDialog(false);
	}
	else if (eDialog == LDG_SaveProject)
	{
		ShowProjectDialog(true);
	}
	else if (eDialog == LDG_ImportKicadSchematic)
	{
		ShowImportKicadDialog();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's property editor dialog for the hovered or selected component,
// as its ReflectivePropertiesPanel: its rotation, then an editor for each
// of its properties (see CComponentProperties) in order, a text field for
// text and numbers (a number's unit after its label), a check box for a
// yes or no, a combo box for a choice, and a divider for a divider.  OK
// replaces the component with one made from them.  Nothing to edit shows
// nothing.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowPropertiesDialog(void)
{
	const char*				aszRotations[] = { "North", "East", "South", "West" };
	CCircuitEditor*			pcEditor;
	CComponentView*			pcComponent;
	CComponentProperties	cProperties;
	SComponentProperty*		psProperty;
	Ptr<CComboBox>			pRotation;
	Ptr<CCheckBox>			pMirrored;
	CArray<CTextField>		aTextFields;
	CArray<CCheckBox>		aCheckBoxes;
	CArray<CComboBox>		aComboBoxes;
	CArrayInt				aiEditors;  //Each property's editor in its type's array, -1 for a divider.
	Ptr<CComboBox>			pComboBox;
	CChars					sz;
	size					i;
	size					j;
	int						iEditor;
	ERotation				eRotation;

	pcEditor = mpLogicimCircuit->GetEditor();
	pcComponent = pcEditor->GetPropertiesComponent();
	if (pcComponent == NULL)
	{
		return;
	}
	cProperties.Init();
	pcEditor->GetProperties(pcComponent, &cProperties);

	sz.Init(pcComponent->GetType());
	sz.Append(" Properties");
	mpDialog = OMalloc<CDialog>(mpWindow, &msDialogLook, sz.Text());
	sz.Kill();

	aTextFields.Init();
	aCheckBoxes.Init();
	aComboBoxes.Init();
	aiEditors.Init();
	pRotation = mpDialog->AddComboBox("Rotation", aszRotations, 4, pcComponent->GetRotation() - ROT_North);
	pMirrored = mpDialog->AddCheckBox("Mirrored", pcComponent->IsMirrored());
	for (i = 0; i < cProperties.NumProperties(); i++)
	{
		psProperty = cProperties.Get(i);
		switch (psProperty->eType)
		{
		case CPT_Boolean:
			aiEditors.Add((int)aCheckBoxes.Size());
			aCheckBoxes.Add(mpDialog->AddCheckBox(psProperty->szLabel.Text(), psProperty->szValue.Equals("true")));
			break;
		case CPT_Choice:
			pComboBox = OMalloc<CComboBox>(mpDialog->GetWindow(), msDialogLook.pcTheme);
			for (j = 0; j < psProperty->aszChoices.NumElements(); j++)
			{
				pComboBox->AddItem(psProperty->aszChoices.GetText(j));
			}
			pComboBox->Select((int32)cProperties.GetChoice(psProperty->szName.Text(), 0));
			pComboBox->SetAlignment(CSH_Stretch, CSV_Center);
			mpDialog->AddEditor(psProperty->szLabel.Text(), pComboBox);
			aiEditors.Add((int)aComboBoxes.Size());
			aComboBoxes.Add(pComboBox);
			break;
		case CPT_Divider:
			mpDialog->AddDivider(psProperty->szLabel.Text());
			aiEditors.Add(-1);
			break;
		default:
			sz.Init(psProperty->szLabel);
			if (!psProperty->szUnit.Empty())
			{
				sz.Append(" (");
				sz.Append(psProperty->szUnit);
				sz.Append(")");
			}
			aiEditors.Add((int)aTextFields.Size());
			aTextFields.Add(mpDialog->AddTextField(sz.Text(), psProperty->szValue.Text()));
			sz.Kill();
			break;
		}
	}

	if (mpDialog->Show(LOGICIM_DIALOG_WIDTH))
	{
		for (i = 0; i < cProperties.NumProperties(); i++)
		{
			psProperty = cProperties.Get(i);
			iEditor = aiEditors.GetValue(i);
			switch (psProperty->eType)
			{
			case CPT_Boolean:
				psProperty->szValue.Set(aCheckBoxes.Get(iEditor)->IsTicked() ? "true" : "false");
				break;
			case CPT_Choice:
				psProperty->szValue.Set(aComboBoxes.Get(iEditor)->GetSelectedText());
				break;
			case CPT_Divider:
				break;
			default:
				psProperty->szValue.Set(aTextFields.Get(iEditor)->GetText());
				break;
			}
		}
		eRotation = (ERotation)(ROT_North + pRotation->GetSelected());
		pcEditor->SetProperties(pcComponent, &cProperties, eRotation, pMirrored->IsTicked());
	}

	pComboBox = NULL;
	pRotation = NULL;
	pMirrored = NULL;
	aiEditors.Kill();
	aComboBoxes.Kill();
	aCheckBoxes.Kill();
	aTextFields.Kill();
	mpDialog = NULL;
	cProperties.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowNewSubcircuitDialog(void)
{
	Ptr<CTextField>		pName;
	char*				szName;

	mpDialog = OMalloc<CDialog>(mpWindow, &msDialogLook, "New Subcircuit");
	pName = mpDialog->AddTextField("Name", "");

	if (mpDialog->Show(LOGICIM_DIALOG_WIDTH))
	{
		szName = pName->GetText();
		if (szName[0] != '\0')
		{
			if (mpLogicimCircuit->GetEditor()->GetCircuits()->Get(szName) != NULL)
			{
				gcLogger.Error2(__METHOD__, " There is already a subcircuit [", szName, "].", NULL);
			}
			else if (mpLogicimCircuit->GetEditor()->AddCircuit(szName) != NULL)
			{
				mpLogicimCircuit->GetEditor()->Simulate(szName);
			}
			UpdateInfo();
			UpdateEditMenu();
		}
	}

	pName = NULL;
	mpDialog = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Named like Logicim's: the top subcircuit and a count.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowNewSimulationDialog(void)
{
	Ptr<CTextField>		pName;
	CChars				szDefault;
	char*				szName;

	szDefault.Init("Top ");
	szDefault.Append(GetListItemText(mpSubcircuits, &maSubcircuitLabels));
	szDefault.Append(" ");
	szDefault.Append(miSimulationCount + 1);

	mpDialog = OMalloc<CDialog>(mpWindow, &msDialogLook, "New Simulation");
	pName = mpDialog->AddTextField("Name", szDefault.Text());
	szDefault.Kill();

	if (mpDialog->Show(LOGICIM_DIALOG_WIDTH))
	{
		szName = pName->GetText();
		if (szName[0] != '\0')
		{
			miSimulationCount++;
			AddListItem(mpSimulations, &maSimulationLabels, szName);
			mpSimulations->Select(mpSimulations->NumRows() - 1);
			UpdateInfo();
		}
	}

	pName = NULL;
	mpDialog = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The zoom as a factor, 1 for 100%, about the middle of the canvas.  Text
// that isn't a number leaves the zoom as it is; a number is kept between
// the least and most zoom.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowSelectZoomDialog(void)
{
	Ptr<CTextField>		pZoom;
	Ptr<CCircuitCanvasDraw>	pDraw;
	CChars				szZoom;
	char*				szText;
	char*				szEnd;
	float64				fZoom;

	pDraw = mpCircuit->GetCircuitDraw();
	szZoom.Init();
	AppendTenths(&szZoom, pDraw->GetZoom());
	mpDialog = OMalloc<CDialog>(mpWindow, &msDialogLook, "Select Zoom");
	pZoom = mpDialog->AddTextField("Zoom (1 is 100%)", szZoom.Text());
	szZoom.Kill();

	if (mpDialog->Show(LOGICIM_DIALOG_WIDTH))
	{
		szText = pZoom->GetText();
		fZoom = strtod(szText, &szEnd);
		while (*szEnd == ' ')
		{
			szEnd++;
		}
		if ((szEnd != szText) && (*szEnd == '\0') && (fZoom > 0))
		{
			pDraw->ZoomAroundCentre((float32)fZoom);
			ShowZoom();
		}
	}

	pZoom = NULL;
	mpDialog = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Places the component chosen, at the pointer, or at the middle of the
// circuit if the pointer has never been over it.  The chooser opens again
// on the library of the component last chosen.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowComponentChooser(void)
{
	CCircuitEditor*			pcEditor;
	char*					szChosen;
	bool					bSubcircuit;
	bool					bPlaced;
	SFloat64Vec2			sScreen;
	SFloat64Vec2			sGrid;
	SInt32Vec2				sSize;

	mpComponentChooser = OMalloc<CLogicimComponentChooser>(mpWindow, &msDialogLook, &mpLogicimCircuit, mszChooserSelected.Text());
	if (mpComponentChooser->Show())
	{
		szChosen = mpComponentChooser->GetChosen(&bSubcircuit);
		mszChooserSelected.Set(szChosen);
		pcEditor = mpLogicimCircuit->GetEditor();
		bPlaced = bSubcircuit ? pcEditor->StartPlaceSubcircuit(szChosen) : pcEditor->StartPlaceComponent(szChosen);
		if (!bPlaced)
		{
			sSize = mpCircuit->GetActualSize();
			sScreen.Init(sSize.x / 2, sSize.y / 2);
			mpCircuit->GetCircuitDraw()->GetViewport()->ScreenToGrid(&sGrid, &sScreen);
			pcEditor->PointerMoved(sGrid);
			if (bSubcircuit)
			{
				pcEditor->StartPlaceSubcircuit(szChosen);
			}
			else
			{
				pcEditor->StartPlaceComponent(szChosen);
			}
		}
	}
	mpComponentChooser = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Asks for the project file to load or save with Logicim's file chooser,
// starting on the one last used.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowProjectDialog(bool bSave)
{
	char*				szFilename;
	bool				bResult;

	mpFileChooser = OMalloc<CFileChooser>(mpWindow, &msDialogLook, bSave ? "Save Project" : "Load Project", bSave, mszProjectFilename.Text(), gasProjectFileTypes, sizeof(gasProjectFileTypes) / sizeof(SFileChooserType), mapIcons.Get(LA_LoadProject), mpFileIcon);
	if (mpFileChooser->Show())
	{
		szFilename = mpFileChooser->GetChosen();
		mszProjectFilename.Set(szFilename);
		bResult = bSave ? mpLogicimCircuit->Save(szFilename) : mpLogicimCircuit->Load(szFilename);
		if (bResult)
		{
			gcLogger.Info2(bSave ? "Saved [" : "Loaded [", szFilename, "].", NULL);
		}
		UpdateInfo();
	}
	mpFileChooser = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Asks for a KiCad schematic and replaces the project with the circuit
// made from it (see CLogicimCircuit::ImportKicadSchematic).  Saving the
// project then offers the schematic's name as an .xml file.  The imported
// circuit is zoomed to fit.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowImportKicadDialog(void)
{
	char*				szFilename;

	mpFileChooser = OMalloc<CFileChooser>(mpWindow, &msDialogLook, "Import KiCad Schematic", false, mszKicadFilename.Text(), gasKicadFileTypes, sizeof(gasKicadFileTypes) / sizeof(SFileChooserType), mapIcons.Get(LA_LoadProject), mpFileIcon);
	if (mpFileChooser->Show())
	{
		szFilename = mpFileChooser->GetChosen();
		mszKicadFilename.Set(szFilename);
		ImportKicadSchematic(szFilename);
	}
	mpFileChooser = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The schematic is imported at the first tick, as LOGICIM_IMPORT asks for
// scripted checks.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::SetStartupImport(const char* szFilename)
{
	mszStartupImport.Set(szFilename);
}


//////////////////////////////////////////////////////////////////////////
//
// Replaces the project with the circuit made from the KiCad schematic, as
// the Import KiCad Schematic dialog does.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ImportKicadSchematic(const char* szFilename)
{
	bool	bResult;

	bResult = mpLogicimCircuit->ImportKicadSchematic(szFilename);
	if (bResult)
	{
		mszProjectFilename.Set(szFilename);
		if (mszProjectFilename.EndsWith(".kicad_sch"))
		{
			mszProjectFilename.RemoveFromEnd(10);
		}
		mszProjectFilename.Append(".xml");
		gcLogger.Info2("Imported [", szFilename, "].", NULL);
		ZoomToFit(false);
	}
	UpdateInfo();
	return bResult;
}
