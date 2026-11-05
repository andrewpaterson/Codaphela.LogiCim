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
#ifndef __LOGICIM_WINDOW_CONTENT_H__
#define __LOGICIM_WINDOW_CONTENT_H__
#include "BaseLib/ArrayInt.h"
#include "SupportLib/Font.h"
#include "SupportLib/FontColours.h"
#include "WindowLib/Window.h"
#include "WindowLib/GridContainer.h"
#include "WindowLib/ScrollPane.h"
#include "WindowLib/ListContainer.h"
#include "WindowLib/MenuBar.h"
#include "WindowLib/Button.h"
#include "WindowLib/Label.h"
#include "LogicimButtonListener.h"
#include "LogicimWindowData.h"
#include "WindowLib/Dialog.h"
#include "CircuitCanvas.h"
#include "LogicimCircuit.h"


//What a toolbar button or menu item does.  The buttons and menu items are those of Logicim's window.
enum ELogicimAction
{
	LA_LoadProject,
	LA_SaveProject,
	LA_NewSimulation,
	LA_ResetSimulation,
	LA_RunSimulation,
	LA_PauseSimulation,
	LA_DefaultSpeed,
	LA_IncreaseSpeed,
	LA_DecreaseSpeed,
	LA_RotateLeft,
	LA_RotateRight,
	LA_FlipHorizontally,
	LA_FlipVertically,
	LA_TogglePointView,
	LA_ZoomIn,
	LA_ZoomOut,
	LA_ResetZoom,
	LA_SelectZoom,
	LA_ZoomFitAll,
	LA_ZoomFitSelection,
	LA_ViewSimulationTree,
	LA_AddComponent,
	LA_AddTrace,
	LA_AddSplitter,
	LA_AddTunnel,
	LA_AddConnection,
	LA_AddPower,
	LA_AddLabel,
	LA_AddSubcircuit,
	LA_NewSubcircuit,
	LA_EditProperties,
	LA_Undo,
	LA_Redo,
	LA_Cut,
	LA_Copy,
	LA_Paste,
	LA_Duplicate,
	LA_Delete,
	LA_ImportKicadSchematic,
	LA_Exit,

	NUM_LOGICIM_ACTIONS
};


class CDialog;
class CFileChooser;
class CLogicimComponentChooser;


//LogiCim's window, laid out like Logicim's in IntelliJ IDEA's new UI dark style.  From the top: the menu bar, the
//main toolbar (load and save, the simulation's controls, rotate and flip), then the editor between the view stripe on
//the left and the add stripe on the right, then the status bar.  The editor has the subcircuits' tool window on its
//left and the simulations' tool window below its circuit canvas.
//
//The dialogs (a component's properties, a new subcircuit, a new simulation) are native windows shown modally.
//
//The content is an object; keep it under the root, see Setup.  It holds the window, its widgets and LogiCim's circuit.
class CLogicimWindowContent : public CObject
{
CONSTRUCTABLE(CLogicimWindowContent);
DESTRUCTABLE(CLogicimWindowContent);
protected:
	Ptr<CWindow>			mpWindow;
	SLogicimWindowData*		mpsData;
	Ptr<CFontColours>		mpFontColours;
	Ptr<CFont>				mpFont;
	Ptr<CFont>				mpDimFont;
	Ptr<CWindowTheme>		mpTheme;
	SDialogLook				msDialogLook;
	CArray<CImageCel>		mapIcons;  //One for each ELogicimAction, NULL if the action has no icon.
	Ptr<CImageCel>			mpFileIcon;  //Beside a file in the file chooser, Load Project's beside a folder.

	Ptr<CMenuBar>			mpMenuBar;
	Ptr<CMenuContainer>		mpEditMenu;  //Its undo and redo are greyed when there's nothing to undo or redo.
	Ptr<CListContainer>		mpSubcircuits;
	Ptr<CListContainer>		mpSimulations;
	CArray<CLabel>			maSubcircuitLabels;  //The text of each row.
	CArray<CLabel>			maSimulationLabels;
	Ptr<CCircuitCanvas>		mpCircuit;
	Ptr<CLabel>				mpSubcircuitInfo;
	Ptr<CLabel>				mpSimulationInfo;
	Ptr<CLabel>				mpViewPathInfo;
	Ptr<CLabel>				mpMouseXInfo;
	Ptr<CLabel>				mpMouseYInfo;
	Ptr<CLabel>				mpZoomInfo;

