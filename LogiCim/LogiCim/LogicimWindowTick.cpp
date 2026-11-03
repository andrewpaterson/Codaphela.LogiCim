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
#include "StandardLib/ClassDefines.h"
#include "SupportLib/ColourARGB32.h"
#include "SupportLib/PNGWriter.h"
#include "WindowLib/Window.h"
#include "LogicimWindowContent.h"
#include "LogicimWindowTick.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowTick::Init(SLogicimWindowData* pcData)
{
	PreInit();

	CWindowTick::Init();
	mpsData = pcData;
	mszSnapshot.Init();
	miSnapshotTicks = -1;

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowTick::Class(void)
{
	CWindowTick::Class();
	U_Pointer(mpsData);
	U_String(mszSnapshot);
	U_Int32(miSnapshotTicks);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowTick::Free(void)
{
	mszSnapshot.Kill();
	CWindowTick::Free();
}


//////////////////////////////////////////////////////////////////////////
//
// Saves the window as a PNG after iTicks ticks and closes it, to check how
// it draws without looking at the screen.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowTick::SetSnapshot(const char* szFilename, int iTicks)
{
	mszSnapshot.Set(szFilename);
	miSnapshotTicks = iTicks;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowTick::Tick(Ptr<CWindow> pWindow, int64 iUpdateTimeInMillieconds, int64 iTotalTimeInMillieconds)
{
	ELogicimDialog	eDialog;

	if (mpsData->pcContent)
	{
		mpsData->pcContent->Tick(iUpdateTimeInMillieconds);
	}

	if (miSnapshotTicks >= 0)
	{
		if (miSnapshotTicks == 0)
		{
			SavePNG(pWindow->GetCanvas()->GetImageOrNull(), mszSnapshot.Text());
			pWindow->Stop();
		}
		else
		{
			//The window would otherwise wait for input before the next tick.
			pWindow->Invalidate();
			pWindow->RequestTick(0);
		}
		miSnapshotTicks--;
	}

	if ((mpsData->eDialog != LDG_None) && mpsData->pcContent)
	{
		eDialog = mpsData->eDialog;
		mpsData->eDialog = LDG_None;
		mpsData->pcContent->ShowDialog(eDialog);
	}
}

