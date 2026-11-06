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
#include "BaseLib/Logger.h"
#include "MicroprocessorLib/Pins.h"
#include "ComponentShapes.h"
#include "Radix.h"
#include "LogicGateView.h"


//////////////////////////////////////////////////////////////////////////
//
// A negated gate has a bubble on its output.  Subclasses add their shapes,
// then the port views, then CalculateBounds.
//
//////////////////////////////////////////////////////////////////////////
void CLogicGateView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int iInputCount, int iInputWidth, bool bNegated)
{
	CIntegratedCircuitView::Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts);
	miInputCount = (iInputCount < 1) ? 1 : iInputCount;
	miInputWidth = (iInputWidth < 1) ? 1 : iInputWidth;
	mbNegated = bNegated;
	miPinSpacing = 1;
}


//////////////////////////////////////////////////////////////////////////
//
// A gate saved with its placement, family and inputs (see InitGate).
//
//////////////////////////////////////////////////////////////////////////
bool CLogicGateView::LoadXML(CMarkupTag* pcTag)
{
	SLogicGateXML	sGate;

	if (!LoadLogicGateXML(pcTag, &sGate))
	{
		return false;
	}
	return InitGate(&sGate);
}


//////////////////////////////////////////////////////////////////////////
//
// A gate from its properties, the gate's own for any missing (see
// InitGate).
//
//////////////////////////////////////////////////////////////////////////
bool CLogicGateView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	SLogicGateXML	sGate;

	GetIntegratedCircuitProperties(x, y, eRotation, pcProperties, &sGate.sIntegratedCircuit);
	sGate.iInputCount = (int32)pcProperties->GetInteger("inputCount", miInputCount);
	sGate.iInputWidth = (int32)pcProperties->GetInteger("inputWidth", miInputWidth);
	sGate.iPinSpacing = (int32)pcProperties->GetInteger("pinSpacing", miPinSpacing);
	return InitGate(&sGate);
}


