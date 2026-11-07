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
			pMenu->AddItem(mpCommands->Find(gasLogicimActions[aeActions[i]].szCommand));
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
	mpMenuBar->AddMenu("Edit", CreateMenu(aeEdit, sizeof(aeEdit) / sizeof(ELogicimAction)));
	mpMenuBar->AddMenu("View", CreateMenu(aeView, sizeof(aeView) / sizeof(ELogicimAction)));
	mpMenuBar->AddMenu("Simulation", CreateMenu(aeSimulation, sizeof(aeSimulation) / sizeof(ELogicimAction)));
	mpMenuBar->AddMenu("Place", CreateMenu(aePlace, sizeof(aePlace) / sizeof(ELogicimAction)));
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
	pButton->SetCommand(mpCommands->Find(gasLogicimActions[eAction].szCommand));
	pButton->SetAlignment(CSH_Center, CSV_Center);
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
	ELogicimAction	aeActions[] = { LA_LoadProject, LA_SaveProject, LA_Separator,
									LA_NewSimulation, LA_ResetSimulation, LA_RunSimulation, LA_PauseSimulation, LA_DefaultSpeed, LA_IncreaseSpeed, LA_DecreaseSpeed, LA_Separator,
									LA_RotateLeft, LA_RotateRight, LA_FlipHorizontally, LA_FlipVertically };

	return CreateToolStrip(aeActions, sizeof(aeActions) / sizeof(ELogicimAction), false);
}


//////////////////////////////////////////////////////////////////////////
//
// A row of tool buttons (the toolbar) or a column of them like IntelliJ's
// tool window stripes (Logicim's display and creation panels), 40 pixels
// across with a little space before the first button.  A separator is a
// line across the strip.  The space after the last button takes up the
// rest of the strip.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimWindowContent::CreateToolStrip(ELogicimAction* aeActions, int iNumActions, bool bVertical)
{
	Ptr<CGridContainer>		pStrip;
	Ptr<CFillContainer>		pFiller;
	Ptr<CCanvas>			pLine;
	int						i;
	int						iCell;

	//A grid only has a size if it is given one.
	pStrip = OMalloc<CGridContainer>(mpWindow);
	pStrip->SetAlignment(CSH_Stretch, CSV_Stretch);
	pStrip->SetDesiredSize(bVertical ? 40 : -1, bVertical ? -1 : 40);

	pFiller = OMalloc<CFillContainer>(mpWindow);
	pFiller->SetDesiredSize(bVertical ? 1 : 4, bVertical ? 4 : 1);
	pStrip->AddComponent(pFiller, 0, 0);
	iCell = 1;
	for (i = 0; i < iNumActions; i++)
	{
		if (aeActions[i] == LA_Separator)
		{
			pLine = CreateLine(bVertical ? 22 : 1, bVertical ? 1 : 22, mpTheme->GetColour(TC_Separator));
			pFiller = OMalloc<CFillContainer>(mpWindow);
			pFiller->SetDesiredSize(bVertical ? 22 : 13, bVertical ? 13 : 22);
			pFiller->AddComponent(pLine);
			pStrip->AddComponent(pFiller, bVertical ? 0 : iCell, bVertical ? iCell : 0);
		}
		else
		{
			pStrip->AddComponent(CreateToolButton(aeActions[i]), bVertical ? 0 : iCell, bVertical ? iCell : 0);
		}
		iCell++;
	}

	pFiller = OMalloc<CFillContainer>(mpWindow);
	pStrip->AddComponent(pFiller, bVertical ? 0 : iCell, bVertical ? iCell : 0, 1, 1, bVertical ? 0 : 1, bVertical ? 1 : 0);
	return pStrip;
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
	pList->SetHighlightDraws(OMalloc<CRoundedBoxCanvasDraw>(mpTheme->GetColour(TC_ListSelected), 4, &sInsets), OMalloc<CRoundedBoxCanvasDraw>(mpTheme->GetColour(TC_ListHover), 4, &sInsets));
	pList->SetAlignment(CSH_Stretch, CSV_Top);
	pList->GetSelectionEvent()->Add(this, &CLogicimWindowContent::UpdateInfo);
	return pList;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::RemoveFromList(Ptr<CListContainer> pList)
{
	if (pList.IsNotNull())
	{
		pList->GetSelectionEvent()->Remove(this);
	}
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
	mpSubcircuits->GetSelectionEvent()->Remove(this);
	mpSubcircuits->GetSelectionEvent()->Add(this, &CLogicimWindowContent::SubcircuitSelected);
	pSubcircuits = CreateToolWindow("Subcircuits", mpSubcircuits);
	pSubcircuits->SetDesiredSize(200, -1);

	mpCircuit = OMalloc<CCircuitCanvas>(mpWindow, OMalloc<CCircuitCanvasDraw>(&mpLogicimCircuit), this, mpLogicimCircuit->GetEditor());
	mpCircuit->SetAlignment(CSH_Stretch, CSV_Stretch);
	mpCanvasMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	mpCanvasMenu->GetItemClickEvent()->Add(this, &CLogicimWindowContent::CanvasMenuItemClicked);
	mpCircuit->SetContextMenu(mpCanvasMenu);
	mpSimulations = CreateList();
	pSimulations = CreateToolWindow("Simulations", mpSimulations);
	pSimulations->SetDesiredSize(-1, 150);

	pCentre = OMalloc<CGridContainer>(mpWindow);
	pCentre->SetAlignment(CSH_Stretch, CSV_Stretch);
	pCentre->AddComponent(mpCircuit, 0, 0, 1, 1, 1, 1);
	pCentre->AddComponent(CreateLine(-1, 1, mpTheme->GetColour(TC_Editor)), 0, 1, 1, 1, 1, 0);
	pCentre->AddComponent(pSimulations, 0, 2, 1, 1, 1, 0);

	pEditor->AddComponent(CreateToolStrip(aeDisplay, sizeof(aeDisplay) / sizeof(ELogicimAction), true), 0, 0, 1, 1, 0, 1);
	pEditor->AddComponent(CreateLine(1, -1, mpTheme->GetColour(TC_Editor)), 1, 0, 1, 1, 0, 1);
	pEditor->AddComponent(pSubcircuits, 2, 0, 1, 1, 0, 1);
	pEditor->AddComponent(CreateLine(1, -1, mpTheme->GetColour(TC_Editor)), 3, 0, 1, 1, 0, 1);
	pEditor->AddComponent(pCentre, 4, 0, 1, 1, 1, 1);
	pEditor->AddComponent(CreateLine(1, -1, mpTheme->GetColour(TC_Editor)), 5, 0, 1, 1, 0, 1);
	pEditor->AddComponent(CreateToolStrip(aeCreation, sizeof(aeCreation) / sizeof(ELogicimAction), true), 6, 0, 1, 1, 0, 1);
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

	pBar->AddComponent(CreateLine(1, 16, mpTheme->GetColour(TC_Separator)), *piColumn, 0);
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
