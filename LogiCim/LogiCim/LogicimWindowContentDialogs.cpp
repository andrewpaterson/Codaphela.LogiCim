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
#include "BaseLib/StringHelper.h"
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
// The property's value from its editor (see AddPropertyEditor).
//
//////////////////////////////////////////////////////////////////////////
static void ReadLogicimPropertyEditor(SComponentProperty* psProperty, int iEditor, CArray<CTextField>* paTextFields, CArray<CCheckBox>* paCheckBoxes, CArray<CComboBox>* paComboBoxes)
{
	switch (psProperty->eType)
	{
	case CPT_Boolean:
		psProperty->szValue.Set(paCheckBoxes->Get(iEditor)->IsTicked() ? "true" : "false");
		break;
	case CPT_Choice:
		psProperty->szValue.Set(paComboBoxes->Get(iEditor)->GetSelectedText());
		break;
	case CPT_Divider:
		break;
	default:
		psProperty->szValue.Set(paTextFields->Get(iEditor)->GetText());
		break;
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
	Ptr<CComboBox>			pRotation;
	Ptr<CCheckBox>			pMirrored;
	CArray<CTextField>		aTextFields;
	CArray<CCheckBox>		aCheckBoxes;
	CArray<CComboBox>		aComboBoxes;
	CArrayInt				aiEditors;  //Each property's editor in its type's array, -1 for a divider.
	CChars					sz;
	size					i;
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
		aiEditors.Add(AddPropertyEditor(&cProperties, cProperties.Get(i), &aTextFields, &aCheckBoxes, &aComboBoxes));
	}

	if (mpDialog->Show(LOGICIM_DIALOG_WIDTH))
	{
		for (i = 0; i < cProperties.NumProperties(); i++)
		{
			ReadLogicimPropertyEditor(cProperties.Get(i), aiEditors.GetValue(i), &aTextFields, &aCheckBoxes, &aComboBoxes);
		}
		eRotation = (ERotation)(ROT_North + pRotation->GetSelected());
		pcEditor->SetProperties(pcComponent, &cProperties, eRotation, pMirrored->IsTicked());
	}

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
// Adds the property's editor to the dialog and to its type's array.
// Returns its index in that array, -1 for a divider.
//
//////////////////////////////////////////////////////////////////////////
int CLogicimWindowContent::AddPropertyEditor(CComponentProperties* pcProperties, SComponentProperty* psProperty, CArray<CTextField>* paTextFields, CArray<CCheckBox>* paCheckBoxes, CArray<CComboBox>* paComboBoxes)
{
	Ptr<CComboBox>	pComboBox;
	CChars			sz;
	size			j;
	int				iEditor;

	switch (psProperty->eType)
	{
	case CPT_Boolean:
		iEditor = (int)paCheckBoxes->NumElements();
		paCheckBoxes->Add(mpDialog->AddCheckBox(psProperty->szLabel.Text(), psProperty->szValue.Equals("true")));
		return iEditor;
	case CPT_Choice:
		pComboBox = OMalloc<CComboBox>(mpDialog->GetWindow(), msDialogLook.pcTheme);
		for (j = 0; j < psProperty->aszChoices.NumElements(); j++)
		{
			pComboBox->AddItem(psProperty->aszChoices.GetText(j));
		}
		pComboBox->Select((int32)pcProperties->GetChoice(psProperty->szName.Text(), 0));
		pComboBox->SetAlignment(CSH_Stretch, CSV_Center);
		mpDialog->AddEditor(psProperty->szLabel.Text(), pComboBox);
		iEditor = (int)paComboBoxes->NumElements();
		paComboBoxes->Add(pComboBox);
		pComboBox = NULL;
		return iEditor;
	case CPT_Divider:
		mpDialog->AddDivider(psProperty->szLabel.Text());
		return -1;
	default:
		sz.Init(psProperty->szLabel);
		if (!psProperty->szUnit.Empty())
		{
			sz.Append(" (");
			sz.Append(psProperty->szUnit);
			sz.Append(")");
		}
		iEditor = (int)paTextFields->NumElements();
		paTextFields->Add(mpDialog->AddTextField(sz.Text(), psProperty->szValue.Text()));
		sz.Kill();
		return iEditor;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowNewSubcircuitDialog(void)
{
	Ptr<CTextField>		pName;
	char*				szName;
	CChars				szMessage;

	mpDialog = OMalloc<CDialog>(mpWindow, &msDialogLook, "New Subcircuit");
	pName = mpDialog->AddTextField("Name", "");

	if (mpDialog->Show(LOGICIM_DIALOG_WIDTH))
	{
		szName = pName->GetText();
		if (szName[0] != '\0')
		{
			if (mpLogicimCircuit->GetEditor()->GetCircuits()->Get(szName) != NULL)
			{
				szMessage.Init("There is already a subcircuit called ");
				szMessage.Append(szName);
				szMessage.Append('.');
				ShowMessage("New Subcircuit", szMessage.Text());
				szMessage.Kill();
			}
			else if (mpLogicimCircuit->GetEditor()->AddCircuit(szName) != NULL)
			{
				mpLogicimCircuit->GetEditor()->Simulate(szName);
			}
			UpdateInfo();
		}
	}

	pName = NULL;
	mpDialog = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// szMessage told in a dialog with only OK.  Show it from a tick (after
// another dialog, say), not from input.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowMessage(const char* szTitle, const char* szMessage)
{
	mpMessageDialog = OMalloc<CMessageDialog>(mpWindow, &msDialogLook, szTitle, szMessage);
	mpMessageDialog->GetDialog()->SetCancelCaption(NULL);
	mpMessageDialog->Show();
	mpMessageDialog = NULL;
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
// starting on GetProjectDialogStart's.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowProjectDialog(bool bSave)
{
	mpFileChooser = OMalloc<CFileChooser>(mpWindow, &msDialogLook, bSave ? "Save Project" : "Load Project", bSave, GetProjectDialogStart(bSave), gasProjectFileTypes, sizeof(gasProjectFileTypes) / sizeof(SFileChooserType), mapIcons.Get(LA_LoadProject), mpFileIcon);
	if (mpFileChooser->Show())
	{
		if (bSave)
		{
			SaveProject(mpFileChooser->GetChosen());
		}
		else
		{
			LoadProject(mpFileChooser->GetChosen());
		}
	}
	mpFileChooser = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Save offers the project's own file; Load opens on the project last
// loaded, as does Save when the project hasn't a file.
//
//////////////////////////////////////////////////////////////////////////
char* CLogicimWindowContent::GetProjectDialogStart(bool bSave)
{
	if (bSave && !StrEmpty(mpLogicimCircuit->GetFilename()))
	{
		return mpLogicimCircuit->GetFilename();
	}
	return mcSettings.GetProjectLoad();
}


//////////////////////////////////////////////////////////////////////////
//
// File, New Project: the old project is closed and an empty one, without
// a file, is edited in its place.  Where Load Project opens, and the
// recent projects, aren't changed.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::NewProject(void)
{
	bool	bResult;

	bResult = mpLogicimCircuit->NewProject();
	if (bResult)
	{
		gcLogger.Info("New project.");
		Zoom(1.0f);
	}
	UpdateInfo();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// As Load Project does with the file chosen: Load Project opens on it
// from now on, if it was loaded.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::LoadProject(const char* szFilename)
{
	bool	bResult;

	bResult = mpLogicimCircuit->Load(szFilename);
	if (bResult)
	{
		mcSettings.SetProjectLoad(szFilename);
		mcSettings.AddRecent(szFilename);
		ShowRecentMenu();
		gcLogger.Info2("Loaded [", szFilename, "].", NULL);
	}
	UpdateInfo();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// As Save Project does with the file chosen.  Where Load Project opens
// isn't changed.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::SaveProject(const char* szFilename)
{
	bool	bResult;

	bResult = mpLogicimCircuit->Save(szFilename);
	if (bResult)
	{
		mcSettings.AddRecent(szFilename);
		ShowRecentMenu();
		gcLogger.Info2("Saved [", szFilename, "].", NULL);
	}
	UpdateInfo();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// Where the user last loaded and imported from, and the recent projects,
// from the user's own files (none for a scripted run, see
// GetLogicimSettingsFile).
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ReadSettings(void)
{
	CChars	szFile;

	GetLogicimSettingsFile(&szFile);
	mcSettings.Read(szFile.Text());
	szFile.Kill();
	GetLogicimRecentFile(&szFile);
	mcSettings.ReadRecent(szFile.Text());
	szFile.Kill();
	ShowRecentMenu();
}


//////////////////////////////////////////////////////////////////////////
//
// An item for each recent project, the latest first, numbered as the
// IDE's recent solutions: "1 /home/.../FlipFlop.xml".  With none, one
// that says so.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowRecentMenu(void)
{
	CRecentFiles*	pcRecent;
	Ptr<CMenuItem>	pItem;
	CChars			szText;
	size			i;

	if (mpRecentMenu.IsNull())
	{
		return;
	}
	pcRecent = mcSettings.GetRecent();
	mpRecentMenu->RemoveAllItems();
	if (pcRecent->NumFiles() == 0)
	{
		mpRecentMenu->AddItem("(No recent projects)")->SetEnabled(false);
		return;
	}
	szText.Init();
	for (i = 0; i < pcRecent->NumFiles(); i++)
	{
		szText.Clear();
		szText.Append((uint32)(i + 1));
		szText.Append(' ');
		szText.Append(pcRecent->GetFile(i));
		pItem = mpRecentMenu->AddItem(szText.Text());
		pItem->SetTag((int32)i);
		pItem->GetClickEvent()->Add(this, &CLogicimWindowContent::RecentItemClicked);
	}
	szText.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// A recent project's item: it is loaded at the next tick, once its menu
// is done with the click.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::RecentItemClicked(CMenuItem* pcItem)
{
	muiQueuedRecent = (size)pcItem->GetTag();
	mpWindow->Post(this, &CLogicimWindowContent::OpenQueuedRecentProject);
}


void CLogicimWindowContent::OpenQueuedRecentProject(void)
{
	size	uiIndex;

	uiIndex = muiQueuedRecent;
	muiQueuedRecent = MAX_SIZE;
	OpenRecentProject(uiIndex);
}


//////////////////////////////////////////////////////////////////////////
//
// The recent project uiIndex (0 the latest) is loaded as Load Project
// would.  One that isn't there any more is taken off.  False if it
// couldn't be loaded, or there isn't one.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::OpenRecentProject(size uiIndex)
{
	CFileUtil	cFileUtil;
	CChars		szFilename;
	bool		bResult;

	if (uiIndex >= mcSettings.GetRecent()->NumFiles())
	{
		return false;
	}
	szFilename.Init(mcSettings.GetRecent()->GetFile(uiIndex));
	bResult = LoadProject(szFilename.Text());
	if (!bResult && !cFileUtil.Exists(szFilename.Text()))
	{
		mcSettings.RemoveRecent(szFilename.Text());
		ShowRecentMenu();
		gcLogger.Warning2(__METHOD__, " [", szFilename.Text(), "] isn't there any more.", NULL);
	}
	szFilename.Kill();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// File, Clear Recent Projects: done at the next tick, not while the menu
// that holds Recent Projects is handling the click.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::QueueClearRecentProjects(void)
{
	mpWindow->Post(this, &CLogicimWindowContent::ClearRecentProjects);
}


void CLogicimWindowContent::ClearRecentProjects(void)
{
	mcSettings.ClearRecent();
	ShowRecentMenu();
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

	mpFileChooser = OMalloc<CFileChooser>(mpWindow, &msDialogLook, "Import KiCad Schematic", false, mcSettings.GetKicadImport(), gasKicadFileTypes, sizeof(gasKicadFileTypes) / sizeof(SFileChooserType), mapIcons.Get(LA_LoadProject), mpFileIcon);
	if (mpFileChooser->Show())
	{
		szFilename = mpFileChooser->GetChosen();
		ImportChosenKicadSchematic(szFilename);
	}
	mpFileChooser = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// As Import KiCad Schematic does with the file chosen: it opens on it
// from now on, if it was imported.  Where Load Project opens isn't
// changed.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ImportChosenKicadSchematic(const char* szFilename)
{
	bool	bResult;

	bResult = ImportKicadSchematic(szFilename);
	if (bResult)
	{
		mcSettings.SetKicadImport(szFilename);
	}
	return bResult;
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
		gcLogger.Info2("Imported [", szFilename, "].", NULL);
		ZoomToFit(false);
	}
	UpdateInfo();
	return bResult;
}