//////////////////////////////////////////////////////////////////////////
//
// Made from what a gate is saved with, so LoadXML and InitFromProperties
// needn't be each gate's own.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicGateView::InitGate(SLogicGateXML* psGate)
{
	return gcLogger.Error2(__METHOD__, " [", GetType(), "] isn't made from a gate's settings.", NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicGateView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CIntegratedCircuitView::SaveXMLAttributes(pcWriter);
	pcWriter->Attribute("inputCount", (int32)miInputCount);
	pcWriter->Attribute("inputWidth", (int32)miInputWidth);
	if (miPinSpacing > 1)
	{
		pcWriter->Attribute("pinSpacing", (int32)miPinSpacing);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The input count and width must be at least one.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicGateView::LoadLogicGateXML(CMarkupTag* pcTag, SLogicGateXML* psGate)
{
	if (!LoadIntegratedCircuitXML(pcTag, &psGate->sIntegratedCircuit))
	{
		return false;
	}
	if (!pcTag->GetInt32Attribute("inputCount", &psGate->iInputCount) || !pcTag->GetInt32Attribute("inputWidth", &psGate->iInputWidth))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs whole number inputCount and inputWidth attributes.", NULL);
	}
	if ((psGate->iInputCount < 1) || (psGate->iInputWidth < 1))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] inputCount and inputWidth must be at least one.", NULL);
	}
	if (!pcTag->GetInt32Attribute("pinSpacing", &psGate->iPinSpacing))
	{
		psGate->iPinSpacing = 1;
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// -n to n for 2n + 1 inputs and -n to n without 0 for 2n.
//
//////////////////////////////////////////////////////////////////////////
void CLogicGateView::CalculatePortOffsets(CArrayInt* paiOffsets, int iInputCount, int iPinSpacing)
{
	int		iEnd;
	int		i;
	bool	bSkipZero;

	iEnd = iInputCount / 2;
	bSkipZero = (iInputCount % 2) == 0;
	for (i = -iEnd; i <= iEnd; i++)
	{
		if ((i == 0) && bSkipZero)
		{
			continue;
		}
		if ((iPinSpacing <= 1) || !bSkipZero)
		{
			paiOffsets->Add(i * ((iPinSpacing <= 1) ? 1 : iPinSpacing));
		}
		else
		{
			//An even count skips zero: the inputs nearest it are a grid unit either side, the rest iPinSpacing on.
			paiOffsets->Add((i > 0) ? (iPinSpacing * i - iPinSpacing + 1) : (iPinSpacing * i + iPinSpacing - 1));
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The input count a gate's body is drawn for (see CalculateWidth) so its
// inputs, iPinSpacing apart, all fit on it: its own count with a spacing
// of one.
//
//////////////////////////////////////////////////////////////////////////
int CLogicGateView::GetBodyInputCount(int iInputCount, int iPinSpacing)
{
	CArrayInt	aiOffsets;
	int			iFurthest;

	if (iPinSpacing <= 1)
	{
		return iInputCount;
	}
	aiOffsets.Init();
	CalculatePortOffsets(&aiOffsets, iInputCount, iPinSpacing);
	iFurthest = (aiOffsets.NumElements() > 0) ? aiOffsets.GetValue(aiOffsets.NumElements() - 1) : 0;
	aiOffsets.Kill();
	return (2 * iFurthest + 1 > iInputCount) ? 2 * iFurthest + 1 : iInputCount;
}


//////////////////////////////////////////////////////////////////////////
//
// Half the gate's height.
//
//////////////////////////////////////////////////////////////////////////
float64 CLogicGateView::CalculateWidth(int iInputCount)
{
	if (iInputCount <= 3)
	{
		return 1.5;
	}
	return 1.5 + ((iInputCount / 2) - 1);
}


//////////////////////////////////////////////////////////////////////////
//
// The names of the ports of input (or output) iPortNumber.  A gate with a
// single port calls it szName alone, as the pins do.
//
//////////////////////////////////////////////////////////////////////////
void CLogicGateView::GetPortNames(CArrayChars* paszNames, const char* szName, int iPortNumber, int iInputWidth, int iInputCount)
{
	int		i;
	CChars	szPortName;

	for (i = iPortNumber * iInputWidth; i < (iPortNumber + 1) * iInputWidth; i++)
	{
		szPortName.Init();
		CPins::CalculatePortName(&szPortName, szName, i, iInputWidth * iInputCount);
		paszNames->Add(&szPortName);
		szPortName.Kill();
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicGateView::CreateGatePortViews(int iInputOffset)
{
	CArrayInt		aiOffsets;
	CArrayChars		aszNames;
	size			i;
	CPortView*		pcOutput;

	CreatePowerPortViews();

	aiOffsets.Init();
	CalculatePortOffsets(&aiOffsets, miInputCount, miPinSpacing);
	for (i = 0; i < aiOffsets.NumElements(); i++)
	{
		aszNames.Init();
		GetPortNames(&aszNames, "Input", (int)i, miInputWidth, miInputCount);
		AddPortView(&aszNames, -(1 + iInputOffset), aiOffsets.GetValue(i));
		aszNames.Kill();
	}
	aiOffsets.Kill();

	pcOutput = AddPortView("Output", 2, 0);
	if (mbNegated)
	{
		pcOutput->SetInverting(ROT_East);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// All the inputs and then all the outputs.
//
//////////////////////////////////////////////////////////////////////////
void CLogicGateView::CreateBufferPortViews(void)
{
	CArrayInt		aiOffsets;
	CArrayChars		aszNames;
	size			i;
	CPortView*		pcOutput;

	CreatePowerPortViews();

	aiOffsets.Init();
	CalculatePortOffsets(&aiOffsets, miInputCount);
	for (i = 0; i < aiOffsets.NumElements(); i++)
	{
		aszNames.Init();
		GetPortNames(&aszNames, "Input", (int)i, miInputWidth, miInputCount);
		AddPortView(&aszNames, -1, aiOffsets.GetValue(i));
		aszNames.Kill();
	}
	for (i = 0; i < aiOffsets.NumElements(); i++)
	{
		aszNames.Init();
		GetPortNames(&aszNames, "Output", (int)i, miInputWidth, miInputCount);
		pcOutput = AddPortView(&aszNames, 1, aiOffsets.GetValue(i));
		aszNames.Kill();
		if (mbNegated)
		{
			pcOutput->SetInverting(ROT_East);
		}
	}
	aiOffsets.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// A gate is three grid units high at least, whatever its shape.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicGateView::IncludeExtraBounds(SFloat64Vec2* psMinimum, SFloat64Vec2* psMaximum, bool bHasBounds)
{
	if (!bHasBounds)
	{
		psMinimum->Init(0, -1.5);
		psMaximum->Init(0, 1.5);
	}
	IncludeBoundsPoint(psMinimum, psMaximum, 0, -1.5);
	IncludeBoundsPoint(psMinimum, psMaximum, 0, 1.5);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int CLogicGateView::GetInputCount(void) { return miInputCount; }
int CLogicGateView::GetInputWidth(void) { return miInputWidth; }
bool CLogicGateView::IsNegated(void) { return mbNegated; }


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's LogicGateProperties (and BufferProperties, whose input
// count is its count of buffers).
//
//////////////////////////////////////////////////////////////////////////
void CLogicGateView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CComponentView::GetProperties(pcProperties, psContext);
	AddPowerProperties(pcProperties, psContext);
	pcProperties->AddInteger("inputCount", miInputCount, 1, RADIX_MAX_BIT_WIDTH);
	pcProperties->AddInteger("inputWidth", miInputWidth, 1, RADIX_MAX_BIT_WIDTH);
	pcProperties->AddInteger("pinSpacing", miPinSpacing, 1, LOGIC_GATE_MAX_PIN_SPACING);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int CLogicGateView::GetPinSpacing(void) { return miPinSpacing; }
void CLogicGateView::SetPinSpacing(int iPinSpacing) { miPinSpacing = (iPinSpacing < 1) ? 1 : ((iPinSpacing > LOGIC_GATE_MAX_PIN_SPACING) ? LOGIC_GATE_MAX_PIN_SPACING : iPinSpacing); }
