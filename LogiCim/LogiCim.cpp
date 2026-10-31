#include <windows.h>
#include "BaseLib/Logger.h"
#include "BaseLib/FileUtil.h"
#include "BaseLib/TypeNames.h"
#include "BaseLib/TypeConverter.h"
#include "BaseLib/GlobalMemory.h"
#include "BaseLib/GlobalDataTypesIO.h"
#include "BaseLib/DebugOutput.h"
#include "StandardLib/Objects.h"
#include "InputLib/Input.h"
#include "WindowLib/Window.h"
#include "WindowLib/ClearCanvasDraw.h"
#include "WindowLib/JewelColours.h"
#include "WindowLib/ComponentInput.h"
#include "WinGdiLib/WinGDIWindowFactory.h"
#include "WinInputLib/WinInput.h"
#include "LogicimWindowTick.h"
#include "LogicimWindowContent.h"


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
		CWinGDIWindowFactory	cNativeFactory;
		Ptr<CWindow>			pLogicimWindow;
		Ptr<CLogicimWindowTick>	pTick;
		SLogicimWindowData		sData;
		CLogicimWindowContent	cContent;
		CInput					cInput;
		CWinInput				cWinInput;
		Ptr<CWindowInputSource>	pInputSource;
		CComponentInput			cComponentInput;

		cNativeFactory.Init(&gcMemoryAllocator,
			hInstance,
			hPrevInstance,
			nCmdShow,
			"LogiCim");

		//Validating every object each time one changes is too slow for the text's glyphs.  They are validated once at the end.
		gcObjects.DisableValidation();

		//The window is in the root so that changing pointers into its components doesn't make the objects work out again which
		//of them are still on the stack.  Its tick is allocated, not on the stack, because stack objects can't be under the root.
		pTick = OMalloc<CLogicimWindowTick>(&sData);
		pLogicimWindow = OMalloc<CWindow>("LogiCim", &cNativeFactory, pTick, OMalloc<CClearCanvasDraw>(JEWEL_PANEL), CFT_RGBX, CCO_BGR, CRGB_24bit, ARGB_8bit);
		ORoot()->Add(pLogicimWindow);

		if (cContent.Init(pLogicimWindow, &sData, "Resources/InterVariable.ttf", "Resources/Icons", "Resources/PropagationTimes.csv", "Resources/Fonts"))
		{
			//The devices are described by the XML so they are added after it is read.
			cInput.Init(NULL);
			cInput.Read("../../CommonLibraries/WinInputLib/xml");
			cWinInput.Init(NULL, false);
			cInput.AddNativeInput(&cWinInput);
			cNativeFactory.SetRawInput(WinRawInputCallback, cWinInput.GetRaw());

			pInputSource = OMalloc<CWindowInputSource>(&cInput);
			cComponentInput.Init(&pInputSource, &pLogicimWindow);
			cComponentInput.AddKeyboards();
			cComponentInput.AddMice();
			pLogicimWindow->SetComponentInput(&cComponentInput);

			pLogicimWindow->SetWindowSize(1280, 800);
			pLogicimWindow->Show();

			pLogicimWindow->SetComponentInput(NULL);
			cNativeFactory.SetRawInput(NULL, NULL);
			cComponentInput.Kill();
			pInputSource = NULL;
			cInput.Kill();
			cWinInput.Kill();
		}
		else
		{
			bResult = false;
		}

		cContent.Kill();
		ORoot()->Remove(pLogicimWindow);
		pLogicimWindow = NULL;
		pTick = NULL;
		ORoot()->RemoveAll();

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

