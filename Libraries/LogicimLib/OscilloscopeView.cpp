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
#include "MicroprocessorLib/Oscilloscope.h"
#include "MicroprocessorLib/Pins.h"
#include "ComponentPlacement.h"
#include "OscilloscopeView.h"


#define OSCILLOSCOPE_SAMPLE_WIDTH		1.5
#define OSCILLOSCOPE_REFERENCE_WIDTH	1.0
#define OSCILLOSCOPE_FULL_SCALE_VOLTS	8.0


//////////////////////////////////////////////////////////////////////////
//
// iInputCount traces, 32 divisions of 30 samples wide, 4 grid units high,
// sampled at 6GHz.
//
//////////////////////////////////////////////////////////////////////////
void SOscilloscopeParams::Init(int32 iInputCount)
{
	this->iInputCount = iInputCount;
	iNumberOfDivsWide = 32;
	iSamplesPerDiv = 30;
	iDivHeight = 4;
	fSampleFrequency = 6000000000.0;
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's power ports are never explicit on an oscilloscope.
//
//////////////////////////////////////////////////////////////////////////
void COscilloscopeView::Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, SOscilloscopeParams* psParams)
{
	SComponentShape*	psShape;
	float64				fBottom;
	int32				i;
	CChars				szPortName;

	CIntegratedCircuitView::Init(x, y, eRotation, szName, szFamily, false);
	miInputCount = psParams->iInputCount;
	miNumberOfDivsWide = psParams->iNumberOfDivsWide;
	miSamplesPerDiv = psParams->iSamplesPerDiv;
	miDivHeight = psParams->iDivHeight;
	mfSampleFrequency = psParams->fSampleFrequency;

	fBottom = (miInputCount - 1) * miDivHeight;
	psShape = mcShapes.AddRoundedRectangle(0, -4, miNumberOfDivsWide + 2, fBottom + 2, 1, true, true);
	psShape->eFill = SC_Explicit;
	psShape->sFill = Set32BitColour((uint8)0, (uint8)208, (uint8)208);
	psShape = mcShapes.AddRectangle(0.8, -3, miNumberOfDivsWide + 1.2, fBottom + 1, true, true);
	psShape->eFill = SC_Explicit;
	psShape->sFill = Set32BitColour((uint8)255, (uint8)255, (uint8)255);

	for (i = 0; i < miInputCount; i++)
	{
		szPortName.Init();
		CPins::CalculatePortName(&szPortName, "Input", i, miInputCount);
		AddPortView(szPortName.Text(), 0, i * miDivHeight);
		szPortName.Kill();
	}
	CalculateBounds();
}


