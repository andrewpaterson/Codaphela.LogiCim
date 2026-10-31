#ifndef __CIRCUIT_CANVAS_DRAW_H__
#define __CIRCUIT_CANVAS_DRAW_H__
#include "SupportLib/ColourARGB32.h"
#include "SupportLib/GeometryMap.h"
#include "SupportLib/SpriteMap.h"
#include "WindowLib/CanvasDraw.h"
#include "LogicimLib/CircuitViewport.h"


class CLogicimCircuit;
class CComponentView;


//Draws the editor: the background, Logicim's point grid and the circuit with its selection (see CCircuitEditor),
//through a geometry map.  Grid 0, 0 is at the centre of the canvas and a grid unit is ten pixels at a zoom of one.
//Like Logicim's the grid isn't drawn when zoomed far out, its dots are lighter from a zoom of 1.5 and become small
//squares from 2.  Text goes in a sprite map drawn over the geometry map.
//
//Given a preview view it draws only that, unsimulated and zoomed to fit the canvas with a grid unit or two around it,
//as KiCad's symbol chooser previews a symbol.
class CCircuitCanvasDraw : public CCanvasDraw
{
CONSTRUCTABLE(CCircuitCanvasDraw);
DESTRUCTABLE(CCircuitCanvasDraw);
protected:
	CLogicimCircuit*		mpcCircuit;
	CComponentView*		mpcPreview;
	Ptr<CGeometryMap>	mpMap;
	Ptr<CSpriteMap>		mpSprites;
	Ptr<CImage>			mpDestImage;
	CCircuitViewport	mcViewport;
	bool				mbGrid;

public:
	void				Init(CLogicimCircuit* pcCircuit);
	void				Class(void) override;
	void				Free(void) override;

	bool				Draw(Ptr<CCanvas> pCanvas) override;

	void				SetZoom(float32 fZoom);
	float32				GetZoom(void);
	void				SetGrid(bool bGrid);
	bool				IsGrid(void);
	float32				GetGridSpacing(void);
	CCircuitViewport*	GetViewport(void);
	void				SetPreview(CComponentView* pcPreview);

protected:
	void				FitPreview(SInt32Vec2 sSize);
};


#endif // __CIRCUIT_CANVAS_DRAW_H__

