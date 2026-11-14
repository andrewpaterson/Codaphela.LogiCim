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
#include <stdlib.h>
#include "BaseLib/Logger.h"
#include "BaseLib/TypeNames.h"
#include "BaseLib/TypeConverter.h"
#include "BaseLib/GlobalMemory.h"
#include "BaseLib/GlobalDataTypesIO.h"
#include "StandardLib/Objects.h"
#include "WindowLib/ClearCanvasDraw.h"
#include "WindowLib/MenuBar.h"
#include "LogicimWindowTick.h"
#include "LogicimApp.h"


//////////////////////////////////////////////////////////////////////////
//
// Before it is set up.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimApp::Init(void)
{
	PreInit();

	memset(&msData, 0, sizeof(SLogicimWindowData));
	mbValid = false;
	mbInputs = false;

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
// The window and its content.  The devices are described by the XML in
// szInputDirectory; pcNativeInput (the platform's keyboard and mouse) is
// added to them.  Run fails if anything here did.
// Call it once the app is in the root, so that what is made here is
// built under the root rather than as an unrooted graph.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimApp::Setup(CNativeWindowFactory* pcNativeFactory, CNativeInput* pcNativeInput, const char* szInputDirectory)
{
	Ptr<CLogicimWindowTick>	pTick;
	Ptr<CClearCanvasDraw>	pBackground;

	pTick = OMalloc<CLogicimWindowTick>(&msData);
	if (getenv("LOGICIM_SNAPSHOT") != NULL)
	{
		pTick->SetSnapshot(getenv("LOGICIM_SNAPSHOT"), (getenv("LOGICIM_SNAPSHOT_TICKS") != NULL) ? atoi(getenv("LOGICIM_SNAPSHOT_TICKS")) : 60);
	}
	pBackground = OMalloc<CClearCanvasDraw>(0);  //Coloured once the content has made the theme.
	mpWindow = OMalloc<CWindow>("LogiCim", pcNativeFactory, pTick, pBackground, CFT_RGBX, CCO_BGR, CRGB_24bit, ARGB_8bit);

	//Only what changed is drawn.  LOGICIM_VERIFY_REDRAW checks every partial paint against drawing all of the window and
	//logs where they differ, for finding a component that changes without being invalidated.
	mpWindow->SetPartialRedraw(true);
	mpWindow->SetVerifyRedraw(getenv("LOGICIM_VERIFY_REDRAW") != NULL);

	mpContent = OMalloc<CLogicimWindowContent>();
	mbValid = mpContent->Setup(mpWindow, &msData, "Resources/InterVariable.ttf", "Resources/Icons", "Resources/PropagationTimes.csv", "Resources/Fonts");
	if (mbValid)
	{
		pBackground->SetColour(mpContent->GetTheme()->GetColour(TC_Panel));
		mpInput = OMalloc<CInput>((CNativeInput*)NULL);
		mbValid = mpInput->Read((char*)szInputDirectory);
		mpInput->AddNativeInput(pcNativeInput);
		mpInputSource = OMalloc<CWindowInputSource>(mpInput);
		mcComponentInput.Init(&mpInputSource, &mpWindow);
		mbInputs = true;
		mcComponentInput.AddKeyboards();
		mcComponentInput.AddMice();
		mpWindow->GetCommands()->BindKeys(&mcComponentInput);  //LogiCim's keys, Control+Alt+Z and so on.
		mpWindow->SetComponentInput(&mcComponentInput);
		mpWindow->SetWindowSize(1280, 800);
		mpContent->ReadSettings();  //Where the user last loaded and imported from, and the recent projects.
		if (getenv("LOGICIM_IMPORT") != NULL)
		{
			mpContent->SetStartupImport(getenv("LOGICIM_IMPORT"));
		}
	}
	return mbValid;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimApp::Class(void)
{
	U_Unknown(SLogicimWindowData, msData);
	M_Pointer(mpWindow);
	M_Pointer(mpContent);
	M_Pointer(mpInput);
	M_Pointer(mpInputSource);
	U_Unknown(CComponentInput, mcComponentInput);
	U_Bool(mbValid);
	U_Bool(mbInputs);
}


//////////////////////////////////////////////////////////////////////////
//
// Only this object's own members: the window, its content and the input
// may already be freed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimApp::Free(void)
{
	if (mbInputs)
	{
		mcComponentInput.Kill();
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimApp::Save(CObjectWriter* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimApp::Load(CObjectReader* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Runs the window until it is closed (if Init worked), then takes the
// component input out of the window.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimApp::Run(void)
{
	if (mbValid)
	{
		mpWindow->Show();
	}

	if (mbInputs)
	{
		mpWindow->SetComponentInput(NULL);
	}
	return mbValid;
}


//////////////////////////////////////////////////////////////////////////
//
// The logger, types, memory and objects LogiCim runs with.
//
//////////////////////////////////////////////////////////////////////////
void InitLogicimGlobals(void)
{
	gcLogger.Init();
	TypesInit();
	TypeConverterInit();
	MemoryInit();
	DataIOInit();
	ObjectsInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void KillLogicimGlobals(void)
{
	ObjectsFlush();
	ObjectsKill();
	DataIOKill();
	MemoryKill();
	TypeConverterKill();
	TypesKill();
	gcLogger.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The comma separated steps of a scripted check (Checks/LogicimChecks.py),
// run once the window is set up but not shown, each printing what is
// checked.  False if a step isn't known.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimApp::Check(const char* szSteps)
{
	CChars		szAll;
	CArrayChars	aszSteps;
	size		i;
	bool		bResult;

	szAll.Init(szSteps);
	aszSteps.Init();
	szAll.Split(&aszSteps, ',');
	bResult = true;
	for (i = 0; bResult && (i < aszSteps.NumElements()); i++)
	{
		bResult = CheckStep(aszSteps.Get(i)->Text());
	}
	aszSteps.Kill();
	szAll.Kill();
	fflush(stdout);
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// commands: every command, its keys and whether the keyboard gives them.
// menus: each menu's items and their commands.  buttons: how many of the
// tool buttons run a command.  command:<name> runs one.  zoom: the
// circuit's zoom.  load:<file>, save:<file> and import:<file> do what
// Load Project, Save Project and Import KiCad Schematic do with the file
// chosen.  files: the project's file and where each of those opens.
// new does what New Project does.  recent: File, Recent Projects' items.  openrecent:<n> loads the nth (1
// the latest) as its item does, and clearrecent does what Clear Recent
// Files does.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimApp::CheckStep(const char* szStep)
{
	size	uiWithCommand;
	size	uiWithout;

	if (StringCompare(szStep, "commands") == 0)
	{
		PrintCommands();
	}
	else if (StringCompare(szStep, "menus") == 0)
	{
		PrintMenus();
	}
	else if (StringCompare(szStep, "buttons") == 0)
	{
		uiWithCommand = 0;
		uiWithout = 0;
		CountButtons(mpWindow->GetContainer(), &uiWithCommand, &uiWithout);
		printf("Buttons: %d with a command, %d without\n", (int)uiWithCommand, (int)uiWithout);
	}
	else if (strncmp(szStep, "command:", 8) == 0)
	{
		printf("Command %s: %s\n", &szStep[8], mpWindow->GetCommands()->Run(&szStep[8]) ? "run" : "not run");
	}
	else if (StringCompare(szStep, "zoom") == 0)
	{
		printf("Zoom: %.2f\n", mpContent->GetZoom());
	}
	else if (StringCompare(szStep, "new") == 0)
	{
		printf("New project: %s\n", mpContent->NewProject() ? "made" : "not made");
	}
	else if (strncmp(szStep, "load:", 5) == 0)
	{
		printf("Load %s: %s\n", &szStep[5], mpContent->LoadProject(&szStep[5]) ? "loaded" : "not loaded");
	}
	else if (strncmp(szStep, "save:", 5) == 0)
	{
		printf("Save %s: %s\n", &szStep[5], mpContent->SaveProject(&szStep[5]) ? "saved" : "not saved");
	}
	else if (strncmp(szStep, "import:", 7) == 0)
	{
		printf("Import %s: %s\n", &szStep[7], mpContent->ImportChosenKicadSchematic(&szStep[7]) ? "imported" : "not imported");
	}
	else if (StringCompare(szStep, "files") == 0)
	{
		printf("Project file: [%s]\n", mpContent->GetLogicimCircuit()->GetFilename());
		printf("Load Project opens: [%s]\n", mpContent->GetProjectDialogStart(false));
		printf("Save Project opens: [%s]\n", mpContent->GetProjectDialogStart(true));
		printf("Import KiCad Schematic opens: [%s]\n", mpContent->GetSettings()->GetKicadImport());
	}
	else if (StringCompare(szStep, "recent") == 0)
	{
		PrintRecent();
	}
	else if (strncmp(szStep, "openrecent:", 11) == 0)
	{
		printf("Open recent %s: %s\n", &szStep[11], mpContent->OpenRecentProject((size)(atoi(&szStep[11]) - 1)) ? "loaded" : "not loaded");
	}
	else if (StringCompare(szStep, "clearrecent") == 0)
	{
		mpContent->ClearRecentProjects();
		printf("Recent projects cleared\n");
	}
	else
	{
		printf("Unknown check step [%s]\n", szStep);
		return false;
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimApp::PrintCommands(void)
{
	Ptr<CCommandSet>	pCommands;
	Ptr<CCommand>		pCommand;
	size				i;

	pCommands = mpWindow->GetCommands();
	printf("Commands: %d\n", (int)pCommands->NumCommands());
	for (i = 0; i < pCommands->NumCommands(); i++)
	{
		pCommand = pCommands->GetCommand(i);
		if (StrEmpty(pCommand->GetKeys()))
		{
			printf("Command: %s\n", pCommand->GetName());
		}
		else
		{
			printf("Command: %s [%s] %s\n", pCommand->GetName(), pCommand->GetKeys(), mcComponentInput.IsBound(pCommand->GetKeys()) ? "bound" : "NOT BOUND");
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// File, Recent Projects' items, in order.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimApp::PrintRecent(void)
{
	Ptr<CMenuContainer>	pMenu;
	size				i;

	pMenu = mpContent->GetRecentMenu();
	printf("Recent Projects: %d\n", (int)pMenu->NumItems());
	for (i = 0; i < pMenu->NumItems(); i++)
	{
		printf("Recent: [%s]%s\n", pMenu->GetItem(i)->GetText(), pMenu->GetItem(i)->IsEnabled() ? "" : " disabled");
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimApp::PrintMenus(void)
{
	Ptr<CMenuBar>		pBar;
	Ptr<CMenuContainer>	pMenu;
	Ptr<CMenuItem>		pItem;
	size				i;
	size				j;

	pBar = mpContent->GetMenuBar();
	for (i = 0; i < pBar->NumMenus(); i++)
	{
		pMenu = pBar->GetItem(i)->GetMenu();
		printf("Menu %s:", pBar->GetItem(i)->GetCaption());
		for (j = 0; j < pMenu->NumItems(); j++)
		{
			pItem = pMenu->GetItem(j);
			printf(" %s [%s]", pItem->GetText(), pItem->GetCommand().IsNotNull() ? pItem->GetCommand()->GetName() : "none");
		}
		printf("\n");
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The buttons in pComponent and its children, but not the menu bar's,
// which open menus.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimApp::CountButtons(Ptr<CComponent> pComponent, size* puiWithCommand, size* puiWithout)
{
	CButton*	pcButton;
	size		i;

	if (pComponent.IsNull())
	{
		return;
	}
	pcButton = dynamic_cast<CButton*>(&pComponent);
	if (pcButton && (dynamic_cast<CMenuBarItem*>(pcButton) == NULL))
	{
		if (pcButton->GetCommand().IsNotNull())
		{
			(*puiWithCommand)++;
		}
		else
		{
			(*puiWithout)++;
		}
	}
	for (i = 0; i < pComponent->NumComponents(); i++)
	{
		CountButtons(pComponent->GetComponent(i), puiWithCommand, puiWithout);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Runs LogiCim on the platform's window factory and input, which must be
// initialised (the app's input reads the devices from the XML in
// szInputDirectory, then adds the native input's).  The app (its window,
// content and input) is held by the root while it runs.  Validating every
// object each time one changes is too slow for the text's glyphs, so
// they're validated once at the end.
//
//////////////////////////////////////////////////////////////////////////
bool RunLogicimApp(CNativeWindowFactory* pcNativeFactory, CNativeInput* pcNativeInput, const char* szInputDirectory)
{
	Ptr<CLogicimApp>	pApp;
	bool				bResult;

	gcObjects.DisableValidation();

	pApp = OMalloc<CLogicimApp>();
	ORoot()->Add(pApp);
	bResult = pApp->Setup(pcNativeFactory, pcNativeInput, szInputDirectory);
	if (bResult)
	{
		//LOGICIM_CHECK="commands,menus,..." runs the steps (see CLogicimApp::Check) instead of the window.
		bResult = (getenv("LOGICIM_CHECK") != NULL) ? pApp->Check(getenv("LOGICIM_CHECK")) : pApp->Run();
	}
	ORoot()->Remove(pApp);
	pApp = NULL;

	gcObjects.EnableValidation();
	gcObjects.ValidateObjectsConsistency();
	return bResult;
}
