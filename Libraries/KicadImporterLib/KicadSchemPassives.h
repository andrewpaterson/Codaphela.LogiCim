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
#ifndef __KICAD_SCHEM_PASSIVES_H__
#define __KICAD_SCHEM_PASSIVES_H__
#include "BaseLib/ArrayTemplate.h"
#include "KicadLib/KicadSchemFile.h"
#include "KicadLib/KicadSchemNetlist.h"


//A resistor's or capacitor's element: its two pins and their nets.  A resistor or capacitor is one element; an
//isolated network is an element for each pair of its pins with the same name (a resistor network's "R1" on pins 1
//and 8), or with only numbers, across the package (a CA0612's pins 1 and 8); a bussed network (a 4609X, a 4816P-1) an element from each pin to its common pin, apcPins[1], which is
//bBussed: its common pin joins no signals together.  The value is the symbol's, in ohms or farads, NaN when it can't
//be read.
struct SKicadSchemPassiveElement
{
	CKicadSchemSymbol*	pcSymbol;
	CKicadSchemLibPin*	apcPins[2];
	CKicadSchemNet*		apcNets[2];  //NULL for a pin on no net.
	float64				fValue;
	bool				bBussed;
};


typedef CArrayTemplate<SKicadSchemPassiveElement>	CArrayKicadSchemPassiveElement;


//Resistors and capacitors in a KiCad sheet, by their references' letters ("R1", "RN5"; "C3", "CN2"), and their values
//as their symbols give them: "4.7KΩ", "220", "16pF", "1μF" and "1µF" (both micro signs), or a part code (a
//CAY16-221J4LF, 4609X-101-103LF or 4816P-1-222LF resistor network, a CA0612KRX7R9BB100 capacitor network: two digits
//and a power of ten, in ohms and picofarads).  Diodes by "D" ("D1"), each diode an element from its anode, pin A (A1
//in a pair, as a BAW101's), to its cathode, K (K1).
bool	IsKicadSchemResistor(CKicadSchemSymbol* pcSymbol);
bool	IsKicadSchemCapacitor(CKicadSchemSymbol* pcSymbol);
bool	IsKicadSchemDiode(CKicadSchemSymbol* pcSymbol);
float64	GetKicadSchemResistance(const char* szValue);
float64	GetKicadSchemCapacitance(const char* szValue);
void	GetKicadSchemPassiveElements(CKicadSchemFile* pcFile, CKicadSchemNetlist* pcNetlist, bool bResistors, CArrayKicadSchemPassiveElement* pasElements);
void	GetKicadSchemDiodeElements(CKicadSchemFile* pcFile, CKicadSchemNetlist* pcNetlist, CArrayKicadSchemPassiveElement* pasElements);
CKicadSchemNet*	GetKicadSchemPinNet(CKicadSchemNetlist* pcNetlist, CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin);
void	GetKicadSchemPassiveElementName(CChars* psz, SKicadSchemPassiveElement* psElement);


#endif // __KICAD_SCHEM_PASSIVES_H__

