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
#include "MicroprocessorLib/ResetSupervisor.h"
#include "MicroprocessorLib/LongTime.h"
#include "ComponentPlacement.h"
#include "ComponentProperties.h"
#include "ResetSupervisorView.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CResetSupervisorView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, float32 fTripVoltage, int64 iResetTime)
{
	CPortView*	pcReset;

	CIntegratedCircuitView::Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts);
	mfTripVoltage = fTripVoltage;
	miResetTime = iResetTime;
	mcShapes.AddRectangle(-1, -1, 1, 1, true, true);
	CreatePowerPortViews();
	pcReset = AddPortView(RESET_SUPERVISOR_PIN_RESET, 2, 0);
	pcReset->SetInverting(ROT_East);
	pcReset->SetText("RST");
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CResetSupervisorView::GetType(void)
{
	return RESET_SUPERVISOR_TYPE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CResetSupervisorView::GetXMLName(void) const
{
	return "ResetSupervisor";
}


//////////////////////////////////////////////////////////////////////////
//
// The reset time is in nanoseconds.
//
//////////////////////////////////////////////////////////////////////////
void CResetSupervisorView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CIntegratedCircuitView::SaveXMLAttributes(pcWriter);
	pcWriter->Attribute("tripVoltage", mfTripVoltage);
	pcWriter->Attribute("resetTime", TimeToNanoseconds(miResetTime));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CResetSupervisorView::LoadXML(CMarkupTag* pcTag)
{
	SIntegratedCircuitXML	sIntegratedCircuit;
	float32					fTripVoltage;
	float64					fResetTime;

	if (!LoadIntegratedCircuitXML(pcTag, &sIntegratedCircuit))
	{
		return false;
	}
	if (!pcTag->GetFloat32Attribute("tripVoltage", &fTripVoltage) || !(fTripVoltage > 0) || !pcTag->GetFloat64Attribute("resetTime", &fResetTime) || !(fResetTime >= 0))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a tripVoltage above 0 and a resetTime of 0 or more nanoseconds.", NULL);
	}
	Init(sIntegratedCircuit.x, sIntegratedCircuit.y, sIntegratedCircuit.eRotation, sIntegratedCircuit.szName, sIntegratedCircuit.szFamily, sIntegratedCircuit.bExplicitPowerPorts, fTripVoltage, NanosecondsToTime(fResetTime));
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CIntegratedCircuit* CResetSupervisorView::CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily)
{
	CResetSupervisor*	pcSupervisor;

	pcSupervisor = &OMalloc<CResetSupervisor>(pcSimulation, mszName.Text(), OMalloc<CResetSupervisorPins>());
	pcSupervisor->SetTripVoltage(mfTripVoltage);
	pcSupervisor->SetResetTime(miResetTime);
	return pcSupervisor;
}


//////////////////////////////////////////////////////////////////////////
//
// The trip voltage in volts and the reset time in milliseconds.
//
//////////////////////////////////////////////////////////////////////////
void CResetSupervisorView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CComponentView::GetProperties(pcProperties, psContext);
	AddPowerProperties(pcProperties, psContext);
	pcProperties->AddFloat("tripVoltage", "V", mfTripVoltage, 0.0, 100.0);
	pcProperties->AddFloat("resetTime", "ms", TimeToNanoseconds(miResetTime) / 1000000.0, 0.0, 10000.0);
}


//////////////////////////////////////////////////////////////////////////
//
// Placed new as a DS1813-10.
//
//////////////////////////////////////////////////////////////////////////
bool CResetSupervisorView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	CChars	szName;

	szName.Init();
	GetFreeReference(&szName, pcCircuit, PLACED_REFERENCE);
	Init(x, y, eRotation, szName.Text(), PLACED_FAMILY, false, RESET_SUPERVISOR_TRIP_VOLTAGE, NanosecondsToTime((int64)RESET_SUPERVISOR_RESET_TIME));
	szName.Kill();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CResetSupervisorView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	Init(x, y, eRotation, pcProperties->GetText("name", ""), pcProperties->GetText("family", ""), pcProperties->GetBoolean("explicitPowerPorts", mbExplicitPowerPorts), (float32)pcProperties->GetFloat("tripVoltage", mfTripVoltage), NanosecondsToTime(pcProperties->GetFloat("resetTime", TimeToNanoseconds(miResetTime) / 1000000.0) * 1000000.0));
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
float32 CResetSupervisorView::GetTripVoltage(void) const { return mfTripVoltage; }
int64 CResetSupervisorView::GetResetTime(void) const { return miResetTime; }

