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
#ifndef __RC_FILTER_VIEW_H__
#define __RC_FILTER_VIEW_H__
#include "IntegratedCircuitView.h"


//The resistance a diode across an RC's resistor charges its capacitor through the fast way: the diode's own and the
//output driving it, a few tens of ohms.
#define RC_FILTER_DIODE_RESISTANCE	25.0f


//Which way a diode across an RC's resistor conducts, the way its capacitor charges quickly.
enum ERCFilterDiode
{
	RCFD_None,
	RCFD_Rising,  //Anode on the input: the capacitor charges quickly and falls through the resistor.
	RCFD_Falling,  //Cathode on the input: it discharges quickly and rises through the resistor.
};


//An RC element (CRCFilter): a resistor from In feeding Out, with a capacitor from Out to ground, drawn as its
//schematic symbols with a diode across the resistor when it has one, and its time constant labelled.  Its resistance
//(ohms), capacitance (farads) and diode are its properties; its time constants are worked out from them.  It is
//passive: it has no power and no family.
class CRCFilterView : public CIntegratedCircuitView
{
CONSTRUCTABLE(CRCFilterView);
protected:
	float32				mfResistance;
	float32				mfCapacitance;
	ERCFilterDiode		meDiode;

public:
			void					Init(int32 x, int32 y, ERotation eRotation, const char* szName, float32 fResistance, float32 fCapacitance, ERCFilterDiode eDiode);
	virtual const char*				GetType(void);
	virtual const char*				GetXMLName(void) const;
	virtual void					GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts);
	virtual bool					LoadXML(CMarkupTag* pcTag);
	virtual CCircuitComponent*		CreateComponent(CViewPath* pcViewPath, CCircuitSimulation* pcCircuitSimulation, CFamilyVoltageConfigurationStore* pcFamilies);
	virtual CIntegratedCircuit*		CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily);
			float32					GetResistance(void) const;
			float32					GetCapacitance(void) const;
			ERCFilterDiode			GetDiode(void) const;
			int64					GetRiseTimeConstant(void) const;
			int64					GetFallTimeConstant(void) const;
protected:
	virtual	void					SaveXMLAttributes(CXMLWriter* pcWriter);
			void					CreateShapes(void);
};


extern const char*	gaszRCFilterDiodeNames[];


#endif // __RC_FILTER_VIEW_H__

