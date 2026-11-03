/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela LogiCim

Codaphela LogiCim is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela LogiCim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Codaphela LogiCim.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#ifndef __LOGICIM_CIRCUIT_H__
#define __LOGICIM_CIRCUIT_H__
#include "MicroprocessorLib/CircuitSimulation.h"
#include "MicroprocessorLib/FamilyVoltageConfigurationStore.h"
#include "LogicimLib/CircuitEditor.h"
#include "LogicimLib/CircuitFonts.h"


//LogiCim's editor and the circuits in it.  It starts with an empty "Main" circuit, simulated on its top view path.  A
//project loaded, or a KiCad schematic imported (see CKicadSchemLogicimConverter), replaces it.
class CLogicimCircuit
{
protected:
	CFamilyVoltageConfigurationStore	mcFamilies;
	CCircuitEditor						mcEditor;
	CCircuitColours						mcColours;
	CCircuitFonts						mcFonts;

public:
	bool				Init(const char* szPropagationTimes, const char* szFontDirectory, EColourOrder eColourOrder);
	void				Kill(void);

	bool				Reset(void);
	bool				Run(int64 iTimeForward);
	bool				Save(const char* szFilename);
	bool				Load(const char* szFilename);
	bool				ImportKicadSchematic(const char* szFilename);

	CCircuitEditor*		GetEditor(void);
	CCircuitColours*	GetColours(void);
	CCircuitFonts*		GetFonts(void);
	int64				GetTime(void);

protected:
	void				RemoveFontFamily(Ptr<CFontFamily> pFamily);
	bool				CreateSimulation(void);
};


#endif // __LOGICIM_CIRCUIT_H__

