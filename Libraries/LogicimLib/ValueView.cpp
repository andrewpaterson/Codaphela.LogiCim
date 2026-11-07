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
#include <math.h>
#include "BaseLib/ConstructorCall.h"
#include "BaseLib/Logger.h"
#include "MicroprocessorLib/Constant.h"
#include "MicroprocessorLib/Probe.h"
#include "MicroprocessorLib/LogicPort.h"
#include "MicroprocessorLib/PowerPinNames.h"
#include "MicroprocessorLib/Pin.h"
#include "MicroprocessorLib/Trace.h"
#include "MicroprocessorLib/Voltage.h"
#include "MicroprocessorLib/FamilyVoltageConfigurationStore.h"
#include "MicroprocessorLib/CircuitSimulation.h"
#include "BaseLib/StringHelper.h"
#include "ComponentStateXML.h"
#include "ComponentPlacement.h"
#include "ValueView.h"


//////////////////////////////////////////////////////////////////////////
//
// Subclasses add their shapes, the port views, the value text and then
// CalculateBounds.
//
//////////////////////////////////////////////////////////////////////////
void CValueView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int32 iBitWidth, ERadix eRadix, SInt32Bounds* psBox)
{
	CIntegratedCircuitView::Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts);
	miBitWidth = iBitWidth;
	meRadix = eRadix;
	miMaxDigits = CalculateMaxDigits(eRadix, iBitWidth);
	msBox = *psBox;
	muiValueText = 0;
}


//////////////////////////////////////////////////////////////////////////
//
// The box for the widest value, as Logicim's createGraphics: its length is
// the width of that many monospaced Xs and half a grid unit, rounded up.
//
//////////////////////////////////////////////////////////////////////////
void CValueView::MeasureBox(CCircuitFonts* pcFonts, int32 iBitWidth, ERadix eRadix, SInt32Bounds* psBox)
{
	CChars				szWidest;
	SCircuitTextSize	sSize;
	int32				iLength;

	szWidest.Init();
	szWidest.Append('X', (size)CalculateMaxDigits(eRadix, iBitWidth));
	sSize = pcFonts->Measure(CF_Monospaced, VALUE_FONT_SIZE, szWidest.Text());
	szWidest.Kill();

	iLength = (int32)ceil(sSize.fWidth + 0.5);
	psBox->Init(SInt32Vec2(-iLength, -1), SInt32Vec2(0, 1));
}


