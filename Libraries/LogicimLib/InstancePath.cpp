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
#include <stdlib.h>
#include "BaseLib/StringHelper.h"
#include "BaseLib/ArrayElementNotFound.h"
#include "SubcircuitInstanceView.h"
#include "CircuitView.h"
#include "InstancePath.h"


//////////////////////////////////////////////////////////////////////////
//
// The step to the instance at uiIndex named szName, after a slash if the
// path has steps already.
//
//////////////////////////////////////////////////////////////////////////
void AppendInstancePathStep(CChars* pszPath, size uiIndex, const char* szName)
{
	const char*	sz;

	if (!pszPath->Empty())
	{
		pszPath->Append('/');
	}
	pszPath->Append((int64)uiIndex);
	if (!StrEmpty(szName))
	{
		pszPath->Append(':');
		for (sz = szName; *sz != '\0'; sz++)
		{
			if ((*sz == '/') || (*sz == '\\'))
			{
				pszPath->Append('\\');
			}
			pszPath->Append(*sz);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The path of the instances down from pcTop, as they are now.
//
//////////////////////////////////////////////////////////////////////////
void AppendInstancePathText(CChars* pszPath, CCircuitView* pcTop, CArrayCircuitInstanceViewPtr* papcPath)
{
	size				i;
	CCircuitView*		pcCircuit;
	CComponentView*		pcInstance;

	pcCircuit = pcTop;
	for (i = 0; (i < papcPath->NumElements()) && (pcCircuit != NULL); i++)
	{
		pcInstance = GetInstanceComponent(papcPath->GetPtr(i));
		AppendInstancePathStep(pszPath, pcCircuit->FindComponentIndex(pcInstance), pcInstance->GetName());
		pcCircuit = papcPath->GetPtr(i)->GetInstanceCircuitView();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A subcircuit instance's own component view.
//
//////////////////////////////////////////////////////////////////////////
CComponentView* GetInstanceComponent(CCircuitInstanceView* pcInstance)
{
	return (CComponentView*)(CSubcircuitInstanceView*)pcInstance;
}


//////////////////////////////////////////////////////////////////////////
//
// The instance in pcCircuit a step names: the only one of its name, or of
// several the one at its index; with no name, the one at its index.  NULL
// if there's none.
//
//////////////////////////////////////////////////////////////////////////
static CCircuitInstanceView* FindInstanceStep(CCircuitView* pcCircuit, long lIndex, const char* szName)
{
	size					i;
	CCircuitInstanceView*	pcFound;
	CCircuitInstanceView*	pcInstance;
	CComponentView*			pcComponent;

	if (szName == NULL)
	{
		if ((lIndex < 0) || ((size)lIndex >= pcCircuit->NumComponents()))
		{
			return NULL;
		}
		pcInstance = pcCircuit->GetComponent((size)lIndex)->GetCircuitInstanceView();
		return ((pcInstance != NULL) && (pcInstance->GetInstanceCircuitView() != NULL)) ? pcInstance : NULL;
	}

	pcFound = NULL;
	for (i = 0; i < pcCircuit->NumComponents(); i++)
	{
		pcComponent = pcCircuit->GetComponent(i);
		pcInstance = pcComponent->GetCircuitInstanceView();
		if ((pcInstance != NULL) && (pcInstance->GetInstanceCircuitView() != NULL) && (StringCompare(pcComponent->GetName(), szName) == 0))
		{
			if ((long)i == lIndex)
			{
				return pcInstance;
			}
			if (pcFound == NULL)
			{
				pcFound = pcInstance;
			}
		}
	}
	return pcFound;
}


//////////////////////////////////////////////////////////////////////////
//
// Follows szPath down from pcTop as far as its steps find instances,
// adding each to papcPath.  True if they all did.
//
//////////////////////////////////////////////////////////////////////////
static bool WalkInstancePath(CCircuitView* pcTop, const char* szPath, CArrayCircuitInstanceViewPtr* papcPath)
{
	CCircuitView*			pcCircuit;
	const char*				sz;
	char*					szEnd;
	long					lIndex;
	CChars					szName;
	bool					bNamed;
	CCircuitInstanceView*	pcInstance;

	pcCircuit = pcTop;
	sz = szPath;
	while (*sz != '\0')
	{
		lIndex = strtol(sz, &szEnd, 10);
		if (szEnd == sz)
		{
			return false;
		}
		sz = szEnd;
		szName.Init();
		bNamed = (*sz == ':');
		if (bNamed)
		{
			for (sz++; (*sz != '\0') && (*sz != '/'); sz++)
			{
				if ((*sz == '\\') && (sz[1] != '\0'))
				{
					sz++;
				}
				szName.Append(*sz);
			}
		}
		pcInstance = ((*sz == '/') || (*sz == '\0')) ? FindInstanceStep(pcCircuit, lIndex, bNamed ? szName.Text() : NULL) : NULL;
		szName.Kill();
		if (pcInstance == NULL)
		{
			return false;
		}
		papcPath->Add(pcInstance);
		pcCircuit = pcInstance->GetInstanceCircuitView();
		if (*sz == '/')
		{
			sz++;
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit at the end of szPath from pcTop, and the instances on it;
// NULL if the path doesn't find them all.
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* FindInstancePath(CCircuitView* pcTop, const char* szPath, CArrayCircuitInstanceViewPtr* papcPath)
{
	if (!WalkInstancePath(pcTop, szPath, papcPath))
	{
		return NULL;
	}
	return (papcPath->NumElements() > 0) ? papcPath->GetPtr(papcPath->NumElements() - 1)->GetInstanceCircuitView() : pcTop;
}


//////////////////////////////////////////////////////////////////////////
//
// szPath as it is now, in pszDest: as far down as its instances are
// still there, each step's index where the instance is now.  True if
// they're all there.
//
//////////////////////////////////////////////////////////////////////////
bool ResolveInstancePath(CCircuitView* pcTop, const char* szPath, CChars* pszDest)
{
	CArrayCircuitInstanceViewPtr	apcPath;
	bool							bWhole;

	apcPath.Init();
	bWhole = WalkInstancePath(pcTop, szPath, &apcPath);
	AppendInstancePathText(pszDest, pcTop, &apcPath);
	apcPath.Kill();
	return bWhole;
}

