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
#include "BaseLib/Logger.h"
#include "BaseLib/StringHelper.h"
#include "MicroprocessorLib/Splitter.h"
#include "ComponentStateXML.h"
#include "Radix.h"
#include "ComponentPlacement.h"
#include "SplitterView.h"


//////////////////////////////////////////////////////////////////////////
//
// Logicim's default split: the bits shared out over the ends in order, as
// SplitterProperties.createSplitIndices.
//
//////////////////////////////////////////////////////////////////////////
void CSplitterView::CreateSplitIndices(int* aiSplitIndices, int32 iBitWidth, int32 iFanOut)
{
	int32	i;
	int32	iFan;
	int32	iShare;

	iFan = 0;
	iShare = iFanOut / 2;
	for (i = 0; i < iBitWidth; i++)
	{
		if (iFanOut == 1)
		{
			aiSplitIndices[i] = 0;
			continue;
		}
		aiSplitIndices[i] = iFan;
		iShare += iFanOut;
		if (iShare >= iBitWidth)
		{
			iShare -= iBitWidth;
			iFan++;
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// iBitWidth bits over iFanOut ends, two grid units apart, left handed,
// the bus a grid unit back.
//
//////////////////////////////////////////////////////////////////////////
void SSplitterParams::Init(int32 iBitWidth, int32 iFanOut)
{
	this->iBitWidth = iBitWidth;
	this->iFanOut = iFanOut;
	iGridSpacing = 2;
	eAppearance = SA_LeftHanded;
	iEndOffset = -1;
}


//////////////////////////////////////////////////////////////////////////
//
// aiSplitIndices has an end for each of the bits; NULL shares the bits
// out over the ends in order (see CreateSplitIndices).
//
//////////////////////////////////////////////////////////////////////////
void CSplitterView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, SSplitterParams* psParams, int* aiSplitIndices)
{
	CArrayInt	aiInOrder;

	if (aiSplitIndices == NULL)
	{
		aiInOrder.Init();
		aiInOrder.GrowToAtLeastNumElements(psParams->iBitWidth + 1, true, 0);
		CreateSplitIndices(aiInOrder.GetData(), psParams->iBitWidth, psParams->iFanOut);
		InitSplit(x, y, eRotation, szName, psParams, aiInOrder.GetData());
		aiInOrder.Kill();
	}
	else
	{
		InitSplit(x, y, eRotation, szName, psParams, aiSplitIndices);
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSplitterView::InitSplit(int32 x, int32 y, ERotation eRotation, const char* szName, SSplitterParams* psParams, int* aiSplitIndices)
{
	int32				i;
	int32				iFan;
	SFloat64Vec2		sStart;
	SFloat64Vec2		sBar;
	SFloat64Vec2		sEnd;
	SFloat64Vec2		sMid;
	SFloat64Vec2		sFirst;
	SFloat64Vec2		sLast;
	CArrayInt			aiBits;
	CChars				szText;
	CArrayChars			aszStartNames;
	CArrayChars			aszEndNames;
	CArrayChars			aszFanNames;
	int32				iName;

	CComponentView::Init(x, y, eRotation, szName);
	miBitWidth = psParams->iBitWidth;
	miFanOut = psParams->iFanOut;
	miGridSpacing = psParams->iGridSpacing;
	meAppearance = psParams->eAppearance;
	miEndOffset = psParams->iEndOffset;
	maiSplitIndices.Init();
	for (i = 0; i < miBitWidth; i++)
	{
		maiSplitIndices.Add(aiSplitIndices[i]);
	}
	maiEndPortViews.Init();

	sStart = GetStartPosition();
	sBar = GetBarPosition();
	mcShapes.AddLine(sStart.x, sStart.y, sBar.x, sBar.y, SPLITTER_BUS_LINE_WIDTH);
	for (iFan = 0; iFan < miFanOut; iFan++)
	{
		sEnd = GetFanPosition(iFan, 2);
		sMid = GetFanPosition(iFan, 0);
		mcShapes.AddLine(sEnd.x, sEnd.y, sMid.x, sMid.y);
	}
	sFirst = GetFanPosition(0, 0);
	sLast = GetFanPosition(miFanOut - 1, 0);
	mcShapes.AddRectangle(-0.1, (sFirst.y < sLast.y) ? sFirst.y : sLast.y, 0.1, (sFirst.y < sLast.y) ? sLast.y : sFirst.y, true, true);

	aiBits.Init();
	for (iFan = 0; iFan < miFanOut; iFan++)
	{
		aiBits.ReInit();
		GetFanBits(&aiBits, iFan);
		szText.Init();
		AppendBitsText(&szText, &aiBits);
		sMid = GetFanPosition(iFan, 0);
		sMid.x -= 0.3;
		sMid.y += (meAppearance == SA_RightHanded) ? -0.4 : 0.4;
		mcShapes.AddText(sMid.x, sMid.y, szText.Text(), CF_SansSerif, SPLITTER_FONT_SIZE, TA_Right);
		szText.Kill();
	}

	aszStartNames.Init();
	aszEndNames.Init();
	GetPortNames(&aszStartNames, &aszEndNames);
	AddPortView(&aszStartNames, (int32)sStart.x, (int32)sStart.y);
	iName = 0;
	for (iFan = 0; iFan < miFanOut; iFan++)
	{
		aiBits.ReInit();
		GetFanBits(&aiBits, iFan);
		if (aiBits.NumElements() == 0)
		{
			maiEndPortViews.Add(-1);
			continue;
		}
		aszFanNames.Init();
		for (i = 0; i < (int32)aiBits.NumElements(); i++)
		{
			aszFanNames.Add(aszEndNames.GetText(iName));
			iName++;
		}
		sEnd = GetFanPosition(iFan, 2);
		maiEndPortViews.Add((int)NumPortViews());
		AddPortView(&aszFanNames, (int32)sEnd.x, (int32)sEnd.y);
		aszFanNames.Kill();
	}
	aszEndNames.Kill();
	aszStartNames.Kill();
	aiBits.Kill();
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSplitterView::Kill(void)
{
	maiEndPortViews.Kill();
	maiSplitIndices.Kill();
	CComponentView::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The bus's ports are "Start b" and the ends' are "End b", fan by fan.
//
//////////////////////////////////////////////////////////////////////////
void CSplitterView::GetPortNames(CArrayChars* paszStartNames, CArrayChars* paszEndNames)
{
	int32		i;
	int32		iFan;
	CArrayInt	aiBits;
	CChars		szName;

	for (i = 0; i < miBitWidth; i++)
	{
		szName.Init("Start ");
		szName.Append(i);
		paszStartNames->Add(szName.Text());
		szName.Kill();
	}
	aiBits.Init();
	for (iFan = 0; iFan < miFanOut; iFan++)
	{
		aiBits.ReInit();
		GetFanBits(&aiBits, iFan);
		for (i = 0; i < (int32)aiBits.NumElements(); i++)
		{
			szName.Init("End ");
			szName.Append(aiBits.GetValue(i));
			paszEndNames->Add(szName.Text());
			szName.Kill();
		}
	}
	aiBits.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The bits that go to the end, lowest first.
//
//////////////////////////////////////////////////////////////////////////
void CSplitterView::GetFanBits(CArrayInt* paiBits, int32 iFan)
{
	int32	i;

	for (i = 0; i < miBitWidth; i++)
	{
		if (maiSplitIndices.GetValue(i) == iFan)
		{
			paiBits->Add(i);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's SplitterView.toText: runs of three or more bits as "0-3",
// others listed, "0,1,5".
//
//////////////////////////////////////////////////////////////////////////
void CSplitterView::AppendBitsText(CChars* psz, CArrayInt* paiBits)
{
	size	i;
	int		iBit;
	int		iPrevious;
	int		iSkipped;

	iSkipped = 0;
	iPrevious = 0;
	for (i = 0; i < paiBits->NumElements(); i++)
	{
		iBit = paiBits->GetValue(i);
		if (i == 0)
		{
			psz->Append(iBit);
		}
		else if (iBit - 1 == iPrevious)
		{
			iSkipped++;
		}
		else
		{
			if (iSkipped == 1)
			{
				psz->Append(',');
				psz->Append(iPrevious);
			}
			else if (iSkipped > 1)
			{
				psz->Append('-');
				psz->Append(iPrevious);
			}
			iSkipped = 0;
			psz->Append(',');
			psz->Append(iBit);
		}
		iPrevious = iBit;
	}
	if (iSkipped == 1)
	{
		psz->Append(',');
		psz->Append(iPrevious);
	}
	else if (iSkipped > 1)
	{
		psz->Append('-');
		psz->Append(iPrevious);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Where an end leaves the bar (fAlong 0) or stops (fAlong 2).
//
//////////////////////////////////////////////////////////////////////////
SFloat64Vec2 CSplitterView::GetFanPosition(int32 iFan, float64 fAlong)
{
	SFloat64Vec2	sPosition;

	if (meAppearance == SA_RightHanded)
	{
		sPosition.Init(-fAlong, iFan * miGridSpacing);
	}
	else
	{
		sPosition.Init(-fAlong, -iFan * miGridSpacing);
	}
	return sPosition;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SFloat64Vec2 CSplitterView::GetStartPosition(void)
{
	SFloat64Vec2	sPosition;

	if (meAppearance == SA_RightHanded)
	{
		sPosition.Init(1, miEndOffset);
	}
	else
	{
		sPosition.Init(1, -miEndOffset);
	}
	return sPosition;
}


//////////////////////////////////////////////////////////////////////////
//
// Where the bus meets the bar: straight left from the start, or the
// nearest end of the bar when the start is past it.
//
//////////////////////////////////////////////////////////////////////////
SFloat64Vec2 CSplitterView::GetBarPosition(void)
{
	SFloat64Vec2	sStart;
	SFloat64Vec2	sBar;

	sStart = GetStartPosition();
	if (meAppearance == SA_RightHanded)
	{
		if (sStart.y > (miFanOut - 1) * miGridSpacing)
		{
			sBar = GetFanPosition(miFanOut - 1, 0);
		}
		else if (sStart.y < 0)
		{
			sBar = GetFanPosition(0, 0);
		}
		else
		{
			sBar.Init(0, sStart.y);
		}
	}
	else
	{
		if (sStart.y > 0)
		{
			sBar = GetFanPosition(0, 0);
		}
		else if (sStart.y < -(miFanOut - 1) * miGridSpacing)
		{
			sBar = GetFanPosition(miFanOut - 1, 0);
		}
		else
		{
			sBar.Init(0, sStart.y);
		}
	}
	return sBar;
}


//////////////////////////////////////////////////////////////////////////
//
// Each bit of the bus to its place on its end.
//
//////////////////////////////////////////////////////////////////////////
void CSplitterView::GetBitJoins(CArrayPortBitJoin* pasJoins)
{
	int32			i;
	int32			iFan;
	SPortBitJoin*	psJoin;
	CArrayInt		aiPlaces;
	int				iPlace;

	aiPlaces.Init();
	for (iFan = 0; iFan < miFanOut; iFan++)
	{
		aiPlaces.Add(0);
	}
	for (i = 0; i < miBitWidth; i++)
	{
		iFan = maiSplitIndices.GetValue(i);
		iPlace = aiPlaces.GetValue(iFan);
		aiPlaces.SetValue(iFan, iPlace + 1);

		psJoin = pasJoins->Add();
		psJoin->pcFrom = GetStartPortView();
		psJoin->uiFromBit = (size)i;
		psJoin->pcTo = GetEndPortView(iFan);
		psJoin->uiToBit = (size)iPlace;
	}
	aiPlaces.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CCircuitComponent* CSplitterView::CreateComponent(CViewPath* pcViewPath, CCircuitSimulation* pcCircuitSimulation, CFamilyVoltageConfigurationStore* pcFamilies)
{
	CSplitter*				pcSplitter;
	CArrayChars				aszStartNames;
	CArrayChars				aszEndNames;
	CSubcircuitSimulation*	pcSimulation;

	pcSimulation = CanCreateComponent(pcViewPath, pcCircuitSimulation);
	if (pcSimulation == NULL)
	{
		return NULL;
	}

	aszStartNames.Init();
	aszEndNames.Init();
	GetPortNames(&aszStartNames, &aszEndNames);
	pcSplitter = &OMalloc<CSplitter>(pcSimulation, mszName.Text(), &aszStartNames, &aszEndNames);
	aszEndNames.Kill();
	aszStartNames.Kill();

	if (!PostCreateComponent(pcViewPath, pcCircuitSimulation, pcSplitter))
	{
		return NULL;
	}
	return pcSplitter;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CComponentStateXML* CSplitterView::GetComponentStateXML(void)
{
	return &gcPassiveStateXML;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CSplitterView::GetType(void)
{
	return "Splitter";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CSplitterView::GetXMLName(void)
{
	return "Splitter";
}


//////////////////////////////////////////////////////////////////////////
//
// The split indices are written with a space between each.
//
//////////////////////////////////////////////////////////////////////////
void CSplitterView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CChars	szSplitIndices;
	int32	i;

	CComponentView::SaveXMLAttributes(pcWriter);
	pcWriter->Attribute("bitWidth", miBitWidth);
	pcWriter->Attribute("fanOut", miFanOut);
	pcWriter->Attribute("gridSpacing", miGridSpacing);
	pcWriter->Attribute("appearance", SplitterAppearanceToString(meAppearance));
	pcWriter->Attribute("endOffset", miEndOffset);
	szSplitIndices.Init();
	for (i = 0; i < miBitWidth; i++)
	{
		if (i != 0)
		{
			szSplitIndices.Append(' ');
		}
		szSplitIndices.Append(maiSplitIndices.GetValue(i));
	}
	pcWriter->Attribute("splitIndices", szSplitIndices.Text());
	szSplitIndices.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's limits: the fan out is from 1 to 64, the spacing from 1 to 12
// and every split index names an end.
//
//////////////////////////////////////////////////////////////////////////
bool CSplitterView::LoadXML(CMarkupTag* pcTag)
{
	int32				x;
	int32				y;
	ERotation			eRotation;
	char*				szName;
	SSplitterParams		sParams;
	char*				szAppearance;
	char*				szSplitIndices;
	char*				szEnd;
	CArrayInt			aiSplitIndices;
	int32				i;
	int					iIndex;

	if (!LoadXMLPlacement(pcTag, &x, &y, &eRotation, &szName))
	{
		return false;
	}
	if (!pcTag->GetInt32Attribute("bitWidth", &sParams.iBitWidth) || !pcTag->GetInt32Attribute("fanOut", &sParams.iFanOut) ||
		!pcTag->GetInt32Attribute("gridSpacing", &sParams.iGridSpacing) || !pcTag->GetInt32Attribute("endOffset", &sParams.iEndOffset))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs whole number bitWidth, fanOut, gridSpacing and endOffset attributes.", NULL);
	}
	if ((sParams.iBitWidth < 1) || (sParams.iBitWidth > 64) || (sParams.iFanOut < 1) || (sParams.iFanOut > 64) || (sParams.iGridSpacing < 1) || (sParams.iGridSpacing > 12))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] has a property out of range.", NULL);
	}
	szAppearance = pcTag->GetStringAttribute("appearance");
	sParams.eAppearance = StringToSplitterAppearance(szAppearance);
	if (sParams.eAppearance == SA_Cannot)
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] has no appearance or an unknown one [", (szAppearance != NULL) ? szAppearance : "", "].", NULL);
	}

	szSplitIndices = pcTag->GetStringAttribute("splitIndices");
	if (szSplitIndices == NULL)
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a splitIndices attribute.", NULL);
	}
	aiSplitIndices.Init();
	for (i = 0; i < sParams.iBitWidth; i++)
	{
		iIndex = (int)strtol(szSplitIndices, &szEnd, 10);
		if ((szEnd == szSplitIndices) || (iIndex < 0) || (iIndex >= sParams.iFanOut))
		{
			aiSplitIndices.Kill();
			return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a split index from 0 to fanOut - 1 for each bit.", NULL);
		}
		aiSplitIndices.Add(iIndex);
		szSplitIndices = szEnd;
	}
	while (*szSplitIndices == ' ')
	{
		szSplitIndices++;
	}
	if (*szSplitIndices != '\0')
	{
		aiSplitIndices.Kill();
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] has more split indices than bits.", NULL);
	}

	Init(x, y, eRotation, szName, &sParams, aiSplitIndices.GetData());
	aiSplitIndices.Kill();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int32 CSplitterView::GetBitWidth(void) { return miBitWidth; }
int32 CSplitterView::GetFanOut(void) { return miFanOut; }
int32 CSplitterView::GetSplitIndex(int32 iBit) { return maiSplitIndices.GetValue(iBit); }
CPortView* CSplitterView::GetStartPortView(void) { return GetPortView((size)0); }


//////////////////////////////////////////////////////////////////////////
//
// NULL for an end with no bits.
//
//////////////////////////////////////////////////////////////////////////
CPortView* CSplitterView::GetEndPortView(int32 iFan)
{
	int		iPortView;

	iPortView = maiEndPortViews.GetValue(iFan);
	if (iPortView == -1)
	{
		return NULL;
	}
	return GetPortView((size)iPortView);
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's names in lower case.
//
//////////////////////////////////////////////////////////////////////////
const char* SplitterAppearanceToString(ESplitterAppearance eAppearance)
{
	if (eAppearance == SA_RightHanded)
	{
		return "right_handed";
	}
	return "left_handed";
}


//////////////////////////////////////////////////////////////////////////
//
// SA_Cannot if it isn't one SplitterAppearanceToString gives.
//
//////////////////////////////////////////////////////////////////////////
ESplitterAppearance StringToSplitterAppearance(const char* szAppearance)
{
	if (szAppearance == NULL)
	{
		return SA_Cannot;
	}
	if (StringCompare(szAppearance, "left_handed") == 0)
	{
		return SA_LeftHanded;
	}
	if (StringCompare(szAppearance, "right_handed") == 0)
	{
		return SA_RightHanded;
	}
	return SA_Cannot;
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's SplitterProperties.  The bits are split in order again when
// it's made from them.
//
//////////////////////////////////////////////////////////////////////////
void CSplitterView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	const char*		aszAppearances[] = { "Left Handed", "Right Handed" };

	CComponentView::GetProperties(pcProperties, psContext);
	pcProperties->AddInteger("bitWidth", miBitWidth, 1, RADIX_MAX_BIT_WIDTH);
	pcProperties->AddInteger("fanOut", miFanOut, 1, RADIX_MAX_BIT_WIDTH);
	pcProperties->AddInteger("gridSpacing", miGridSpacing, 1, 16);
	pcProperties->AddChoice("appearance", aszAppearances, 2, (size)meAppearance - SA_LeftHanded);
	pcProperties->AddInteger("endOffset", miEndOffset, -RADIX_MAX_BIT_WIDTH, RADIX_MAX_BIT_WIDTH);
}


//////////////////////////////////////////////////////////////////////////
//
// Placed new splitting two bits into two ends.
//
//////////////////////////////////////////////////////////////////////////
bool CSplitterView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	SSplitterParams		sParams;

	sParams.Init(2, 2);
	Init(x, y, eRotation, "", &sParams);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CSplitterView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	SSplitterParams		sParams;

	sParams.iBitWidth = (int32)pcProperties->GetInteger("bitWidth", miBitWidth);
	sParams.iFanOut = (int32)pcProperties->GetInteger("fanOut", miFanOut);
	sParams.iGridSpacing = (int32)pcProperties->GetInteger("gridSpacing", miGridSpacing);
	sParams.eAppearance = (ESplitterAppearance)(SA_LeftHanded + pcProperties->GetChoice("appearance", (size)meAppearance - SA_LeftHanded));
	sParams.iEndOffset = (int32)pcProperties->GetInteger("endOffset", miEndOffset);
	Init(x, y, eRotation, pcProperties->GetText("name", ""), &sParams);
	return true;
}
