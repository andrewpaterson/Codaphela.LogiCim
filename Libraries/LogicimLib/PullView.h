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
#ifndef __PULL_VIEW_H__
#define __PULL_VIEW_H__
#include "ComponentView.h"


//Which way a pull pulls: up to a supply or down to ground.
enum EPullDirection
{
	PD_Up,
	PD_Down,
};


//A pull-up or pull-down (CPull): a resistor from its signal, on its right, to a supply bar (up) or a ground symbol
//(down), its resistance shown.  Its supply is a rail, joined by name as an imported chip's power is (SetRail), or
//without one a supply of its own at its voltage (0V pulling down).  A capacitance (SetCapacitance, an importer's
//estimate of its net's) is given its signal's trace, for the curve the trace takes when it's let go; without one the
//trace keeps its own.  A trace only pulls hold is drawn fainter than a driven one (CCircuitColours::GetColourForTrace).
class CPullView : public CComponentView
{
CONSTRUCTABLE(CPullView);
protected:
	float32			mfResistance;
	EPullDirection	meDirection;
	float32			mfVoltage;
	float32			mfCapacitance;  //Farads, 0 for none.
	CChars			mszRail;

public:
			void					Init(int32 x, int32 y, ERotation eRotation, const char* szName, float32 fResistance, EPullDirection eDirection, float32 fVoltage);
	virtual void					Kill(void);
	virtual const char*				GetType(void);
	virtual const char*				GetXMLName(void) const;
	virtual void					GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts);
	virtual bool					LoadXML(CMarkupTag* pcTag);
	virtual CCircuitComponent*		CreateComponent(CViewPath* pcViewPath, CCircuitSimulation* pcCircuitSimulation, CFamilyVoltageConfigurationStore* pcFamilies);
			void					SetRail(const char* szRail);
			void					SetCapacitance(float32 fCapacitance);
			float32					GetCapacitance(void) const;
			const char*				GetRail(void);
			float32					GetResistance(void) const;
			EPullDirection			GetDirection(void) const;
			float32					GetVoltage(void) const;
protected:
	virtual	void					SaveXMLAttributes(CXMLWriter* pcWriter);
};


extern const char*	gaszPullDirectionNames[];


#endif // __PULL_VIEW_H__

