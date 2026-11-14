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
#ifndef __LOGICIM_WINDOW_DATA_H__
#define __LOGICIM_WINDOW_DATA_H__
#include "BaseLib/PrimitiveTypes.h"


//Dialogs are shown from the window's tick, not from the listener of the button or menu item that asked for them,
//because a modal dialog reads the input itself.
enum ELogicimDialog
{
	LDG_None,
	LDG_Properties,
	LDG_ChooseComponent,
	LDG_NewSubcircuit,
	LDG_NewSimulation,
	LDG_SelectZoom,
	LDG_NewProject,
	LDG_LoadProject,
	LDG_SaveProject,
	LDG_ImportKicadSchematic,
};


class CLogicimWindowContent;


//What the window's tick needs from the window content: the content to tick and the dialog it has asked for.
struct SLogicimWindowData
{
	CLogicimWindowContent*	pcContent;
	ELogicimDialog			eDialog;  //The dialog to show next tick.
};


#endif // __LOGICIM_WINDOW_DATA_H__

