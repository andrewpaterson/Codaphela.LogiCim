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
#include "BaseLib/Logger.h"
#include "MicroprocessorLib/RCFilter.h"
#include "MicroprocessorLib/LongTime.h"
#include "ComponentPlacement.h"
#include "ComponentProperties.h"
#include "CircuitFonts.h"
#include "RCFilterView.h"


#define RC_FILTER_VIEW_FONT_SIZE	8.0


const char* gaszRCFilterDiodeNames[] = { "None", "Rising", "Falling" };


//////////////////////////////////////////////////////////////////////////
//
// fResistance in ohms, fCapacitance in farads.
//
//////////////////////////////////////////////////////////////////////////
void CRCFilterView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, float32 fResistance, float32 fCapacitance, ERCFilterDiode eDiode)
{
	CIntegratedCircuitView::Init(x, y, eRotation, szName, PLACED_FAMILY, false);
	mfResistance = fResistance;
	mfCapacitance = fCapacitance;
	meDiode = eDiode;
	CreateShapes();
	AddPortView(RC_FILTER_PIN_IN, -3, 0);
	AddPortView(RC_FILTER_PIN_OUT, 3, 0);
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
// A time like "12.0ns", "470.0us" or "5.0ms".
//
//////////////////////////////////////////////////////////////////////////
static void AppendRCFilterTime(CChars* psz, float64 fSeconds)
{
	int64		iTenths;
	const char*	szUnit;

	if (fSeconds < 1e-6)
	{
		iTenths = (int64)(fSeconds * 1e10 + 0.5);
		szUnit = "ns";
	}
	else if (fSeconds < 1e-3)
	{
		iTenths = (int64)(fSeconds * 1e7 + 0.5);
		szUnit = "us";
	}
	else
	{
		iTenths = (int64)(fSeconds * 1e4 + 0.5);
		szUnit = "ms";
	}
	psz->Append(iTenths / 10);
	psz->Append('.');
	psz->Append(iTenths % 10);
	psz->Append(szUnit);
}


//////////////////////////////////////////////////////////////////////////
//
// The resistor from In, the capacitor down to ground from Out's line, the
// diode over the resistor, pointing the way it conducts, and the time
// constant under the resistor.
//
//////////////////////////////////////////////////////////////////////////
void CRCFilterView::CreateShapes(void)
{
	CChars	szTime;

	mcShapes.AddLine(-3, 0, -2, 0);
	mcShapes.AddRectangle(-2, -0.4, 0, 0.4, false, true);
	mcShapes.AddLine(0, 0, 3, 0);

	mcShapes.AddLine(1.5, 0, 1.5, 0.8);
	mcShapes.AddLine(0.9, 0.8, 2.1, 0.8);
	mcShapes.AddLine(0.9, 1.1, 2.1, 1.1);
	mcShapes.AddLine(1.5, 1.1, 1.5, 1.6);
	mcShapes.AddLine(1.0, 1.6, 2.0, 1.6);
	mcShapes.AddLine(1.2, 1.85, 1.8, 1.85);
	mcShapes.AddLine(1.4, 2.1, 1.6, 2.1);

	if (meDiode != RCFD_None)
	{
		mcShapes.AddLine(-2.5, 0, -2.5, -1.2);
		mcShapes.AddLine(-2.5, -1.2, -1.4, -1.2);
		mcShapes.AddLine(-0.6, -1.2, 0.5, -1.2);
		mcShapes.AddLine(0.5, -1.2, 0.5, 0);
		if (meDiode == RCFD_Rising)
		{
			mcShapes.AddTriangle(-1.4, -1.6, -1.4, -0.8, -0.6, -1.2, true, true);
			mcShapes.AddLine(-0.6, -1.6, -0.6, -0.8);
		}
		else
		{
			mcShapes.AddTriangle(-0.6, -1.6, -0.6, -0.8, -1.4, -1.2, true, true);
			mcShapes.AddLine(-1.4, -1.6, -1.4, -0.8);
		}
	}

	szTime.Init();
	AppendRCFilterTime(&szTime, (float64)mfResistance * mfCapacitance);
	mcShapes.AddText(-1, 1.2, szTime.Text(), CF_SansSerif, RC_FILTER_VIEW_FONT_SIZE, TA_Centre);
	szTime.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CRCFilterView::GetType(void)
{
	return RC_FILTER_TYPE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CRCFilterView::GetXMLName(void) const
{
	return "RCFilter";
}


//////////////////////////////////////////////////////////////////////////
//
// No family or power: the resistance in ohms, the capacitance in
// picofarads and the diode.
//
//////////////////////////////////////////////////////////////////////////
void CRCFilterView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CComponentView::SaveXMLAttributes(pcWriter);
	pcWriter->Attribute("resistance", mfResistance);
	pcWriter->Attribute("capacitance", mfCapacitance * 1e12f);
	if (meDiode != RCFD_None)
	{
		pcWriter->Attribute("diode", gaszRCFilterDiodeNames[meDiode]);
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CRCFilterView::LoadXML(CMarkupTag* pcTag)
{
	int32		x;
	int32		y;
	ERotation	eRotation;
	char*		szName;
	float32		fResistance;
	float32		fCapacitance;
	char*		szDiode;
	int			iDiode;

	if (!LoadXMLPlacement(pcTag, &x, &y, &eRotation, &szName))
	{
		return false;
	}
	if (!pcTag->GetFloat32Attribute("resistance", &fResistance) || !(fResistance > 0) || !pcTag->GetFloat32Attribute("capacitance", &fCapacitance) || !(fCapacitance > 0))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a resistance (ohms) and a capacitance (picofarads) above 0.", NULL);
	}
	iDiode = RCFD_None;
	szDiode = pcTag->GetStringAttribute("diode");
	if (szDiode != NULL)
	{
		for (iDiode = RCFD_None; iDiode <= RCFD_Falling; iDiode++)
		{
			if (StringCompare(szDiode, gaszRCFilterDiodeNames[iDiode]) == 0)
			{
				break;
			}
		}
		if (iDiode > RCFD_Falling)
		{
			return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "]'s diode [", szDiode, "] isn't None, Rising or Falling.", NULL);
		}
	}
	Init(x, y, eRotation, szName, fResistance, fCapacitance * 1e-12f, (ERCFilterDiode)iDiode);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// It has no power, so no family or supplies.
//
//////////////////////////////////////////////////////////////////////////
CCircuitComponent* CRCFilterView::CreateComponent(CViewPath* pcViewPath, CCircuitSimulation* pcCircuitSimulation, CFamilyVoltageConfigurationStore* pcFamilies)
{
	CSubcircuitSimulation*	pcSimulation;
	CIntegratedCircuit*		pcFilter;

	pcSimulation = CanCreateComponent(pcViewPath, pcCircuitSimulation);
	if (pcSimulation == NULL)
	{
		return NULL;
	}
	pcFilter = CreateIntegratedCircuit(pcSimulation, NULL);
	if (!PostCreateComponent(pcViewPath, pcCircuitSimulation, pcFilter))
	{
		return NULL;
	}
	return pcFilter;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CIntegratedCircuit* CRCFilterView::CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily)
{
	return &OMalloc<CRCFilter>(pcSimulation, mszName.Text(), OMalloc<CRCFilterPins>(), GetRiseTimeConstant(), GetFallTimeConstant());
}


//////////////////////////////////////////////////////////////////////////
//
// The resistance in ohms, the capacitance in picofarads.
//
//////////////////////////////////////////////////////////////////////////
void CRCFilterView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CComponentView::GetProperties(pcProperties, psContext);
	pcProperties->AddFloat("resistance", "ohm", mfResistance, 0.001, 1e9);
	pcProperties->AddFloat("capacitance", "pF", mfCapacitance * 1e12, 0.001, 1e12);
	pcProperties->AddChoice("diode", gaszRCFilterDiodeNames, RCFD_Falling + 1, meDiode);
}


//////////////////////////////////////////////////////////////////////////
//
// Placed new as the Clock Doubler's: 750 ohms into 16pF, 12ns.
//
//////////////////////////////////////////////////////////////////////////
bool CRCFilterView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	CChars	szName;

	szName.Init();
	GetFreeReference(&szName, pcCircuit, "RC");
	Init(x, y, eRotation, szName.Text(), 750.0f, 16e-12f, RCFD_None);
	szName.Kill();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CRCFilterView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	Init(x, y, eRotation, pcProperties->GetText("name", ""), (float32)pcProperties->GetFloat("resistance", mfResistance), (float32)(pcProperties->GetFloat("capacitance", mfCapacitance * 1e12) * 1e-12), (ERCFilterDiode)pcProperties->GetChoice("diode", meDiode));
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// R x C, or through the diode's resistance the way it conducts.
//
//////////////////////////////////////////////////////////////////////////
int64 CRCFilterView::GetRiseTimeConstant(void) const
{
	return NanosecondsToTime((float64)((meDiode == RCFD_Rising) ? RC_FILTER_DIODE_RESISTANCE : mfResistance) * mfCapacitance * 1e9);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int64 CRCFilterView::GetFallTimeConstant(void) const
{
	return NanosecondsToTime((float64)((meDiode == RCFD_Falling) ? RC_FILTER_DIODE_RESISTANCE : mfResistance) * mfCapacitance * 1e9);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
float32 CRCFilterView::GetResistance(void) const { return mfResistance; }
float32 CRCFilterView::GetCapacitance(void) const { return mfCapacitance; }
ERCFilterDiode CRCFilterView::GetDiode(void) const { return meDiode; }

