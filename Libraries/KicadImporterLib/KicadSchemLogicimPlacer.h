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
#ifndef __KICAD_SCHEM_LOGICIM_PLACER_H__
#define __KICAD_SCHEM_LOGICIM_PLACER_H__
#include "BaseLib/ArrayTemplate.h"
#include "BaseLib/ArrayInt.h"
#include "LogicimLib/CircuitFonts.h"
#include "KicadSchemLogicimModel.h"
#include "KicadSchemLogicimCounts.h"
#include "KicadSchemLogicimBuses.h"
#include "KicadSchemLogicimWiring.h"


//The gap kept between components' boxes.
#define KICAD_SCHEM_LOGICIM_SPACING			2


//How much more a component lined up with another (see SKicadSchemLogicimAlignment) would rather be moved along the
//line than off it when it's moved clear of the others.
#define KICAD_SCHEM_LOGICIM_ALIGN_WEIGHT	4


//Knots are wire turns and crossings.  An area of DENSE_WINDOW grid units square with more than DENSE_KNOTS is dense,
//and is exploded: EXPLODE_SPACE grid units are put in across it each way.
#define KICAD_SCHEM_LOGICIM_DENSE_WINDOW	12
#define KICAD_SCHEM_LOGICIM_DENSE_KNOTS		24
#define KICAD_SCHEM_LOGICIM_DENSE_AREAS		3
#define KICAD_SCHEM_LOGICIM_DENSE_SCAN		12
#define KICAD_SCHEM_LOGICIM_EXPLODE_SPACE	4
#define KICAD_SCHEM_LOGICIM_EXPLODE_PASSES	4


//How far a component is moved to keep it clear of the others before giving up.
#define KICAD_SCHEM_LOGICIM_SEPARATE_RADIUS	400


//Two components that are all of a subnet, wired by one straight wire in KiCad (their pins level or one above the
//other): their ports are lined up when they're placed so the wire stays straight.
struct SKicadSchemLogicimAlignment
{
	size		uiFromItem;
	size		uiToItem;
	CPortView*	pcFromPort;
	CPortView*	pcToPort;
	bool		bHorizontal;
};


typedef CArrayTemplate<SKicadSchemLogicimAlignment>		CArrayKicadSchemLogicimAlignment;


//Places the model's items (see CKicadSchemLogicimConverter): where KiCad has them, spread out, with tunnels and
//supplies on their pins, splitters beside their components and components a straight wire joins lined up, moved clear
//of each other.  Dense areas are exploded and rewired.
class CKicadSchemLogicimPlacer
{
protected:
	CCircuitFonts*						mpcFonts;
	CKicadSchemLogicimModel*			mpcModel;
	CKicadSchemLogicimBuses*			mpcBuses;
	CKicadSchemLogicimWiring*			mpcWiring;
	SKicadSchemLogicimCounts*			mpsCounts;
	CArrayKicadSchemLogicimAlignment	masAlignments;
	float64								mfSpread;

public:
	void		Init(CCircuitFonts* pcFonts, CKicadSchemLogicimModel* pcModel, CKicadSchemLogicimBuses* pcBuses, CKicadSchemLogicimWiring* pcWiring, SKicadSchemLogicimCounts* psCounts);
	void		Kill(void);

	void		AttachItems(void);
	void		MeasureImportBoxes(void);
	void		FindAlignments(void);
	void		Place(float64 fSpread);
	void		Explode(void);

protected:
	CPortView*	FindAttachTarget(size uiItem);
	bool		IsOnlySubnetConnection(SKicadSchemLogicimConnection* psConnection);
	bool		IsAlignable(SKicadSchemLogicimConnection* psConnection, bool* pbHorizontal);
	void		PlaceSplitters(void);
	void		PlaceSplitter(SKicadSchemLogicimSplitter* psSplitter);
	int32		MeasureSplitterPairs(SKicadSchemLogicimSplitter* psSplitter, bool bAcrossX, int iOut, int32* piPinAcross, int32* piEndAcross);
	int32		GetSplitterStep(SKicadSchemLogicimSplitter* psSplitter, bool bAcrossX, int32 iAlong);
	void		PlaceAttachedItems(void);
	void		AlignItems(void);
	void		AlignItem(SKicadSchemLogicimAlignment* psAlignment, CArrayInt* paiFixed);
	void		SeparateItems(void);
	void		GetSeparateOrder(CArrayInt* paiOrder);
	bool		GetItemBox(size uiItem, SFloat64Bounds* psBox);
	size		FindDenseAreas(CArrayTemplate<SFloat64Vec2>* pasCentres);
	void		ExplodeAreas(CArrayTemplate<SFloat64Vec2>* pasCentres);
	void		GetOffsets(CArrayTemplate<SInt32Vec2>* pasOffsets);
	void		RestoreOffsets(CArrayTemplate<SInt32Vec2>* pasOffsets);
};


#endif // __KICAD_SCHEM_LOGICIM_PLACER_H__

