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
#include "MicroprocessorLib/PowerSource.h"
#include "MicroprocessorLib/PowerPinNames.h"
#include "MicroprocessorLib/Voltage.h"
#include "MicroprocessorLib/LongTime.h"
#include "ComponentStateXML.h"
#include "ComponentPlacement.h"
#include "PowerSourceView.h"


//////////////////////////////////////////////////////////////////////////
//
// Subclasses add their shapes, the port view and then CalculateBounds.
//
//////////////////////////////////////////////////////////////////////////
void CPowerSourceView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, float32 fVoltage)
{
	CComponentView::Init(x, y, eRotation, szName);
	mfVoltage = fVoltage;
	miRiseTime = 0;
	mbOnAtStart = true;
	mfDropout = -1.0f;
	mszRail.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPowerSourceView::Kill(void)
{
	mszRail.Kill();
	CComponentView::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The rail it is on, which it joins wherever it is; NULL for none.
//
//////////////////////////////////////////////////////////////////////////
const char* CPowerSourceView::GetConnectionName(void)
{
	return mszRail.Empty() ? NULL : mszRail.Text();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPowerSourceView::SetRail(const char* szRail)
{
	mszRail.Set(szRail);
}


//////////////////////////////////////////////////////////////////////////
//
// Its name, and its rail if it is on one.
//
//////////////////////////////////////////////////////////////////////////
void CPowerSourceView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CComponentView::GetProperties(pcProperties, psContext);
	if (!mszRail.Empty())
	{
		pcProperties->AddText("rail", mszRail.Text());
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A supply has no family.
//
//////////////////////////////////////////////////////////////////////////
CCircuitComponent* CPowerSourceView::CreateComponent(CViewPath* pcViewPath, CCircuitSimulation* pcCircuitSimulation, CFamilyVoltageConfigurationStore* pcFamilies)
{
	CPowerSource*			pcPowerSource;
	CSubcircuitSimulation*	pcSimulation;

	pcSimulation = CanCreateComponent(pcViewPath, pcCircuitSimulation);
	if (pcSimulation == NULL)
	{
		return NULL;
	}

	pcPowerSource = &OMalloc<CPowerSource>(pcSimulation, mszName.Text(), mfVoltage);
	pcPowerSource->SetRiseTime(miRiseTime);
	pcPowerSource->SetOnAtStart(mbOnAtStart);
	if (IsFed())
	{
		pcPowerSource->SetFeed(mfDropout);
	}
	if (!PostCreateComponent(pcViewPath, pcCircuitSimulation, pcPowerSource))
	{
		return NULL;
	}
	return pcPowerSource;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CComponentStateXML* CPowerSourceView::GetComponentStateXML(void) const
{
	return &gcPowerSourceStateXML;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
float32 CPowerSourceView::GetVoltage(void) const { return mfVoltage; }
void CPowerSourceView::SetRiseTime(int64 iRiseTime) { miRiseTime = iRiseTime; }
int64 CPowerSourceView::GetRiseTime(void) const { return miRiseTime; }
void CPowerSourceView::SetOnAtStart(bool bOnAtStart) { mbOnAtStart = bOnAtStart; }
bool CPowerSourceView::IsOnAtStart(void) const { return mbOnAtStart; }
bool CPowerSourceView::IsFed(void) const { return mfDropout >= 0.0f; }
float32 CPowerSourceView::GetDropout(void) const { return mfDropout; }


//////////////////////////////////////////////////////////////////////////
//
// The rise time, in nanoseconds, if it has one, off at start if it is,
// and the dropout if it is fed.
//
//////////////////////////////////////////////////////////////////////////
void CPowerSourceView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CComponentView::SaveXMLAttributes(pcWriter);
	if (miRiseTime > 0)
	{
		pcWriter->Attribute("riseTime", TimeToNanoseconds(miRiseTime));
	}
	if (!mbOnAtStart)
	{
		pcWriter->Attribute("onAtStart", false);
	}
	if (IsFed())
	{
		pcWriter->Attribute("dropout", mfDropout);
	}
	if (!mszRail.Empty())
	{
		pcWriter->Attribute("rail", mszRail.Text());
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The attributes SaveXMLAttributes writes, each optional.
//
//////////////////////////////////////////////////////////////////////////
bool CPowerSourceView::LoadXMLAfterInit(CMarkupTag* pcTag)
{
	float64	fRiseTime;
	bool	bOnAtStart;

	if (!CComponentView::LoadXMLAfterInit(pcTag))
	{
		return false;
	}
	if (pcTag->GetFloat64Attribute("riseTime", &fRiseTime))
	{
		if (!(fRiseTime >= 0))
		{
			return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a riseTime of 0 or more nanoseconds.", NULL);
		}
		miRiseTime = NanosecondsToTime(fRiseTime);
	}
	if (pcTag->GetBoolAttribute("onAtStart", &bOnAtStart))
	{
		mbOnAtStart = bOnAtStart;
	}
	if (pcTag->GetStringAttribute("rail") != NULL)
	{
		SetRail(pcTag->GetStringAttribute("rail"));
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The rise time in milliseconds and whether it is on at start, for a
// view that lists them (CPositivePowerView).
//
//////////////////////////////////////////////////////////////////////////
void CPowerSourceView::InitAfterProperties(CComponentProperties* pcProperties)
{
	CComponentView::InitAfterProperties(pcProperties);
	SetRiseTime(NanosecondsToTime(pcProperties->GetFloat("riseTime", TimeToNanoseconds(miRiseTime) / 1000000.0) * 1000000.0));
	SetOnAtStart(pcProperties->GetBoolean("onAtStart", mbOnAtStart));
	SetRail(pcProperties->GetText("rail", mszRail.Text()));
}


//////////////////////////////////////////////////////////////////////////
//
// Three shrinking bars beyond a stem left from the port.
//
//////////////////////////////////////////////////////////////////////////
void CGroundView::Init(int32 x, int32 y, ERotation eRotation, const char* szName)
{
	float64		fOffset;

	CPowerSourceView::Init(x, y, eRotation, szName, 0.0f);

	fOffset = -0.5;
	mcShapes.AddLine(fOffset, -0.9, fOffset, 0.9);
	mcShapes.AddLine(fOffset - 0.333, -0.6, fOffset - 0.333, 0.6);
	mcShapes.AddLine(fOffset - 0.666, -0.3, fOffset - 0.666, 0.3);
	mcShapes.AddLine(0, 0, fOffset, 0);
	AddPortView(POWER_PIN_POWER, 0, 0);
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CGroundView::GetType(void)
{
	return "Ground";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CGroundView::GetXMLName(void) const
{
	return "Ground";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CGroundView::LoadXML(CMarkupTag* pcTag)
{
	int32		x;
	int32		y;
	ERotation	eRotation;
	char*		szName;

	if (!LoadXMLPlacement(pcTag, &x, &y, &eRotation, &szName))
	{
		return false;
	}
	Init(x, y, eRotation, szName);
	return true;
}



//////////////////////////////////////////////////////////////////////////
//
// The box runs from 1 left of the position to 0.4 right of it, with its
// point at the port.
//
//////////////////////////////////////////////////////////////////////////
void CPositivePowerView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, float32 fVoltage)
{
	SFloat64Vec2		asPoints[5];
	SComponentShape*	psShape;
	CChars				szVoltage;

	CPowerSourceView::Init(x, y, eRotation, szName, fVoltage);

	asPoints[0].Init(1, 0);
	asPoints[1].Init(0.4, -0.9);
	asPoints[2].Init(-1, -0.9);
	asPoints[3].Init(-1, 0.9);
	asPoints[4].Init(0.4, 0.9);
	psShape = mcShapes.AddPolygon(asPoints, 5, true, true);
	if (fVoltage > 4.0f)
	{
		psShape->eFill = SC_DarkShape;
	}

	szVoltage.Init();
	AppendVoltage(&szVoltage, fVoltage, false);
	muiVoltageText = mcShapes.NumShapes();
	mcShapes.AddCentredText(-0.1, 0, szVoltage.Text(), CF_SansSerifBold, 11);
	szVoltage.Kill();

	AddPortView(POWER_PIN_POWER, 1, 0);
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
// Makes it a regulator, fed through an "In" port at the back of its box.
//
//////////////////////////////////////////////////////////////////////////
void CPositivePowerView::SetFeed(float32 fDropout)
{
	if (!IsFed())
	{
		AddPortView(POWER_PIN_FEED, -1, 0);
		CalculateBounds();
	}
	mfDropout = fDropout;
}


//////////////////////////////////////////////////////////////////////////
//
// The rail its In port joins wherever it is.  After SetFeed.
//
//////////////////////////////////////////////////////////////////////////
void CPositivePowerView::SetFeedRail(const char* szFeedRail)
{
	CPortView*	pcIn;

	pcIn = GetPortView(POWER_PIN_FEED);
	if (pcIn != NULL)
	{
		pcIn->SetConnectionName(szFeedRail);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The rail its In port joins, NULL for none.
//
//////////////////////////////////////////////////////////////////////////
const char* CPositivePowerView::GetFeedRail(void)
{
	CPortView*	pcIn;

	pcIn = GetPortView(POWER_PIN_FEED);
	if (pcIn == NULL)
	{
		return NULL;
	}
	return pcIn->GetConnectionName();
}


//////////////////////////////////////////////////////////////////////////
//
// A supply's attributes, and a regulator's dropout.
//
//////////////////////////////////////////////////////////////////////////
bool CPositivePowerView::LoadXMLAfterInit(CMarkupTag* pcTag)
{
	float32	fDropout;

	if (!CPowerSourceView::LoadXMLAfterInit(pcTag))
	{
		return false;
	}
	if (pcTag->GetFloat32Attribute("dropout", &fDropout))
	{
		if (!(fDropout >= 0))
		{
			return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a dropout of 0 or more.", NULL);
		}
		SetFeed(fDropout);
		if (pcTag->GetStringAttribute("feedRail") != NULL)
		{
			SetFeedRail(pcTag->GetStringAttribute("feedRail"));
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPositivePowerView::InitAfterProperties(CComponentProperties* pcProperties)
{
	CPowerSourceView::InitAfterProperties(pcProperties);
	if (pcProperties->GetBoolean("fed", false))
	{
		SetFeed((float32)pcProperties->GetFloat("dropout", 0.0));
		SetFeedRail(pcProperties->GetText("feedRail", ""));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The voltage is written in its trace colour.
//
//////////////////////////////////////////////////////////////////////////
void CPositivePowerView::Paint(CCircuitPainter* pcPainter)
{
	mcShapes.SetBorder(muiVoltageText, SC_Explicit, pcPainter->GetColours()->GetTraceVoltage(mfVoltage));
	CPowerSourceView::Paint(pcPainter);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CPositivePowerView::GetType(void)
{
	return "Positive Power";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CPositivePowerView::GetXMLName(void) const
{
	return "PositivePower";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPositivePowerView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CPowerSourceView::SaveXMLAttributes(pcWriter);
	pcWriter->Attribute("voltage", mfVoltage);
	if (IsFed() && (GetFeedRail() != NULL))
	{
		pcWriter->Attribute("feedRail", GetFeedRail());
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The voltage is in Volts.
//
//////////////////////////////////////////////////////////////////////////
bool CPositivePowerView::LoadXML(CMarkupTag* pcTag)
{
	int32		x;
	int32		y;
	ERotation	eRotation;
	char*		szName;
	float32		fVoltage;

	if (!LoadXMLPlacement(pcTag, &x, &y, &eRotation, &szName))
	{
		return false;
	}
	if (!pcTag->GetFloat32Attribute("voltage", &fVoltage) || !(fVoltage > 0))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a voltage above 0.", NULL);
	}
	Init(x, y, eRotation, szName, fVoltage);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
bool CGroundView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	Init(x, y, eRotation, "");
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CGroundView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	Init(x, y, eRotation, pcProperties->GetText("name", ""));
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's PositivePowerProperties.
//
//////////////////////////////////////////////////////////////////////////
void CPositivePowerView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CPowerSourceView::GetProperties(pcProperties, psContext);
	pcProperties->AddFloat("voltage", "V", mfVoltage, 0.0, 100.0);
	pcProperties->AddFloat("riseTime", "ms", TimeToNanoseconds(miRiseTime) / 1000000.0, 0.0, 1000.0);
	pcProperties->AddBoolean("onAtStart", mbOnAtStart);
	pcProperties->AddBoolean("fed", IsFed());
	pcProperties->AddFloat("dropout", "V", IsFed() ? mfDropout : 0.0, 0.0, 100.0);
	if (IsFed() && (GetFeedRail() != NULL))
	{
		pcProperties->AddText("feedRail", GetFeedRail());
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Placed new at 3.3V.
//
//////////////////////////////////////////////////////////////////////////
bool CPositivePowerView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	Init(x, y, eRotation, "", 3.3f);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CPositivePowerView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	Init(x, y, eRotation, pcProperties->GetText("name", ""), (float32)pcProperties->GetFloat("voltage", mfVoltage));
	return true;
}
