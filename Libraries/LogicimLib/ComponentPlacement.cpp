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
#include "BaseLib/ConstructorCall.h"
#include "BaseLib/StringHelper.h"
#include "Logic74View.h"
#include "ComponentViewFactory.h"
#include "SubcircuitInstanceView.h"
#include "ComponentPlacement.h"




//////////////////////////////////////////////////////////////////////////
//
// As Logicim's ComponentNameHelper.getUniqueName over the views of the
// same element name: a name already used (by number) is given the next
// number after the largest used, so a second "Pin" is "Pin 1".  An empty
// name is left empty.
//
//////////////////////////////////////////////////////////////////////////
void GetUniqueComponentName(CChars* pszName, CCircuitView* pcCircuit, const char* szXMLName, const char* szName)
{
	CChars				szStem;
	int					iNumber;
	CChars				szOtherStem;
	int					iOtherNumber;
	size				i;
	CComponentView*		pcComponent;
	bool				bSameStem;
	bool				bExactNumber;
	int					iMaxNumber;

	pszName->Init(szName);
	pszName->StripWhitespace();
	if (pszName->Empty())
	{
		return;
	}

	SplitComponentName(pszName->Text(), &szStem, &iNumber);
	bSameStem = false;
	bExactNumber = false;
	iMaxNumber = 0;
	for (i = 0; i < pcCircuit->NumComponents(); i++)
	{
		pcComponent = pcCircuit->GetComponent(i);
		if (StringCompare(pcComponent->GetXMLName(), szXMLName) == 0)
		{
			SplitComponentName(pcComponent->GetName(), &szOtherStem, &iOtherNumber);
			if (szOtherStem.Equals(&szStem))
			{
				bSameStem = true;
				if (iOtherNumber == iNumber)
				{
					bExactNumber = true;
				}
				if (iOtherNumber > iMaxNumber)
				{
					iMaxNumber = iOtherNumber;
				}
			}
			szOtherStem.Kill();
		}
	}
	if (bSameStem && (bExactNumber || (iMaxNumber == 0)))
	{
		pszName->Set(szStem);
		pszName->Append(' ');
		pszName->Append(iMaxNumber + 1);
	}
	szStem.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// A name's trailing number (0 if it has none) and what comes before it,
// trimmed.
//
//////////////////////////////////////////////////////////////////////////
void SplitComponentName(const char* szName, CChars* pszStem, int* piNumber)
{
	size	uiEnd;

	pszStem->Init(szName);
	pszStem->StripWhitespace();
	uiEnd = pszStem->Length();
	while ((uiEnd > 0) && (pszStem->Text()[uiEnd - 1] >= '0') && (pszStem->Text()[uiEnd - 1] <= '9'))
	{
		uiEnd--;
	}
	*piNumber = (uiEnd < pszStem->Length()) ? atoi(&pszStem->Text()[uiEnd]) : 0;
	pszStem->SetLength(uiEnd);
	pszStem->StripWhitespace();
}


//////////////////////////////////////////////////////////////////////////
//
// The prefix and the lowest number after it that no component in the
// circuit is named (U1, U2 and so on).
//
//////////////////////////////////////////////////////////////////////////
void GetFreeReference(CChars* psz, CCircuitView* pcCircuit, const char* szPrefix)
{
	size	i;
	size	uiPrefix;
	int		iNumber;
	int		iUsed;
	char*	szName;
	char*	szEnd;

	uiPrefix = StrLen(szPrefix);
	iUsed = 0;
	if (pcCircuit != NULL)
	{
		for (i = 0; i < pcCircuit->NumComponents(); i++)
		{
			szName = pcCircuit->GetComponent(i)->GetName();
			if ((szName != NULL) && (strncmp(szName, szPrefix, uiPrefix) == 0) && (szName[uiPrefix] >= '0') && (szName[uiPrefix] <= '9'))
			{
				iNumber = (int)strtol(&szName[uiPrefix], &szEnd, 10);
				if ((*szEnd == '\0') && (iNumber > iUsed))
				{
					iUsed = iNumber;
				}
			}
		}
	}
	psz->Append(szPrefix);
	psz->Append((int32)(iUsed + 1));
}


//////////////////////////////////////////////////////////////////////////
//
// A real 74 series part ("Logic74Package:00") or its first unit
// ("Logic74Unit:00"), of the placed family and with the next free
// reference.  The units placed after a unit (see NewNextUnit) take its
// reference, as one part's.
//
//////////////////////////////////////////////////////////////////////////
static CComponentView* NewPlacedLogic74View(const char* szXMLName, const char* szPartType, int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit)
{
	CChars					szName;
	SLogic74PartType		sType;
	CLogic74PackageView*	pcPackage;
	CLogic74UnitView*		pcUnit;
	CComponentView*			pcView;

	if (!FindLogic74Part(&sType, szPartType))
	{
		return NULL;
	}

	szName.Init();
	GetFreeReference(&szName, pcCircuit, PLACED_REFERENCE);
	pcView = NULL;
	if (strncmp(szXMLName, "Logic74Package:", 15) == 0)
	{
		pcPackage = NewMalloc<CLogic74PackageView>();
		pcPackage->Init(x, y, eRotation, szName.Text(), PLACED_FAMILY, false, szPartType);
		pcView = pcPackage;
	}
	else if (strncmp(szXMLName, "Logic74Unit:", 12) == 0)
	{
		pcUnit = NewMalloc<CLogic74UnitView>();
		pcUnit->Init(x, y, eRotation, szName.Text(), PLACED_FAMILY, false, szPartType, 0);
		pcView = pcUnit;
	}
	szName.Kill();
	return pcView;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CComponentView* NewPlacedComponentView(const char* szXMLName, int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	const char*			szPartType;
	CComponentView*		pcView;

	szPartType = strchr(szXMLName, PLACED_PART_SEPARATOR);
	if (szPartType != NULL)
	{
		return NewPlacedLogic74View(szXMLName, szPartType + 1, x, y, eRotation, pcCircuit);
	}

	pcView = NewComponentView(szXMLName);
	if (pcView == NULL)
	{
		return NULL;
	}
	if (!pcView->InitPlaced(x, y, eRotation, pcCircuit, pcFonts))
	{
		//Not initialised, so only its memory is freed.
		free(pcView);
		return NULL;
	}
	return pcView;
}


//////////////////////////////////////////////////////////////////////////
//
// Measured with the fonts, named and commented nothing.
//
//////////////////////////////////////////////////////////////////////////
CComponentView* NewPlacedSubcircuitInstanceView(CCircuitView* pcSubcircuit, int32 x, int32 y, ERotation eRotation, CCircuitFonts* pcFonts)
{
	CSubcircuitInstanceView*	pcInstance;

	pcInstance = NewMalloc<CSubcircuitInstanceView>();
	pcInstance->Init(x, y, eRotation, "", "", pcSubcircuit, pcFonts);
	return pcInstance;
}

