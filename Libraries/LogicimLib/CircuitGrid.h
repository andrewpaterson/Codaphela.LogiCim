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
#ifndef __CIRCUIT_GRID_H__
#define __CIRCUIT_GRID_H__
#include "BaseLib/ArrayTemplate.h"
#include "StandardLib/Array.h"
#include "SupportLib/Image.h"
#include "SupportLib/ImageCel.h"
#include "CircuitPainter.h"


#define CIRCUIT_GRID_DEFAULT_TILE_WIDTH		256
#define CIRCUIT_GRID_MAX_PERIOD				16
#define CIRCUIT_GRID_MIN_ZOOM				0.3


//Logicim's point grid, with the background, drawn as tiles: as PaintCircuitGrid draws it, pixel for pixel, but
//without a shape for every dot.  A tile is one row of dots and the background under it to the next row, so the tiles
//cover the canvas and nothing need be drawn first.  Each row's tiles are placed at the row's own pixel (rows needn't
//be a whole number of pixels apart) and cut to its height.
//
//The dots are rounded to pixels, so a row only repeats along itself where a whole number of grid units is a whole
//number of pixels (ten at zoom one; two, twenty five pixels, at zoom 1.25).  Then a tile is that many pixels, at least
//the minimum tile width; otherwise it is the canvas' width.  Tiles are short and wide, which copies fastest a row at
//a time.  On a 990 pixel wide canvas, tiles of 256 pixels drew within 2% of tiles the canvas' width, the narrowest
//that did (64 pixels took 10% longer, one dot's width 50%).  The tile is made again only when what it looks like
//changes: the zoom, the colours, the canvas or where the columns fall in their pixels.
class CGeometryMap;


class CCircuitGrid : public CObject
{
CONSTRUCTABLE(CCircuitGrid);
DESTRUCTABLE(CCircuitGrid);
protected:
	Ptr<CImage>				mpTile;
	CArray<CImageCel>		maCels;  //The tile cut to each row height drawn, as rows can be a pixel apart.
	CArrayTemplate<int32>	maiCelHeights;
	int32					miMinTileWidth;
	uint32					muiLayout;  //How the viewport's pixels are laid out, which the tile's are made to match.
	int32					miViewportWidth;
	float64					mfSpacing;
	float64					mfAnchorX;
	int32					miDot;
	ARGB32					msBackground;
	ARGB32					msDot;
	int32					miTileLeft;  //Where a tile goes, in canvas pixels; the others are a tile width apart.
	size					muiBuilds;
	size					muiTiles;

public:
	void			Init(int32 iMinTileWidth = CIRCUIT_GRID_DEFAULT_TILE_WIDTH);
	void			Class(void) override;
	void			Free(void) override;
	bool			Save(CObjectWriter* pcFile) override;
	bool			Load(CObjectReader* pcFile) override;

	bool			Paint(CCircuitPainter* pcPainter);

	void			SetMinTileWidth(int32 iMinTileWidth);
	int32			GetTileWidth(void);
	int32			GetTileHeight(void);
	size			NumBuilds(void);
	size			NumTiles(void);

protected:
	void			Build(CCircuitViewport* pcViewport, Ptr<CImage> pViewport, int32 iTileWidth, int32 iTileHeight, bool bRepeats);
	CImageCel*		GetCel(int32 iHeight);
	int32			GetRowTop(CCircuitViewport* pcViewport, int32 iRow);
	void			AddTiles(CCircuitViewport* pcViewport, CGeometryMap* pcMap, int32 iTileWidth, bool bRepeats);
	uint32			GetLayout(Ptr<CImage> pImage);
};


#endif // __CIRCUIT_GRID_H__
