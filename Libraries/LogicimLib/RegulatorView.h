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
#ifndef __REGULATOR_VIEW_H__
#define __REGULATOR_VIEW_H__
#include "PowerSourceView.h"


#define REGULATOR_FONT_SIZE			10.0
#define REGULATOR_VOLTAGE_FONT_SIZE	11.0


//A voltage regulator: a supply (CPowerSource) fed through its In port and grounded through its GND port, both of
//which are drawn and wired to what feeds them; nothing about where its power comes from is implied.  It is on while
//its GND is at 0V and its In is at its voltage plus its dropout or above.
//
//It's drawn as a three pin IC, an upside down tombstone (its bottom corners cut at 45 degrees): IN on its left, OUT
//(its "Power" port) on its right and GND at the middle of its bottom, with its voltage and its part ("LD1117S50TR"),
//or its name if it has no part, written in the box.  Its position is the middle of the box and it faces East, its
//OUT on the right.  Only its OUT is on its rail (SetRail): its IN and GND join only what they are wired to.
class CRegulatorView : public CPowerSourceView
{
CONSTRUCTABLE(CRegulatorView);
protected:
	float32		mfDropout;
	CChars		mszPart;
	size		muiVoltageText;

public:
			void					Init(int32 x, int32 y, ERotation eRotation, const char* szName, float32 fVoltage, float32 fDropout, const char* szPart);
	virtual void					Kill(void);

	virtual const char*				GetType(void);
	virtual const char*				GetXMLName(void) const;
	virtual const char*				GetConnectionName(void);
	virtual void					SetRail(const char* szRail);
	virtual void					GetProperties(CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitFromProperties(int32 x, int32 y, ERotation eRotation, CComponentProperties* pcProperties, SComponentPropertiesContext* psContext);
	virtual bool					InitPlaced(int32 x, int32 y, ERotation eRotation, CCircuitView* pcCircuit, CCircuitFonts* pcFonts);
	virtual bool					LoadXML(CMarkupTag* pcTag);
	virtual void					Paint(CCircuitPainter* pcPainter);

			float32					GetDropout(void) const;
			const char*				GetPart(void);

protected:
	virtual	void					SaveXMLAttributes(CXMLWriter* pcWriter);
	virtual void					InitPowerSource(CPowerSource* pcPowerSource);
};


#endif // __REGULATOR_VIEW_H__
