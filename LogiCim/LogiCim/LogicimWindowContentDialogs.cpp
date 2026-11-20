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
	else if (eDialog == LDG_NewProject)
	{
		if (AskToSaveProject())
		{
			NewProject();
		}
	}
	else if (eDialog == LDG_LoadProject)
	{
		if (AskToSaveProject())
		{
			ShowProjectDialog(false);
		}
	}
	else if (eDialog == LDG_SaveProject)
	{
		ShowProjectDialog(true);
	}
	else if (eDialog == LDG_ImportKicadSchematic)
	{
		if (AskToSaveProject())
		{
			ShowImportKicadDialog();
		}
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
	const char*			szName;
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
	const char*			szName;

	szDefault.Init("Top ");
	szDefault.Append(GetEditedCircuitName());
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
			mszSimulationName.Set(szName);
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
	const char*			szText;
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
// The component chosen is placed (see PlaceComponent).  The chooser opens
// again on the library of the component last chosen.  Shortcuts given in
// it are kept, whether or not a component was chosen.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowComponentChooser(void)
{
	char*	szChosen;
	bool	bSubcircuit;

	mpComponentChooser = OMalloc<CLogicimComponentChooser>(mpWindow, &msDialogLook, &mpLogicimCircuit, mszChooserSelected.Text(), &mpCommands, mpWindow->GetComponentInput());
	if (mpComponentChooser->Show())
	{
		szChosen = mpComponentChooser->GetChosen(&bSubcircuit);
		mszChooserSelected.Set(szChosen);
		PlaceComponent(szChosen, bSubcircuit);
	}
	if (mpComponentChooser->KeysChanged())
	{
		KeepKeymap();
	}
	mpComponentChooser = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// A component (szName its XML name) or a subcircuit (its type's name)
// follows the pointer to be placed, from the middle of the circuit if the
// pointer has never been over it.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::PlaceComponent(const char* szName, bool bSubcircuit)
{
	CCircuitEditor*	pcEditor;
	SFloat64Vec2	sScreen;
	SFloat64Vec2	sGrid;
	SInt32Vec2		sSize;

	pcEditor = mpLogicimCircuit->GetEditor();
	if (bSubcircuit ? pcEditor->StartPlaceSubcircuit(szName) : pcEditor->StartPlaceComponent(szName))
	{
		return;
	}
	sSize = mpCircuit->GetActualSize();
	sScreen.Init(sSize.x / 2, sSize.y / 2);
	mpCircuit->GetCircuitDraw()->GetViewport()->ScreenToGrid(&sGrid, &sScreen);
	pcEditor->PointerMoved(sGrid);
	if (bSubcircuit)
	{
		pcEditor->StartPlaceSubcircuit(szName);
	}
	else
	{
		pcEditor->StartPlaceComponent(szName);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Asks for the project file to load or save with Logicim's file chooser,
// starting on GetProjectDialogStart's.  False if it was cancelled, or
// the file couldn't be loaded or saved.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ShowProjectDialog(bool bSave)
{
	bool	bResult;

	bResult = false;
	mpFileChooser = OMalloc<CFileChooser>(mpWindow, &msDialogLook, bSave ? "Save Project" : "Load Project", bSave, GetProjectDialogStart(bSave), gasProjectFileTypes, sizeof(gasProjectFileTypes) / sizeof(SFileChooserType), mapIcons.Get(LA_LoadProject), mpFileIcon);
	if (mpFileChooser->Show())
	{
		if (bSave)
		{
			bResult = SaveProject(mpFileChooser->GetChosen());
		}
		else
		{
			bResult = LoadProject(mpFileChooser->GetChosen());
		}
	}
	mpFileChooser = NULL;
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// Before the project is closed (New, Load or Import, or a recent project
// loaded): if it has changes, asks whether to save them.  Save saves it
// (to its file, or asks for one if it hasn't one), Don't Save drops them;
// Cancel, or a save that didn't happen, leaves the project open.  True if
// the project can be closed.  Show it from a tick, not from input.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::AskToSaveProject(void)
{
	CFileUtil	cFileUtil;
	CChars		szMessage;
	CChars		szName;
	CChars		szClicked;
	char*		szFilename;
	bool		bSave;

	if (!mpLogicimCircuit->IsModified())
	{
		return true;
	}

	szFilename = mpLogicimCircuit->GetFilename();
	szMessage.Init("Save changes to ");
	if (StrEmpty(szFilename))
	{
		szMessage.Append("the new project?");
	}
	else
	{
		szName.Init();
		cFileUtil.GetFileName(&szName, szFilename);
		szMessage.Append(szName);
		szMessage.Append('?');
		szName.Kill();
	}
	mpMessageDialog = OMalloc<CMessageDialog>(mpWindow, &msDialogLook, "Save Changes", szMessage.Text());
	szMessage.Kill();
	mpMessageDialog->GetDialog()->SetOKCaption("Save");
	mpMessageDialog->GetDialog()->AddExtraButton("Don't Save");
	bSave = mpMessageDialog->Show();
	szClicked.Init(mpMessageDialog->GetClicked());
	mpMessageDialog = NULL;

	if (bSave)
	{
		szClicked.Kill();
		szFilename = mpLogicimCircuit->GetFilename();
		if (StrEmpty(szFilename))
		{
			return ShowProjectDialog(true);
		}
		szName.Init(szFilename);
		bSave = SaveProject(szName.Text());
		szName.Kill();
		return bSave;
	}
	bSave = szClicked.Equals("Don't Save");
	szClicked.Kill();
	return bSave;
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
		CloseExtraTabs();
		mcHistory.Clear();
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
		RestoreTabs();
		mcHistory.Clear();
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

	StoreTabs();
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
	CChars		szFile;
	CFileUtil	cFileUtil;

	GetLogicimSettingsFile(&szFile);
	mcSettings.Read(szFile.Text());
	szFile.Kill();
	GetLogicimRecentFile(&szFile);
	mcSettings.ReadRecent(szFile.Text());
	szFile.Kill();
	ShowRecentMenu();

	//The window and its tool windows as they were left; the window takes its place and size as it's shown.
	GetLogicimLayoutFile(&szFile);
	mszLayoutFile.Set(szFile.Text());
	szFile.Kill();
	if (!mszLayoutFile.Empty() && cFileUtil.Exists(mszLayoutFile.Text()) && !mpDock->LoadLayout(mszLayoutFile.Text()))
	{
		gcLogger.Warning2(__METHOD__, " Layout [", mszLayoutFile.Text(), "] couldn't be read: the tool windows start where they do.", NULL);
		mpDock->ResetLayout();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The window and its tool windows, written for next time (as LogiCim
// closes).  True if they were, or nothing is kept.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::SaveLayout(void)
{
	CFileUtil	cFileUtil;

	if (mszLayoutFile.Empty())
	{
		return true;
	}
	cFileUtil.TouchDir(mszLayoutFile.Text(), true);
	if (!mpDock->SaveLayout(mszLayoutFile.Text()))
	{
		return gcLogger.Error2(__METHOD__, " Layout [", mszLayoutFile.Text(), "] couldn't be written.", NULL);
	}
	return true;
}


//The keymap's groups, in the order Settings' Keymap shows them: the part of a command's name before its dot and what's
//shown.
static SKeymapGroup	gasLogicimKeymapGroups[] =
{
	{ "File", "File" },
	{ "Edit", "Edit" },
	{ "View", "View" },
	{ "Simulation", "Simulation" },
	{ "Place", "Place" },
	{ "Circuit", "Circuit" },
	{ "Window", "Window" },
};


//////////////////////////////////////////////////////////////////////////
//
// The keys the user changed, from their file (none for a scripted run,
// see GetLogicimSettingsFile), before the commands' keys are bound.  One
// that can't be read is said, and the commands keep their own keys.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ReadKeymap(void)
{
	CFileUtil	cFileUtil;

	mszKeymapFile.Kill();
	GetLogicimKeymapFile(&mszKeymapFile);
	if (!mszKeymapFile.Empty() && cFileUtil.Exists(mszKeymapFile.Text()) && !mpCommands->ReadKeys(mszKeymapFile.Text()))
	{
		gcLogger.Warning2(__METHOD__, " Keymap [", mszKeymapFile.Text(), "] couldn't be read: the default keys are used.", NULL);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// File, Settings: a dialog can't be shown from the keys or a menu item's
// input action, so it's shown at the next tick.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::RequestSettings(void)
{
	mpWindow->Post(this, &CLogicimWindowContent::ShowSettingsDialog);
}


//////////////////////////////////////////////////////////////////////////
//
// Settings' Keymap (WindowLib's CKeymapPage) on pDialog, with LogiCim's
// commands in their menus' groups.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CKeymapPage> CLogicimWindowContent::CreateKeymapPage(Ptr<CSettingsDialog> pDialog)
{
	return OMalloc<CKeymapPage>(pDialog->GetDialog(), mpCommands, mpWindow->GetComponentInput(), gasLogicimKeymapGroups, sizeof(gasLogicimKeymapGroups) / sizeof(gasLogicimKeymapGroups[0]));
}


//////////////////////////////////////////////////////////////////////////
//
// File, Settings, as IntelliJ IDEA's: its Keymap page.  What OK keeps
// applies now, and is written for next time.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowSettingsDialog(void)
{
	bool	bOK;

	mpSettingsDialog = OMalloc<CSettingsDialog>(mpWindow, &msDialogLook);
	mpSettingsDialog->AddPage(CreateKeymapPage(mpSettingsDialog));
	bOK = mpSettingsDialog->Show();
	mpSettingsDialog = NULL;
	if (bOK)
	{
		KeepKeymap();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The commands' keys, as the Keymap page or the component chooser left
// them, bound and written to the user's file, the subcircuits' to the
// project.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::KeepKeymap(void)
{
	KeepSubcircuitKeys();
	mpCommands->BindKeys(mpWindow->GetComponentInput());
	if (mszKeymapFile.Empty())
	{
		gcLogger.Info("Changed the keymap.");
	}
	else if (mpCommands->WriteKeys(mszKeymapFile.Text()))
	{
		gcLogger.Info2("Saved the keymap [", mszKeymapFile.Text(), "].", NULL);
	}
	else
	{
		gcLogger.Warning2(__METHOD__, " Changed the keymap, but it couldn't be saved for next time [", mszKeymapFile.Text(), "].", NULL);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// For checking: what a user would do on Settings' Keymap (szEdits, see
// CKeymapPage::ApplyEdits), then OK, without the dialog being shown.
// False if there's no window input to bind the keys to.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ApplyKeymapEdits(const char* szEdits)
{
	Ptr<CSettingsDialog>	pDialog;
	Ptr<CKeymapPage>		pPage;

	if (mpWindow->GetComponentInput() == NULL)
	{
		return false;
	}
	pDialog = OMalloc<CSettingsDialog>(mpWindow, &msDialogLook);
	pPage = CreateKeymapPage(pDialog);
	pDialog->AddPage(pPage);
	pPage->ApplyEdits(szEdits);
	pPage->Keep();
	KeepKeymap();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// For checking: what a user would do with the component chooser's
// shortcut (szEdits, see CLogicimComponentChooser::ApplyEdits), then
// closing it.  False if there are no keyboards to bind keys to.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ApplyChooserEdits(const char* szEdits)
{
	Ptr<CLogicimComponentChooser>	pChooser;

	if (mpWindow->GetComponentInput() == NULL)
	{
		return false;
	}
	pChooser = OMalloc<CLogicimComponentChooser>(mpWindow, &msDialogLook, &mpLogicimCircuit, mszChooserSelected.Text(), &mpCommands, mpWindow->GetComponentInput());
	pChooser->ApplyEdits(szEdits);
	if (pChooser->KeysChanged())
	{
		KeepKeymap();
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Window, Reset Window Layout: the tool windows back where they start.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ResetLayout(void)
{
	mpDock->ResetLayout();
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


//////////////////////////////////////////////////////////////////////////
//
// File, Exit and the window's close button: true if LogiCim can close
// now.  With changes it asks to save them at the next tick (a dialog
// can't be shown from the input that exits), and closes itself then
// unless asked not to.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::RequestExit(void)
{
	if (!mpLogicimCircuit->IsModified())
	{
		return true;
	}
	mpWindow->Post(this, &CLogicimWindowContent::AskToSaveAndExit);
	return false;
}


void CLogicimWindowContent::AskToSaveAndExit(void)
{
	if (AskToSaveProject())
	{
		mpWindow->Stop();
	}
}


void CLogicimWindowContent::OpenQueuedRecentProject(void)
{
	size	uiIndex;

	uiIndex = muiQueuedRecent;
	muiQueuedRecent = MAX_SIZE;
	if (AskToSaveProject())
	{
		OpenRecentProject(uiIndex);
	}
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
// How a KiCad import is going, in its dialog: the step last told
// ("Importing Selector 12bit: Placing") above a bar filled as far as the
// boards' steps have been told.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowImportProgress(void)
{
	CChars	szText;
	int64	iDone;
	int64	iTotal;

	mcImportProgress.Get(&szText, &iDone, &iTotal);
	mpImportDialog->SetText(szText.Text());
	mpImportDialog->SetProgress(iDone, iTotal);
	szText.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The import dialog's tick: how the import is going, and once every
// circuit is laid out the dialog is done with.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ImportTick(void)
{
	ShowImportProgress();
	if (mpLogicimCircuit->IsImportKicadSchematicDone())
	{
		mpImportDialog->Finish();
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimImportProgress::Init(void)
{
	mcMutex.Init();
	mszBoard.Init();
	mszStage.Init();
	miBoards = 0;
	miSteps = 0;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimImportProgress::Kill(void)
{
	mszStage.Kill();
	mszBoard.Kill();
	mcMutex.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimImportProgress::Reset(void)
{
	mcMutex.Lock();
	mszBoard.Set("");
	mszStage.Set("");
	miBoards = 0;
	miSteps = 0;
	mcMutex.Unlock();
}


//////////////////////////////////////////////////////////////////////////
//
// A board is counted when it's read, which is its first step.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimImportProgress::Progress(const char* szBoard, const char* szStage, size uiStep, size uiSteps)
{
	mcMutex.Lock();
	mszBoard.Set(szBoard);
	mszStage.Set(szStage);
	miBoards += (uiStep == KICAD_SCHEM_IMPORT_STEP_READ) ? 1 : 0;
	miSteps++;
	mcMutex.Unlock();
}


//////////////////////////////////////////////////////////////////////////
//
// The step last told, as the import's dialog says it, and the steps told of
// all that the boards so far could have (a board whose circuit is placed
// in fewer tries than it might be has fewer, so every step told is short
// of them all until the import is done).
//
//////////////////////////////////////////////////////////////////////////
void CLogicimImportProgress::Get(CChars* pszText, int64* piDone, int64* piTotal)
{
	mcMutex.Lock();
	pszText->Init("Importing ");
	pszText->Append(mszBoard.Text());
	pszText->Append(": ");
	pszText->Append(mszStage.Text());
	*piDone = miSteps;
	*piTotal = miBoards * KICAD_SCHEM_IMPORT_STEPS;
	mcMutex.Unlock();
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
	const char*				szFilename;

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
// changed.  bCancel is for checking, see ImportKicadSchematic.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ImportChosenKicadSchematic(const char* szFilename, bool bCancel)
{
	bool	bResult;

	bResult = ImportKicadSchematic(szFilename, bCancel);
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
// the Import KiCad Schematic dialog does.  Its circuits are laid out on
// other threads: while they are a dialog over the window says how it's
// going, and Cancel there gives the import up with the project as it was.
// With no window shown, as in a scripted check, it just waits for them.
// Call it from a tick, as it shows a dialog.
//
// bCancel is for checking: the import is cancelled once it has begun, as
// the dialog's Cancel clicked straight away would, with no dialog shown.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ImportKicadSchematic(const char* szFilename, bool bCancel)
{
	bool	bResult;

	mcImportProgress.Reset();
	if (mpLogicimCircuit->BeginImportKicadSchematic(szFilename, &mcImportProgress) && mpWindow->IsShown() && !bCancel)
	{
		bCancel = !ShowImportProgressDialog();
	}
	if (bCancel)
	{
		mpLogicimCircuit->CancelImportKicadSchematic();
		gcLogger.Info2("Cancelled importing [", szFilename, "].", NULL);
	}

	bResult = mpLogicimCircuit->EndImportKicadSchematic();
	if (bResult)
	{
		gcLogger.Info2("Imported [", szFilename, "].", NULL);
		CloseExtraTabs();
		mcHistory.Clear();
		ZoomToFit(false);
	}
	UpdateInfo();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// Shows the import's progress dialog over the window until the import
// begun has laid its circuits out.  False if the dialog was cancelled
// first: the import is still going.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ShowImportProgressDialog(void)
{
	bool	bLaidOut;

	mpImportDialog = OMalloc<CProgressDialog>(mpWindow, &msDialogLook, "Import KiCad Schematic", "");
	mpImportDialog->SetTicker(this, &CLogicimWindowContent::ImportTick);
	ShowImportProgress();
	bLaidOut = mpImportDialog->Show();
	mpImportDialog = NULL;
	return bLaidOut;
}
