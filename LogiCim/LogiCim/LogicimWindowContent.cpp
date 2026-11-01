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
#include "BaseLib/Logger.h"
#include "BaseLib/FileUtil.h"
#include "StandardLib/Objects.h"
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


//IntelliJ IDEA's new UI dark theme.  The dim text is LogiCim's own, for the status bar and dividers.
#define LOGICIM_DIM_TEXT			Set32BitColour((uint8)0x9D, (uint8)0xA0, (uint8)0xA8)
#define LOGICIM_DEFAULT_SPEED	0.25  //Simulated nanoseconds per millisecond.

#define LOGICIM_ROW_HEIGHT		24
#define LOGICIM_DIALOG_WIDTH		440


//The glyphs generated: printable ASCII.
#define LOGICIM_FONT_CHARACTERS	" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"


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


struct SLogicimActionDesc
{
	const char*		szName;
	const char*		szIcon;  //The Logicim icon's name, without _24.png.  NULL for none.
};


//In ELogicimAction's order.
static SLogicimActionDesc gasLogicimActions[NUM_LOGICIM_ACTIONS] =
{
	{ "Load Project", "directory_open" },
	{ "Save Project", "save" },
	{ "New Simulation...", "new_simulation" },
	{ "Reset Simulation", "sim_reset" },
	{ "Run Simulation", "sim_run" },
	{ "Pause Simulation", "sim_stop" },
	{ "Default Speed", "sim_default" },
	{ "Increase Speed", "sim_faster" },
	{ "Decrease Speed", "sim_slower" },
	{ "Rotate Left", "rotate_ccw" },
	{ "Rotate Right", "rotate_cw" },
	{ "Flip Horizontally", "mirror_h" },
	{ "Flip Vertically", "mirror_v" },
	{ "Toggle Point View", "grid" },
	{ "Zoom In", "zoom_in" },
	{ "Zoom Out", "zoom_out" },
	{ "Reset Zoom", "zoom_center_on_screen" },
	{ "Select Zoom...", NULL },
	{ "Zoom to Fit All", "zoom_fit_in_page" },
	{ "Zoom to Fit Selection", "zoom_selection" },
	{ "Simulation Tree...", "simulation_tree" },
	{ "Add Component...", "add_component" },
	{ "Add Trace", "add_line" },
	{ "Add Splitter", "add_line2bus" },
	{ "Add Tunnel", "add_glabel" },
	{ "Add Connection", "add_junction" },
	{ "Add Power", "add_power" },
	{ "Add Label", "add_label" },
	{ "Add Subcircuit", "module" },
	{ "New Subcircuit...", "new_footprint" },
	{ "Properties...", "edit" },
	{ "Undo", "undo" },
	{ "Redo", "redo" },
	{ "Cut", "cut" },
	{ "Copy", "copy" },
	{ "Paste", NULL },
	{ "Duplicate", "duplicate" },
	{ "Delete", "trash" },
	{ "Import KiCad Schematic...", "import" },
	{ "Exit", "exit" },
};


