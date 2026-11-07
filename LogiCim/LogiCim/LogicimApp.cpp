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
#include "WindowLib/JewelColours.h"
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

	pTick = OMalloc<CLogicimWindowTick>(&msData);
	if (getenv("LOGICIM_SNAPSHOT") != NULL)
	{
		pTick->SetSnapshot(getenv("LOGICIM_SNAPSHOT"), (getenv("LOGICIM_SNAPSHOT_TICKS") != NULL) ? atoi(getenv("LOGICIM_SNAPSHOT_TICKS")) : 60);
	}
	mpWindow = OMalloc<CWindow>("LogiCim", pcNativeFactory, pTick, OMalloc<CClearCanvasDraw>(JEWEL_PANEL), CFT_RGBX, CCO_BGR, CRGB_24bit, ARGB_8bit);

	mpContent = OMalloc<CLogicimWindowContent>();
	mbValid = mpContent->Setup(mpWindow, &msData, "Resources/InterVariable.ttf", "Resources/Icons", "Resources/PropagationTimes.csv", "Resources/Fonts");
	if (mbValid)
	{
		mpInput = OMalloc<CInput>((CNativeInput*)NULL);
		mbValid = mpInput->Read((char*)szInputDirectory);
		mpInput->AddNativeInput(pcNativeInput);
		mpInputSource = OMalloc<CWindowInputSource>(mpInput);
		mcComponentInput.Init(&mpInputSource, &mpWindow);
		mbInputs = true;
		mcComponentInput.AddKeyboards();
		mcComponentInput.AddMice();
		mpWindow->SetComponentInput(&mcComponentInput);
		mpWindow->SetWindowSize(1280, 800);
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
	bResult = pApp->Setup(pcNativeFactory, pcNativeInput, szInputDirectory) && pApp->Run();
	ORoot()->Remove(pApp);
	pApp = NULL;

	gcObjects.EnableValidation();
	gcObjects.ValidateObjectsConsistency();
	return bResult;
}
