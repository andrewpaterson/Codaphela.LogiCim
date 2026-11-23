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
#include "LogicimNavigationHistory.h"


//////////////////////////////////////////////////////////////////////////
//
// Each place Killed and the side emptied.
//
//////////////////////////////////////////////////////////////////////////
static void ClearPlaces(CQueueCircuitPlace* pasPlaces)
{
	size	i;

	for (i = 0; i < pasPlaces->NumElements(); i++)
	{
		pasPlaces->Get(i)->Kill();
	}
	pasPlaces->ReInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimNavigationHistory::Init(void)
{
	masBack.Init();
	masForward.Init();
}


void CLogicimNavigationHistory::Kill(void)
{
	Clear();
	masForward.Kill();
	masBack.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Nowhere to go back or forward to, as for a project just opened.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimNavigationHistory::Clear(void)
{
	ClearPlaces(&masBack);
	ClearPlaces(&masForward);
}


//////////////////////////////////////////////////////////////////////////
//
// A move left psLeft: it's where Back goes first, and there's nothing to
// go forward to.  Leaving the place Back would go to anyway keeps it once,
// as it was last shown.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimNavigationHistory::Record(SCircuitPlace* psLeft)
{
	SCircuitPlace*	psLast;

	ClearPlaces(&masForward);
	psLast = masBack.Back();
	if ((psLast != NULL) && psLast->IsSameView(psLeft))
	{
		psLast->Copy(psLeft);
		return;
	}
	Put(true, psLeft);
}


//////////////////////////////////////////////////////////////////////////
//
// The latest place on Back's side (bBack) or Forward's, taken off it.
// False if there's none.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimNavigationHistory::Take(bool bBack, SCircuitPlace* psDest)
{
	CQueueCircuitPlace*	pasPlaces;
	SCircuitPlace*		psLast;

	pasPlaces = GetSide(bBack);
	psLast = pasPlaces->Back();
	if (psLast == NULL)
	{
		return false;
	}
	psDest->Copy(psLast);
	psLast->Kill();
	pasPlaces->RemoveBack();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// psPlace goes on Back's side (bBack) or Forward's, as its latest, the
// oldest forgotten if there are too many.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimNavigationHistory::Put(bool bBack, SCircuitPlace* psPlace)
{
	CQueueCircuitPlace*	pasPlaces;
	SCircuitPlace*		psNew;

	pasPlaces = GetSide(bBack);
	if (pasPlaces->NumElements() >= LOGICIM_NAVIGATION_HISTORY_SIZE)
	{
		pasPlaces->Front()->Kill();
		pasPlaces->Pop();
	}
	psNew = pasPlaces->Push();
	psNew->Init();
	psNew->Copy(psPlace);
}


size CLogicimNavigationHistory::Num(bool bBack) { return GetSide(bBack)->NumElements(); }
CQueueCircuitPlace* CLogicimNavigationHistory::GetSide(bool bBack) { return bBack ? &masBack : &masForward; }


//////////////////////////////////////////////////////////////////////////
//
// For checking: a side's places, the latest first, separated by commas.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimNavigationHistory::Print(bool bBack, CChars* pszDest)
{
	CQueueCircuitPlace*	pasPlaces;
	size				i;

	pasPlaces = GetSide(bBack);
	for (i = pasPlaces->NumElements(); i > 0; i--)
	{
		if (i < pasPlaces->NumElements())
		{
			pszDest->Append(", ");
		}
		pasPlaces->Get(i - 1)->Print(pszDest);
	}
}

