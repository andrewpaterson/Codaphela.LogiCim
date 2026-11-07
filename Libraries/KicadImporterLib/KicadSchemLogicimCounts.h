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
#ifndef __KICAD_SCHEM_LOGICIM_COUNTS_H__
#define __KICAD_SCHEM_LOGICIM_COUNTS_H__
#include "BaseLib/PrimitiveTypes.h"


//What the converter made from a sheet, and how well it wired and placed it.
struct SKicadSchemLogicimCounts
{
	size	uiGates;				//Gates and flip flops: generic ones and 74 series units and packages.
	size	uiBlocks;				//74 series parts that aren't gates (see AddLogic74Block).
	size	uiPlaceholders;			//Placeholder subcircuit instances.
	size	uiBoards;				//Instances of boards drawn as symbols.
	size	uiBoardPins;			//The circuit's pins, for its connectors' nets.
	size	uiPowers;				//Grounds and positive supplies.
	size	uiRegulators;			//Voltage regulators (not drawn).
	size	uiTunnels;				//Labels' and buses' tunnels.
	size	uiSplitters;			//Buses' splitters.
	size	uiSkipped;				//Symbols left out: passive parts, mechanical symbols and power flags.
	size	uiUnconnected;			//Connections that couldn't be routed.
	size	uiOverlaps;				//Items that couldn't be placed clear of the others.
	size	uiExplosions;			//Explosions of dense areas that were kept.
	size	uiExcessKnotsBefore;	//Knots over the dense areas' limit, before and after exploding them.
	size	uiExcessKnotsAfter;

	void	Init(void);
};


#endif // __KICAD_SCHEM_LOGICIM_COUNTS_H__

