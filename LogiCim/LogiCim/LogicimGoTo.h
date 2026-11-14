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
#ifndef __LOGICIM_GO_TO_H__
#define __LOGICIM_GO_TO_H__
#include "BaseLib/ArrayTemplate.h"
#include "WindowLib/GoToDialog.h"


class CLogicimCircuit;
class CCircuitView;
class CComponentView;


//Go To's tabs.
enum ELogicimGoToTab
{
	LGT_Components,
	LGT_Nets,
	LGT_Subcircuits,
	NUM_LOGICIM_GO_TO_TABS
};


//Where a result is: the path into the circuit simulated (as CCircuitEditor::GetInstancePathText writes it) and the
//component's index in the circuit there; for a subcircuit, the circuit itself, at its top.
struct SLogicimGoToTarget
{
	CChars	szCircuit;
	CChars	szPath;
	size	uiComponent;  //ARRAY_ELEMENT_NOT_FOUND for a subcircuit.

	void	Init(const char* szCircuit, const char* szPath, size uiComponent);
	void	Kill(void);
};


typedef CArrayTemplate<SLogicimGoToTarget>	CArrayLogicimGoToTarget;


//What Go To finds in a project: every component in the circuit simulated and in each of its instances, by its name
//and what it is ("U12 74HC00", "U3 Board"), with where it is after it ("Main > U1 > U3"); every tunnel and label, by
//its name; and every circuit, by its name.  A component inside two instances is found twice, once in each.
class CLogicimGoTo : public CGoToSearcher
{
protected:
	CLogicimCircuit*		mpcCircuit;  //Not held.
	CArrayLogicimGoToTarget	masTargets;  //The last search's results', by their data.

public:
	void				Init(CLogicimCircuit* pcCircuit);
	void				Kill(void);

	void				GoToSearch(size uiTab, CNameMatcher* pcMatcher, CGoToItems* pcDest) override;
	SLogicimGoToTarget*	GetTarget(SGoToItem* psItem);

protected:
	void				SearchCircuit(size uiTab, CNameMatcher* pcMatcher, CGoToItems* pcDest, CCircuitView* pcCircuit, const char* szPath, const char* szLocation);
	void				AddResult(CNameMatcher* pcMatcher, CGoToItems* pcDest, const char* szName, const char* szLocation, const char* szCircuit, const char* szPath, size uiComponent);
	void				ClearTargets(void);
};


bool	IsLogicimNet(CComponentView* pcComponent);


#endif // __LOGICIM_GO_TO_H__

