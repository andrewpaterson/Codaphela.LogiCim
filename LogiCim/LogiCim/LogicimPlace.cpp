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
#include "BaseLib/StringHelper.h"
#include "LogicimPlace.h"


//////////////////////////////////////////////////////////////////////////
//
// Nowhere: no circuit, fitted when shown.
//
//////////////////////////////////////////////////////////////////////////
void SLogicimPlace::Init(void)
{
	Init(NULL, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit at szPath (NULL or empty for the top) in szCircuit,
// fitted to the canvas when it's shown.
//
//////////////////////////////////////////////////////////////////////////
void SLogicimPlace::Init(const char* szCircuit, const char* szPath)
{
	this->szCircuit.Init(StrEmpty(szCircuit) ? "" : szCircuit);
	this->szPath.Init(StrEmpty(szPath) ? "" : szPath);
	sPosition.Init(0, 0);
	fZoom = 0;
}


void SLogicimPlace::Kill(void)
{
	szPath.Kill();
	szCircuit.Kill();
}


void SLogicimPlace::Copy(SLogicimPlace* psSource)
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
bool SLogicimPlace::IsSameView(SLogicimPlace* psOther)
{
	return szCircuit.Equals(psOther->szCircuit.Text()) && szPath.Equals(psOther->szPath.Text());
}


//////////////////////////////////////////////////////////////////////////
//
// For checking: "Main 0/1", or "Main" at the top.
//
//////////////////////////////////////////////////////////////////////////
void SLogicimPlace::Print(CChars* pszDest)
{
	pszDest->Append(szCircuit.Text());
	if (!szPath.Empty())
	{
		pszDest->Append(" ");
		pszDest->Append(szPath.Text());
	}
}

