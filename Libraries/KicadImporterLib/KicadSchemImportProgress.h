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
#ifndef __KICAD_SCHEM_IMPORT_PROGRESS_H__
#define __KICAD_SCHEM_IMPORT_PROGRESS_H__
#include "BaseLib/PrimitiveTypes.h"


//The steps of making one board's circuit, as told to a CKicadSchemImportProgress.
#define KICAD_SCHEM_IMPORT_STEP_READ		0	//Reading the schematic.
#define KICAD_SCHEM_IMPORT_STEP_PARTS		1	//Making its parts, and the boards it draws (which tell their own steps).
#define KICAD_SCHEM_IMPORT_STEP_BUSES		2
#define KICAD_SCHEM_IMPORT_STEP_WIRES		3	//Finding the wires to make.
#define KICAD_SCHEM_IMPORT_STEP_PLACE		4	//Placing and wiring, a step for each spread tried (up to KICAD_SCHEM_IMPORT_PLACE_STEPS).
#define KICAD_SCHEM_IMPORT_PLACE_STEPS		6
#define KICAD_SCHEM_IMPORT_STEP_UNTANGLE	(KICAD_SCHEM_IMPORT_STEP_PLACE + KICAD_SCHEM_IMPORT_PLACE_STEPS)
#define KICAD_SCHEM_IMPORT_STEP_DONE		(KICAD_SCHEM_IMPORT_STEP_UNTANGLE + 1)
#define KICAD_SCHEM_IMPORT_STEPS			KICAD_SCHEM_IMPORT_STEP_DONE


//Told how a KiCad import is going, between its steps: the board being made (its circuit's type name), what's being
//done to it and how far through its steps (uiStep of KICAD_SCHEM_IMPORT_STEPS) it is.  A board drawn on another is
//made in the middle of the other's parts step, so the boards' steps nest.  The import doesn't stop for it: an
//application uses it to show the progress (painting its window, say).
class CKicadSchemImportProgress
{
public:
	virtual void	Progress(const char* szBoard, const char* szStage, size uiStep, size uiSteps) = 0;
};


#endif // __KICAD_SCHEM_IMPORT_PROGRESS_H__

