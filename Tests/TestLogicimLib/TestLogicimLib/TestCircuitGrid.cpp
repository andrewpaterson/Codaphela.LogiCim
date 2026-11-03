#include <math.h>
#include "BaseLib/GlobalDataTypesIO.h"
#include "StandardLib/Objects.h"
#include "SupportLib/Image.h"
#include "SupportLib/GeometryMap.h"
#include "SupportLib/SpriteMap.h"
#include "LogicimLib/CircuitView.h"
#include "LogicimLib/CircuitGrid.h"
#include "TestLib/Assert.h"


//////////////////////////////////////////////////////////////////////////
//
// The grid drawn by pcGrid's tiles onto an image filled with rubbish
// first (the tiles must cover it), or, without a grid, as dots over the
// background.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CImage> RenderGrid(CCircuitGrid* pcGrid, CCircuitColours* pcColours, int32 iWidth, int32 iHeight, float64 fZoom, float64 fScrollX, float64 fScrollY, bool* pbDrawn)
{
	Ptr<CImage>			pImage;
	Ptr<CGeometryMap>	pMap;
	Ptr<CSpriteMap>		pSprites;
	CCircuitViewport	cViewport;
	CCircuitPainter		cPainter;

	pImage = OMalloc<CImage>(iWidth, iHeight, PT_uint8, IMAGE_DIFFUSE_RED, IMAGE_DIFFUSE_GREEN, IMAGE_DIFFUSE_BLUE, CHANNEL_STOP);
	memset(pImage->GetData(), 0xC3, (size_t)((lsize)pImage->GetPixelByteStride() * iWidth * iHeight));
	pMap = OMalloc<CGeometryMap>();
	pSprites = OMalloc<CSpriteMap>();

	cViewport.Init(iWidth, iHeight);
	cViewport.SetZoom(fZoom);
	cViewport.Scroll(fScrollX, fScrollY);
	if (pcGrid)
	{
		cPainter.InitImage(pImage, &cViewport, &pMap, &pSprites, pcColours, NULL, NULL, NULL, false);
		*pbDrawn = pcGrid->Paint(&cPainter);
		if (!*pbDrawn)
		{
			cPainter.PaintBackground();
		}
	}
	else
	{
		cPainter.InitImage(pImage, &cViewport, &pMap, &pSprites, pcColours, NULL, NULL, NULL);
		PaintCircuitGrid(&cPainter);
	}
	AssertTrue(cPainter.BlitImage());
	cPainter.Kill();
	cViewport.Kill();
	return pImage;
}


//////////////////////////////////////////////////////////////////////////
//
// True if the images are the same, byte for byte.
//
//////////////////////////////////////////////////////////////////////////
static bool SameImages(Ptr<CImage> pA, Ptr<CImage> pB)
{
	lsize	uiBytes;

	if ((pA->GetWidth() != pB->GetWidth()) || (pA->GetHeight() != pB->GetHeight()) || (pA->GetPixelByteStride() != pB->GetPixelByteStride()))
	{
		return false;
	}
	uiBytes = (lsize)pA->GetPixelByteStride() * pA->GetWidth() * pA->GetHeight();
	return memcmp(pA->GetData(), pB->GetData(), (size_t)uiBytes) == 0;
}