//A separator in a menu or a toolbar.
#define LA_Separator	NUM_LOGICIM_ACTIONS


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::Init(Ptr<CWindow> pWindow, SLogicimWindowData* psData, const char* szFontFilename, const char* szIconDirectory, const char* szPropagationTimes, const char* szCircuitFontDirectory)
{
	CColourFormatHelper		cFormat;
	Ptr<CGridContainer>		pGrid;
	Ptr<CFont>				pWhite;

	mpWindow = pWindow;
	mpsData = psData;
	mpsData->pcContent = this;
	mpsData->eDialog = LDG_None;
	miSimulationCount = 0;
	mbRunning = true;
	mfNanosecondsPerMillisecond = LOGICIM_DEFAULT_SPEED;
	maButtons.Init();
	maiButtonActions.Init();
	maMenuItems.Init();
	maiMenuItemActions.Init();
	mapIcons.Init();
	maSubcircuitLabels.Init();
	maSimulationLabels.Init();
	mcButtonListener.Init(this);
	mcMenuListener.Init(this);
	mcListListener.Init(this);

	//Before anything that can fail; Kill always kills it.
	pWindow->GetColourFormat(&cFormat);
	mszProjectFilename.Init("Project.xml");
	mszKicadFilename.Init("Schematic.kicad_sch");
	mszChooserSelected.Init();
	if (!mcCircuit.Init(szPropagationTimes, szCircuitFontDirectory, cFormat.meOrder))
	{
		return false;
	}

	//The font is white and coloured as it is drawn.
	pWhite = GenerateFont(szFontFilename, "Inter 13px", 13.0f, cFormat.meOrder);
	if (pWhite.IsNull())
	{
		return false;
	}
	//Nothing in the window holds the font colours or the theme, the widgets hold their parts, so they are rooted.
	mpFontColours = OMalloc<CFontColours>(pWhite);
	ORoot()->Add(mpFontColours);
	mpFont = mpFontColours->GetFont(JEWEL_TEXT);
	mpDimFont = mpFontColours->GetFont(LOGICIM_DIM_TEXT);

	if (!LoadIcons(szIconDirectory))
	{
		return false;
	}
	CreateLooks();

	pGrid = OMalloc<CGridContainer>(pWindow);
	pWindow->SetContainer(pGrid);
	pGrid->AddComponent(CreateMenuBar(), 0, 0, 1, 1, 1, 0);
	pGrid->AddComponent(CreateToolbar(), 0, 1, 1, 1, 1, 0);
	pGrid->AddComponent(CreateLine(-1, 1, JEWEL_EDITOR), 0, 2, 1, 1, 1, 0);
	pGrid->AddComponent(CreateEditor(), 0, 3, 1, 1, 1, 1);
	pGrid->AddComponent(CreateLine(-1, 1, JEWEL_EDITOR), 0, 4, 1, 1, 1, 0);
	pGrid->AddComponent(CreateStatusBar(), 0, 5, 1, 1, 1, 0);

	//Like Logicim the project starts with a top level subcircuit and a simulation of it.
	AddListItem(mpSubcircuits, &maSubcircuitLabels, "Top");
	mpSubcircuits->Select(0);
	AddListItem(mpSimulations, &maSimulationLabels, "Top Top 1");
	miSimulationCount = 1;
	mpSimulations->Select(0);
	UpdateInfo();
	Zoom(1.0f);

	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::Kill(void)
{
	size	i;

	for (i = 0; i < maButtons.Size(); i++)
	{
		maButtons.Get(i)->RemoveListener(&mcButtonListener);
	}
	if (mpMenuBar.IsNotNull())
	{
		mpMenuBar->RemoveListener(&mcMenuListener);
	}
	if (mpSubcircuits.IsNotNull())
	{
		mpSubcircuits->RemoveListener(&mcListListener);
		mpSimulations->RemoveListener(&mcListListener);
	}
	if (mpsData)
	{
		mpsData->pcContent = NULL;
	}

	maButtons.Kill();
	maiButtonActions.Kill();
	maMenuItems.Kill();
	maiMenuItemActions.Kill();
	mapIcons.Kill();
	mpFileIcon = NULL;
	maSubcircuitLabels.Kill();
	maSimulationLabels.Kill();
	msDialogLook.Kill();
	if (mpTheme.IsNotNull())
	{
		ORoot()->Remove(mpTheme);
	}
	mpTheme = NULL;

	mpMenuBar = NULL;
	mpEditMenu = NULL;
	mpSubcircuits = NULL;
	mpSimulations = NULL;
	mpCircuit = NULL;
	mpSubcircuitInfo = NULL;
	mpSimulationInfo = NULL;
	mpViewPathInfo = NULL;
	mpMouseXInfo = NULL;
	mpMouseYInfo = NULL;
	mpZoomInfo = NULL;
	mpFont = NULL;
	mpDimFont = NULL;
	if (mpFontColours.IsNotNull())
	{
		ORoot()->Remove(mpFontColours);
	}
	mpFontColours = NULL;
	mpWindow = NULL;
	mcCircuit.Kill();
	mszKicadFilename.Kill();
	mszProjectFilename.Kill();
	mszChooserSelected.Kill();
}


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
	mpTheme = CJewelTheme::CreateDarkTheme(mpWindow, mpFont);
	ORoot()->Add(mpTheme);
	msDialogLook.pTheme = mpTheme;
	msDialogLook.pDimFont = mpDimFont;
	msDialogLook.sBackground = JEWEL_PANEL;
	msDialogLook.sDivider = JEWEL_SEPARATOR;
}