//////////////////////////////////////////////////////////////////////////
//
// The bus at the position and, with explicit power, VCC and GND on the
// box's top and bottom a grid unit along.
//
//////////////////////////////////////////////////////////////////////////
void CValueView::CreateValuePortViews(const char* szPortName)
{
	CArrayChars		aszNames;
	CChars			szName;
	int32			i;

	aszNames.Init();
	for (i = 0; i < miBitWidth; i++)
	{
		szName.Init(szPortName);
		szName.Append(' ');
		szName.Append(i);
		aszNames.Add(szName.Text());
		szName.Kill();
	}
	AddPortView(&aszNames, 0, 0);
	aszNames.Kill();

	if (mbExplicitPowerPorts)
	{
		AddPortView(POWER_PIN_VCC, msBox.sMaximum.x - 1, msBox.sMinimum.y);
		AddPortView(POWER_PIN_GND, msBox.sMaximum.x - 1, msBox.sMaximum.y);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim draws these views turned twice to the right, so their text
// faces the other way to the component's rotation.
//
//////////////////////////////////////////////////////////////////////////
void CValueView::AddValueText(void)
{
	SFloat64Vec2	sCentre;

	sCentre = GetBoxCentre();
	muiValueText = mcShapes.NumShapes();
	mcShapes.AddText(sCentre.x, sCentre.y, "", CF_Monospaced, VALUE_FONT_SIZE, TA_Centre, 2);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SFloat64Vec2 CValueView::GetBoxCentre(void)
{
	SFloat64Vec2	sCentre;

	sCentre.Init((msBox.sMinimum.x + msBox.sMaximum.x) / 2.0, (msBox.sMinimum.y + msBox.sMaximum.y) / 2.0);
	return sCentre;
}


//////////////////////////////////////////////////////////////////////////
//
// The value on the bus in the circuit simulation of the path at its time,
// or nothing without a simulation or a component in it (as Logicim's
// RadixHelper.getStringValue).
//
//////////////////////////////////////////////////////////////////////////
void CValueView::GetValueText(CChars* psz, CViewPath* pcViewPath, CCircuitSimulation* pcCircuitSimulation)
{
	CPortView*			pcBus;
	STraceValue			asValues[RADIX_MAX_BIT_WIDTH];
	int32				i;
	CPort*				pcPort;
	CCircuitComponent*	pcComponent;
	int64				iTime;

	if (pcCircuitSimulation == NULL)
	{
		return;
	}
	pcComponent = GetComponent(pcViewPath, pcCircuitSimulation);
	pcBus = GetPortView((size)0);
	if ((pcComponent == NULL) || (pcBus == NULL) || (pcBus->NumPorts(pcViewPath, pcCircuitSimulation) != (size)miBitWidth))
	{
		return;
	}

	iTime = pcCircuitSimulation->GetTime();
	for (i = 0; i < miBitWidth; i++)
	{
		pcPort = pcBus->GetPort(pcViewPath, pcCircuitSimulation, i);
		asValues[i] = ReadPortValue(pcComponent, pcPort, iTime);
	}
	AppendTraceValues(psz, asValues, miBitWidth, meRadix, miMaxDigits);
}


//////////////////////////////////////////////////////////////////////////
//
// An IC's ports are logic ports.
//
//////////////////////////////////////////////////////////////////////////
STraceValue CValueView::ReadPortValue(CCircuitComponent* pcComponent, CPort* pcPort, int64 iTime)
{
	return ((CLogicPort*)pcPort)->ReadValue(iTime);
}


//////////////////////////////////////////////////////////////////////////
//
// The value is written as it is in the painter's simulation.
//
//////////////////////////////////////////////////////////////////////////
void CValueView::Paint(CCircuitPainter* pcPainter)
{
	CChars	szValue;

	szValue.Init();
	GetValueText(&szValue, pcPainter->GetViewPath(), pcPainter->GetCircuitSimulation());
	mcShapes.SetText(muiValueText, szValue.Text());
	szValue.Kill();

	CIntegratedCircuitView::Paint(pcPainter);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CValueView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CIntegratedCircuitView::SaveXMLAttributes(pcWriter);
	pcWriter->Attribute("bitWidth", miBitWidth);
	pcWriter->Attribute("radix", RadixToString(meRadix));
	SaveXMLBox(pcWriter, &msBox);
}


//////////////////////////////////////////////////////////////////////////
//
// A value view saved with its placement, family, bits, radix and box (see
// InitValue).
//
//////////////////////////////////////////////////////////////////////////
bool CValueView::LoadXML(CMarkupTag* pcTag)
{
	SValueViewXML	sValue;

	if (!LoadValueXML(pcTag, &sValue))
	{
		return false;
	}
	return InitValue(&sValue, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
// A value view from its properties, the view's own for any missing, its
// box measured with the fonts (see InitValue).
//
//////////////////////////////////////////////////////////////////////////
bool CValueView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	SValueViewXML	sValue;

	GetIntegratedCircuitProperties(x, y, eRotation, pcProperties, &sValue.sIntegratedCircuit);
	sValue.iBitWidth = (int32)pcProperties->GetInteger("bitWidth", miBitWidth);
	sValue.eRadix = GetRadixProperty(pcProperties, meRadix);
	sValue.sBox.Init();
	return InitValue(&sValue, psContext->pcFonts);
}


//////////////////////////////////////////////////////////////////////////
//
// Made from what a value view is saved with, its box measured with the
// fonts unless they're NULL, so LoadXML and InitFromProperties needn't be
// each view's own.
//
//////////////////////////////////////////////////////////////////////////
bool CValueView::InitValue(SValueViewXML* psValue, CCircuitFonts* pcFonts)
{
	return gcLogger.Error2(__METHOD__, " [", GetType(), "] isn't made from a value view's settings.", NULL);
}


//////////////////////////////////////////////////////////////////////////
//
// The box is loaded rather than measured, so the component keeps the size
// it was made with.
//
//////////////////////////////////////////////////////////////////////////
bool CValueView::LoadValueXML(CMarkupTag* pcTag, SValueViewXML* psValue)
{
	char*	szRadix;

	if (!LoadIntegratedCircuitXML(pcTag, &psValue->sIntegratedCircuit))
	{
		return false;
	}
	if (!pcTag->GetInt32Attribute("bitWidth", &psValue->iBitWidth) || (psValue->iBitWidth < 1) || (psValue->iBitWidth > RADIX_MAX_BIT_WIDTH))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a bitWidth from 1 to 64.", NULL);
	}
	szRadix = pcTag->GetStringAttribute("radix");
	psValue->eRadix = StringToRadix(szRadix);
	if (psValue->eRadix == RADIX_Cannot)
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] has no radix or an unknown one [", (szRadix != NULL) ? szRadix : "", "].", NULL);
	}
	return LoadXMLBox(pcTag, &psValue->sBox);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int32 CValueView::GetBitWidth(void) { return miBitWidth; }
ERadix CValueView::GetRadix(void) { return meRadix; }
void CValueView::GetBox(SInt32Bounds* psBox) { *psBox = msBox; }


//////////////////////////////////////////////////////////////////////////
//
// A new constant, its box measured to fit its widest value.
//
//////////////////////////////////////////////////////////////////////////
void CConstantView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int32 iBitWidth, ERadix eRadix, CCircuitFonts* pcFonts)
{
	SInt32Bounds	sBox;

	MeasureBox(pcFonts, iBitWidth, eRadix, &sBox);
	Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iBitWidth, eRadix, &sBox);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CConstantView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int32 iBitWidth, ERadix eRadix, SInt32Bounds* psBox)
{
	CValueView::Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iBitWidth, eRadix, psBox);
	mcShapes.AddRectangle(msBox.sMinimum.x, msBox.sMinimum.y, msBox.sMaximum.x, msBox.sMaximum.y, true, true);
	CreateValuePortViews("Output");
	AddValueText();
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CConstantView::GetType(void)
{
	return "Constant";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CConstantView::GetXMLName(void)
{
	return "Constant";
}


//////////////////////////////////////////////////////////////////////////
//
// Placed new two bits wide, in binary.
//
//////////////////////////////////////////////////////////////////////////
bool CConstantView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	Init(x, y, eRotation, "", PLACED_FAMILY, false, 2, RADIX_Binary, pcFonts);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
bool CConstantView::InitValue(SValueViewXML* psValue, CCircuitFonts* pcFonts)
{
	if (pcFonts != NULL)
	{
		Init(psValue->sIntegratedCircuit.x, psValue->sIntegratedCircuit.y, psValue->sIntegratedCircuit.eRotation, psValue->sIntegratedCircuit.szName, psValue->sIntegratedCircuit.szFamily, psValue->sIntegratedCircuit.bExplicitPowerPorts, psValue->iBitWidth, psValue->eRadix, pcFonts);
	}
	else
	{
		Init(psValue->sIntegratedCircuit.x, psValue->sIntegratedCircuit.y, psValue->sIntegratedCircuit.eRotation, psValue->sIntegratedCircuit.szName, psValue->sIntegratedCircuit.szFamily, psValue->sIntegratedCircuit.bExplicitPowerPorts, psValue->iBitWidth, psValue->eRadix, &psValue->sBox);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The value starts at 0, as Logicim's.
//
//////////////////////////////////////////////////////////////////////////
CIntegratedCircuit* CConstantView::CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily)
{
	CConstant*		pcConstant;

	pcConstant = &OMalloc<CConstant>(pcSimulation, mszName.Text(), OMalloc<CConstantPins>(pcFamily, miBitWidth), 0);
	return pcConstant;
}


//////////////////////////////////////////////////////////////////////////
//
// A new probe, its box measured to fit its widest value.
//
//////////////////////////////////////////////////////////////////////////
void CProbeView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int32 iBitWidth, ERadix eRadix, CCircuitFonts* pcFonts)
{
	SInt32Bounds	sBox;

	MeasureBox(pcFonts, iBitWidth, eRadix, &sBox);
	Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iBitWidth, eRadix, &sBox);
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's FrameView with arcs of one grid unit is the box with its
// corners rounded by one.
//
//////////////////////////////////////////////////////////////////////////
void CProbeView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int32 iBitWidth, ERadix eRadix, SInt32Bounds* psBox)
{
	SFloat64Vec2	sCentre;

	CValueView::Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iBitWidth, eRadix, psBox);
	if (miMaxDigits == 1)
	{
		sCentre = GetBoxCentre();
		mcShapes.AddCircle(sCentre.x, sCentre.y, (msBox.sMaximum.y - msBox.sMinimum.y) / 2.0, true, true);
	}
	else
	{
		mcShapes.AddRoundedRectangle(msBox.sMinimum.x, msBox.sMinimum.y, msBox.sMaximum.x, msBox.sMaximum.y, 1, true, true);
	}
	CreateValuePortViews("Input");
	AddValueText();
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CProbeView::GetType(void)
{
	return "Probe";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CProbeView::GetXMLName(void)
{
	return "Probe";
}


//////////////////////////////////////////////////////////////////////////
//
// Placed new 32 bits wide, in hexadecimal.
//
//////////////////////////////////////////////////////////////////////////
bool CProbeView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	Init(x, y, eRotation, "", PLACED_FAMILY, false, 32, RADIX_Hexadecimal, pcFonts);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
bool CProbeView::InitValue(SValueViewXML* psValue, CCircuitFonts* pcFonts)
{
	if (pcFonts != NULL)
	{
		Init(psValue->sIntegratedCircuit.x, psValue->sIntegratedCircuit.y, psValue->sIntegratedCircuit.eRotation, psValue->sIntegratedCircuit.szName, psValue->sIntegratedCircuit.szFamily, psValue->sIntegratedCircuit.bExplicitPowerPorts, psValue->iBitWidth, psValue->eRadix, pcFonts);
	}
	else
	{
		Init(psValue->sIntegratedCircuit.x, psValue->sIntegratedCircuit.y, psValue->sIntegratedCircuit.eRotation, psValue->sIntegratedCircuit.szName, psValue->sIntegratedCircuit.szFamily, psValue->sIntegratedCircuit.bExplicitPowerPorts, psValue->iBitWidth, psValue->eRadix, &psValue->sBox);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CIntegratedCircuit* CProbeView::CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily)
{
	CProbe*			pcProbe;

	pcProbe = &OMalloc<CProbe>(pcSimulation, mszName.Text(), OMalloc<CProbePins>(pcFamily, miBitWidth));
	return pcProbe;
}


//////////////////////////////////////////////////////////////////////////
//
// A new pin, its box measured to fit its widest value.  It's shown on the
// left of an instance of its subcircuit until SetSubcircuitAppearance.
//
//////////////////////////////////////////////////////////////////////////
void CPinView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int32 iBitWidth, ERadix eRadix, CCircuitFonts* pcFonts)
{
	SInt32Bounds	sBox;

	MeasureBox(pcFonts, iBitWidth, eRadix, &sBox);
	Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iBitWidth, eRadix, &sBox);
}


