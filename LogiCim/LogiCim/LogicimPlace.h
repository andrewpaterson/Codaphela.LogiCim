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
#ifndef __LOGICIM_PLACE_H__
#define __LOGICIM_PLACE_H__
#include "BaseLib/Chars.h"
#include "BaseLib/Float64Vec2.h"


//Where the editor is, as Back and Forward, the tabs and Go To keep it: the circuit simulated (its type name), the path
//of instances into the circuit edited (as CCircuitEditor::GetInstancePathText writes it, empty at the top) and how the
//canvas shows it, its viewport's position and zoom.  A zoom of 0 fits the circuit to the canvas when it's shown.
struct SLogicimPlace
{
	CChars			szCircuit;
	CChars			szPath;
	SFloat64Vec2	sPosition;
	float64			fZoom;

	void	Init(void);
	void	Init(const char* szCircuit, const char* szPath);
	void	Kill(void);
	void	Copy(SLogicimPlace* psSource);
	bool	IsSameView(SLogicimPlace* psOther);
	void	Print(CChars* pszDest);
};


#endif // __LOGICIM_PLACE_H__

