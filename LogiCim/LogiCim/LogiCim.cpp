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
#include <windows.h>
#include "BaseLib/Logger.h"
#include "BaseLib/FileUtil.h"
#include "BaseLib/TypeNames.h"
#include "BaseLib/TypeConverter.h"
#include "BaseLib/GlobalMemory.h"
#include "BaseLib/GlobalDataTypesIO.h"
#include "BaseLib/DebugOutput.h"
#include "StandardLib/Objects.h"
#include "WinGdiLib/WinGDIWindowFactory.h"
#include "WinInputLib/WinInput.h"
#include "LogicimApp.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int PASCAL WinMain(HINSTANCE hInstance,	HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	_CrtSetBreakAlloc(0);

	gcLogger.Init();
	TypesInit();
	TypeConverterInit();
	MemoryInit();
	DataIOInit();
	ObjectsInit();

	bool bResult;

	bResult = true;

	{
		CWinGDIWindowFactory		cNativeFactory;
		CWinInput					cWinInput;
		Ptr<CLogicimApp>			pApp;

		cNativeFactory.Init(&gcMemoryAllocator,
			hInstance,
			hPrevInstance,
			nCmdShow,
			"LogiCim");

		//Validating every object each time one changes is too slow for the text's glyphs.  They are validated once at the end.
		gcObjects.DisableValidation();

		//The app's input reads the devices from the XML, then adds these.
		cWinInput.Init(NULL, false);
		cNativeFactory.SetRawInput(WinRawInputCallback, cWinInput.GetRaw());

		//The app (its window, content and input) is held by the root while it runs.
		pApp = OMalloc<CLogicimApp>();
		ORoot()->Add(pApp);
		bResult = pApp->Setup(&cNativeFactory, &cWinInput, "../../../CommonLibraries/Libraries/WinInputLib/xml") && pApp->Run();
		ORoot()->Remove(pApp);
		pApp = NULL;

		cNativeFactory.SetRawInput(NULL, NULL);
		cWinInput.Kill();

		gcObjects.EnableValidation();
		gcObjects.ValidateObjectsConsistency();

		cNativeFactory.Kill();
	}


	ObjectsFlush();
	ObjectsKill();
	DataIOKill();
	MemoryKill();
	TypeConverterKill();
	TypesKill();

	gcLogger.Kill();

	_CrtDumpMemoryLeaks();

	if (bResult)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

