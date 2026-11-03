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
#include "InputLib/Input.h"
#include "WindowLib/Window.h"
#include "WindowLib/ClearCanvasDraw.h"
#include "WindowLib/JewelColours.h"
#include "WindowLib/ComponentInput.h"
#include "X11Lib/X11WindowFactory.h"
#include "X11Lib/X11Input.h"
#include "LogicimWindowTick.h"
#include "LogicimWindowContent.h"


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
		Ptr<CWindow>				pLogicimWindow;
		Ptr<CLogicimWindowTick>		pTick;
		SLogicimWindowData			sData;
		Ptr<CLogicimWindowContent>	pContent;
		CInput						cInput;
		CX11Input					cX11Input;
		Ptr<CWindowInputSource>		pInputSource;
		CComponentInput				cComponentInput;

		bResult = cNativeFactory.Init(&gcMemoryAllocator, NULL);
		if (bResult)
		{
			//Validating every object each time one changes is too slow for the text's glyphs.  They are validated once at the end.
			gcObjects.DisableValidation();

			//The window is in the root so that changing pointers into its components doesn't make the objects work out again which
			//of them are still on the stack.  Its tick is allocated, not on the stack, because stack objects can't be under the root.
			pTick = OMalloc<CLogicimWindowTick>(&sData);
			if (getenv("LOGICIM_SNAPSHOT") != NULL)
			{
				pTick->SetSnapshot(getenv("LOGICIM_SNAPSHOT"), (getenv("LOGICIM_SNAPSHOT_TICKS") != NULL) ? atoi(getenv("LOGICIM_SNAPSHOT_TICKS")) : 60);
			}
			pLogicimWindow = OMalloc<CWindow>("LogiCim", &cNativeFactory, pTick, OMalloc<CClearCanvasDraw>(JEWEL_PANEL), CFT_RGBX, CCO_BGR, CRGB_24bit, ARGB_8bit);
			ORoot()->Add(pLogicimWindow);

			pContent = OMalloc<CLogicimWindowContent>();
			ORoot()->Add(pContent);
			if (pContent->Setup(pLogicimWindow, &sData, "Resources/InterVariable.ttf", "Resources/Icons", "Resources/PropagationTimes.csv", "Resources/Fonts"))
			{
				//The devices are described by the XML so they are added after it is read.
				cInput.Init(NULL);
				cInput.Read("../../../commonlibraries/Libraries/WinInputLib/xml");
				cX11Input.Init();
				cInput.AddNativeInput(&cX11Input);
				cNativeFactory.SetInput(&cX11Input);

				pInputSource = OMalloc<CWindowInputSource>(&cInput);
				cComponentInput.Init(&pInputSource, &pLogicimWindow);
				cComponentInput.AddKeyboards();
				cComponentInput.AddMice();
				pLogicimWindow->SetComponentInput(&cComponentInput);

				pLogicimWindow->SetWindowSize(1280, 800);
				pLogicimWindow->Show();

				pLogicimWindow->SetComponentInput(NULL);
				cNativeFactory.SetInput(NULL);
				cComponentInput.Kill();
				pInputSource = NULL;
				cInput.Kill();
				cX11Input.Kill();
			}
			else
			{
				bResult = false;
			}

			ORoot()->Remove(pContent);
			pContent = NULL;
			ORoot()->Remove(pLogicimWindow);
			pLogicimWindow = NULL;
			pTick = NULL;
			ORoot()->RemoveAll();

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

