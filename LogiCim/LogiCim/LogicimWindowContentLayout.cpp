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
// The icons converted from Logicim's by Tools/ConvertLogicimIcons.py, in
// the window's colour order.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::LoadIcons(const char* szIconDirectory)
{
	Ptr<CImageCel>		pIcon;
	int					i;
	bool				bResult;

	bResult = true;
	for (i = 0; i < NUM_LOGICIM_ACTIONS; i++)
	{
		if (gasLogicimActions[i].szIcon == NULL)
		{
			mapIcons.Add(NULL);
			continue;
		}

		pIcon = ReadIcon(szIconDirectory, gasLogicimActions[i].szIcon);
		if (pIcon.IsNull())
		{
			bResult = false;
		}
		mapIcons.Add(pIcon);
	}

	mpFileIcon = ReadIcon(szIconDirectory, "new_generic");
	if (mpFileIcon.IsNull())
	{
		bResult = false;
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// NULL if the icon can't be read.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CImageCel> CLogicimWindowContent::ReadIcon(const char* szIconDirectory, const char* szIcon)
{
	CFileUtil			cFileUtil;
	CChars				szFilename;
	Ptr<CImage>			pImage;

	szFilename.Init(szIconDirectory);
	cFileUtil.AppendToPath(&szFilename, szIcon);
	szFilename.Append("_24.png");
	pImage = ReadImage(szFilename.Text());
	if (pImage.IsNull())
	{
		gcLogger.Error2(__METHOD__, " Could not read icon [", szFilename.Text(), "].", NULL);
		szFilename.Kill();
		return NULL;
	}
	szFilename.Kill();

	pImage = CImageColourConverter::ConvertColour(pImage, mpWindow->GetColourOrder());
	return OMalloc<CImageCel>(pImage);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::CreateLooks(void)
{
	//The dialogs share the window's theme, they have the same colour format.
	mpTheme = CJewelTheme::CreateDarkTheme(mpWindow, mpFontColours);
	mpTheme->SetMetric(TM_ToolbarThickness, 40);  //IntelliJ IDEA's main toolbar and stripes.
	mpTheme->SetMetric(TM_StatusBarHeight, 26);
	mpTheme->SetMetric(TM_StatusFieldGap, 10);
	mpFont = mpTheme->GetFont();
	msDialogLook.pcTheme = &mpTheme;
	msDialogLook.pcDimFont = &mpDimFont;
}


//////////////////////////////////////////////////////////////////////////
//
// The actions are the menu's items in order, LA_Separator between groups.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CMenuContainer> CLogicimWindowContent::CreateMenu(ELogicimAction* aeActions, int iNumActions)
{
	Ptr<CMenuContainer>		pMenu;

	pMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	AddMenuActions(pMenu, aeActions, iNumActions);
	return pMenu;
}


//////////////////////////////////////////////////////////////////////////
//
// The actions as pMenu's next items, LA_Separator between groups.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::AddMenuActions(Ptr<CMenuContainer> pMenu, ELogicimAction* aeActions, int iNumActions)
{
	int		i;

	for (i = 0; i < iNumActions; i++)
	{
		if (aeActions[i] == LA_Separator)
		{
			pMenu->AddSeparator();
		}
		else
		{
			pMenu->AddItem(mpCommands->Find(gasLogicimActions[aeActions[i]].szCommand));
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// New, Load and Save Project, Recent Projects (see ShowRecentMenu) and Clear
// Recent Projects, then Import KiCad Schematic, Settings and Exit.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CMenuContainer> CLogicimWindowContent::CreateFileMenu(void)
{
	ELogicimAction			aeProject[] = { LA_NewProject, LA_LoadProject, LA_SaveProject };
	ELogicimAction			aeImport[] = { LA_Separator, LA_ImportKicadSchematic, LA_Separator };
	ELogicimAction			aeExit[] = { LA_Separator, LA_Exit };
	Ptr<CMenuContainer>		pMenu;

	pMenu = CreateMenu(aeProject, sizeof(aeProject) / sizeof(ELogicimAction));
	mpRecentMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	pMenu->AddItem("Recent Projects", NULL, mpRecentMenu);
	pMenu->AddItem(mpCommands->Find("File.ClearRecentProjects"));
	AddMenuActions(pMenu, aeImport, sizeof(aeImport) / sizeof(ELogicimAction));
	pMenu->AddItem(mpCommands->Find("File.Settings"));
	AddMenuActions(pMenu, aeExit, sizeof(aeExit) / sizeof(ELogicimAction));
	ShowRecentMenu();
	return pMenu;
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's menu bar is only a placeholder; these menus hold its actions.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimWindowContent::CreateMenuBar(void)
{
	ELogicimAction	aeEdit[] = { LA_Undo, LA_Redo, LA_Separator, LA_Cut, LA_Copy, LA_Paste, LA_Duplicate, LA_Delete, LA_Separator, LA_RotateLeft, LA_RotateRight, LA_FlipHorizontally, LA_FlipVertically, LA_Separator, LA_EditProperties };
	ELogicimAction	aeSimulation[] = { LA_NewSimulation, LA_Separator, LA_ResetSimulation, LA_RunSimulation, LA_PauseSimulation, LA_Separator, LA_DefaultSpeed, LA_IncreaseSpeed, LA_DecreaseSpeed };
	ELogicimAction	aePlace[] = { LA_AddComponent, LA_AddTrace, LA_AddSplitter, LA_AddTunnel, LA_AddConnection, LA_AddPower, LA_AddLabel, LA_AddSubcircuit, LA_Separator, LA_NewSubcircuit };
	Ptr<CMenuContainer>	pWindowMenu;
	Ptr<CMenuContainer>	pSimulationMenu;

	mpMenuBar = OMalloc<CMenuBar>(mpWindow, mpTheme);
	mpMenuBar->AddMenu("File", CreateFileMenu());
	mpMenuBar->AddMenu("Edit", CreateMenu(aeEdit, sizeof(aeEdit) / sizeof(ELogicimAction)));
	mpMenuBar->AddMenu("View", CreateViewMenu());
	pSimulationMenu = CreateMenu(aeSimulation, sizeof(aeSimulation) / sizeof(ELogicimAction));
	pSimulationMenu->AddSeparator();
	pSimulationMenu->AddItem(mpCommands->Find("Simulation.Power"));
	pSimulationMenu->AddSeparator();
	pSimulationMenu->AddItem(mpCommands->Find("Simulation.PowerUpZero"));
	pSimulationMenu->AddItem(mpCommands->Find("Simulation.PowerUpRandom"));
	pSimulationMenu->AddItem(mpCommands->Find("Simulation.PowerUpHalfSupply"));
	pSimulationMenu->AddItem(mpCommands->Find("Simulation.NextPowerUpSeed"));
	mpMenuBar->AddMenu("Simulation", pSimulationMenu);
	mpMenuBar->AddMenu("Place", CreateMenu(aePlace, sizeof(aePlace) / sizeof(ELogicimAction)));
	pWindowMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	pWindowMenu->AddItem(mpCommands->Find("Window.NewTab"));
	pWindowMenu->AddItem(mpCommands->Find("Window.ResetLayout"));
	mpMenuBar->AddMenu("Window", pWindowMenu);
	mpMenuBar->SetAlignment(CSH_Stretch, CSV_Stretch);
	return mpMenuBar;
}


//////////////////////////////////////////////////////////////////////////
//
// A line iWidth by iHeight, -1 to stretch along an axis.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CCanvas> CLogicimWindowContent::CreateLine(int32 iWidth, int32 iHeight, ARGB32 sColour)
{
	Ptr<CCanvas>	pLine;

	pLine = OMalloc<CCanvas>(mpWindow, OMalloc<CClearCanvasDraw>(sColour));
	pLine->SetDesiredSize(iWidth, iHeight);
	pLine->SetAlignment(iWidth < 0 ? CSH_Stretch : CSH_Center, iHeight < 0 ? CSV_Stretch : CSV_Center);
	return pLine;
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's toolbar: new, load and save, the simulation, then rotate and
// flip.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CToolbar> CLogicimWindowContent::CreateToolbar(void)
{
	ELogicimAction	aeActions[] = { LA_NewProject, LA_LoadProject, LA_SaveProject, LA_Separator,
									LA_NewSimulation, LA_ResetSimulation, LA_RunSimulation, LA_PauseSimulation, LA_DefaultSpeed, LA_IncreaseSpeed, LA_DecreaseSpeed, LA_Separator,
									LA_RotateLeft, LA_RotateRight, LA_FlipHorizontally, LA_FlipVertically };

	return CreateToolStrip(aeActions, sizeof(aeActions) / sizeof(ELogicimAction), false);
}


//////////////////////////////////////////////////////////////////////////
//
// A toolbar of the actions' buttons, or a column of them like IntelliJ's
// tool window stripes (Logicim's display and creation panels).
//
//////////////////////////////////////////////////////////////////////////
Ptr<CToolbar> CLogicimWindowContent::CreateToolStrip(ELogicimAction* aeActions, int iNumActions, bool bVertical)
{
	Ptr<CToolbar>	pStrip;
	int				i;

	pStrip = OMalloc<CToolbar>(mpWindow, mpTheme, bVertical);
	for (i = 0; i < iNumActions; i++)
	{
		if (aeActions[i] == LA_Separator)
		{
			pStrip->AddSeparator();
		}
		else
		{
			pStrip->AddCommand(mpCommands->Find(gasLogicimActions[aeActions[i]].szCommand));
		}
	}
	return pStrip;
}


//////////////////////////////////////////////////////////////////////////
//
// A tree, its rows highlighted as a list's; a double click (or enter)
// activates a node.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CTreeContainer> CLogicimWindowContent::CreateTree(void)
{
	Ptr<CTreeContainer>		pTree;
	SInsets					sInsets;
	int						aiInsets[4] = { 0, 6, 0, 6 };

	sInsets.Init(aiInsets);
	pTree = OMalloc<CTreeContainer>(mpWindow, mpTheme);
	pTree->SetHighlightDraws(OMalloc<CRoundedBoxCanvasDraw>(mpTheme->GetColour(TC_ListSelected), 4, &sInsets), OMalloc<CRoundedBoxCanvasDraw>(mpTheme->GetColour(TC_ListHover), 4, &sInsets));
	pTree->SetAlignment(CSH_Stretch, CSV_Top);
	pTree->SetDoubleClickActivates(true);
	return pTree;
}


//////////////////////////////////////////////////////////////////////////
//
// A tool window's content: its list, scrolled.  Its dock pane shows its
// title.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimWindowContent::CreateToolWindow(Ptr<CListContainer> pList)
{
	Ptr<CScrollPane>		pScrollPane;

	pScrollPane = OMalloc<CScrollPane>(mpWindow, mpTheme);
	pScrollPane->SetPolicy(SBO_Horizontal, SPP_Never);
	pScrollPane->SetContent(pList);
	pScrollPane->SetAlignment(CSH_Stretch, CSV_Stretch);
	return pScrollPane;
}


//////////////////////////////////////////////////////////////////////////
//
// The view stripe, the dock area and the add stripe.  The circuit is the
// document in the dock's well, with the subcircuits' tool window on its
// left and the simulations' below it; they can be moved, floated, auto
// hidden and hidden as Visual Studio's (see CDockManager), and the layout
// is kept for next time (see SaveLayout).
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimWindowContent::CreateEditor(void)
{
	SDockPlace				sPlace;
	ELogicimAction			aeDisplay[] = { LA_TogglePointView, LA_Separator, LA_ZoomIn, LA_ZoomOut, LA_ResetZoom, LA_ZoomFitAll, LA_ZoomFitSelection, LA_Separator, LA_ViewSimulationTree };
	ELogicimAction			aeCreation[] = { LA_AddComponent, LA_AddTrace, LA_AddSplitter, LA_AddTunnel, LA_AddConnection, LA_AddPower, LA_AddLabel, LA_AddSubcircuit, LA_Separator, LA_NewSubcircuit };
	Ptr<CGridContainer>		pEditor;
	Ptr<CGridContainer>		pCentre;
	Ptr<CComponent>			pSubcircuits;
	Ptr<CComponent>			pSimulation;

	pEditor = OMalloc<CGridContainer>(mpWindow);
	pEditor->SetAlignment(CSH_Stretch, CSV_Stretch);

	mpSubcircuitTree = CreateTree();
	mpSubcircuitTree->GetSelectionEvent()->Add(this, &CLogicimWindowContent::SubcircuitSelected);
	pSubcircuits = CreateToolWindow(mpSubcircuitTree);

	mpCanvasMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	mpCanvasMenu->GetItemClickEvent()->Add(this, &CLogicimWindowContent::CanvasMenuItemClicked);
	mpSimulationTree = CreateTree();
	mpSimulationTree->GetActivationEvent()->Add(this, &CLogicimWindowContent::SimulationNodeActivated);
	pSimulation = CreateToolWindow(mpSimulationTree);

	//The trees' menu is opened on a node, which their own action finds before the menu's opens.
	mpTreeMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	mpTreeMenu->AddItem("Open in New Tab");
	mpTreeMenu->GetItemClickEvent()->Add(this, &CLogicimWindowContent::TreeMenuItemClicked);
	mpSimulationTree->AddInputAction(CIC_ContextMenu, this, &CLogicimWindowContent::SimulationTreeMenuAction);
	mpSimulationTree->SetContextMenu(mpTreeMenu);
	mpSubcircuitTree->AddInputAction(CIC_ContextMenu, this, &CLogicimWindowContent::SubcircuitTreeMenuAction);
	mpSubcircuitTree->SetContextMenu(mpTreeMenu);

	mpDock = OMalloc<CDockManager>(mpWindow, mpTheme, Ptr<CFont>());
	mpDock->SetListener(this);
	CreateTab(NULL);
	MakeTabActive(0);
	sPlace.BesideWell(DS_Left, 200);
	mpDock->AddPane(LOGICIM_SUBCIRCUITS_PANE, "Subcircuits", Ptr<CImageCel>(), pSubcircuits, DPK_Tool, &sPlace);
	sPlace.BesideWell(DS_Bottom, 150);
	mpDock->AddPane(LOGICIM_SIMULATION_PANE, "Simulation", Ptr<CImageCel>(), pSimulation, DPK_Tool, &sPlace);

	pCentre = OMalloc<CGridContainer>(mpWindow);
	pCentre->SetAlignment(CSH_Stretch, CSV_Stretch);
	pCentre->AddComponent(mpDock->GetArea(), 0, 0, 1, 1, 1, 1);

	pEditor->AddComponent(CreateToolStrip(aeDisplay, sizeof(aeDisplay) / sizeof(ELogicimAction), true), 0, 0, 1, 1, 0, 1);
	pEditor->AddComponent(CreateLine(1, -1, mpTheme->GetColour(TC_Editor)), 1, 0, 1, 1, 0, 1);
	pEditor->AddComponent(pCentre, 2, 0, 1, 1, 1, 1);
	pEditor->AddComponent(CreateLine(1, -1, mpTheme->GetColour(TC_Editor)), 3, 0, 1, 1, 0, 1);
	pEditor->AddComponent(CreateToolStrip(aeCreation, sizeof(aeCreation) / sizeof(ELogicimAction), true), 4, 0, 1, 1, 0, 1);
	return pEditor;
}


//////////////////////////////////////////////////////////////////////////
//
// View: Logicim's view actions, then an item for each tool window, which
// shows it wherever it was.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CMenuContainer> CLogicimWindowContent::CreateViewMenu(void)
{
	ELogicimAction			aeView[] = { LA_TogglePointView, LA_Separator, LA_ZoomIn, LA_ZoomOut, LA_ResetZoom, LA_SelectZoom, LA_ZoomFitAll, LA_ZoomFitSelection };
	Ptr<CMenuContainer>		pMenu;

	pMenu = CreateMenu(aeView, sizeof(aeView) / sizeof(ELogicimAction));
	pMenu->AddSeparator();
	pMenu->AddItem(mpCommands->Find("View.Subcircuits"));
	pMenu->AddItem(mpCommands->Find("View.SimulationTree"));
	pMenu->AddSeparator();
	pMenu->AddItem(mpCommands->Find("View.Back"));
	pMenu->AddItem(mpCommands->Find("View.Forward"));
	pMenu->AddItem(mpCommands->Find("View.GoTo"));
	return pMenu;
}


//////////////////////////////////////////////////////////////////////////
//
// View, Subcircuits and Simulation Tree: the tool window shown, wherever it
// was, and made active.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowToolWindow(const char* szPaneID)
{
	Ptr<CDockPane>	pPane;

	pPane = mpDock->GetPane(szPaneID);
	if (pPane.IsNotNull())
	{
		mpDock->Activate(pPane);
	}
}


void CLogicimWindowContent::ShowSubcircuits(void) { ShowToolWindow(LOGICIM_SUBCIRCUITS_PANE); }
void CLogicimWindowContent::ShowSimulationTree(void) { ShowToolWindow(LOGICIM_SIMULATION_PANE); }


//////////////////////////////////////////////////////////////////////////
//
// Logicim's circuit information: the subcircuit, the simulation and the
// view path on the left, what undo has to say in the space left over
// (see PressUndo), and the pointer and zoom on the right.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CStatusBar> CLogicimWindowContent::CreateStatusBar(void)
{
	Ptr<CStatusBar>	pBar;

	pBar = OMalloc<CStatusBar>(mpWindow, mpTheme, mpDimFont);
	mpSubcircuitInfo = AddStatusField(pBar, 190);
	mpSimulationInfo = AddStatusField(pBar, 250);
	mpViewPathInfo = AddStatusField(pBar, 250);
	mpMessageInfo = pBar->AddMessageField();  //The space left over, before the pointer and zoom.
	mpMouseXInfo = AddStatusField(pBar, 56);
	mpMouseYInfo = AddStatusField(pBar, 56);
	mpZoomInfo = AddStatusField(pBar, 44);
	return pBar;
}


//////////////////////////////////////////////////////////////////////////
//
// A field iWidth wide followed by a separator.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CLabel> CLogicimWindowContent::AddStatusField(Ptr<CStatusBar> pBar, int32 iWidth)
{
	Ptr<CLabel>	pField;

	pField = pBar->AddField(iWidth);
	pBar->AddSeparator();
	return pField;
}


//////////////////////////////////////////////////////////////////////////
//
// Rasterises the glyphs from the TrueType font in white into an image in
// the window's colour order and makes a font from them.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CFont> CLogicimWindowContent::GenerateFont(const char* szFontFilename, const char* szName, float32 fPixelSize, EColourOrder eOrder)
{
	CTrueTypeFont			cTrueTypeFont;
	CTrueTypeFontFactory	cFactory;
	CImageColourRGB			cColour;
	CChars					szCharacters;
	Ptr<CFontDescription>	pDescription;
	Ptr<CImage>				pImage;
	Ptr<CFont>				pFont;

	cTrueTypeFont.Init();
	if (!cTrueTypeFont.Load((char*)szFontFilename))
	{
		cTrueTypeFont.Kill();
		gcLogger.Error2(__METHOD__, " Could not load font [", szFontFilename, "].", NULL);
		return NULL;
	}

	cColour.Init(1.0f, 1.0f, 1.0f);
	szCharacters.Init(LOGICIM_FONT_CHARACTERS);
	cFactory.Init(&cTrueTypeFont, fPixelSize);
	pDescription = cFactory.Describe((char*)szName, szCharacters.Text());
	pImage = cFactory.Rasterise(pDescription, &cColour);
	if (pImage.IsNotNull())
	{
		if (pImage->GetColourOrder() != eOrder)
		{
			pImage = CImageColourConverter::ConvertColour(pImage, eOrder);
		}
		pFont = OMalloc<CFont>(pDescription, pImage);
	}
	cFactory.Kill();
	szCharacters.Kill();
	cTrueTypeFont.Kill();

	if (pFont.IsNull())
	{
		gcLogger.Error2(__METHOD__, " Could not make a font from [", szFontFilename, "].", NULL);
	}
	return pFont;
}
