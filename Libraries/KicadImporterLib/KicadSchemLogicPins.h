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
#ifndef __KICAD_SCHEM_LOGIC_PINS_H__
#define __KICAD_SCHEM_LOGIC_PINS_H__
#include "BaseLib/Chars.h"
#include "BaseLib/ArrayChars.h"
#include "BaseLib/ArrayInt.h"
#include "BaseLib/ArrayTemplatePtr.h"
#include "BaseLib/Float64Vec2.h"
#include "KicadLib/KicadSchemLibPin.h"
#include "KicadLib/KicadSchemPinType.h"
#include "KicadSchemLogicFunction.h"


//A logic part's pins (not its power pins), each split into its gate number (an index into aszIndices) and letters,
//with whether it's negated and where KiCad has it.
struct SKicadSchemLogicPins
{
	CArrayChars							aszIndices;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	CArrayInt							aiGroups;
	CArrayChars							aszLetters;
	CArrayInt							aiNegated;
	CArrayFloat64Vec2					asPositions;

	void	Init(void);
	void	Kill(void);
};


//What a logic part's pin names say: a gate number, letters and negation; flip flop ports; outputs; and the labels of
//74 series parts' ports.
void		SplitKicadSchemLogicPinName(const char* szName, CChars* pszIndex, CChars* pszLetters, bool* pbNegated);
bool		IsKicadSchemOutputPinType(EKicadSchemPinType eType);
const char*	GetKicadSchemFlipFlopPortName(CChars* pszLetters, bool bNegated);
bool		IsKicadSchemLogicOutput(SKicadSchemLogicPins* psPins, size uiPin, EKicadSchemLogicFunction eFunction);
void		NameUnnamedKicadSchemLogicPins(SKicadSchemLogicPins* psPins);
void		GetKicadSchemPinLabel(const char* szName, CChars* pszLabel, bool* pbNegated);
bool		GetKicadSchemPinLabelAlias(const char* szLabel, CChars* pszAlias);


#endif // __KICAD_SCHEM_LOGIC_PINS_H__

