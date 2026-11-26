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
#include "MicroprocessorLib/Pull.h"
#include "MicroprocessorLib/Voltage.h"
#include "ComponentPlacement.h"
#include "ComponentProperties.h"
#include "CircuitFonts.h"
#include "IntegratedCircuitView.h"
#include "PullView.h"


#define PULL_VIEW_FONT_SIZE		8.0


const char* gaszPullDirectionNames[] = { "Up", "Down" };


//////////////////////////////////////////////////////////////////////////
//
// A resistance like "10k", "4.7k", "330" or "1M".
//
//////////////////////////////////////////////////////////////////////////
static void AppendPullResistance(CChars* psz, float32 fResistance)
{
	const char*	szMultiplier;
	float64		fValue;
	int64		iTenths;

	szMultiplier = "";
	fValue = fResistance;
	if (fValue >= 1e6)
	{
		fValue /= 1e6;
		szMultiplier = "M";
	}
	else if (fValue >= 1e3)
	{
		fValue /= 1e3;
		szMultiplier = "k";
	}
	iTenths = (int64)(fValue * 10 + 0.5);
	psz->Append(iTenths / 10);
	if (iTenths % 10 != 0)
	{
		psz->Append('.');
		psz->Append(iTenths % 10);
	}
	psz->Append(szMultiplier);
}


//////////////////////////////////////////////////////////////////////////
//
// The resistor runs left from the signal, on the right at (2, 0), to a
// supply's bar or a ground symbol.
//
//////////////////////////////////////////////////////////////////////////
void CPullView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, float32 fResistance, EPullDirection eDirection, float32 fVoltage)
{
	CChars	szText;

	CComponentView::Init(x, y, eRotation, szName);
	mfResistance = fResistance;
	meDirection = eDirection;
	mfVoltage = (eDirection == PD_Down) ? 0.0f : fVoltage;
	mfCapacitance = 0.0f;
	mszRail.Init();

	mcShapes.AddLine(1.4, 0, 2, 0);
	mcShapes.AddRectangle(-0.6, -0.35, 1.4, 0.35, false, true);
	mcShapes.AddLine(-0.6, 0, -1.2, 0);
	if (eDirection == PD_Up)
	{
		mcShapes.AddLine(-1.2, -0.9, -1.2, 0.9);
		szText.Init();
		AppendVoltage(&szText, mfVoltage, true);
		mcShapes.AddText(-1.5, 0, szText.Text(), CF_SansSerif, PULL_VIEW_FONT_SIZE, TA_Right);
		szText.Kill();
	}
	else
	{
		mcShapes.AddLine(-1.2, -0.9, -1.2, 0.9);
		mcShapes.AddLine(-1.533, -0.6, -1.533, 0.6);
		mcShapes.AddLine(-1.866, -0.3, -1.866, 0.3);
	}
	szText.Init();
	AppendPullResistance(&szText, fResistance);
	mcShapes.AddText(0.4, -0.9, szText.Text(), CF_SansSerif, PULL_VIEW_FONT_SIZE, TA_Centre);
	szText.Kill();

	AddPortView(PULL_PIN_SIGNAL, 2, 0);
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPullView::Kill(void)
{
	mszRail.Kill();
	CComponentView::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Its supply joins the rail named szRail, by name, as a hidden port.
//
//////////////////////////////////////////////////////////////////////////
void CPullView::SetRail(const char* szRail)
{
	if (!mszRail.Empty() || StrEmpty(szRail))
	{
		return;
	}
	mszRail.Set(szRail);
	AddHiddenPortView(PULL_PIN_SUPPLY, szRail);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CPullView::GetType(void)
{
	return "Pull";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CPullView::GetXMLName(void) const
{
	return "Pull";
}


//////////////////////////////////////////////////////////////////////////
//
// The resistance in ohms, the direction, and the supply: a rail, or the
// voltage of its own supply when it pulls up.
//
//////////////////////////////////////////////////////////////////////////
void CPullView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CComponentView::SaveXMLAttributes(pcWriter);
	pcWriter->Attribute("resistance", mfResistance);
	pcWriter->Attribute("direction", gaszPullDirectionNames[meDirection]);
	if (!mszRail.Empty())
	{
		pcWriter->Attribute("rail", mszRail.Text());
	}
	else if (meDirection == PD_Up)
	{
		pcWriter->Attribute("voltage", mfVoltage);
	}
	if (mfCapacitance > 0.0f)
	{
		pcWriter->Attribute("capacitance", mfCapacitance * 1e12f);
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CPullView::LoadXML(CMarkupTag* pcTag)
{
	int32		x;
	int32		y;
	ERotation	eRotation;
	char*		szName;
	float32		fResistance;
	char*		szDirection;
	float32		fVoltage;
	float32		fCapacitance;
	int			iDirection;

	if (!LoadXMLPlacement(pcTag, &x, &y, &eRotation, &szName))
	{
		return false;
	}
	if (!pcTag->GetFloat32Attribute("resistance", &fResistance) || !(fResistance > 0))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a resistance above 0 ohms.", NULL);
	}
	szDirection = pcTag->GetStringAttribute("direction");
	iDirection = -1;
	if (StringCompare(szDirection, gaszPullDirectionNames[PD_Up]) == 0)
	{
		iDirection = PD_Up;
	}
	else if (StringCompare(szDirection, gaszPullDirectionNames[PD_Down]) == 0)
	{
		iDirection = PD_Down;
	}
	if (iDirection < 0)
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a direction, Up or Down.", NULL);
	}
	fVoltage = 0.0f;
	if ((pcTag->GetStringAttribute("voltage") != NULL) && !pcTag->GetFloat32Attribute("voltage", &fVoltage))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "]'s voltage must be a number.", NULL);
	}
	fCapacitance = 0.0f;
	if ((pcTag->GetStringAttribute("capacitance") != NULL) && (!pcTag->GetFloat32Attribute("capacitance", &fCapacitance) || !(fCapacitance > 0)))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "]'s capacitance must be picofarads above 0.", NULL);
	}
	Init(x, y, eRotation, szName, fResistance, (EPullDirection)iDirection, fVoltage);
	SetRail(pcTag->GetStringAttribute("rail"));
	SetCapacitance(fCapacitance * 1e-12f);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The pull, and without a rail a supply of its own for it.
