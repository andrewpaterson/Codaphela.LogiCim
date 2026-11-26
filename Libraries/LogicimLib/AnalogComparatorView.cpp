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
#include "MicroprocessorLib/AnalogComparator.h"
#include "ComponentPlacement.h"
#include "ComponentProperties.h"
#include "CircuitFonts.h"
#include "AnalogComparatorView.h"


#define ANALOG_COMPARATOR_VIEW_FONT_SIZE	8.0


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CAnalogComparatorView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts)
{
	CIntegratedCircuitView::Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts);
	mcShapes.AddTriangle(-2, -2, -2, 2, 2, 0, true, true);
	mcShapes.AddLine(-3, -1, -2, -1);
	mcShapes.AddLine(-3, 1, -2, 1);
	mcShapes.AddLine(2, 0, 3, 0);
	mcShapes.AddText(-1.6, -1, "+", CF_SansSerifBold, ANALOG_COMPARATOR_VIEW_FONT_SIZE, TA_Left);
	mcShapes.AddText(-1.6, 1, "-", CF_SansSerifBold, ANALOG_COMPARATOR_VIEW_FONT_SIZE, TA_Left);
	CreatePowerPortViews();
	AddPortView(ANALOG_COMPARATOR_PIN_PLUS, -3, -1);
	AddPortView(ANALOG_COMPARATOR_PIN_MINUS, -3, 1);
	AddPortView(ANALOG_COMPARATOR_PIN_OUT, 3, 0);
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CAnalogComparatorView::GetType(void)
{
	return ANALOG_COMPARATOR_TYPE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CAnalogComparatorView::GetXMLName(void) const
{
	return "AnalogComparator";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CAnalogComparatorView::LoadXML(CMarkupTag* pcTag)
{
	SIntegratedCircuitXML	sIntegratedCircuit;

	if (!LoadIntegratedCircuitXML(pcTag, &sIntegratedCircuit))
	{
		return false;
	}
	Init(sIntegratedCircuit.x, sIntegratedCircuit.y, sIntegratedCircuit.eRotation, sIntegratedCircuit.szName, sIntegratedCircuit.szFamily, sIntegratedCircuit.bExplicitPowerPorts);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CIntegratedCircuit* CAnalogComparatorView::CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily)
{
	return &OMalloc<CAnalogComparator>(pcSimulation, mszName.Text(), OMalloc<CAnalogComparatorPins>());
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CAnalogComparatorView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CComponentView::GetProperties(pcProperties, psContext);
	AddPowerProperties(pcProperties, psContext);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CAnalogComparatorView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	CChars	szName;

	szName.Init();
	GetFreeReference(&szName, pcCircuit, PLACED_REFERENCE);
	Init(x, y, eRotation, szName.Text(), PLACED_FAMILY, false);
	szName.Kill();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CAnalogComparatorView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	Init(x, y, eRotation, pcProperties->GetText("name", ""), pcProperties->GetText("family", ""), pcProperties->GetBoolean("explicitPowerPorts", mbExplicitPowerPorts));
	return true;
}

