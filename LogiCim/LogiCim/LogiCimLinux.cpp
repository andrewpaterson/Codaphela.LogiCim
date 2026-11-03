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
#include "BaseLib/TypeNames.h"
#include "BaseLib/TypeConverter.h"
#include "BaseLib/GlobalMemory.h"
#include "BaseLib/GlobalDataTypesIO.h"
#include "StandardLib/Objects.h"
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
	gcLogger.Init();
	TypesInit();
	TypeConverterInit();
	MemoryInit();
	DataIOInit();
	ObjectsInit();

	bool bResult;

	{
		CX11WindowFactory			cNativeFactory;
		CX11Input					cX11Input;
		Ptr<CLogicimApp>			pApp;

		bResult = cNativeFactory.Init(&gcMemoryAllocator, NULL);
		if (bResult)
		{
			//Validating every object each time one changes is too slow for the text's glyphs.  They are validated once at the end.
			gcObjects.DisableValidation();

			//The app's input reads the devices from the XML, then adds these.
			cX11Input.Init();
			cNativeFactory.SetInput(&cX11Input);

			//The app (its window, content and input) is held by the root while it runs.
			pApp = OMalloc<CLogicimApp>(&cNativeFactory, &cX11Input, "../../../commonlibraries/Libraries/WinInputLib/xml");
			ORoot()->Add(pApp);
			bResult = pApp->Run();
			ORoot()->Remove(pApp);
			pApp = NULL;

			cNativeFactory.SetInput(NULL);
			cX11Input.Kill();

			gcObjects.EnableValidation();
			gcObjects.ValidateObjectsConsistency();
		}

		cNativeFactory.Kill();
	}

	ObjectsFlush();
	ObjectsKill();
	DataIOKill();
	MemoryKill();
	TypeConverterKill();
	TypesKill();

	gcLogger.Kill();

	if (bResult)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

