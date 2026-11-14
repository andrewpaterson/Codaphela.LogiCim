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
#include "BaseLib/StringHelper.h"
#include "CircuitPlace.h"


//////////////////////////////////////////////////////////////////////////
//
// Nowhere: no circuit, fitted when shown.
//
//////////////////////////////////////////////////////////////////////////
void SCircuitPlace::Init(void)
{
	Init(NULL, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit at szPath (NULL or empty for the top) in szCircuit,
// fitted to the canvas when it's shown.
//
//////////////////////////////////////////////////////////////////////////
void SCircuitPlace::Init(const char* szCircuit, const char* szPath)
{
	this->szCircuit.Init(StrEmpty(szCircuit) ? "" : szCircuit);
	this->szPath.Init(StrEmpty(szPath) ? "" : szPath);
	sPosition.Init(0, 0);
	fZoom = 0;
}


void SCircuitPlace::Kill(void)
{
	szPath.Kill();
	szCircuit.Kill();
}


void SCircuitPlace::Copy(SCircuitPlace* psSource)
{
	szCircuit.Set(psSource->szCircuit.Text());
	szPath.Set(psSource->szPath.Text());
	sPosition = psSource->sPosition;
	fZoom = psSource->fZoom;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether both are the same circuit along the same path, however they're
// zoomed.
//
//////////////////////////////////////////////////////////////////////////
bool SCircuitPlace::IsSameView(SCircuitPlace* psOther)
{
	return szCircuit.Equals(psOther->szCircuit.Text()) && szPath.Equals(psOther->szPath.Text());
}


//////////////////////////////////////////////////////////////////////////
//
// For checking: "Main 0/1", or "Main" at the top.
//
//////////////////////////////////////////////////////////////////////////
void SCircuitPlace::Print(CChars* pszDest)
{
	pszDest->Append(szCircuit.Text());
	if (!szPath.Empty())
	{
		pszDest->Append(" ");
		pszDest->Append(szPath.Text());
	}
}


//////////////////////////////////////////////////////////////////////////
//
// pasDest (initialised) has copies of pasSource's places, and none of its
// own.  They mustn't be the same array.
//
//////////////////////////////////////////////////////////////////////////
void CopyCircuitPlaces(CArrayCircuitPlace* pasDest, CArrayCircuitPlace* pasSource)
{
	size			i;
	SCircuitPlace*	psPlace;

	ClearCircuitPlaces(pasDest);
	for (i = 0; i < pasSource->NumElements(); i++)
	{
		psPlace = pasDest->Add();
		psPlace->Init();
		psPlace->Copy(pasSource->Get(i));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each place killed and taken out; the array can still be added to.
//
//////////////////////////////////////////////////////////////////////////
void ClearCircuitPlaces(CArrayCircuitPlace* pasPlaces)
{
	size	i;

	for (i = 0; i < pasPlaces->NumElements(); i++)
	{
		pasPlaces->Get(i)->Kill();
	}
	pasPlaces->ReInit();
}

