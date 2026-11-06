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
#include "BaseLib/ConstructorCall.h"
#include "MicroprocessorLib/OrGate.h"
#include "MicroprocessorLib/XorGate.h"
#include "OrGateView.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void COrGateView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int iInputCount, int iInputWidth, int iPinSpacing)
{
	InitOr(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iInputCount, iInputWidth, false, false, iPinSpacing);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void COrGateView::InitOr(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int iInputCount, int iInputWidth, bool bNegated, bool bExclusive, int iPinSpacing)
{
	CLogicGateView::Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iInputCount, iInputWidth, bNegated);
	SetPinSpacing(iPinSpacing);
	mbExclusive = bExclusive;
	CreateShapes();
	CreateGatePortViews(mbExclusive ? 1 : 0);
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
// The body is filled by two pie slices from the side arcs' centres and two
// triangles at the back corners, then the three curves are outlined.
//
//////////////////////////////////////////////////////////////////////////
void COrGateView::CreateShapes(void)
{
	AddOrShapes(&mcShapes, 0, 0, GetBodyInputCount(miInputCount, miPinSpacing), mbExclusive);
}


//////////////////////////////////////////////////////////////////////////
//
// An OR (or exclusive OR) gate of iInputCount inputs with its position at
// (x, y), so its inputs are at x - 1 (x - 2 exclusive) and its output at
// x + 2.
//
//////////////////////////////////////////////////////////////////////////
void COrGateView::AddOrShapes(CComponentShapes* pcShapes, float64 x, float64 y, int iInputCount, bool bExclusive)
{
	float64			fHeight;
	float64			fRadius;
	float64			fCentre;
	float64			fBack;
	float64			fMiddle;
	float64			fZero;

	fHeight = CalculateWidth(iInputCount);
	fRadius = fHeight * 2;
	fCentre = x - 0.6;
	fBack = x - 3.5;
	fMiddle = x - 0.9;
	fZero = x;

	pcShapes->AddArc(fCentre, y + fHeight, 3, fRadius, OR_ARC_LOWER_START, OR_ARC_SIDE_LENGTH - OR_ARC_FILL_OFFSET, true, false);
	pcShapes->AddArc(fCentre, y - fHeight, 3, fRadius, OR_ARC_UPPER_START + OR_ARC_FILL_OFFSET, OR_ARC_SIDE_LENGTH - OR_ARC_FILL_OFFSET, true, false);
	pcShapes->AddTriangle(fMiddle, y + fHeight, fZero, y + fHeight - OR_FILL_OFFSET, fZero, y, true, false);
	pcShapes->AddTriangle(fMiddle, y - fHeight, fZero, y - fHeight + OR_FILL_OFFSET, fZero, y, true, false);
	pcShapes->AddArc(fCentre, y + fHeight, 3, fRadius, OR_ARC_LOWER_START, OR_ARC_SIDE_LENGTH, false, true);
	pcShapes->AddArc(fCentre, y - fHeight, 3, fRadius, OR_ARC_UPPER_START, OR_ARC_SIDE_LENGTH, false, true);
	pcShapes->AddArc(fBack, y, 3, fRadius, OR_ARC_BACK_START, OR_ARC_BACK_LENGTH, false, true);
	if (bExclusive)
	{
		pcShapes->AddArc(fBack - 1, y, 3, fRadius, OR_ARC_BACK_START, OR_ARC_BACK_LENGTH, false, true);
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* COrGateView::GetType(void)
{
	return "OR Gate";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CIntegratedCircuit* COrGateView::CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily)
{
	if (mbExclusive)
	{
		if (mbNegated)
		{
			return &OMalloc<CXnorGate>(pcSimulation, mszName.Text(), OMalloc<CXorGatePins>(miInputCount * miInputWidth, pcFamily));
		}
		return &OMalloc<CXorGate>(pcSimulation, mszName.Text(), OMalloc<CXorGatePins>(miInputCount * miInputWidth, pcFamily));
	}
	else
	{
		if (mbNegated)
		{
			return &OMalloc<CNorGate>(pcSimulation, mszName.Text(), OMalloc<COrGatePins>(miInputCount * miInputWidth, pcFamily));
		}
		return &OMalloc<COrGate>(pcSimulation, mszName.Text(), OMalloc<COrGatePins>(miInputCount * miInputWidth, pcFamily));
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool COrGateView::IsExclusive(void)
{
	return mbExclusive;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CNorGateView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int iInputCount, int iInputWidth, int iPinSpacing)
{
	InitOr(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iInputCount, iInputWidth, true, false, iPinSpacing);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CNorGateView::GetType(void)
{
	return "NOR Gate";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CXorGateView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int iInputCount, int iInputWidth, int iPinSpacing)
{
	InitOr(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iInputCount, iInputWidth, false, true, iPinSpacing);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CXorGateView::GetType(void)
{
	return "XOR Gate";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CXnorGateView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int iInputCount, int iInputWidth, int iPinSpacing)
{
	InitOr(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iInputCount, iInputWidth, true, true, iPinSpacing);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CXnorGateView::GetType(void)
{
	return "XNOR Gate";
}



//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* COrGateView::GetXMLName(void)
{
	return "OrGate";
}


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
bool COrGateView::InitGate(SLogicGateXML* psGate)
{
	Init(psGate->sIntegratedCircuit.x, psGate->sIntegratedCircuit.y, psGate->sIntegratedCircuit.eRotation, psGate->sIntegratedCircuit.szName, psGate->sIntegratedCircuit.szFamily, psGate->sIntegratedCircuit.bExplicitPowerPorts, psGate->iInputCount, psGate->iInputWidth, psGate->iPinSpacing);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CNorGateView::GetXMLName(void)
{
	return "NorGate";
}


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
bool CNorGateView::InitGate(SLogicGateXML* psGate)
{
	Init(psGate->sIntegratedCircuit.x, psGate->sIntegratedCircuit.y, psGate->sIntegratedCircuit.eRotation, psGate->sIntegratedCircuit.szName, psGate->sIntegratedCircuit.szFamily, psGate->sIntegratedCircuit.bExplicitPowerPorts, psGate->iInputCount, psGate->iInputWidth, psGate->iPinSpacing);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CXorGateView::GetXMLName(void)
{
	return "XorGate";
}


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
bool CXorGateView::InitGate(SLogicGateXML* psGate)
{
	Init(psGate->sIntegratedCircuit.x, psGate->sIntegratedCircuit.y, psGate->sIntegratedCircuit.eRotation, psGate->sIntegratedCircuit.szName, psGate->sIntegratedCircuit.szFamily, psGate->sIntegratedCircuit.bExplicitPowerPorts, psGate->iInputCount, psGate->iInputWidth, psGate->iPinSpacing);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CXnorGateView::GetXMLName(void)
{
	return "XnorGate";
}


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
bool CXnorGateView::InitGate(SLogicGateXML* psGate)
{
	Init(psGate->sIntegratedCircuit.x, psGate->sIntegratedCircuit.y, psGate->sIntegratedCircuit.eRotation, psGate->sIntegratedCircuit.szName, psGate->sIntegratedCircuit.szFamily, psGate->sIntegratedCircuit.bExplicitPowerPorts, psGate->iInputCount, psGate->iInputWidth, psGate->iPinSpacing);
	return true;
}

