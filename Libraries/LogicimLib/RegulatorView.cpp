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
#include "CircuitPainter.h"
#include "CircuitColours.h"
#include "ComponentProperties.h"
#include "RegulatorView.h"


//////////////////////////////////////////////////////////////////////////
//
// The box is 8 grid units wide and 4 high, about the position, its
// bottom corners cut 1 unit in at 45 degrees.  IN and OUT are at the
// middle of its sides, GND at the middle of its bottom.
//
//////////////////////////////////////////////////////////////////////////
void CRegulatorView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, float32 fVoltage, float32 fDropout, const char* szPart)
{
	SFloat64Vec2		asPoints[6];
	SComponentShape*	psShape;
	CChars				szVoltage;
	CPortView*			pcPortView;

	CPowerSourceView::Init(x, y, eRotation, szName, fVoltage);
	mfDropout = fDropout;
	mszPart.Init(szPart);

	asPoints[0].Init(-4, -2);
	asPoints[1].Init(4, -2);
	asPoints[2].Init(4, 1);
	asPoints[3].Init(3, 2);
	asPoints[4].Init(-3, 2);
	asPoints[5].Init(-4, 1);
	psShape = mcShapes.AddPolygon(asPoints, 6, true, true);
	if (fVoltage > 4.0f)
	{
		psShape->eFill = SC_DarkShape;
	}

	mcShapes.AddText(0, -1.1, mszPart.Empty() ? mszName.Text() : mszPart.Text(), CF_SansSerif, REGULATOR_FONT_SIZE, TA_Centre);
	szVoltage.Init();
	AppendVoltage(&szVoltage, fVoltage, true);
	muiVoltageText = mcShapes.NumShapes();
	mcShapes.AddCentredText(0, 0, szVoltage.Text(), CF_SansSerifBold, REGULATOR_VOLTAGE_FONT_SIZE);
	szVoltage.Kill();
	mcShapes.AddText(-3.4, 0, "IN", CF_SansSerif, REGULATOR_FONT_SIZE, TA_Left);
	mcShapes.AddText(3.4, 0, "OUT", CF_SansSerif, REGULATOR_FONT_SIZE, TA_Right);
	mcShapes.AddText(0, 1.3, "GND", CF_SansSerif, REGULATOR_FONT_SIZE, TA_Centre);

	pcPortView = AddPortView(POWER_PIN_POWER, 4, 0);
	pcPortView->SetText("OUT");
	pcPortView = AddPortView(POWER_PIN_FEED, -4, 0);
	pcPortView->SetText("IN");
	pcPortView = AddPortView(POWER_PIN_GND, 0, 2);
	pcPortView->SetText("GND");
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CRegulatorView::Kill(void)
{
	mszPart.Kill();
	CPowerSourceView::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The regulator joins nothing by name: only its OUT port is on its rail.
//
//////////////////////////////////////////////////////////////////////////
const char* CRegulatorView::GetConnectionName(void)
{
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Its OUT port joins the rail's nets wherever it is; IN and GND don't.
//
//////////////////////////////////////////////////////////////////////////
void CRegulatorView::SetRail(const char* szRail)
{
	CPortView*	pcOut;

	CPowerSourceView::SetRail(szRail);
	pcOut = GetPortView(POWER_PIN_POWER);
	if (pcOut != NULL)
	{
		pcOut->SetConnectionName(szRail);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Its supply is fed, so it has In and GND ports to connect.
//
//////////////////////////////////////////////////////////////////////////
void CRegulatorView::InitPowerSource(CPowerSource* pcPowerSource)
{
	pcPowerSource->SetFeed(mfDropout);
}


//////////////////////////////////////////////////////////////////////////
//
// The voltage is written in its trace colour.
//
//////////////////////////////////////////////////////////////////////////
void CRegulatorView::Paint(CCircuitPainter* pcPainter)
{
	mcShapes.SetBorder(muiVoltageText, SC_Explicit, pcPainter->GetColours()->GetTraceVoltage(mfVoltage));
	CPowerSourceView::Paint(pcPainter);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CRegulatorView::GetType(void) { return "Regulator"; }
const char* CRegulatorView::GetXMLName(void) const { return "Regulator"; }
float32 CRegulatorView::GetDropout(void) const { return mfDropout; }
const char* CRegulatorView::GetPart(void) { return mszPart.Text(); }


//////////////////////////////////////////////////////////////////////////
//
// A supply's attributes, its voltage, its dropout and its part if it has
// one.
//
//////////////////////////////////////////////////////////////////////////
void CRegulatorView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CPowerSourceView::SaveXMLAttributes(pcWriter);
	pcWriter->Attribute("voltage", mfVoltage);
	pcWriter->Attribute("dropout", mfDropout);
	if (!mszPart.Empty())
	{
		pcWriter->Attribute("part", mszPart.Text());
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The voltage and dropout are in Volts.
//
//////////////////////////////////////////////////////////////////////////
bool CRegulatorView::LoadXML(CMarkupTag* pcTag)
{
	int32		x;
	int32		y;
	ERotation	eRotation;
	char*		szName;
	float32		fVoltage;
	float32		fDropout;
	char*		szPart;

	if (!LoadXMLPlacement(pcTag, &x, &y, &eRotation, &szName))
	{
		return false;
	}
	if (!pcTag->GetFloat32Attribute("voltage", &fVoltage) || !(fVoltage > 0))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a voltage above 0.", NULL);
	}
	if (!pcTag->GetFloat32Attribute("dropout", &fDropout) || !(fDropout >= 0))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a dropout of 0 or more.", NULL);
	}
	szPart = pcTag->GetStringAttribute("part");
	Init(x, y, eRotation, szName, fVoltage, fDropout, (szPart != NULL) ? szPart : "");
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CRegulatorView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CPowerSourceView::GetProperties(pcProperties, psContext);
	pcProperties->AddText("part", mszPart.Text());
	pcProperties->AddFloat("voltage", "V", mfVoltage, 0.0, 100.0);
	pcProperties->AddFloat("dropout", "V", mfDropout, 0.0, 100.0);
	pcProperties->AddFloat("riseTime", "ms", TimeToNanoseconds(miRiseTime) / 1000000.0, 0.0, 1000.0);
}


//////////////////////////////////////////////////////////////////////////
//
// Placed new as a 5V regulator with a 1V dropout.
//
//////////////////////////////////////////////////////////////////////////
bool CRegulatorView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	Init(x, y, eRotation, "", 5.0f, 1.0f, "");
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CRegulatorView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	Init(x, y, eRotation, pcProperties->GetText("name", ""), (float32)pcProperties->GetFloat("voltage", 5.0), (float32)pcProperties->GetFloat("dropout", 1.0), pcProperties->GetText("part", ""));
	return true;
}
