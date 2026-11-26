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
#ifndef __POWER_SOURCE_VIEW_H__
#define __POWER_SOURCE_VIEW_H__
#include "ComponentView.h"


//A view of a supply with one "Power" port, as Logicim's PowerSourceView.  Its supply (CPowerSource) is on at start
//unless told otherwise, and rises over its rise time, none by default (at once, as Logicim's).  A positive supply can
//be fed (a regulator): it then has an "In" port, on its feed, and a dropout.
//
//A supply on a rail (SetRail) joins the nets of the rail's name wherever it is, as KiCad's power symbols do, so the
//ICs whose hidden power ports are on the rail (CIntegratedCircuitView::SetPowerRails) are powered by it; a
//regulator's In port joins its feed rail's nets the same way.
class CPowerSourceView : public CComponentView
{
CONSTRUCTABLE(CPowerSourceView);
protected:
	float32		mfVoltage;
	int64		miRiseTime;
	bool		mbOnAtStart;
	float32		mfDropout;  //Below zero when it isn't fed.
	CChars		mszRail;  //Empty when it is on no rail.

public:
			void					Init(int32 x, int32 y, ERotation eRotation, const char* szName, float32 fVoltage);

	virtual CCircuitComponent*		CreateComponent(CViewPath* pcViewPath, CCircuitSimulation* pcCircuitSimulation, CFamilyVoltageConfigurationStore* pcFamilies);
	virtual CComponentStateXML*		GetComponentStateXML(void) const;
			float32					GetVoltage(void) const;
			void					SetRiseTime(int64 iRiseTime);
			int64					GetRiseTime(void) const;
			void					SetOnAtStart(bool bOnAtStart);
			bool					IsOnAtStart(void) const;
			bool					IsFed(void) const;
			float32					GetDropout(void) const;
	virtual bool					LoadXMLAfterInit(CMarkupTag* pcTag);
	virtual void					InitAfterProperties(CComponentProperties* pcProperties);
	virtual void					Kill(void);
	virtual const char*				GetConnectionName(void);
	virtual void					GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
			void					SetRail(const char* szRail);

protected:
	virtual	void					SaveXMLAttributes(CXMLWriter* pcWriter);
};


//A 0V supply drawn as Logicim's ground symbol; the port is at the end of its stem.  It faces the way its stem points
//from the bars, so an upright ground, its port on top, faces North.
class CGroundView : public CPowerSourceView
{
CONSTRUCTABLE(CGroundView);
public:
			void					Init(int32 x, int32 y, ERotation eRotation, const char* szName);

	virtual const char*				GetType(void);
	virtual const char*				GetXMLName(void) const;
	virtual bool					InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts);
	virtual bool					LoadXML(CMarkupTag* pcTag);
};


//A positive supply, as Logicim's PositivePowerView: a box with a point to its port and the voltage written upright in
//the box in the voltage's colour.  Over 4V the box is the dark fill.  It faces the way its point does, so an upright
//supply, its port below, faces South.
class CPositivePowerView : public CPowerSourceView
{
CONSTRUCTABLE(CPositivePowerView);
protected:
	size	muiVoltageText;

public:
			void					Init(int32 x, int32 y, ERotation eRotation, const char* szName, float32 fVoltage);
			void					SetFeed(float32 fDropout);
			void					SetFeedRail(const char* szFeedRail);
			const char*				GetFeedRail(void);
	virtual bool					LoadXMLAfterInit(CMarkupTag* pcTag);
	virtual void					InitAfterProperties(CComponentProperties* pcProperties);

	virtual void					Paint(CCircuitPainter* pcPainter);

	virtual const char*				GetType(void);
	virtual const char*				GetXMLName(void) const;
	virtual void					GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts);
	virtual bool					LoadXML(CMarkupTag* pcTag);

protected:
	virtual	void					SaveXMLAttributes(CXMLWriter* pcWriter);
};


#endif // __POWER_SOURCE_VIEW_H__