//
//////////////////////////////////////////////////////////////////////////
CCircuitComponent* CPullView::CreateComponent(CViewPath* pcViewPath, CCircuitSimulation* pcCircuitSimulation, CFamilyVoltageConfigurationStore* pcFamilies)
{
	CSubcircuitSimulation*		pcSimulation;
	CPull*						pcPull;
	CArrayCircuitComponentPtr	apcSupplies;
	bool						bResult;

	pcSimulation = CanCreateComponent(pcViewPath, pcCircuitSimulation);
	if (pcSimulation == NULL)
	{
		return NULL;
	}
	pcPull = &OMalloc<CPull>(pcSimulation, mszName.Text(), mfResistance);
	pcPull->SetCapacitance(mfCapacitance);
	apcSupplies.Init();
	if (mszRail.Empty())
	{
		AddImplicitSupply(pcSimulation, pcPull->GetSupplyPort(), mfVoltage, &apcSupplies);
	}
	bResult = PostCreateComponent(pcViewPath, pcCircuitSimulation, pcPull, &apcSupplies);
	apcSupplies.Kill();
	return bResult ? pcPull : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPullView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CComponentView::GetProperties(pcProperties, psContext);
	pcProperties->AddFloat("resistance", "ohm", mfResistance, 0.001, 1e9);
	pcProperties->AddChoice("direction", gaszPullDirectionNames, PD_Down + 1, meDirection);
	pcProperties->AddFloat("voltage", "V", mfVoltage, 0.0, 100.0);
	pcProperties->AddText("rail", mszRail.Text());
	pcProperties->AddFloat("capacitance", "pF", mfCapacitance * 1e12, 0.0, 1e12);
}


//////////////////////////////////////////////////////////////////////////
//
// Placed new as a 10k pull-up to 3.3V.
//
//////////////////////////////////////////////////////////////////////////
bool CPullView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	CChars	szName;

	szName.Init();
	GetFreeReference(&szName, pcCircuit, "R");
	Init(x, y, eRotation, szName.Text(), 10000.0f, PD_Up, 3.3f);
	szName.Kill();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CPullView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	Init(x, y, eRotation, pcProperties->GetText("name", ""), (float32)pcProperties->GetFloat("resistance", mfResistance), (EPullDirection)pcProperties->GetChoice("direction", meDirection), (float32)pcProperties->GetFloat("voltage", mfVoltage));
	SetRail(pcProperties->GetText("rail", ""));
	SetCapacitance((float32)(pcProperties->GetFloat("capacitance", mfCapacitance * 1e12) * 1e-12));
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPullView::SetCapacitance(float32 fCapacitance) { mfCapacitance = fCapacitance; }
float32 CPullView::GetCapacitance(void) const { return mfCapacitance; }
const char* CPullView::GetRail(void) { return mszRail.Text(); }
float32 CPullView::GetResistance(void) const { return mfResistance; }
EPullDirection CPullView::GetDirection(void) const { return meDirection; }
float32 CPullView::GetVoltage(void) const { return mfVoltage; }

