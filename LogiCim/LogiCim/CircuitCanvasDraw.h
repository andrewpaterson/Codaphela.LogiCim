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
#ifndef __CIRCUIT_CANVAS_DRAW_H__
#define __CIRCUIT_CANVAS_DRAW_H__
#include "SupportLib/ColourARGB32.h"
#include "SupportLib/GeometryMap.h"
#include "SupportLib/SpriteMap.h"
#include "WindowLib/CanvasDraw.h"
#include "LogicimLib/CircuitViewport.h"
#include "LogicimLib/ComponentSpriteCache.h"
#include "LogicimLib/CircuitGrid.h"


class CLogicimCircuit;
class CComponentView;


//Draws the editor: the background, Logicim's point grid and the circuit with its selection (see CCircuitEditor),
//through a geometry map.  Grid 0, 0 is at the centre of the canvas and a grid unit is ten pixels at a zoom of one.
//Like Logicim's the grid isn't drawn when zoomed far out, its dots are lighter from a zoom of 1.5 and become small
//squares from 2.  Text goes in a sprite map drawn over the geometry map.
//
//Given a preview view it draws only that, unsimulated and zoomed to fit the canvas with a grid unit or two around it,
//as KiCad's symbol chooser previews a symbol.  A preview draw (SetPreviewOnly) draws only the background without one,
//never the project's circuit.
class CCircuitCanvasDraw : public CCanvasDraw
{
CONSTRUCTABLE(CCircuitCanvasDraw);
DESTRUCTABLE(CCircuitCanvasDraw);
protected:
	CLogicimCircuit*		mpcCircuit;
	CComponentView*		mpcPreview;
	Ptr<CGeometryMap>	mpMap;
	Ptr<CSpriteMap>		mpSprites;
	Ptr<CComponentSpriteCache>	mpSpriteCache;  //Components' shapes, rasterised once.
	Ptr<CCircuitGrid>	mpGrid;  //The grid and background as tiles.
	Ptr<CImage>			mpDestImage;
	CCircuitViewport	mcViewport;
	bool				mbGrid;
	bool				mbPreviewOnly;  //Only ever draws its preview (a chooser's), not the circuit being edited.

public:
	void				Init(CLogicimCircuit* pcCircuit);
	void				Class(void) override;
	void				Free(void) override;

	bool				Draw(Ptr<CCanvas> pCanvas) override;

	void				SetZoom(float32 fZoom);
	float32				GetZoom(void);
	void				ZoomAroundCentre(float32 fZoom);
	void				ZoomToFit(SFloat64Bounds* psBox);
	void				SetGrid(bool bGrid);
	bool				IsGrid(void);
	float32				GetGridSpacing(void);
	CCircuitViewport*	GetViewport(void);
	void				SetPreview(CComponentView* pcPreview);
	void				SetPreviewOnly(bool bPreviewOnly);

protected:
	void				FitPreview(SInt32Vec2 sSize);
};


#endif // __CIRCUIT_CANVAS_DRAW_H__

