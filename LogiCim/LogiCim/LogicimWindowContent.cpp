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


//IntelliJ IDEA's new UI dark theme.  The dim text is LogiCim's own, for the status bar and dividers.
#define LOGICIM_DIM_TEXT			Set32BitColour((uint8)0x9D, (uint8)0xA0, (uint8)0xA8)
#define LOGICIM_DEFAULT_SPEED	0.25  //Simulated nanoseconds per millisecond.
#define LOGICIM_SIMULATION_TICK_MILLISECONDS	16  //How often a running simulation is stepped.


//In ELogicimAction's order.
SLogicimActionDesc gasLogicimActions[NUM_LOGICIM_ACTIONS] =
{
	{ "Load Project", "directory_open", LDG_LoadProject, NULL },
	{ "Save Project", "save", LDG_SaveProject, NULL },
	{ "New Simulation...", "new_simulation", LDG_NewSimulation, NULL },
	{ "Reset Simulation", "sim_reset", LDG_None, NULL },
	{ "Run Simulation", "sim_run", LDG_None, NULL },
	{ "Pause Simulation", "sim_stop", LDG_None, NULL },
	{ "Default Speed", "sim_default", LDG_None, NULL },
	{ "Increase Speed", "sim_faster", LDG_None, NULL },
	{ "Decrease Speed", "sim_slower", LDG_None, NULL },
	{ "Rotate Left", "rotate_ccw", LDG_None, NULL },
	{ "Rotate Right", "rotate_cw", LDG_None, NULL },
	{ "Flip Horizontally", "mirror_h", LDG_None, NULL },
	{ "Flip Vertically", "mirror_v", LDG_None, NULL },
	{ "Toggle Point View", "grid", LDG_None, NULL },
	{ "Zoom In", "zoom_in", LDG_None, NULL },
	{ "Zoom Out", "zoom_out", LDG_None, NULL },
	{ "Reset Zoom", "zoom_center_on_screen", LDG_None, NULL },
	{ "Select Zoom...", NULL, LDG_SelectZoom, NULL },
	{ "Zoom to Fit All", "zoom_fit_in_page", LDG_None, NULL },
	{ "Zoom to Fit Selection", "zoom_selection", LDG_None, NULL },
	{ "Simulation Tree...", "simulation_tree", LDG_None, NULL },
	{ "Add Component...", "add_component", LDG_ChooseComponent, NULL },
	{ "Add Trace", "add_line", LDG_None, NULL },
	{ "Add Splitter", "add_line2bus", LDG_None, "Splitter" },
	{ "Add Tunnel", "add_glabel", LDG_None, "Tunnel" },
	{ "Add Connection", "add_junction", LDG_None, NULL },
	{ "Add Power", "add_power", LDG_None, "PositivePower" },
	{ "Add Label", "add_label", LDG_None, "Label" },
	{ "Add Subcircuit", "module", LDG_None, NULL },
	{ "New Subcircuit...", "new_footprint", LDG_NewSubcircuit, NULL },
	{ "Properties...", "edit", LDG_Properties, NULL },
	{ "Undo", "undo", LDG_None, NULL },
	{ "Redo", "redo", LDG_None, NULL },
	{ "Cut", "cut", LDG_None, NULL },
	{ "Copy", "copy", LDG_None, NULL },
	{ "Paste", NULL, LDG_None, NULL },
	{ "Duplicate", "duplicate", LDG_None, NULL },
	{ "Delete", "trash", LDG_None, NULL },
	{ "Import KiCad Schematic...", "import", LDG_ImportKicadSchematic, NULL },
	{ "Exit", "exit", LDG_None, NULL },
};


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::Init(void)
{
	PreInit();

	mpsData = NULL;
	miSimulationCount = 0;
	mbRunning = true;
	mbRanLastTick = false;
	mfNanosecondsPerMillisecond = LOGICIM_DEFAULT_SPEED;
	maButtons.Init();
	maiButtonActions.Init();
	maMenuItems.Init();
	maiMenuItemActions.Init();
	mapIcons.Init();
	maSubcircuitLabels.Init();
	maSimulationLabels.Init();
	mszProjectFilename.Init("Project.xml");
	mszKicadFilename.Init("Schematic.kicad_sch");
	mszStartupImport.Init();
	mbListingSubcircuits = false;
	mszChooserSelected.Init();
	mpLogicimCircuit = OMalloc<CLogicimCircuit>();

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::Class(void)
{
	M_Pointer(mpWindow);
	U_Pointer(mpsData);
	M_Pointer(mpFontColours);
	M_Pointer(mpFont);
	M_Pointer(mpDimFont);
	M_Pointer(mpTheme);
	U_Pointer(msDialogLook.pcTheme);
	U_Pointer(msDialogLook.pcDimFont);
	U_Data(ARGB32, msDialogLook.sBackground);
	U_Data(ARGB32, msDialogLook.sDivider);
	M_Embedded(mapIcons);
	M_Pointer(mpFileIcon);
	M_Pointer(mpMenuBar);
	M_Pointer(mpEditMenu);
	M_Pointer(mpSubcircuits);
	M_Pointer(mpCanvasMenu);
	M_Pointer(mpEnterItem);
	M_Pointer(mpPropertiesItem);
	M_Pointer(mpLeaveItem);
	M_Pointer(mpSimulations);
	M_Embedded(maSubcircuitLabels);
	M_Embedded(maSimulationLabels);
	M_Pointer(mpCircuit);
	M_Pointer(mpSubcircuitInfo);
	M_Pointer(mpSimulationInfo);
	M_Pointer(mpViewPathInfo);
	M_Pointer(mpMouseXInfo);
	M_Pointer(mpMouseYInfo);
	M_Pointer(mpZoomInfo);
	M_Embedded(maButtons);
	U_Unknown(CArrayInt, maiButtonActions);
	M_Embedded(maMenuItems);
	U_Unknown(CArrayInt, maiMenuItemActions);
	M_Pointer(mpLogicimCircuit);
	M_Pointer(mpDialog);
	M_Pointer(mpMessageDialog);
	M_Pointer(mpComponentChooser);
	M_Pointer(mpFileChooser);
	U_String(mszProjectFilename);
	U_String(mszKicadFilename);
	U_String(mszStartupImport);
	U_Bool(mbListingSubcircuits);
	U_String(mszChooserSelected);
	U_Int32(miSimulationCount);
	U_Bool(mbRunning);
	U_Bool(mbRanLastTick);
	U_Data(float64, mfNanosecondsPerMillisecond);
}


//////////////////////////////////////////////////////////////////////////
//
// The content isn't saved; a project is, see LogicimCircuit.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::Save(CObjectWriter* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::Load(CObjectReader* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The window's widgets and LogiCim's circuit.  Free still frees whatever
// was made when this fails.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::Setup(Ptr<CWindow> pWindow, SLogicimWindowData* psData, const char* szFontFilename, const char* szIconDirectory, const char* szPropagationTimes, const char* szCircuitFontDirectory)
{
	CColourFormatHelper		cFormat;
	Ptr<CGridContainer>		pGrid;
	Ptr<CFont>				pWhite;

	mpWindow = pWindow;
	mpsData = psData;
	mpsData->pcContent = this;
	mpsData->eDialog = LDG_None;

	pWindow->GetColourFormat(&cFormat);
	if (!mpLogicimCircuit->Setup(szPropagationTimes, szCircuitFontDirectory, cFormat.meOrder))
	{
		return false;
	}

	//The font is white and coloured as it is drawn.
	pWhite = GenerateFont(szFontFilename, "Inter 13px", 13.0f, cFormat.meOrder);
	if (pWhite.IsNull())
	{
		return false;
	}
	//The content holds the font colours and the theme; the widgets only hold their parts.
	mpFontColours = OMalloc<CFontColours>(pWhite);
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

	//Like Logicim the project starts with a top level subcircuit and a simulation of it.  The subcircuits are listed
	//by UpdateInfo.
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
void CLogicimWindowContent::Free(void)
{
	//An object's memory is freed without its destructor running so the content takes itself off the widgets still
	//here.  The widgets, the theme, the fonts and the circuit are freed with the content's pointers.
	RemoveFromButtons();
	RemoveFromList(mpSubcircuits);
	if (mpCanvasMenu.IsNotNull())
	{
		mpCanvasMenu->GetItemClickEvent()->Remove(this);
	}
	RemoveFromList(mpSimulations);
	if (mpMenuBar.IsNotNull())
	{
		mpMenuBar->GetItemClickEvent()->Remove(this);
	}
	if (mpsData)
	{
		mpsData->pcContent = NULL;
	}

	maiButtonActions.Kill();
	maiMenuItemActions.Kill();
	mszStartupImport.Kill();
	mszKicadFilename.Kill();
	mszProjectFilename.Kill();
	mszChooserSelected.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::UpdateInfo(void)
{
	CChars	sz;

	ListSubcircuits();

	sz.Init("Subcircuit: ");
	sz.Append(GetListItemText(mpSubcircuits, &maSubcircuitLabels));
	mpSubcircuitInfo->SetText(sz.Text());
	sz.Kill();

	sz.Init("Simulation: ");
	sz.Append(GetListItemText(mpSimulations, &maSimulationLabels));
	mpSimulationInfo->SetText(sz.Text());
	sz.Kill();

	sz.Init("View Path: ");
	if (mpLogicimCircuit->GetEditor()->GetViewPath() != NULL)
	{
		mpLogicimCircuit->GetEditor()->GetViewPath()->AppendDescription(&sz);
	}
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
	SFloat64Bounds		sBox;
	bool				bHasBounds;

	pcEditor = mpLogicimCircuit->GetEditor();
	if (bSelection)
	{
		bHasBounds = pcEditor->GetSelection()->GetBoundingBox(&sBox);
	}
	else
	{
		bHasBounds = (pcEditor->GetCircuit() != NULL) && pcEditor->GetCircuit()->GetBoundingBox(&sBox);
	}

	if (bHasBounds)
	{
		mpCircuit->GetCircuitDraw()->ZoomToFit(&sBox);
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
void CLogicimWindowContent::RemoveFromButtons(void)
{
	size			i;
	Ptr<CButton>	pButton;

	for (i = 0; i < maButtons.Size(); i++)
	{
		pButton = maButtons.Get(i);
		if (pButton.IsNotNull())
		{
			pButton->GetClickEvent()->Remove(this);
		}
	}
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
// The view, dialog and editing actions work, the rest are logged.  A
// dialog (see SLogicimActionDesc) is shown by the window's next tick.
// Logicim's add actions don't do anything yet; here those for a component
// place one.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::DoAction(ELogicimAction eAction)
{
	Ptr<CCircuitCanvasDraw>	pDraw;
	CCircuitEditor*			pcEditor;

	pDraw = mpCircuit->GetCircuitDraw();
	pcEditor = mpLogicimCircuit->GetEditor();
	if (gasLogicimActions[eAction].eDialog != LDG_None)
	{
		mpsData->eDialog = gasLogicimActions[eAction].eDialog;
		return;
	}
	if (gasLogicimActions[eAction].szPlaced != NULL)
	{
		pcEditor->StartPlaceComponent(gasLogicimActions[eAction].szPlaced);
		return;
	}

	switch (eAction)
	{
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
	case LA_Undo:
		Undo(false);
		break;
	case LA_Redo:
		Redo(false);
		break;
	case LA_Delete:
		pcEditor->Delete();
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
		mpLogicimCircuit->Reset();
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
// speed, and paints the window again if anything happened in it.  The
// window waits for events between ticks, so a running simulation asks to
// be ticked again.  The time before the simulation started running isn't
// simulated.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::Tick(int64 iUpdateTimeInMilliseconds)
{
	CChars			szImport;
	SFloat64Vec2	sCentre;
	SFloat64Bounds	sBox;
	float64			fSize;

	//Once the window is laid out, so the import is zoomed to fit the canvas.
	if (!mszStartupImport.Empty())
	{
		szImport.Init(mszStartupImport.Text());
		mszStartupImport.Set("");
		ImportKicadSchematic(szImport.Text());
		szImport.Kill();

		//LOGICIM_VIEW="x,y,size" shows the square of grid units about x, y instead, for scripted checks.
		if ((getenv("LOGICIM_VIEW") != NULL) && (sscanf(getenv("LOGICIM_VIEW"), "%lf,%lf,%lf", &sCentre.x, &sCentre.y, &fSize) == 3))
		{
			sBox.Init(SFloat64Vec2(sCentre.x - fSize / 2, sCentre.y - fSize / 2), SFloat64Vec2(sCentre.x + fSize / 2, sCentre.y + fSize / 2));
			mpCircuit->GetCircuitDraw()->ZoomToFit(&sBox);
			ShowZoom();
		}
	}
	if (mbRunning)
	{
		if (mbRanLastTick && mpLogicimCircuit->Run(NanosecondsToTime(iUpdateTimeInMilliseconds * mfNanosecondsPerMillisecond)))
		{
			mpWindow->Invalidate();
		}
		mpWindow->RequestTick(LOGICIM_SIMULATION_TICK_MILLISECONDS);
	}
	mbRanLastTick = mbRunning;
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
	mpLogicimCircuit->GetEditor()->Undo(bStep);
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
	mpLogicimCircuit->GetEditor()->Redo(bStep);
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
				bEnabled = (eAction == LA_Undo) ? mpLogicimCircuit->GetEditor()->CanUndo() : mpLogicimCircuit->GetEditor()->CanRedo();
				if (mpEditMenu->IsRowEnabled(iRow) != bEnabled)
				{
					mpEditMenu->SetRowEnabled(iRow, bEnabled);
				}
			}
		}
	}
}