//////////////////////////////////////////////////////////////////////////
//
// The actions are the menu's items in order, LA_Separator between groups.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CMenuContainer> CLogicimWindowContent::CreateMenu(ELogicimAction* aeActions, int iNumActions)
{
	Ptr<CMenuContainer>		pMenu;
	Ptr<CMenuItem>			pItem;
	int						i;

	pMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	for (i = 0; i < iNumActions; i++)
	{
		if (aeActions[i] == LA_Separator)
		{
			pMenu->AddSeparator();
		}
		else
		{
			pItem = pMenu->AddItem(gasLogicimActions[aeActions[i]].szName, mapIcons.Get(aeActions[i]));
			maMenuItems.Add(pItem);
			maiMenuItemActions.Add(aeActions[i]);
		}
	}
	return pMenu;
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's menu bar is only a placeholder; these menus hold its actions.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimWindowContent::CreateMenuBar(void)
{
	ELogicimAction	aeFile[] = { LA_LoadProject, LA_SaveProject, LA_Separator, LA_ImportKicadSchematic, LA_Separator, LA_Exit };
	ELogicimAction	aeEdit[] = { LA_Undo, LA_Redo, LA_Separator, LA_Cut, LA_Copy, LA_Paste, LA_Duplicate, LA_Delete, LA_Separator, LA_RotateLeft, LA_RotateRight, LA_FlipHorizontally, LA_FlipVertically, LA_Separator, LA_EditProperties };
	ELogicimAction	aeView[] = { LA_TogglePointView, LA_Separator, LA_ZoomIn, LA_ZoomOut, LA_ResetZoom, LA_SelectZoom, LA_ZoomFitAll, LA_ZoomFitSelection, LA_Separator, LA_ViewSimulationTree };
	ELogicimAction	aeSimulation[] = { LA_NewSimulation, LA_Separator, LA_ResetSimulation, LA_RunSimulation, LA_PauseSimulation, LA_Separator, LA_DefaultSpeed, LA_IncreaseSpeed, LA_DecreaseSpeed };
	ELogicimAction	aePlace[] = { LA_AddComponent, LA_AddTrace, LA_AddSplitter, LA_AddTunnel, LA_AddConnection, LA_AddPower, LA_AddLabel, LA_AddSubcircuit, LA_Separator, LA_NewSubcircuit };

	mpMenuBar = OMalloc<CMenuBar>(mpWindow, mpTheme);
	mpMenuBar->AddMenu("File", CreateMenu(aeFile, sizeof(aeFile) / sizeof(ELogicimAction)));
	mpEditMenu = CreateMenu(aeEdit, sizeof(aeEdit) / sizeof(ELogicimAction));
	mpMenuBar->AddMenu("Edit", mpEditMenu);
	mpMenuBar->AddMenu("View", CreateMenu(aeView, sizeof(aeView) / sizeof(ELogicimAction)));
	mpMenuBar->AddMenu("Simulation", CreateMenu(aeSimulation, sizeof(aeSimulation) / sizeof(ELogicimAction)));
	mpMenuBar->AddMenu("Place", CreateMenu(aePlace, sizeof(aePlace) / sizeof(ELogicimAction)));
	mpMenuBar->AddListener(&mcMenuListener);
	mpMenuBar->SetAlignment(CSH_Stretch, CSV_Stretch);
	return mpMenuBar;
}


//////////////////////////////////////////////////////////////////////////
//
// A flat button showing the action's icon, highlighted under the pointer.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CButton> CLogicimWindowContent::CreateToolButton(ELogicimAction eAction)
{
	Ptr<CButton>	pButton;

	pButton = OMalloc<CButton>(mpWindow, mpTheme, BL_Flat, mapIcons.Get(eAction), (const char*)NULL);
	pButton->SetPadding(0, 0);
	pButton->SetShowsPushed(true);
	pButton->AddListener(&mcButtonListener);
	pButton->SetAlignment(CSH_Center, CSV_Center);
	maButtons.Add(pButton);
	maiButtonActions.Add(eAction);
	return pButton;
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
// Logicim's toolbar: load and save, the simulation, then rotate and flip.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimWindowContent::CreateToolbar(void)
{
	ELogicimAction			aeActions[] = { LA_LoadProject, LA_SaveProject, LA_Separator,
											LA_NewSimulation, LA_ResetSimulation, LA_RunSimulation, LA_PauseSimulation, LA_DefaultSpeed, LA_IncreaseSpeed, LA_DecreaseSpeed, LA_Separator,
											LA_RotateLeft, LA_RotateRight, LA_FlipHorizontally, LA_FlipVertically };
	Ptr<CGridContainer>		pBar;
	Ptr<CFillContainer>		pFiller;
	Ptr<CCanvas>			pLine;
	int						i;
	int						iColumn;

	//A grid only has a size if it is given one.
	pBar = OMalloc<CGridContainer>(mpWindow);
	pBar->SetAlignment(CSH_Stretch, CSV_Stretch);
	pBar->SetDesiredSize(-1, 40);

	//A little space before the first button.
	pFiller = OMalloc<CFillContainer>(mpWindow);
	pFiller->SetDesiredSize(4, 1);
	pBar->AddComponent(pFiller, 0, 0);
	iColumn = 1;
	for (i = 0; i < (int)(sizeof(aeActions) / sizeof(ELogicimAction)); i++)
	{
		if (aeActions[i] == LA_Separator)
		{
			pLine = CreateLine(1, 22, JEWEL_SEPARATOR);
			pFiller = OMalloc<CFillContainer>(mpWindow);
			pFiller->SetDesiredSize(13, 22);
			pFiller->AddComponent(pLine);
			pBar->AddComponent(pFiller, iColumn, 0);
		}
		else
		{
			pBar->AddComponent(CreateToolButton(aeActions[i]), iColumn, 0);
		}
		iColumn++;
	}

	pFiller = OMalloc<CFillContainer>(mpWindow);
	pBar->AddComponent(pFiller, iColumn, 0, 1, 1, 1, 0);
	return pBar;
}


//////////////////////////////////////////////////////////////////////////
//
// A column of tool buttons like IntelliJ's tool window stripes; Logicim's
// display and creation panels.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimWindowContent::CreateStripe(ELogicimAction* aeActions, int iNumActions)
{
	Ptr<CGridContainer>		pStripe;
	Ptr<CFillContainer>		pFiller;
	Ptr<CCanvas>			pLine;
	int						i;
	int						iRow;

	pStripe = OMalloc<CGridContainer>(mpWindow);
	pStripe->SetAlignment(CSH_Stretch, CSV_Stretch);
	pStripe->SetDesiredSize(40, -1);

	pFiller = OMalloc<CFillContainer>(mpWindow);
	pFiller->SetDesiredSize(1, 4);
	pStripe->AddComponent(pFiller, 0, 0);
	iRow = 1;
	for (i = 0; i < iNumActions; i++)
	{
		if (aeActions[i] == LA_Separator)
		{
			pLine = CreateLine(22, 1, JEWEL_SEPARATOR);
			pFiller = OMalloc<CFillContainer>(mpWindow);
			pFiller->SetDesiredSize(22, 13);
			pFiller->AddComponent(pLine);
			pStripe->AddComponent(pFiller, 0, iRow);
		}
		else
		{
			pStripe->AddComponent(CreateToolButton(aeActions[i]), 0, iRow);
		}
		iRow++;
	}

	pFiller = OMalloc<CFillContainer>(mpWindow);
	pStripe->AddComponent(pFiller, 0, iRow, 1, 1, 0, 1);
	return pStripe;
}


//////////////////////////////////////////////////////////////////////////
//
// A list whose rows are highlighted with rounded blocks, like IntelliJ's
// trees.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CListContainer> CLogicimWindowContent::CreateList(void)
{
	Ptr<CListContainer>		pList;
	SInsets					sInsets;
	int						aiInsets[4] = { 0, 6, 0, 6 };

	sInsets.Init(aiInsets);
	pList = OMalloc<CListContainer>(mpWindow, mpTheme);
	pList->SetHighlightDraws(OMalloc<CRoundedBoxCanvasDraw>(mpTheme->GetListSelectedColour(), 4, &sInsets), OMalloc<CRoundedBoxCanvasDraw>(mpTheme->GetListHoverColour(), 4, &sInsets));
	pList->SetAlignment(CSH_Stretch, CSV_Top);
	pList->AddListener(&mcListListener);
	return pList;
}


//////////////////////////////////////////////////////////////////////////
//
// A row is a gap and the text, a row high.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::AddListItem(Ptr<CListContainer> pList, CArray<CLabel>* paLabels, const char* szText)
{
	Ptr<CFillContainer>		pGap;
	Ptr<CLabel>				pLabel;
	int32					iRow;

	iRow = pList->AddRow(true);
	pGap = OMalloc<CFillContainer>(mpWindow);
	pGap->SetDesiredSize(16, LOGICIM_ROW_HEIGHT);
	pLabel = OMalloc<CLabel>(mpWindow, mpFont, szText);
	pLabel->SetAlignment(CSH_Left, CSV_Center);
	pList->AddToRow(iRow, pGap, 0);
	pList->AddToRow(iRow, pLabel, 1, 1, 1);
	paLabels->Add(pLabel);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
char* CLogicimWindowContent::GetListItemText(Ptr<CListContainer> pList, CArray<CLabel>* paLabels)
{
	int32	iRow;

	iRow = pList->GetSelected();
	if ((iRow < 0) || ((size)iRow >= paLabels->Size()))
	{
		return "";
	}
	return paLabels->Get(iRow)->GetText();
}


//////////////////////////////////////////////////////////////////////////
//
// A tool window: its title at the top then its list, scrolled.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimWindowContent::CreateToolWindow(const char* szTitle, Ptr<CListContainer> pList)
{
	Ptr<CGridContainer>		pToolWindow;
	Ptr<CFillContainer>		pGap;
	Ptr<CLabel>				pTitle;
	Ptr<CScrollPane>		pScrollPane;

	pToolWindow = OMalloc<CGridContainer>(mpWindow);
	pToolWindow->SetAlignment(CSH_Stretch, CSV_Stretch);

	pGap = OMalloc<CFillContainer>(mpWindow);
	pGap->SetDesiredSize(12, 32);
	pTitle = OMalloc<CLabel>(mpWindow, mpFont, szTitle);
	pTitle->SetAlignment(CSH_Left, CSV_Center);
	pToolWindow->AddComponent(pGap, 0, 0);
	pToolWindow->AddComponent(pTitle, 1, 0, 1, 1, 1, 0);

	pScrollPane = OMalloc<CScrollPane>(mpWindow, mpTheme);
	pScrollPane->SetPolicy(SBO_Horizontal, SPP_Never);
	pScrollPane->SetContent(pList);
	pScrollPane->SetAlignment(CSH_Stretch, CSV_Stretch);
	pToolWindow->AddComponent(pScrollPane, 0, 1, 2, 1, 1, 1);
	return pToolWindow;
}


//////////////////////////////////////////////////////////////////////////
//
// The view stripe, the subcircuits' tool window, the circuit with the
// simulations' tool window below it, and the add stripe.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimWindowContent::CreateEditor(void)
{
	ELogicimAction			aeDisplay[] = { LA_TogglePointView, LA_Separator, LA_ZoomIn, LA_ZoomOut, LA_ResetZoom, LA_ZoomFitAll, LA_ZoomFitSelection, LA_Separator, LA_ViewSimulationTree };
	ELogicimAction			aeCreation[] = { LA_AddComponent, LA_AddTrace, LA_AddSplitter, LA_AddTunnel, LA_AddConnection, LA_AddPower, LA_AddLabel, LA_AddSubcircuit, LA_Separator, LA_NewSubcircuit };
	Ptr<CGridContainer>		pEditor;
	Ptr<CGridContainer>		pCentre;
	Ptr<CComponent>			pSubcircuits;
	Ptr<CComponent>			pSimulations;

	pEditor = OMalloc<CGridContainer>(mpWindow);
	pEditor->SetAlignment(CSH_Stretch, CSV_Stretch);

	mpSubcircuits = CreateList();
	pSubcircuits = CreateToolWindow("Subcircuits", mpSubcircuits);
	pSubcircuits->SetDesiredSize(200, -1);

	mpCircuit = OMalloc<CCircuitCanvas>(mpWindow, OMalloc<CCircuitCanvasDraw>(&mcCircuit), this, mcCircuit.GetEditor());
	mpCircuit->SetAlignment(CSH_Stretch, CSV_Stretch);
	mpSimulations = CreateList();
	pSimulations = CreateToolWindow("Simulations", mpSimulations);
	pSimulations->SetDesiredSize(-1, 150);

	pCentre = OMalloc<CGridContainer>(mpWindow);
	pCentre->SetAlignment(CSH_Stretch, CSV_Stretch);
	pCentre->AddComponent(mpCircuit, 0, 0, 1, 1, 1, 1);
	pCentre->AddComponent(CreateLine(-1, 1, JEWEL_EDITOR), 0, 1, 1, 1, 1, 0);
	pCentre->AddComponent(pSimulations, 0, 2, 1, 1, 1, 0);

	pEditor->AddComponent(CreateStripe(aeDisplay, sizeof(aeDisplay) / sizeof(ELogicimAction)), 0, 0, 1, 1, 0, 1);
	pEditor->AddComponent(CreateLine(1, -1, JEWEL_EDITOR), 1, 0, 1, 1, 0, 1);
	pEditor->AddComponent(pSubcircuits, 2, 0, 1, 1, 0, 1);
	pEditor->AddComponent(CreateLine(1, -1, JEWEL_EDITOR), 3, 0, 1, 1, 0, 1);
	pEditor->AddComponent(pCentre, 4, 0, 1, 1, 1, 1);
	pEditor->AddComponent(CreateLine(1, -1, JEWEL_EDITOR), 5, 0, 1, 1, 0, 1);
	pEditor->AddComponent(CreateStripe(aeCreation, sizeof(aeCreation) / sizeof(ELogicimAction)), 6, 0, 1, 1, 0, 1);
	return pEditor;
}


//////////////////////////////////////////////////////////////////////////
//
// A label iWidth wide followed by a separator.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CLabel> CLogicimWindowContent::AddStatusLabel(Ptr<CGridContainer> pBar, int* piColumn, int32 iWidth)
{
	Ptr<CLabel>			pLabel;
	Ptr<CFillContainer>	pGap;

	pGap = OMalloc<CFillContainer>(mpWindow);
	pGap->SetDesiredSize(10, 26);
	pBar->AddComponent(pGap, *piColumn, 0);
	(*piColumn)++;

	pLabel = OMalloc<CLabel>(mpWindow, mpDimFont, "");
	pLabel->SetDesiredSize(iWidth, -1);
	pLabel->SetAlignment(CSH_Left, CSV_Center);
	pBar->AddComponent(pLabel, *piColumn, 0);
	(*piColumn)++;

	pBar->AddComponent(CreateLine(1, 16, JEWEL_SEPARATOR), *piColumn, 0);
	(*piColumn)++;
	return pLabel;
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's circuit information: the subcircuit, the simulation and the
// view path on the left, the pointer and zoom on the right.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimWindowContent::CreateStatusBar(void)
{
	Ptr<CGridContainer>		pBar;
	Ptr<CFillContainer>		pFiller;
	int						iColumn;

	pBar = OMalloc<CGridContainer>(mpWindow);
	pBar->SetAlignment(CSH_Stretch, CSV_Stretch);
	pBar->SetDesiredSize(-1, 26);

	iColumn = 0;
	mpSubcircuitInfo = AddStatusLabel(pBar, &iColumn, 190);
	mpSimulationInfo = AddStatusLabel(pBar, &iColumn, 250);
	mpViewPathInfo = AddStatusLabel(pBar, &iColumn, 250);

	pFiller = OMalloc<CFillContainer>(mpWindow);
	pBar->AddComponent(pFiller, iColumn, 0, 1, 1, 1, 0);
	iColumn++;

	mpMouseXInfo = AddStatusLabel(pBar, &iColumn, 56);
	mpMouseYInfo = AddStatusLabel(pBar, &iColumn, 56);
	mpZoomInfo = AddStatusLabel(pBar, &iColumn, 44);
	return pBar;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::UpdateInfo(void)
{
	CChars	sz;

	sz.Init("Subcircuit: ");
	sz.Append(GetListItemText(mpSubcircuits, &maSubcircuitLabels));
	mpSubcircuitInfo->SetText(sz.Text());
	sz.Kill();

	sz.Init("Simulation: ");
	sz.Append(GetListItemText(mpSimulations, &maSimulationLabels));
	mpSimulationInfo->SetText(sz.Text());
	sz.Kill();

	sz.Init("View Path: ");
	sz.Append(GetListItemText(mpSubcircuits, &maSubcircuitLabels));
	if (mbRunning)
	{
		sz.Append(" (running)");
	}
	mpViewPathInfo->SetText(sz.Text());
	sz.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Appends f to one decimal place, like Logicim's "%.1f".
//
//////////////////////////////////////////////////////////////////////////
void AppendTenths(CChars* psz, float32 f)
{
	int32	iTenths;

	if (f < 0.0f)
	{
		iTenths = (int32)(-f * 10.0f + 0.5f);
		if (iTenths != 0)
		{
			psz->Append("-");
		}
	}
	else
	{
		iTenths = (int32)(f * 10.0f + 0.5f);
	}
	psz->Append(iTenths / 10);
	psz->Append(".");
	psz->Append(iTenths % 10);
}


//////////////////////////////////////////////////////////////////////////
//
// x and y are relative to the circuit's top left.  Shown in grid units,
// as Logicim does.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::PointerOverCircuit(int x, int y)
{
	CChars			sz;
	SFloat64Vec2	sScreen;
	SFloat64Vec2	sGrid;

	sScreen.Init(x, y);
	mpCircuit->GetCircuitDraw()->GetViewport()->ScreenToGrid(&sGrid, &sScreen);

	sz.Init("X ");
	AppendTenths(&sz, (float32)sGrid.x);
	mpMouseXInfo->SetText(sz.Text());
	sz.Kill();

	sz.Init("Y ");
	AppendTenths(&sz, (float32)sGrid.y);
	mpMouseYInfo->SetText(sz.Text());
	sz.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::Zoom(float32 fZoom)
{
	mpCircuit->GetCircuitDraw()->SetZoom(fZoom);
	ShowZoom();
}


//////////////////////////////////////////////////////////////////////////
//
// Zoom to Fit All fits the whole circuit on the canvas, Zoom to Fit
// Selection the selection.  Nothing to fit leaves the zoom as it is.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ZoomToFit(bool bSelection)
{
	CCircuitEditor*		pcEditor;
	SFloat64Vec2		sMinimum;
	SFloat64Vec2		sMaximum;
	bool				bHasBounds;

	pcEditor = mcCircuit.GetEditor();
	if (bSelection)
	{
		bHasBounds = pcEditor->GetSelection()->GetBoundingBox(&sMinimum, &sMaximum);
	}
	else
	{
		bHasBounds = (pcEditor->GetCircuit() != NULL) && pcEditor->GetCircuit()->GetBoundingBox(&sMinimum, &sMaximum);
	}

	if (bHasBounds)
	{
		mpCircuit->GetCircuitDraw()->ZoomToFit(sMinimum, sMaximum);
		ShowZoom();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The status bar's zoom.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowZoom(void)
{
	CChars	sz;

	sz.Init("Z ");
	AppendTenths(&sz, mpCircuit->GetCircuitDraw()->GetZoom());
	mpZoomInfo->SetText(sz.Text());
	sz.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Shows the properties of the hovered or selected component on the next
// tick, as the Properties action does.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::EditProperties(void)
{
	mpsData->eDialog = LDG_Properties;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ButtonClicked(CButton* pcButton)
{
	size	i;

	for (i = 0; i < maButtons.Size(); i++)
	{
		if (&maButtons.Get(i) == pcButton)
		{
			DoAction((ELogicimAction)maiButtonActions.GetValue(i));
			return;
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::MenuItemClicked(CMenuItem* pcItem)
{
	size	i;

	for (i = 0; i < maMenuItems.Size(); i++)
	{
		if (&maMenuItems.Get(i) == pcItem)
		{
			DoAction((ELogicimAction)maiMenuItemActions.GetValue(i));
			return;
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ListSelectionChanged(CListContainer* pcList)
{
	UpdateInfo();
}


//////////////////////////////////////////////////////////////////////////
//
// The view, dialog and editing actions work, the rest are logged.  A
// dialog is shown by the window's next tick.  Logicim's add actions don't
// do anything yet; here those for a component place one.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::DoAction(ELogicimAction eAction)
{
	Ptr<CCircuitCanvasDraw>	pDraw;
	CCircuitEditor*			pcEditor;

	pDraw = mpCircuit->GetCircuitDraw();
	pcEditor = mcCircuit.GetEditor();
	switch (eAction)
	{
	case LA_LoadProject:
		mpsData->eDialog = LDG_LoadProject;
		break;
	case LA_SaveProject:
		mpsData->eDialog = LDG_SaveProject;
		break;
	case LA_ImportKicadSchematic:
		mpsData->eDialog = LDG_ImportKicadSchematic;
		break;
	case LA_RotateLeft:
		pcEditor->RotateLeft();
		break;
	case LA_RotateRight:
		pcEditor->RotateRight();
		break;
	case LA_FlipHorizontally:
		pcEditor->FlipHorizontally();
		break;
	case LA_FlipVertically:
		pcEditor->FlipVertically();
		break;
	case LA_AddSplitter:
		pcEditor->StartPlaceComponent("Splitter");
		break;
	case LA_AddTunnel:
		pcEditor->StartPlaceComponent("Tunnel");
		break;
	case LA_AddPower:
		pcEditor->StartPlaceComponent("PositivePower");
		break;
	case LA_AddLabel:
		pcEditor->StartPlaceComponent("Label");
		break;
	case LA_Undo:
		Undo(false);
		break;
	case LA_Redo:
		Redo(false);
		break;
	case LA_Delete:
		pcEditor->Delete();
		break;
	case LA_NewSimulation:
		mpsData->eDialog = LDG_NewSimulation;
		break;
	case LA_NewSubcircuit:
		mpsData->eDialog = LDG_NewSubcircuit;
		break;
	case LA_AddComponent:
		mpsData->eDialog = LDG_ChooseComponent;
		break;
	case LA_EditProperties:
		mpsData->eDialog = LDG_Properties;
		break;
	case LA_RunSimulation:
		mbRunning = true;
		UpdateInfo();
		break;
	case LA_PauseSimulation:
		mbRunning = false;
		UpdateInfo();
		break;
	case LA_ResetSimulation:
		mcCircuit.Reset();
		break;
	case LA_DefaultSpeed:
		mfNanosecondsPerMillisecond = LOGICIM_DEFAULT_SPEED;
		break;
	case LA_IncreaseSpeed:
		mfNanosecondsPerMillisecond *= 2;
		break;
	case LA_DecreaseSpeed:
		mfNanosecondsPerMillisecond /= 2;
		break;
	case LA_TogglePointView:
		pDraw->SetGrid(!pDraw->IsGrid());
		break;
	case LA_ZoomIn:
		Zoom(pDraw->GetZoom() + 1.0f);
		break;
	case LA_ZoomOut:
		Zoom(pDraw->GetZoom() - 1.0f);
		break;
	case LA_ResetZoom:
		Zoom(1.0f);
		break;
	case LA_SelectZoom:
		mpsData->eDialog = LDG_SelectZoom;
		break;
	case LA_ZoomFitAll:
		ZoomToFit(false);
		break;
	case LA_ZoomFitSelection:
		ZoomToFit(true);
		break;
	case LA_Exit:
		mpWindow->Stop();
		break;
	default:
		gcLogger.Info2(gasLogicimActions[eAction].szName, " isn't done yet.", NULL);
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Runs the simulation on by the time since the last tick, scaled by the
// speed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::Tick(int64 iUpdateTimeInMilliseconds)
{
	if (mbRunning)
	{
		mcCircuit.Run(NanosecondsToTime(iUpdateTimeInMilliseconds * mfNanosecondsPerMillisecond));
	}
	UpdateEditMenu();
}


//////////////////////////////////////////////////////////////////////////
//
// Ctrl+Z, or the menu's Undo, goes back to before the last change that
// wasn't only to the selection; bStep (Ctrl+Shift+Z) goes back one change.
// The simulation goes back with the circuit, but keeps running or paused.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::Undo(bool bStep)
{
	mcCircuit.GetEditor()->Undo(bStep);
	UpdateInfo();
	UpdateEditMenu();
}


//////////////////////////////////////////////////////////////////////////
//
// Ctrl+Y, or the menu's Redo; bStep is Ctrl+Shift+Y.  As Undo.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::Redo(bool bStep)
{
	mcCircuit.GetEditor()->Redo(bStep);
	UpdateInfo();
	UpdateEditMenu();
}


//////////////////////////////////////////////////////////////////////////
//
// Greys the Edit menu's Undo and Redo when there's nothing to undo or
// redo, or an edit is in progress.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::UpdateEditMenu(void)
{
	size			i;
	ELogicimAction	eAction;
	int32			iRow;
	bool			bEnabled;

	if (mpEditMenu.IsNull())
	{
		return;
	}

	for (i = 0; i < maMenuItems.Size(); i++)
	{
		eAction = (ELogicimAction)maiMenuItemActions.GetValue(i);
		if ((eAction == LA_Undo) || (eAction == LA_Redo))
		{
			iRow = mpEditMenu->GetItemRow(maMenuItems.Get(i));
			if (iRow >= 0)
			{
				bEnabled = (eAction == LA_Undo) ? mcCircuit.GetEditor()->CanUndo() : mcCircuit.GetEditor()->CanRedo();
				if (mpEditMenu->IsRowEnabled(iRow) != bEnabled)
				{
					mpEditMenu->SetRowEnabled(iRow, bEnabled);
				}
			}
		}
	}
}


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
	CDialog					cDialog;
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

	pcEditor = mcCircuit.GetEditor();
	pcComponent = pcEditor->GetPropertiesComponent();
	if (pcComponent == NULL)
	{
		return;
	}
	cProperties.Init();
	pcEditor->GetProperties(pcComponent, &cProperties);

	sz.Init(pcComponent->GetType());
	sz.Append(" Properties");
	cDialog.Init(mpWindow, &msDialogLook, sz.Text());
	sz.Kill();

	aTextFields.Init();
	aCheckBoxes.Init();
	aComboBoxes.Init();
	aiEditors.Init();
	pRotation = cDialog.AddComboBox("Rotation", aszRotations, 4, pcComponent->GetRotation() - ROT_North);
	pMirrored = cDialog.AddCheckBox("Mirrored", pcComponent->IsMirrored());
	for (i = 0; i < cProperties.NumProperties(); i++)
	{
		psProperty = cProperties.Get(i);
		switch (psProperty->eType)
		{
		case CPT_Boolean:
			aiEditors.Add((int)aCheckBoxes.Size());
			aCheckBoxes.Add(cDialog.AddCheckBox(psProperty->szLabel.Text(), psProperty->szValue.Equals("true")));
			break;
		case CPT_Choice:
			pComboBox = OMalloc<CComboBox>(cDialog.GetWindow(), msDialogLook.pTheme);
			for (j = 0; j < psProperty->aszChoices.NumElements(); j++)
			{
				pComboBox->AddItem(psProperty->aszChoices.GetText(j));
			}
			pComboBox->Select((int32)cProperties.GetChoice(psProperty->szName.Text(), 0));
			pComboBox->SetAlignment(CSH_Stretch, CSV_Center);
			cDialog.AddEditor(psProperty->szLabel.Text(), pComboBox);
			aiEditors.Add((int)aComboBoxes.Size());
			aComboBoxes.Add(pComboBox);
			break;
		case CPT_Divider:
			cDialog.AddDivider(psProperty->szLabel.Text());
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
			aTextFields.Add(cDialog.AddTextField(sz.Text(), psProperty->szValue.Text()));
			sz.Kill();
			break;
		}
	}

	if (cDialog.Show(LOGICIM_DIALOG_WIDTH))
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
	cDialog.Kill();
	cProperties.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowNewSubcircuitDialog(void)
{
	CDialog				cDialog;
	Ptr<CTextField>		pName;
	char*				szName;

	cDialog.Init(mpWindow, &msDialogLook, "New Subcircuit");
	pName = cDialog.AddTextField("Name", "");

	if (cDialog.Show(LOGICIM_DIALOG_WIDTH))
	{
		szName = pName->GetText();
		if (szName[0] != '\0')
		{
			AddListItem(mpSubcircuits, &maSubcircuitLabels, szName);
			mpSubcircuits->Select(mpSubcircuits->NumRows() - 1);
			UpdateInfo();
		}
	}

	pName = NULL;
	cDialog.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Named like Logicim's: the top subcircuit and a count.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowNewSimulationDialog(void)
{
	CDialog				cDialog;
	Ptr<CTextField>		pName;
	CChars				szDefault;
	char*				szName;

	szDefault.Init("Top ");
	szDefault.Append(GetListItemText(mpSubcircuits, &maSubcircuitLabels));
	szDefault.Append(" ");
	szDefault.Append(miSimulationCount + 1);

	cDialog.Init(mpWindow, &msDialogLook, "New Simulation");
	pName = cDialog.AddTextField("Name", szDefault.Text());
	szDefault.Kill();

	if (cDialog.Show(LOGICIM_DIALOG_WIDTH))
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
	cDialog.Kill();
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
	CDialog				cDialog;
	Ptr<CTextField>		pZoom;
	Ptr<CCircuitCanvasDraw>	pDraw;
	CChars				szZoom;
	char*				szText;
	char*				szEnd;
	float64				fZoom;

	pDraw = mpCircuit->GetCircuitDraw();
	szZoom.Init();
	AppendTenths(&szZoom, pDraw->GetZoom());
	cDialog.Init(mpWindow, &msDialogLook, "Select Zoom");
	pZoom = cDialog.AddTextField("Zoom (1 is 100%)", szZoom.Text());
	szZoom.Kill();

	if (cDialog.Show(LOGICIM_DIALOG_WIDTH))
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
	cDialog.Kill();
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
	CLogicimComponentChooser	cChooser;
	CCircuitEditor*			pcEditor;
	char*					szChosen;
	bool					bSubcircuit;
	bool					bPlaced;
	SFloat64Vec2			sScreen;
	SFloat64Vec2			sGrid;
	SInt32Vec2				sSize;

	cChooser.Init(mpWindow, &msDialogLook, &mcCircuit, mszChooserSelected.Text());
	if (cChooser.Show())
	{
		szChosen = cChooser.GetChosen(&bSubcircuit);
		mszChooserSelected.Set(szChosen);
		pcEditor = mcCircuit.GetEditor();
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
	cChooser.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Asks for the project file to load or save with Logicim's file chooser,
// starting on the one last used.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowProjectDialog(bool bSave)
{
	CFileChooser		cChooser;
	char*				szFilename;
	bool				bResult;

	cChooser.Init(mpWindow, &msDialogLook, bSave ? "Save Project" : "Load Project", bSave, mszProjectFilename.Text(), gasProjectFileTypes, sizeof(gasProjectFileTypes) / sizeof(SFileChooserType), mapIcons.Get(LA_LoadProject), mpFileIcon);
	if (cChooser.Show())
	{
		szFilename = cChooser.GetChosen();
		mszProjectFilename.Set(szFilename);
		bResult = bSave ? mcCircuit.Save(szFilename) : mcCircuit.Load(szFilename);
		if (bResult)
		{
			gcLogger.Info2(bSave ? "Saved [" : "Loaded [", szFilename, "].", NULL);
		}
		UpdateInfo();
	}
	cChooser.Kill();
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
	CFileChooser		cChooser;
	char*				szFilename;

	cChooser.Init(mpWindow, &msDialogLook, "Import KiCad Schematic", false, mszKicadFilename.Text(), gasKicadFileTypes, sizeof(gasKicadFileTypes) / sizeof(SFileChooserType), mapIcons.Get(LA_LoadProject), mpFileIcon);
	if (cChooser.Show())
	{
		szFilename = cChooser.GetChosen();
		mszKicadFilename.Set(szFilename);
		if (mcCircuit.ImportKicadSchematic(szFilename))
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
	}
	cChooser.Kill();
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

