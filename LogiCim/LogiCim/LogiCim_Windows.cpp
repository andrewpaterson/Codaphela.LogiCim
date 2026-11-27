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
#ifdef _WIN32
#include <windows.h>
#include "BaseLib/LeakDetector.h"
#include "BaseLib/GlobalMemory.h"
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
	bool	bResult;

	BreakOnAllocation(0);
	InitLogicimGlobals();

	{
		Ptr<CWinGDIWindowFactory>	pNativeFactory;
		CWinInput					cWinInput;

		//Rooted while LogiCim runs.  Its windows hold it too, so it's freed after the last of them.
		pNativeFactory = OMalloc<CWinGDIWindowFactory>(&gcMemoryAllocator,
			hInstance,
			hPrevInstance,
			nCmdShow,
			"LogiCim");
		ORoot()->Add(pNativeFactory);

		cWinInput.Init(NULL, false);
		pNativeFactory->SetRawInput(WinRawInputCallback, cWinInput.GetRaw());

		bResult = RunLogicimApp(&pNativeFactory, &cWinInput, "../../../CommonLibraries/Libraries/WinInputLib/xml");

		pNativeFactory->SetRawInput(NULL, NULL);
		cWinInput.Kill();
		ORoot()->Remove(pNativeFactory);
		pNativeFactory = NULL;
	}

	KillLogicimGlobals();
	LeakDetection();
	return bResult ? 0 : 1;
}


#endif // _WIN32