//////////////////////////////////////////////////////////////////////////
//
// The tiles draw exactly what the dots over the background do, covering
// the whole canvas: at zooms whose rows repeat every few pixels and ones
// that don't, scrolled to any part of a pixel, at any tile width, in
// either colours.
//
//////////////////////////////////////////////////////////////////////////
void TestCircuitGridMatchesDots(void)
{
	CCircuitColours		cColours;
	Ptr<CCircuitGrid>	pGrid;
	Ptr<CImage>			pDots;
	Ptr<CImage>			pTiles;
	bool				bDrawn;
	float64				afZooms[] = { 0.3, 0.5, 0.75, 1, 1.25, 4.0 / 3.0, 1.5, 1.7, 1.2345, 2, 2.25, 3, 4.5 };
	float64				afScrolls[] = { 0, 0, 0.37, 0.61, 12.5, -3.25 };
	int32				aiWidths[] = { 900, 333 };
	int32				aiHeights[] = { 300, 177 };
	int32				aiTileWidths[] = { 1, CIRCUIT_GRID_DEFAULT_TILE_WIDTH, 5000 };
	size				uiZoom;
	size				uiScroll;
	size				uiSize;
	size				uiTile;
	size				uiColours;
	bool				bSame;

	ObjectsInit();
	pGrid = OMalloc<CCircuitGrid>();

	bSame = true;
	for (uiColours = 0; uiColours < 2; uiColours++)
	{
		if (uiColours == 0)
		{
			cColours.InitLogicim();
		}
		else
		{
			cColours.InitDark();
		}
		for (uiTile = 0; uiTile < 3; uiTile++)
		{
			pGrid->SetMinTileWidth(aiTileWidths[uiTile]);
			for (uiZoom = 0; uiZoom < sizeof(afZooms) / sizeof(float64); uiZoom++)
			{
				for (uiScroll = 0; uiScroll < 3; uiScroll++)
				{
					for (uiSize = 0; uiSize < 2; uiSize++)
					{
						pDots = RenderGrid(NULL, &cColours, aiWidths[uiSize], aiHeights[uiSize], afZooms[uiZoom], afScrolls[uiScroll * 2], afScrolls[uiScroll * 2 + 1], NULL);
						pTiles = RenderGrid(&pGrid, &cColours, aiWidths[uiSize], aiHeights[uiSize], afZooms[uiZoom], afScrolls[uiScroll * 2], afScrolls[uiScroll * 2 + 1], &bDrawn);
						bSame &= bDrawn;
						bSame &= SameImages(pDots, pTiles);
					}
				}
			}
		}
		cColours.Kill();
	}
	AssertTrue(bSame);

	pTiles = NULL;
	pDots = NULL;
	pGrid = NULL;
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// Too far out there's no grid: nothing is drawn and the background must
// be, which is then as it was.
//
//////////////////////////////////////////////////////////////////////////
void TestCircuitGridZoomedOut(void)
{
	CCircuitColours		cColours;
	Ptr<CCircuitGrid>	pGrid;
	Ptr<CImage>			pDots;
	Ptr<CImage>			pTiles;
	bool				bDrawn;

	ObjectsInit();
	cColours.InitLogicim();
	pGrid = OMalloc<CCircuitGrid>();

	pTiles = RenderGrid(&pGrid, &cColours, 200, 100, 0.25, 0, 0, &bDrawn);
	AssertFalse(bDrawn);
	AssertSize(0, pGrid->NumTiles());
	AssertSize(0, pGrid->NumBuilds());
	pDots = RenderGrid(NULL, &cColours, 200, 100, 0.25, 0, 0, NULL);
	AssertTrue(SameImages(pDots, pTiles));

	cColours.Kill();
	pTiles = NULL;
	pDots = NULL;
	pGrid = NULL;
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// A tile is one row tall and, where the dots repeat, a whole number of
// repeats at least the minimum width wide; where they don't, the canvas'
// width.  So rows of tiles cover the canvas.
//
//////////////////////////////////////////////////////////////////////////
void TestCircuitGridTileSizes(void)
{
	CCircuitColours		cColours;
	Ptr<CCircuitGrid>	pGrid;
	Ptr<CImage>			pTiles;
	bool				bDrawn;

	ObjectsInit();
	cColours.InitLogicim();
	pGrid = OMalloc<CCircuitGrid>(100);

	//Ten pixels a grid unit: ten pixels repeat.
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 1, 0, 0, &bDrawn);
	AssertInt(100, pGrid->GetTileWidth());
	AssertInt(10, pGrid->GetTileHeight());
	AssertTrue(pGrid->NumTiles() >= (size)(300 / 10) * (900 / 100));

	//Twelve and a half pixels: two grid units, twenty five pixels, repeat.
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 1.25, 0, 0, &bDrawn);
	AssertInt(100, pGrid->GetTileWidth());
	AssertInt(13, pGrid->GetTileHeight());

	//Thirteen and a third: three units, forty pixels.
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 4.0 / 3.0, 0, 0, &bDrawn);
	AssertInt(120, pGrid->GetTileWidth());

	//Twelve point three four five pixels never repeat in a few units: one tile across the canvas each row.
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 1.2345, 0, 0, &bDrawn);
	AssertInt(900, pGrid->GetTileWidth());
	AssertInt(13, pGrid->GetTileHeight());
	AssertTrue(pGrid->NumTiles() <= 300 / 12 + 4);

	//Wider than the canvas is allowed.
	pGrid->SetMinTileWidth(1000);
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 1, 0, 0, &bDrawn);
	AssertInt(1000, pGrid->GetTileWidth());

	cColours.Kill();
	pTiles = NULL;
	pGrid = NULL;
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// The tile is only made again when what it looks like changes: the zoom,
// the colours, the canvas' size where it's a canvas wide, or where the
// columns fall in their pixels.  Scrolling up and down never does.
//
//////////////////////////////////////////////////////////////////////////
void TestCircuitGridBuilds(void)
{
	CCircuitColours		cColours;
	Ptr<CCircuitGrid>	pGrid;
	Ptr<CImage>			pTiles;
	bool				bDrawn;
	size				uiBuilds;

	ObjectsInit();
	cColours.InitLogicim();
	pGrid = OMalloc<CCircuitGrid>();

	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 1.5, 0, 0, &bDrawn);
	AssertSize(1, pGrid->NumBuilds());
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 1.5, 0, 0, &bDrawn);
	AssertSize(1, pGrid->NumBuilds());
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 1.5, 0, 7.3, &bDrawn);
	AssertSize(1, pGrid->NumBuilds());
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 1.5, 0, -0.41, &bDrawn);
	AssertSize(1, pGrid->NumBuilds());

	//A repeating tile doesn't depend on the canvas' size (both centres are on a column, so the columns fall the same).
	pTiles = RenderGrid(&pGrid, &cColours, 600, 500, 1.5, 0, 0, &bDrawn);
	AssertSize(1, pGrid->NumBuilds());

	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 2, 0, 0, &bDrawn);
	AssertSize(2, pGrid->NumBuilds());
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 2, 0.37, 0, &bDrawn);
	AssertSize(3, pGrid->NumBuilds());

	cColours.Kill();
	cColours.InitDark();
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 2, 0.37, 0, &bDrawn);
	AssertSize(4, pGrid->NumBuilds());

	//A tile as wide as the canvas is made again for another width.
	pTiles = RenderGrid(&pGrid, &cColours, 900, 300, 1.2345, 0, 0, &bDrawn);
	uiBuilds = pGrid->NumBuilds();
	pTiles = RenderGrid(&pGrid, &cColours, 901, 300, 1.2345, 0, 0, &bDrawn);
	AssertSize(uiBuilds + 1, pGrid->NumBuilds());

	cColours.Kill();
	pTiles = NULL;
	pGrid = NULL;
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestCircuitGrid(void)
{
	BeginTests();
	DataIOInit();

	TestCircuitGridMatchesDots();
	TestCircuitGridZoomedOut();
	TestCircuitGridTileSizes();
	TestCircuitGridBuilds();

	DataIOKill();
	TestStatistics();
}