//////////////////////////////////////////////////////////////////////////
//
// The name is a quarter of a grid unit past the end of the box, turned
// with the value, as big as a tunnel's name.
//
//////////////////////////////////////////////////////////////////////////
void CPinView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, int32 iBitWidth, ERadix eRadix, SInt32Bounds* psBox)
{
	SFloat64Vec2	sCentre;

	CValueView::Init(x, y, eRotation, szName, szFamily, bExplicitPowerPorts, iBitWidth, eRadix, psBox);
	meAlignment = PA_Left;
	meAnchour = PAN_Centre;
	miWeight = 0;
	mbInverting = false;
	mbClockNotch = false;
	mpcFamily = NULL;

	if (miMaxDigits == 1)
	{
		sCentre = GetBoxCentre();
		mcShapes.AddCircle(sCentre.x, sCentre.y, (msBox.sMaximum.y - msBox.sMinimum.y) / 2.0, true, true);
	}
	else
	{
		mcShapes.AddRoundedRectangle(msBox.sMinimum.x, msBox.sMinimum.y, msBox.sMaximum.x, msBox.sMaximum.y, 1, true, true);
	}
	CreateValuePortViews("Port");
	AddValueText();
	mcShapes.AddText(msBox.sMinimum.x - 0.25, 0, szName, CF_SansSerifBold, PIN_NAME_FONT_SIZE, TA_Left, 2);
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPinView::SetSubcircuitAppearance(EPinAlignment eAlignment, EPinAnchour eAnchour, int32 iWeight, bool bInverting, bool bClockNotch)
{
	meAlignment = eAlignment;
	meAnchour = eAnchour;
	miWeight = iWeight;
	mbInverting = bInverting;
	mbClockNotch = bClockNotch;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CPinView::GetType(void)
{
	return "Pin";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CPinView::GetXMLName(void)
{
	return "Pin";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPinView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CValueView::SaveXMLAttributes(pcWriter);
	pcWriter->Attribute("alignment", PinAlignmentToString(meAlignment));
	pcWriter->Attribute("anchour", PinAnchourToString(meAnchour));
	pcWriter->Attribute("weight", miWeight);
	pcWriter->Attribute("inverting", mbInverting);
	pcWriter->Attribute("clockNotch", mbClockNotch);
}


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
bool CPinView::InitValue(SValueViewXML* psValue, CCircuitFonts* pcFonts)
{
	if (pcFonts != NULL)
	{
		Init(psValue->sIntegratedCircuit.x, psValue->sIntegratedCircuit.y, psValue->sIntegratedCircuit.eRotation, psValue->sIntegratedCircuit.szName, psValue->sIntegratedCircuit.szFamily, psValue->sIntegratedCircuit.bExplicitPowerPorts, psValue->iBitWidth, psValue->eRadix, pcFonts);
	}
	else
	{
		Init(psValue->sIntegratedCircuit.x, psValue->sIntegratedCircuit.y, psValue->sIntegratedCircuit.eRotation, psValue->sIntegratedCircuit.szName, psValue->sIntegratedCircuit.szFamily, psValue->sIntegratedCircuit.bExplicitPowerPorts, psValue->iBitWidth, psValue->eRadix, &psValue->sBox);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CPinView::LoadXML(CMarkupTag* pcTag)
{
	SValueViewXML	sValue;
	EPinAlignment	eAlignment;
	EPinAnchour		eAnchour;
	int32			iWeight;
	bool			bInverting;
	bool			bClockNotch;

	if (!LoadValueXML(pcTag, &sValue))
	{
		return false;
	}
	eAlignment = StringToPinAlignment(pcTag->GetStringAttribute("alignment"));
	eAnchour = StringToPinAnchour(pcTag->GetStringAttribute("anchour"));
	if ((eAlignment == PA_Cannot) || (eAnchour == PAN_Cannot))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] has no alignment or anchour or an unknown one.", NULL);
	}
	if (!pcTag->GetInt32Attribute("weight", &iWeight) || !pcTag->GetBoolAttribute("inverting", &bInverting) || !pcTag->GetBoolAttribute("clockNotch", &bClockNotch))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs a whole number weight and inverting and clockNotch true or false.", NULL);
	}
	InitValue(&sValue, NULL);
	SetSubcircuitAppearance(eAlignment, eAnchour, iWeight, bInverting, bClockNotch);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// A pin is a passive; unless its power ports are explicit it's powered at
// its family's default voltage, as an IC is.
//
//////////////////////////////////////////////////////////////////////////
CCircuitComponent* CPinView::CreateComponent(CViewPath* pcViewPath, CCircuitSimulation* pcCircuitSimulation, CFamilyVoltageConfigurationStore* pcFamilies)
{
	CPin*						pcPin;
	CSubcircuitSimulation*		pcSimulation;
	CArrayCircuitComponentPtr	apcSupplies;
	bool						bResult;

	pcSimulation = CanCreateComponent(pcViewPath, pcCircuitSimulation);
	if (pcSimulation == NULL)
	{
		return NULL;
	}

	mpcFamily = pcFamilies->Get(mszFamily.Text());
	if (mpcFamily == NULL)
	{
		gcLogger.Error2(__METHOD__, " Cannot find family [", mszFamily.Text(), "].", NULL);
		return NULL;
	}

	pcPin = &OMalloc<CPin>(pcSimulation, mszName.Text(), miBitWidth);
	apcSupplies.Init();
	if (!mbExplicitPowerPorts)
	{
		if (!PowerPortsImplicitly(pcSimulation, pcPin->GetVoltageCommon(), pcPin->GetVoltageGround(), mpcFamily, &apcSupplies, mfImpliedVcc))
		{
			//Nothing points to the pin yet, so removing it frees it.
			apcSupplies.Kill();
			pcSimulation->GetCircuit()->Remove(pcPin);
			return NULL;
		}
	}
	bResult = PostCreateComponent(pcViewPath, pcCircuitSimulation, pcPin, &apcSupplies);
	apcSupplies.Kill();
	if (!bResult)
	{
		return NULL;
	}
	return pcPin;
}


//////////////////////////////////////////////////////////////////////////
//
// A pin isn't an IC; see CreateComponent.
//
//////////////////////////////////////////////////////////////////////////
CIntegratedCircuit* CPinView::CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily)
{
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CComponentStateXML* CPinView::GetComponentStateXML(void)
{
	return &gcPassiveStateXML;
}


//////////////////////////////////////////////////////////////////////////
//
// The level of the port's trace against the voltage on the pin's VCC, as
// Logicim's PortView.getValue.
//
//////////////////////////////////////////////////////////////////////////
STraceValue CPinView::ReadPortValue(CCircuitComponent* pcComponent, CPort* pcPort, int64 iTime)
{
	STraceValue		sValue;
	CTrace*			pcVCC;
	float32			fVcc;

	sValue.Init(TV_Undriven);
	if ((mpcFamily == NULL) || (pcPort->GetTrace() == NULL))
	{
		return sValue;
	}
	pcVCC = ((CPin*)pcComponent)->GetVoltageCommon()->GetTrace();
	fVcc = (pcVCC != NULL) ? pcVCC->GetVoltage(iTime) : NoVoltage();
	sValue.Init(mpcFamily->GetValue(pcPort->GetTrace()->GetVoltage(iTime), fVcc));
	return sValue;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
EPinAlignment CPinView::GetAlignment(void) { return meAlignment; }
EPinAnchour CPinView::GetAnchour(void) { return meAnchour; }
int32 CPinView::GetWeight(void) { return miWeight; }
bool CPinView::IsInverting(void) { return mbInverting; }
bool CPinView::IsClockNotch(void) { return mbClockNotch; }


//////////////////////////////////////////////////////////////////////////
//
// Logicim's names in lower case.
//
//////////////////////////////////////////////////////////////////////////
static const char* gaszPinAlignments[] = { "", "left", "right", "top", "bottom", "not_shown" };
static const char* gaszPinAnchours[] = { "", "negative", "center", "positive" };


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* PinAlignmentToString(EPinAlignment eAlignment)
{
	return gaszPinAlignments[eAlignment];
}


//////////////////////////////////////////////////////////////////////////
//
// PA_Cannot if it isn't one PinAlignmentToString gives.
//
//////////////////////////////////////////////////////////////////////////
EPinAlignment StringToPinAlignment(const char* szAlignment)
{
	int		i;

	if (szAlignment != NULL)
	{
		for (i = PA_Left; i <= PA_NotShown; i++)
		{
			if (StringCompare(szAlignment, gaszPinAlignments[i]) == 0)
			{
				return (EPinAlignment)i;
			}
		}
	}
	return PA_Cannot;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* PinAnchourToString(EPinAnchour eAnchour)
{
	return gaszPinAnchours[eAnchour];
}


//////////////////////////////////////////////////////////////////////////
//
// PAN_Cannot if it isn't one PinAnchourToString gives.
//
//////////////////////////////////////////////////////////////////////////
EPinAnchour StringToPinAnchour(const char* szAnchour)
{
	int		i;

	if (szAnchour != NULL)
	{
		for (i = PAN_Negative; i <= PAN_Positive; i++)
		{
			if (StringCompare(szAnchour, gaszPinAnchours[i]) == 0)
			{
				return (EPinAnchour)i;
			}
		}
	}
	return PAN_Cannot;
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's ConstantProperties and ProbeProperties.
//
//////////////////////////////////////////////////////////////////////////
void CValueView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CComponentView::GetProperties(pcProperties, psContext);
	AddPowerProperties(pcProperties, psContext);
	pcProperties->AddInteger("bitWidth", miBitWidth, 1, RADIX_MAX_BIT_WIDTH);
	AddRadixProperty(pcProperties, meRadix);
}


//////////////////////////////////////////////////////////////////////////
//
// A radix as a choice, as Logicim's Radix editor.
//
//////////////////////////////////////////////////////////////////////////
void AddRadixProperty(CComponentProperties* pcProperties, ERadix eRadix)
{
	const char*		aszRadices[] = { "Binary", "Decimal", "Hexadecimal" };

	pcProperties->AddChoice("radix", aszRadices, 3, (size)eRadix - RADIX_Binary);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
ERadix GetRadixProperty(CComponentProperties* pcProperties, ERadix eDefault)
{
	return (ERadix)(RADIX_Binary + pcProperties->GetChoice("radix", (size)eDefault - RADIX_Binary));
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's PinProperties: where and how it's shown on an instance of
// its subcircuit after its name, width and power.
//
//////////////////////////////////////////////////////////////////////////
void CPinView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	const char*		aszAlignments[] = { "Left", "Right", "Top", "Bottom", "Not Shown" };
	const char*		aszAnchours[] = { "Negative", "Centre", "Positive" };

	CComponentView::GetProperties(pcProperties, psContext);
	pcProperties->AddInteger("bitWidth", miBitWidth, 1, RADIX_MAX_BIT_WIDTH);
	AddPowerProperties(pcProperties, psContext);
	pcProperties->AddDivider("instanceLocation");
	pcProperties->AddChoice("alignment", aszAlignments, 5, (size)meAlignment - PA_Left);
	pcProperties->AddChoice("anchour", aszAnchours, 3, (size)meAnchour - PAN_Negative);
	pcProperties->AddInteger("weight", miWeight, -1000, 1000);
	pcProperties->AddDivider("instanceDisplay");
	pcProperties->AddBoolean("inverting", mbInverting);
	pcProperties->AddBoolean("clockNotch", mbClockNotch);
	AddRadixProperty(pcProperties, meRadix);
}


//////////////////////////////////////////////////////////////////////////
//
// Placed new one bit wide, in binary, with a name no other pin in the
// circuit has, as Logicim's ComponentNameHelper.
//
//////////////////////////////////////////////////////////////////////////
bool CPinView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	CChars	szName;

	GetUniqueComponentName(&szName, pcCircuit, GetXMLName(), "Pin");
	Init(x, y, eRotation, szName.Text(), PLACED_FAMILY, false, 1, RADIX_Binary, pcFonts);
	szName.Kill();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CPinView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CValueView::InitFromProperties(x, y, eRotation, pcProperties, psContext);
	SetSubcircuitAppearance((EPinAlignment)(PA_Left + pcProperties->GetChoice("alignment", (size)meAlignment - PA_Left)), (EPinAnchour)(PAN_Negative + pcProperties->GetChoice("anchour", (size)meAnchour - PAN_Negative)), (int32)pcProperties->GetInteger("weight", miWeight), pcProperties->GetBoolean("inverting", mbInverting), pcProperties->GetBoolean("clockNotch", mbClockNotch));
	return true;
}