	CArray<CButton>			maButtons;
	CArrayInt				maiButtonActions;
	CArray<CMenuItem>		maMenuItems;
	CArrayInt				maiMenuItemActions;

	CLogicimMenuListener		mcMenuListener;

	Ptr<CLogicimCircuit>	mpLogicimCircuit;
	Ptr<CDialog>					mpDialog;  //The dialog being shown, NULL when none is.
	Ptr<CLogicimComponentChooser>	mpComponentChooser;
	Ptr<CFileChooser>				mpFileChooser;
	CChars					mszProjectFilename;  //The project last loaded or saved.
	CChars					mszKicadFilename;  //The KiCad schematic last imported.
	CChars					mszChooserSelected;  //The component last chosen.
	int						miSimulationCount;
	bool					mbRunning;
	bool					mbRanLastTick;  //The simulation ran in the last tick, so the time since it is simulated.
	float64					mfNanosecondsPerMillisecond;  //How fast the simulation runs.

public:
	void			Init(void);
	void			Class(void) override;
	void			Free(void) override;

	bool			Save(CObjectWriter* pcFile) override;
	bool			Load(CObjectReader* pcFile) override;

	bool			Setup(Ptr<CWindow> pWindow, SLogicimWindowData* psData, const char* szFontFilename, const char* szIconDirectory, const char* szPropagationTimes, const char* szCircuitFontDirectory);

	void			ButtonClicked(CButton* pcButton);
	void			MenuItemClicked(CMenuItem* pcItem);
	void			PointerOverCircuit(int x, int y);
	void			Zoom(float32 fZoom);
	void			ZoomToFit(bool bSelection);
	void			EditProperties(void);
	void			Undo(bool bStep);
	void			Redo(bool bStep);
	void			ShowDialog(ELogicimDialog eDialog);
	void			Tick(int64 iUpdateTimeInMilliseconds);

protected:
	Ptr<CFont>				GenerateFont(const char* szFontFilename, const char* szName, float32 fPixelSize, EColourOrder eOrder);
	bool					LoadIcons(const char* szIconDirectory);
	Ptr<CImageCel>			ReadIcon(const char* szIconDirectory, const char* szIcon);
	void					CreateLooks(void);

	Ptr<CComponent>			CreateMenuBar(void);
	Ptr<CMenuContainer>		CreateMenu(ELogicimAction* aeActions, int iNumActions);
	Ptr<CComponent>			CreateToolbar(void);
	Ptr<CComponent>			CreateStripe(ELogicimAction* aeActions, int iNumActions);
	Ptr<CComponent>			CreateEditor(void);
	Ptr<CComponent>			CreateToolWindow(const char* szTitle, Ptr<CListContainer> pList);
	Ptr<CComponent>			CreateStatusBar(void);

	Ptr<CButton>			CreateToolButton(ELogicimAction eAction);
	void					RemoveFromButtons(void);
	void					RemoveFromList(Ptr<CListContainer> pList);
	Ptr<CCanvas>			CreateLine(int32 iWidth, int32 iHeight, ARGB32 sColour);
	Ptr<CListContainer>		CreateList(void);
	void					AddListItem(Ptr<CListContainer> pList, CArray<CLabel>* paLabels, const char* szText);
	char*					GetListItemText(Ptr<CListContainer> pList, CArray<CLabel>* paLabels);
	Ptr<CLabel>				AddStatusLabel(Ptr<CGridContainer> pBar, int* piColumn, int32 iWidth);

	void					DoAction(ELogicimAction eAction);
	void					UpdateInfo(void);
	void					UpdateEditMenu(void);
	void					ShowPropertiesDialog(void);
	void					ShowNewSubcircuitDialog(void);
	void					ShowNewSimulationDialog(void);
	void					ShowSelectZoomDialog(void);
	void					ShowZoom(void);
	void					ShowProjectDialog(bool bSave);
	void					ShowImportKicadDialog(void);
	void					ShowComponentChooser(void);
};


#endif // __LOGICIM_WINDOW_CONTENT_H__

