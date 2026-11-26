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
#ifndef __ANALOG_COMPARATOR_VIEW_H__
#define __ANALOG_COMPARATOR_VIEW_H__
#include "IntegratedCircuitView.h"


//An analog comparator, a MAX9010 (CAnalogComparator): its schematic triangle, IN+ and IN- on its left, OUT at its
//point.  Its family is only for where its power comes from when it isn't on rails; its output has the MAX9010's own
//levels.
class CAnalogComparatorView : public CIntegratedCircuitView
{
CONSTRUCTABLE(CAnalogComparatorView);
public:
			void					Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts);
	virtual const char*				GetType(void);
	virtual const char*				GetXMLName(void) const;
	virtual void					GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts);
	virtual bool					LoadXML(CMarkupTag* pcTag);
	virtual CIntegratedCircuit*		CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily);
};


#endif // __ANALOG_COMPARATOR_VIEW_H__

