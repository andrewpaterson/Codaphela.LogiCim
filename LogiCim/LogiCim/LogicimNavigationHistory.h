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
#ifndef __LOGICIM_NAVIGATION_HISTORY_H__
#define __LOGICIM_NAVIGATION_HISTORY_H__
#include "BaseLib/ArrayTemplate.h"
#include "LogicimPlace.h"


#define LOGICIM_NAVIGATION_HISTORY_SIZE	50  //Places kept back, the oldest forgotten first.


typedef CArrayTemplate<SLogicimPlace>	CArrayLogicimPlace;


//The places left, for Back, and the places gone back from, for Forward, the latest last.  A move records the place it
//left (Record), which forgets those gone back from; Back takes the latest place left and puts the place it leaves on
//Forward's side, and Forward the other way.  The places are copies: one that's gone by the time it's taken is for
//whoever takes it to skip.
class CLogicimNavigationHistory
{
protected:
	CArrayLogicimPlace	masBack;
	CArrayLogicimPlace	masForward;

public:
	void	Init(void);
	void	Kill(void);
	void	Clear(void);

	void	Record(SLogicimPlace* psLeft);
	bool	Take(bool bBack, SLogicimPlace* psDest);
	void	Put(bool bBack, SLogicimPlace* psPlace);
	size	Num(bool bBack);
	void	Print(bool bBack, CChars* pszDest);

protected:
	CArrayLogicimPlace*	GetSide(bool bBack);
	void				ClearSide(CArrayLogicimPlace* pasPlaces);
};


#endif // __LOGICIM_NAVIGATION_HISTORY_H__

