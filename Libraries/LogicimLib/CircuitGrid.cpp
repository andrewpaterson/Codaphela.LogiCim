/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela LogicimLib

Codaphela LogicimLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela LogicimLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela LogicimLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include <math.h>
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "SupportLib/GeometryMap.h"
#include "CircuitGrid.h"


//////////////////////////////////////////////////////////////////////////
//
// Tiles are at least iMinTileWidth pixels wide where the dots repeat.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitGrid::Init(int32 iMinTileWidth)
{
	PreInit();
	mpTile = NULL;
	maCels.Init();
	maiCelHeights.Init();
	miMinTileWidth = iMinTileWidth;
	muiLayout = 0;
	miViewportWidth = 0;
	mfSpacing = 0;
	mfAnchorX = 0;
	miDot = 0;
	msBackground = 0;
	msDot = 0;
	miTileLeft = 0;
	muiBuilds = 0;
	muiTiles = 0;
	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitGrid::Class(void)
{
	M_Pointer(mpTile);
	M_Embedded(maCels);
	U_Data(CArrayTemplate<int32>, maiCelHeights);
	U_Int32(miMinTileWidth);
	U_UInt32(muiLayout);
	U_Int32(miViewportWidth);
	U_Float64(mfSpacing);
	U_Float64(mfAnchorX);
	U_Int32(miDot);
	U_UInt32(msBackground);
	U_UInt32(msDot);
	U_Int32(miTileLeft);
	U_Size(muiBuilds);
	U_Size(muiTiles);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitGrid::Free(void)
{
	maiCelHeights.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitGrid::Save(CObjectWriter* pcFile) { return false; }
bool CCircuitGrid::Load(CObjectReader* pcFile) { return false; }


//////////////////////////////////////////////////////////////////////////
//
// The pixels across the fewest columns that are a whole number of pixels
// (up to CIRCUIT_GRID_MAX_PERIOD columns), 0 if none are: the dots repeat
// that far apart.
//
//////////////////////////////////////////////////////////////////////////
static int32 GetCircuitGridPeriod(float64 fSpacing)
{
	int32		iColumns;
	float64		fPeriod;

	for (iColumns = 1; iColumns <= CIRCUIT_GRID_MAX_PERIOD; iColumns++)
	{
		fPeriod = iColumns * fSpacing;
		if (fabs(fPeriod - floor(fPeriod + 0.5)) < 1e-9)
		{
			return (int32)floor(fPeriod + 0.5);
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
//
// Puts the grid's tiles first in the painter's map, covering its image.
// Returns false if zoomed too far out for a grid (as PaintCircuitGrid):
// nothing is drawn and the background must be.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitGrid::Paint(CCircuitPainter* pcPainter)
{
	CCircuitViewport*	pcViewport;
	CCircuitColours*	pcColours;
	CGeometryMap*		pcMap;
	Ptr<CImage>			pViewport;
	float64				fZoom;
	float64				fSpacing;
	SFloat64Vec2		sScreen;
	SFloat64Vec2		sGrid;
	SInt32Vec2			sSize;
	int32				iDot;
	ARGB32				sDot;
	int32				iPeriodPixels;
	int32				iTileWidth;
	int32				iTileHeight;
	float64				fAnchorX;
	int32				iTileLeft;
	bool				bRepeats;

	pcViewport = pcPainter->GetViewport();
	pcColours = pcPainter->GetColours();
	pcMap = pcPainter->GetMap();
	pViewport = pcMap->GetViewport();
	fZoom = pcViewport->GetZoom();
	muiTiles = 0;
	if ((fZoom < CIRCUIT_GRID_MIN_ZOOM) || pViewport.IsNull())
	{
		return false;
	}

	sDot = (fZoom < 1.5) ? pcColours->msSmallGridDot : pcColours->msLargeGridDot;
	iDot = (fZoom < 2.0) ? 1 : (int32)fZoom;
	sSize = pcViewport->GetSize();
	fSpacing = pcViewport->GetPixelsPerGrid();

	//The first column at or right of the canvas' left edge.
	sScreen.Init(0, 0);
	pcViewport->ScreenToGrid(&sGrid, &sScreen);
	fAnchorX = pcViewport->GridToScreenX((int32)ceil(sGrid.x));

	//Tiles a whole number of periods wide, or the canvas wide if the dots don't repeat.
	iPeriodPixels = GetCircuitGridPeriod(fSpacing);
	bRepeats = (iPeriodPixels > 0);
	if (bRepeats)
	{
		iTileWidth = iPeriodPixels * ((miMinTileWidth + iPeriodPixels - 1) / iPeriodPixels);
		if (iTileWidth < iPeriodPixels)
		{
			iTileWidth = iPeriodPixels;
		}
		iTileLeft = (int32)floor(fAnchorX + 0.5) - iDot / 2;
	}
	else
	{
		iTileWidth = sSize.x;
		iTileLeft = 0;
	}

	//Rows are at most the spacing rounded up apart.
	iTileHeight = (int32)ceil(fSpacing);
	if ((iTileWidth <= 0) || (iTileHeight <= 0))
	{
		return false;
	}

	if (mpTile.IsNull() || (muiLayout != GetLayout(pViewport)) || ((int32)mpTile->GetWidth() != iTileWidth) || ((int32)mpTile->GetHeight() != iTileHeight) ||
		(mfSpacing != fSpacing) || (mfAnchorX != fAnchorX) || (miDot != iDot) || (msBackground != pcColours->msBackground) || (msDot != sDot) ||
		(!bRepeats && (miViewportWidth != sSize.x)))
	{
		muiLayout = GetLayout(pViewport);
		miViewportWidth = sSize.x;
		mfSpacing = fSpacing;
		mfAnchorX = fAnchorX;
		miDot = iDot;
		msBackground = pcColours->msBackground;
		msDot = sDot;
		miTileLeft = iTileLeft;
		Build(pcViewport, pViewport, iTileWidth, iTileHeight, bRepeats);
	}
	AddTiles(pcViewport, pcMap, iTileWidth, bRepeats);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The tiles from the first row at or above the canvas' top, so they cover
// the top edge, down to past the bottom: across the canvas if the dots
// repeat, otherwise one canvas wide tile a row.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitGrid::AddTiles(CCircuitViewport* pcViewport, CGeometryMap* pcMap, int32 iTileWidth, bool bRepeats)
{
	SFloat64Vec2	sScreen;
	SFloat64Vec2	sGrid;
	SInt32Vec2		sSize;
	int32			iRow;
	int32			iTop;
	int32			iNextTop;
	int32			x;
	CImageCel*		pcCel;

	sSize = pcViewport->GetSize();
	sScreen.Init(0, 0);
	pcViewport->ScreenToGrid(&sGrid, &sScreen);
	iRow = (int32)floor(sGrid.y) - 1;
	iTop = GetRowTop(pcViewport, iRow);
	while (iTop < sSize.y)
	{
		iNextTop = GetRowTop(pcViewport, iRow + 1);
		pcCel = GetCel(iNextTop - iTop);
		if (bRepeats)
		{
			for (x = miTileLeft - iTileWidth * ((miTileLeft + iTileWidth - 1) / iTileWidth); x < sSize.x; x += iTileWidth)
			{
				pcMap->AddTile(pcCel, x, iTop);
				muiTiles++;
			}
		}
		else
		{
			pcMap->AddTile(pcCel, 0, iTop);
			muiTiles++;
		}
		iRow++;
		iTop = iNextTop;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The pixel's size and where its channels are in it, a byte each: tiles
// made for one image are copied a row at a time onto another like it.
//
//////////////////////////////////////////////////////////////////////////
uint32 CCircuitGrid::GetLayout(Ptr<CImage> pImage)
{
	uint32		uiLayout;
	EChannel	aeChannels[4] = { IMAGE_DIFFUSE_RED, IMAGE_DIFFUSE_GREEN, IMAGE_DIFFUSE_BLUE, IMAGE_OPACITY };
	CChannel*	pcChannel;
	size		i;

	uiLayout = (uint32)pImage->GetPixelBitStride();
	for (i = 0; i < 4; i++)
	{
		pcChannel = pImage->GetChannel(aeChannels[i]);
		uiLayout = uiLayout * 31 + ((pcChannel != NULL) ? (uint32)pcChannel->miBitOffset + 1 : 0);
		uiLayout = uiLayout * 31 + ((pcChannel != NULL) ? (uint32)pcChannel->eType : 0);
	}
	return uiLayout;
}


//////////////////////////////////////////////////////////////////////////
//
// The top of a row of dots, in canvas pixels: as PaintCircuitGrid places
// them.
//
//////////////////////////////////////////////////////////////////////////
int32 CCircuitGrid::GetRowTop(CCircuitViewport* pcViewport, int32 iRow)
{
	return (int32)floor(pcViewport->GridToScreenY(iRow) + 0.5) - miDot / 2;
}


//////////////////////////////////////////////////////////////////////////
//
// Draws the tile: the background with a row of dots along its top, at
// the columns' pixels from the tile's left.  Its pixels are laid out as
// the viewport's so tiles are copied a row at a time.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitGrid::Build(CCircuitViewport* pcViewport, Ptr<CImage> pViewport, int32 iTileWidth, int32 iTileHeight, bool bRepeats)
{
	Ptr<CGeometryMap>	pMap;
	CInt32Rect			cRect;
	SFloat64Vec2		sScreen;
	SFloat64Vec2		sGrid;
	int32				iFirst;
	int32				iLast;
	int32				iColumn;
	int32				iLeft;

	mpTile = OMalloc<CImage>((uint32)iTileWidth, (uint32)iTileHeight, pViewport);
	maCels.RemoveAll();
	maiCelHeights.ReInit();

	pMap = OMalloc<CGeometryMap>();
	pMap->SetViewport(mpTile);
	pMap->AddRectangle(0, 0, iTileWidth, iTileHeight, msBackground);

	//Every column that touches the tile: a repeating tile starts at a column's dot; one the canvas' width at its left.
	sScreen.Init(miTileLeft, 0);
	pcViewport->ScreenToGrid(&sGrid, &sScreen);
	iFirst = (int32)floor(sGrid.x) - 1;
	sScreen.Init(miTileLeft + iTileWidth, 0);
	pcViewport->ScreenToGrid(&sGrid, &sScreen);
	iLast = (int32)ceil(sGrid.x) + 1;
	for (iColumn = iFirst; iColumn <= iLast; iColumn++)
	{
		iLeft = (int32)floor(pcViewport->GridToScreenX(iColumn) + 0.5) - miDot / 2 - miTileLeft;
		pMap->AddRectangle(iLeft, 0, miDot, miDot, msDot);
	}
	cRect.Init(0, 0, iTileWidth, iTileHeight);
	pMap->Blit(&cRect, false);
	pMap = NULL;
	muiBuilds++;
}


//////////////////////////////////////////////////////////////////////////
//
// The tile cut to a row iHeight pixels tall.
//
//////////////////////////////////////////////////////////////////////////
CImageCel* CCircuitGrid::GetCel(int32 iHeight)
{
	size			i;
	Ptr<CImageCel>	pCel;

	for (i = 0; i < maiCelHeights.NumElements(); i++)
	{
		if (*maiCelHeights.Get(i) == iHeight)
		{
			return &maCels.Get(i);
		}
	}
	if (iHeight > (int32)mpTile->GetHeight())
	{
		iHeight = (int32)mpTile->GetHeight();
	}
	pCel = OMalloc<CImageCel>(mpTile, 0, 0, (int32)mpTile->GetWidth(), iHeight);
	maCels.Add(pCel);
	maiCelHeights.Add(&iHeight);
	return &pCel;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitGrid::SetMinTileWidth(int32 iMinTileWidth) { miMinTileWidth = iMinTileWidth; mpTile = NULL; }
int32 CCircuitGrid::GetTileWidth(void) { return mpTile.IsNotNull() ? (int32)mpTile->GetWidth() : 0; }
int32 CCircuitGrid::GetTileHeight(void) { return mpTile.IsNotNull() ? (int32)mpTile->GetHeight() : 0; }
size CCircuitGrid::NumBuilds(void) { return muiBuilds; }
size CCircuitGrid::NumTiles(void) { return muiTiles; }
