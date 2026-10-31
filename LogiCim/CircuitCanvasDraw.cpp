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

	sSize = pCanvas->GetActualSize();
	pImage = pCanvas->GetImageOrNull();
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
		cPainter.InitImage(pImage, &mcViewport, &mpMap, &mpSprites, pcColours, mpcCircuit->GetFonts(), mpcCircuit->GetEditor()->GetViewPath(), mpcCircuit->GetEditor()->GetSimulation());
		if (mbGrid)
		{
			PaintCircuitGrid(&cPainter);
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
	SFloat64Vec2	sMinimum;
	SFloat64Vec2	sMaximum;
	float64			fZoomX;
	float64			fZoomY;
	float64			fZoom;
	float64			fPixelsPerGrid;

	if (!mpcPreview->GetBoundingBox(&sMinimum, &sMaximum))
	{
		sMinimum.Init(-1, -1);
		sMaximum.Init(1, 1);
	}
	mcViewport.SetZoom(1);
	fPixelsPerGrid = mcViewport.GetPixelsPerGrid();
	fZoomX = sSize.x / ((sMaximum.x - sMinimum.x + 4) * fPixelsPerGrid);
	fZoomY = sSize.y / ((sMaximum.y - sMinimum.y + 4) * fPixelsPerGrid);
	fZoom = (fZoomX < fZoomY) ? fZoomX : fZoomY;
	if (fZoom > 4)
	{
		fZoom = 4;
	}
	else if (fZoom < 0.2)
	{
		fZoom = 0.2;
	}
	mcViewport.SetZoom(fZoom);
	fPixelsPerGrid = mcViewport.GetPixelsPerGrid();
	mcViewport.SetPosition(-(sMinimum.x + sMaximum.x) / 2 * fPixelsPerGrid, -(sMinimum.y + sMaximum.y) / 2 * fPixelsPerGrid);
}

