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
#include "StandardLib/ClassDefines.h"
#include "WindowLib/Canvas.h"
#include "LogicimLib/CircuitPainter.h"
#include "LogicimCircuit.h"
#include "CircuitCanvasDraw.h"


//////////////////////////////////////////////////////////////////////////
//
// The circuit isn't owned; it must outlive the draw.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvasDraw::Init(CLogicimCircuit* pcCircuit)
{
	PreInit();
	CCanvasDraw::Init();
	mpcCircuit = pcCircuit;
	mpcPreview = NULL;
	mpMap = OMalloc<CGeometryMap>();
	mpSprites = OMalloc<CSpriteMap>();
	mpSpriteCache = OMalloc<CComponentSpriteCache>();
	mpGrid = OMalloc<CCircuitGrid>();
	mpDestImage = NULL;
	mcViewport.Init(0, 0);
	mbGrid = true;
	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvasDraw::Class(void)
{
	CCanvasDraw::Class();
	U_Pointer(mpcCircuit);
	U_Pointer(mpcPreview);
	M_Pointer(mpMap);
	M_Pointer(mpSprites);
	M_Pointer(mpSpriteCache);
	M_Pointer(mpGrid);
	M_Pointer(mpDestImage);
	U_Data(CCircuitViewport, mcViewport);
	U_Bool(mbGrid);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvasDraw::Free(void)
{
	mcViewport.Kill();
	CCanvasDraw::Free();
}


//////////////////////////////////////////////////////////////////////////
//
// Canvases without an image of their own are drawn into an image the size
// of the canvas, which is then copied on.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitCanvasDraw::Draw(Ptr<CCanvas> pCanvas)
{
	SInt32Vec2			sSize;
	Ptr<CImage>			pImage;
	CCircuitPainter		cPainter;
	CCircuitColours*	pcColours;
	bool				bResult;

	if (!pCanvas->IsValid())
	{
		return true;
	}

	//The painter draws the whole canvas, so a native canvas holding only part of it is drawn through an image of its own.
	sSize = pCanvas->GetActualSize();
	pImage = pCanvas->GetImageOrNull();
	if (pImage.IsNotNull() && ((pCanvas->GetNativeOrigin().x != 0) || (pCanvas->GetNativeOrigin().y != 0) || (pCanvas->GetNativeSize().x != sSize.x) || (pCanvas->GetNativeSize().y != sSize.y)))
	{
		pImage = NULL;
	}
	if (pImage.IsNull())
	{
		if (mpDestImage.IsNull() || (mpDestImage->GetWidth() != (uint32)sSize.x) || (mpDestImage->GetHeight() != (uint32)sSize.y) ||
			(mpDestImage->GetColourFormat() != pCanvas->GetColourFormat()) || (mpDestImage->GetAlphaBits() != pCanvas->GetAlphaBits()))
		{
			mpDestImage = OMalloc<CImage>(sSize.x, sSize.y, pCanvas->GetColourFormat(), pCanvas->GetColourOrder(), pCanvas->GetColourBits(), pCanvas->GetAlphaBits());
		}
		pImage = mpDestImage;
	}
	else
	{
		mpDestImage = NULL;
	}

	pcColours = mpcCircuit->GetColours();
	if (mpcPreview != NULL)
	{
		cPainter.InitImage(pImage, &mcViewport, &mpMap, &mpSprites, pcColours, mpcCircuit->GetFonts(), NULL, NULL);
		FitPreview(sSize);
		mpcPreview->Paint(&cPainter);
	}
	else
	{
		//The grid's tiles cover the canvas, background and all, so nothing is drawn under them.
		cPainter.InitImage(pImage, &mcViewport, &mpMap, &mpSprites, pcColours, mpcCircuit->GetFonts(), mpcCircuit->GetEditor()->GetViewPath(), mpcCircuit->GetEditor()->GetSimulation(), false);
		mpSpriteCache->BeginFrame();
		cPainter.SetSpriteCache(&mpSpriteCache);
		if (!mbGrid || !mpGrid->Paint(&cPainter))
		{
			cPainter.PaintBackground();
		}
		mpcCircuit->GetEditor()->Paint(&cPainter);
	}
	bResult = cPainter.BlitImage();
	cPainter.Kill();
	if (mpDestImage.IsNotNull())
	{
		pCanvas->DrawImage(0, 0, mpDestImage);
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's zoom goes from 0.2 to 6.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvasDraw::SetZoom(float32 fZoom)
{
	mcViewport.SetZoom(fZoom);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
float32 CCircuitCanvasDraw::GetZoom(void) { return (float32)mcViewport.GetZoom(); }
void CCircuitCanvasDraw::SetGrid(bool bGrid) { mbGrid = bGrid; }
bool CCircuitCanvasDraw::IsGrid(void) { return mbGrid; }
float32 CCircuitCanvasDraw::GetGridSpacing(void) { return (float32)mcViewport.GetPixelsPerGrid(); }
CCircuitViewport* CCircuitCanvasDraw::GetViewport(void) { return &mcViewport; }
void CCircuitCanvasDraw::SetPreview(CComponentView* pcPreview) { mpcPreview = pcPreview; }


//////////////////////////////////////////////////////////////////////////
//
// Zooms (no further in than 4 and no further out than 0.2) and scrolls so
// the preview's box, two grid units bigger each way, fills the canvas.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvasDraw::FitPreview(SInt32Vec2 sSize)
{
	SFloat64Bounds	sBox;

	if (!mpcPreview->GetBoundingBox(&sBox))
	{
		sBox.Init(SFloat64Vec2(-1, -1), SFloat64Vec2(1, 1));
	}
	mcViewport.SetSize(sSize.x, sSize.y);
	mcViewport.ZoomToFit(&sBox, 2, 4);
}


//////////////////////////////////////////////////////////////////////////
//
// Zooms keeping the grid point in the middle of the canvas where it is.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvasDraw::ZoomAroundCentre(float32 fZoom)
{
	SInt32Vec2	sSize;

	sSize = mcViewport.GetSize();
	mcViewport.ZoomAround(sSize.x / 2.0, sSize.y / 2.0, fZoom);
}


//////////////////////////////////////////////////////////////////////////
//
// Zooms (no further in than the most) and scrolls so the grid box, one
// grid unit bigger each way, fills the canvas.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvasDraw::ZoomToFit(SFloat64Bounds* psBox)
{
	mcViewport.ZoomToFit(psBox, 1, CIRCUIT_MAXIMUM_ZOOM);
}

