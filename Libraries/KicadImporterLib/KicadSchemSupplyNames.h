/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela KicadImporterLib

Codaphela KicadImporterLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela KicadImporterLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela KicadImporterLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#ifndef __KICAD_SCHEM_SUPPLY_NAMES_H__
#define __KICAD_SCHEM_SUPPLY_NAMES_H__
#include "BaseLib/PrimitiveTypes.h"
#include "KicadLib/KicadSchemSymbol.h"
#include "KicadLib/KicadSchemLibSymbol.h"


//What KiCad's names say about supplies: grounds and voltages, the power pins of logic parts, the nets of supplies,
//voltage regulators' pins and power flags, the symbols that are left out, and the labels that are supplies.
bool	IsKicadSchemGroundName(const char* szName);
float32	GetKicadSchemSupplyVoltage(const char* szName);
bool	HasKicadSchemVoltage(const char* szText);
bool	IsKicadSchemPowerPinName(const char* szName);
bool	IsKicadSchemNoConnectName(const char* szName);
bool	IsKicadSchemPowerNetName(const char* szName);
bool	IsKicadSchemRegulatorPinName(const char* szName, const char* szEnd);
bool	IsKicadSchemPowerFlag(const char* szPowerName, CKicadSchemLibSymbol* pcLibSymbol);
bool	IsKicadSchemSkippedSymbol(CKicadSchemSymbol* pcSymbol);
bool	IsKicadSchemSupplyLabel(const char* szText);
bool	IsKicadSchemGroundLabel(const char* szText);
bool	IsKicadSchemVccLabel(const char* szText);
bool	IsKicadSchemVoltageLabel(const char* szText);
float32	GetKicadSchemSupplyLabelVoltage(const char* szText);


//A voltage regulator's dropout and how long its output takes to rise (its start-up, charging its output
//capacitors), by the part its symbol's value names; a power-on reset supervisor's trip voltage the same way.
struct SKicadSchemRegulatorPart
{
	const char*		szPart;
	float32			fDropout;
	int64			iRiseNanoseconds;
};
SKicadSchemRegulatorPart*	GetKicadSchemRegulatorPart(const char* szValue);
float32						GetKicadSchemSupervisorTripVoltage(const char* szValue);


#endif // __KICAD_SCHEM_SUPPLY_NAMES_H__

