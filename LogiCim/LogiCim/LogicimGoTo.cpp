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
#include "BaseLib/ArrayElementNotFound.h"
#include "LogicimLib/CircuitEditor.h"
#include "LogicimCircuit.h"
#include "LogicimWindowContent.h"
#include "LogicimGoTo.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SLogicimGoToTarget::Init(const char* szCircuit, const char* szPath, size uiComponent)
{
	this->szCircuit.Init(szCircuit);
	this->szPath.Init(szPath);
	this->uiComponent = uiComponent;
}


void SLogicimGoToTarget::Kill(void)
{
	szPath.Kill();
	szCircuit.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// pcCircuit isn't held; it must outlive the searcher.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimGoTo::Init(CLogicimCircuit* pcCircuit)
{
	mpcCircuit = pcCircuit;
	masTargets.Init();
}


void CLogicimGoTo::Kill(void)
{
	ClearTargets();
	masTargets.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The tab's results for the pattern, each with its target (GetTarget).
//
//////////////////////////////////////////////////////////////////////////
void CLogicimGoTo::GoToSearch(size uiTab, CNameMatcher* pcMatcher, CGoToItems* pcDest)
{
	CCircuitViews*	pcCircuits;
	CCircuitView*	pcTop;
	size			i;

	ClearTargets();
	pcCircuits = mpcCircuit->GetEditor()->GetCircuits();
	if (pcCircuits == NULL)
	{
		return;
	}
	if (uiTab == LGT_Subcircuits)
	{
		for (i = 0; i < pcCircuits->NumCircuits(); i++)
		{
			AddResult(pcMatcher, pcDest, pcCircuits->Get(i)->GetTypeName(), "", pcCircuits->Get(i)->GetTypeName(), "", ARRAY_ELEMENT_NOT_FOUND);
		}
		return;
	}
	pcTop = mpcCircuit->GetEditor()->GetTopCircuit();
	if (pcTop != NULL)
	{
		SearchCircuit(uiTab, pcMatcher, pcDest, pcTop, "", pcTop->GetTypeName());
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The components (or nets) in pcCircuit, at szPath, shown as being at
// szLocation, and those in its instances.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimGoTo::SearchCircuit(size uiTab, CNameMatcher* pcMatcher, CGoToItems* pcDest, CCircuitView* pcCircuit, const char* szPath, const char* szLocation)
{
	size				i;
	CComponentView*		pcComponent;
	CCircuitView*		pcSubcircuit;
	CChars				szName;
	CChars				szChildPath;
	CChars				szChildLocation;
	const char*			szTop;

	szTop = mpcCircuit->GetEditor()->GetTopCircuit()->GetTypeName();
	for (i = 0; i < pcCircuit->NumComponents(); i++)
	{
		pcComponent = pcCircuit->GetComponent(i);
		szName.Init(pcComponent->GetName());
		if (uiTab == LGT_Components)
		{
			if (!szName.Empty())
			{
				szName.Append(' ');
			}
			pcComponent->AppendPartName(&szName);
			AddResult(pcMatcher, pcDest, szName.Text(), szLocation, szTop, szPath, i);
		}
		else if (IsLogicimNet(pcComponent) && !szName.Empty())
		{
			AddResult(pcMatcher, pcDest, szName.Text(), szLocation, szTop, szPath, i);
		}
		szName.Kill();

		pcSubcircuit = GetInstanceCircuit(pcComponent);
		if (pcSubcircuit != NULL)
		{
			szChildPath.Init(szPath);
			if (!StrEmpty(szPath))
			{
				szChildPath.Append('/');
			}
			szChildPath.Append((int64)i);
			szChildLocation.Init(szLocation);
			szChildLocation.Append(" > ");
			szChildLocation.Append(pcComponent->GetName());
			SearchCircuit(uiTab, pcMatcher, pcDest, pcSubcircuit, szChildPath.Text(), szChildLocation.Text());
			szChildLocation.Kill();
			szChildPath.Kill();
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A result for szName if the pattern matches it.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimGoTo::AddResult(CNameMatcher* pcMatcher, CGoToItems* pcDest, const char* szName, const char* szLocation, const char* szCircuit, const char* szPath, size uiComponent)
{
	SNameMatch	sMatch;

	if (pcMatcher->Match(szName, &sMatch))
	{
		pcDest->Add(szName, szLocation, -1, masTargets.NumElements(), &sMatch);
		masTargets.Add()->Init(szCircuit, szPath, uiComponent);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Where the result found by the last search is, NULL if it isn't one.
//
//////////////////////////////////////////////////////////////////////////
SLogicimGoToTarget* CLogicimGoTo::GetTarget(SGoToItem* psItem)
{
	return ((psItem != NULL) && (psItem->uiData < masTargets.NumElements())) ? masTargets.Get(psItem->uiData) : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimGoTo::ClearTargets(void)
{
	size	i;

	for (i = 0; i < masTargets.NumElements(); i++)
	{
		masTargets.Get(i)->Kill();
	}
	masTargets.ReInit();
}


//////////////////////////////////////////////////////////////////////////
//
// A component that names a net: a tunnel or a label.
//
//////////////////////////////////////////////////////////////////////////
bool IsLogicimNet(CComponentView* pcComponent)
{
	return (StringCompare(pcComponent->GetXMLName(), "Tunnel") == 0) || (StringCompare(pcComponent->GetXMLName(), "Label") == 0);
}

