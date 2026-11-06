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
#include "BaseLib/GlobalMemory.h"
#include "X11Lib/X11WindowFactory.h"
#include "X11Lib/X11Input.h"
#include "LogicimApp.h"


//////////////////////////////////////////////////////////////////////////
//
// LogiCim's window on Linux.  Run from the LogiCim directory, the
// resources and WinInputLib's XML are found relative to it.
//
//////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	bool	bResult;

	InitLogicimGlobals();

	{
		CX11WindowFactory			cNativeFactory;
		CX11Input					cX11Input;

		bResult = cNativeFactory.Init(&gcMemoryAllocator, NULL);
		if (bResult)
		{
			cX11Input.Init();
			cNativeFactory.SetInput(&cX11Input);

			bResult = RunLogicimApp(&cNativeFactory, &cX11Input, "../../../commonlibraries/Libraries/WinInputLib/xml");

			cNativeFactory.SetInput(NULL);
			cX11Input.Kill();
		}
		cNativeFactory.Kill();
	}

	KillLogicimGlobals();
	return bResult ? 0 : 1;
}

