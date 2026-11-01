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
#ifndef __LOGICIM_WINDOW_TICK_H__
#define __LOGICIM_WINDOW_TICK_H__
#include "WindowLib/WindowTick.h"
#include "LogicimWindowData.h"


//Each tick ticks the window content (running its simulation) and paints the window, then shows the dialog the content asked for
//(see SLogicimWindowData).  A snapshot (SetSnapshot) saves the window as a PNG after a number of ticks and closes it.
class CLogicimWindowTick : public CWindowTick
{
CONSTRUCTABLE(CLogicimWindowTick);
DESTRUCTABLE(CLogicimWindowTick);
protected:
	SLogicimWindowData*	mpsData;
	CChars				mszSnapshot;
	int					miSnapshotTicks;

public:
	void	Init(SLogicimWindowData* pcData);
	void	Class(void) override;
	void	Free(void) override;
	void	SetSnapshot(const char* szFilename, int iTicks);
	void	Tick(Ptr<CWindow> pWindow, int64 iUpdateTimeInMillieconds, int64 iTotalTimeInMillieconds) override;
};


#endif // __LOGICIM_WINDOW_TICK_H__

