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
#ifndef __RESET_SUPERVISOR_VIEW_H__
#define __RESET_SUPERVISOR_VIEW_H__
#include "IntegratedCircuitView.h"


//A power-on reset supervisor, a DS1813 (CResetSupervisor): a two grid unit box with its active low reset, RST, on its
//right, and its trip voltage and reset time as properties.  Its family is only for where its power comes from when it
//isn't on rails; its reset has the DS1813's own levels.
class CResetSupervisorView : public CIntegratedCircuitView
{
CONSTRUCTABLE(CResetSupervisorView);
protected:
	float32		mfTripVoltage;
	int64		miResetTime;

public:
			void					Init(int32 x, int32 y, ERotation eRotation, const char* szName, const char* szFamily, bool bExplicitPowerPorts, float32 fTripVoltage, int64 iResetTime);

	virtual const char*				GetType(void);
	virtual const char*				GetXMLName(void) const;
	virtual void					GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts);
	virtual bool					LoadXML(CMarkupTag* pcTag);
	virtual CIntegratedCircuit*		CreateIntegratedCircuit(CSubcircuitSimulation* pcSimulation, CFamilyVoltageConfiguration* pcFamily);
			float32					GetTripVoltage(void) const;
			int64					GetResetTime(void) const;

protected:
	virtual	void					SaveXMLAttributes(CXMLWriter* pcWriter);
};


#endif // __RESET_SUPERVISOR_VIEW_H__

