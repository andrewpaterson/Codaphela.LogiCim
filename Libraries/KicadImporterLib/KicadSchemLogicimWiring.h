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
#ifndef __KICAD_SCHEM_LOGICIM_WIRING_H__
#define __KICAD_SCHEM_LOGICIM_WIRING_H__
#include "BaseLib/ArrayTemplatePrimitive.h"
#include "KicadLib/KicadSchemNetlist.h"
#include "KicadSchemLogicimModel.h"
#include "KicadSchemLogicimBuses.h"


class CTraceAutoConnector;
class CKicadSchemSubnetGraph;


//Wires the model's terminals (see CKicadSchemLogicimConverter): each subnet's terminals as a tree in the order KiCad's
//wires join them, each splitter beside its component wired straight to its pins, and the circuit routed from those
//connections.
class CKicadSchemLogicimWiring
{
protected:
	CKicadSchemNetlist*			mpcNetlist;
	CKicadSchemLogicimModel*	mpcModel;
	CKicadSchemLogicimBuses*	mpcBuses;

public:
	void	Init(CKicadSchemNetlist* pcNetlist, CKicadSchemLogicimModel* pcModel, CKicadSchemLogicimBuses* pcBuses);
	void	Kill(void);

	void	AddConnections(void);
	size	Route(void);

protected:
	void	AddSubnetConnections(CKicadSchemSubnet* pcSubnet);
	void	ConnectSubnetTerminals(CKicadSchemSubnet* pcSubnet, CKicadSchemSubnetGraph* pcGraph, CArrayInt* paiTerminals, CArrayInt* paiNodes);
	void	JoinSplitterPins(void);
	void	JoinSplitterEnd(size uiEnd, size uiPin);
	bool	IsConnectionBetween(size uiTerminal1, size uiTerminal2);
	bool	ReachesTerminal(size uiFrom, size uiTo, size uiWithout);
	void	SetRouteBoxes(CTraceAutoConnector* pcConnector);
	void	AddRouteConnections(CTraceAutoConnector* pcConnector, CArrayTemplatePrimitive<size>* pauiRouted);
};


#endif // __KICAD_SCHEM_LOGICIM_WIRING_H__