//////////////////////////////////////////////////////////////////////////
//
// The samples go over the screen and under the ports.
//
//////////////////////////////////////////////////////////////////////////
void COscilloscopeView::Paint(CCircuitPainter* pcPainter)
{
	pcPainter->SetTransform(msPosition, meRotation, mbMirrored);
	PaintShapes(pcPainter);
	PaintSamples(pcPainter);
	PaintPorts(pcPainter);
	pcPainter->ClearTransform();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void COscilloscopeView::PaintSamples(CCircuitPainter* pcPainter)
{
	COscilloscope*		pcOscilloscope;
	ARGB32				sReference;
	ARGB32				sColour;
	float64				fBottom;
	float64				fTop;
	float64				fTick;
	float64				fX;
	float64				fY;
	size				uiInput;
	size				uiSample;
	size				uiSamples;
	size				uiTick;
	float64				fBehind;
	float64				fColour;
	float32				fMin;
	float32				fMax;

	pcOscilloscope = (COscilloscope*)GetComponent(pcPainter);
	if (pcOscilloscope == NULL)
	{
		return;
	}
	uiSamples = pcOscilloscope->GetSampleCount();
	if (uiSamples == 0)
	{
		return;
	}

	sReference = Set32BitColour((uint8)232, (uint8)232, (uint8)232);
	uiTick = pcOscilloscope->GetTickPosition();
	fBottom = (miInputCount - 1) * miDivHeight + 1 - 0.25;
	fTop = -3 + 0.25;
	fTick = 1 + (float64)uiTick / miSamplesPerDiv;
	pcPainter->Line(fTick, fTop, fTick, fBottom, OSCILLOSCOPE_REFERENCE_WIDTH, sReference);

	for (uiInput = 0; uiInput < pcOscilloscope->GetInputCount(); uiInput++)
	{
		fY = (float64)uiInput * miDivHeight;
		pcPainter->Line(1, fY, miNumberOfDivsWide, fY, OSCILLOSCOPE_REFERENCE_WIDTH, sReference);
		for (uiSample = 0; uiSample < uiSamples; uiSample++)
		{
			fMin = pcOscilloscope->GetMinVoltage(uiInput, uiSample);
			fMax = pcOscilloscope->GetMaxVoltage(uiInput, uiSample);
			if (isnan(fMin) || isnan(fMax))
			{
				continue;
			}

			//How far the sample is from being overwritten; the nearest quarter fades to white.
			fBehind = (uiTick >= uiSample) ? (float64)(uiTick - uiSample) : (float64)(uiTick + uiSamples - uiSample);
			fBehind = uiSamples - fBehind;
			sColour = pcPainter->GetColours()->GetColourForVoltage((fMin + fMax) / 2);
			if (fBehind < uiSamples / 4.0)
			{
				fColour = fBehind / (uiSamples / 4.0);
				sColour = Set32BitColour((uint8)(Get8BitRedColour(sColour) * fColour + 255 * (1 - fColour)),
										 (uint8)(Get8BitGreenColour(sColour) * fColour + 255 * (1 - fColour)),
										 (uint8)(Get8BitBlueColour(sColour) * fColour + 255 * (1 - fColour)));
			}

			fX = 1 + (float64)uiSample / miSamplesPerDiv;
			pcPainter->Line(fX, fY - (fMin / OSCILLOSCOPE_FULL_SCALE_VOLTS) * miDivHeight, fX, fY - (fMax / OSCILLOSCOPE_FULL_SCALE_VOLTS) * miDivHeight, OSCILLOSCOPE_SAMPLE_WIDTH, sColour);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* COscilloscopeView::GetType(void)
{
	return "Oscilloscope";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* COscilloscopeView::GetXMLName(void)
{
	return "Oscilloscope";
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void COscilloscopeView::SaveXMLAttributes(CXMLWriter* pcWriter)
{
	CComponentView::SaveXMLAttributes(pcWriter);
	pcWriter->Attribute("family", mszFamily.Text());
	pcWriter->Attribute("inputCount", miInputCount);
	pcWriter->Attribute("numberOfDivsWide", miNumberOfDivsWide);
	pcWriter->Attribute("samplesPerDiv", miSamplesPerDiv);
	pcWriter->Attribute("divHeight", miDivHeight);
	pcWriter->Attribute("sampleFrequency", mfSampleFrequency);
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's limits: 1 to 24 inputs, 4 to 128 divs wide, 4 to 64 samples
// a div and divs 2 to 6 high.
//
//////////////////////////////////////////////////////////////////////////
bool COscilloscopeView::LoadXML(CMarkupTag* pcTag)
{
	SIntegratedCircuitXML	sIntegratedCircuit;
	SOscilloscopeParams		sParams;

	if (!LoadIntegratedCircuitXML(pcTag, &sIntegratedCircuit))
	{
		return false;
	}
	if (!pcTag->GetInt32Attribute("inputCount", &sParams.iInputCount) || !pcTag->GetInt32Attribute("numberOfDivsWide", &sParams.iNumberOfDivsWide) ||
		!pcTag->GetInt32Attribute("samplesPerDiv", &sParams.iSamplesPerDiv) || !pcTag->GetInt32Attribute("divHeight", &sParams.iDivHeight) ||
		!pcTag->GetFloat64Attribute("sampleFrequency", &sParams.fSampleFrequency))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] needs whole number inputCount, numberOfDivsWide, samplesPerDiv and divHeight and a sampleFrequency.", NULL);
	}
	if ((sParams.iInputCount < 1) || (sParams.iInputCount > 24) || (sParams.iNumberOfDivsWide < 4) || (sParams.iNumberOfDivsWide > 128) ||
		(sParams.iSamplesPerDiv < 4) || (sParams.iSamplesPerDiv > 64) || (sParams.iDivHeight < 2) || (sParams.iDivHeight > 6) || !(sParams.fSampleFrequency > 0))
	{
		return gcLogger.Error2(__METHOD__, " [", pcTag->GetName(), "] has a property out of range.", NULL);
	}
	Init(sIntegratedCircuit.x, sIntegratedCircuit.y, sIntegratedCircuit.eRotation, sIntegratedCircuit.szName, sIntegratedCircuit.szFamily, &sParams);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The oscilloscope's inputs read voltages, not levels; it has no family.
//
//////////////////////////////////////////////////////////////////////////
CIntegratedCircuit* COscilloscopeView::CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily)
{
	COscilloscope*		pcOscilloscope;

	pcOscilloscope = &OMalloc<COscilloscope>(pcSimulation, mszName.Text(), OMalloc<COscilloscopePins>(miInputCount), (float32)mfSampleFrequency, miNumberOfDivsWide, miSamplesPerDiv);
	return pcOscilloscope;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int32 COscilloscopeView::GetInputCount(void) { return miInputCount; }
int32 COscilloscopeView::GetNumberOfDivsWide(void) { return miNumberOfDivsWide; }
int32 COscilloscopeView::GetSamplesPerDiv(void) { return miSamplesPerDiv; }
int32 COscilloscopeView::GetDivHeight(void) { return miDivHeight; }
float64 COscilloscopeView::GetSampleFrequency(void) { return mfSampleFrequency; }


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's OscilloscopeProperties, clamped as its clampProperties.
// Its power is always implicit.
//
//////////////////////////////////////////////////////////////////////////
void COscilloscopeView::GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	CComponentView::GetProperties(pcProperties, psContext);
	AddFamilyProperty(pcProperties, psContext);
	pcProperties->AddInteger("inputCount", miInputCount, 1, 24);
	pcProperties->AddInteger("numberOfDivsWide", miNumberOfDivsWide, 4, 128);
	pcProperties->AddInteger("divHeightInGrids", miDivHeight, 2, 6);
	pcProperties->AddInteger("samplesPerDiv", miSamplesPerDiv, 4, 64);
	pcProperties->AddFloat("samplingFrequency", "GHz", mfSampleFrequency / 1000000000.0, 0.00000000001, 999.0);
}


//////////////////////////////////////////////////////////////////////////
//
// Placed new with two inputs.
//
//////////////////////////////////////////////////////////////////////////
bool COscilloscopeView::InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts)
{
	SOscilloscopeParams		sParams;

	sParams.Init(2);
	Init(x, y, eRotation, "", PLACED_FAMILY, &sParams);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool COscilloscopeView::InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext)
{
	SOscilloscopeParams		sParams;

	sParams.iInputCount = (int32)pcProperties->GetInteger("inputCount", miInputCount);
	sParams.iNumberOfDivsWide = (int32)pcProperties->GetInteger("numberOfDivsWide", miNumberOfDivsWide);
	sParams.iSamplesPerDiv = (int32)pcProperties->GetInteger("samplesPerDiv", miSamplesPerDiv);
	sParams.iDivHeight = (int32)pcProperties->GetInteger("divHeightInGrids", miDivHeight);
	sParams.fSampleFrequency = pcProperties->GetFloat("samplingFrequency", mfSampleFrequency / 1000000000.0) * 1000000000.0;
	Init(x, y, eRotation, pcProperties->GetText("name", ""), pcProperties->GetText("family", ""), &sParams);
	return true;
}
