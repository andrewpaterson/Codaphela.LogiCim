/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela LogicimLib

Codaphela LogicimLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela LogicimLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela LogicimLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#ifndef __INSTANCE_PATH_H__
#define __INSTANCE_PATH_H__
#include "BaseLib/Chars.h"
#include "BaseLib/ArrayTemplatePtr.h"


class CCircuitView;
class CCircuitInstanceView;
class CComponentView;


typedef CArrayTemplatePtr<CCircuitInstanceView>	CArrayCircuitInstanceViewPtr;


//A path of subcircuit instances down from a circuit, as places keep it: a step for each instance, its index in the
//circuit it's in and its name, "1:U2/1:U3", a slash or backslash in a name escaped with a backslash.  The instance is
//found by its name, so a path still finds it after other components in its circuit come and go; by its index if the
//name is in the circuit more than once, or the step has none (a path written before steps had names, "1/1").
void			AppendInstancePathStep(CChars* pszPath, size uiIndex, const char* szName);
void			AppendInstancePathText(CChars* pszPath, CCircuitView* pcTop, CArrayCircuitInstanceViewPtr* papcPath);
CCircuitView*	FindInstancePath(CCircuitView* pcTop, const char* szPath, CArrayCircuitInstanceViewPtr* papcPath);
bool			ResolveInstancePath(CCircuitView* pcTop, const char* szPath, CChars* pszDest);
CComponentView*	GetInstanceComponent(CCircuitInstanceView* pcInstance);


#endif // __INSTANCE_PATH_H__

