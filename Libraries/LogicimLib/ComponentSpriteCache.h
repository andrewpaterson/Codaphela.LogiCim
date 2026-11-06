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
#ifndef __COMPONENT_SPRITE_CACHE_H__
#define __COMPONENT_SPRITE_CACHE_H__
#include "BaseLib/ArrayTemplate.h"
#include "BaseLib/MapBlock.h"
#include "StandardLib/Array.h"
#include "SupportLib/GeometryMap.h"
#include "SupportLib/Image.h"
#include "SupportLib/ImageCel.h"


#define COMPONENT_SPRITE_CACHE_DEFAULT_BYTES	(64 * 1024 * 1024)
#define COMPONENT_SPRITE_PAGE_SIZE				512
#define COMPONENT_SPRITE_NO_PAGE				((size)-1)
#define COMPONENT_SPRITE_STATE_START			((uint64)14695981039346656037ULL)


class CComponentSpriteCache;


//A sprite as the cache keeps it: its cel on an atlas page, and its top left in pixels from its component's origin.
struct SComponentSprite
{
	size			uiPage;  //COMPONENT_SPRITE_NO_PAGE for shapes that drew nothing.
	uint32			uiGeneration;  //The page's when the sprite was put on it; the page is reused when it changes.
	CImageCel*		pcCel;
	SInt32Vec2		sOffset;
};


//What a component (or a port) last drew from a cache, so it can draw it again without painting its shapes to find
//it.  uiState sums up everything its shapes depend on but text: their geometry, colours, the zoom, the turn and where
//in a pixel the component is.  The sprite is only used while its page hasn't been reused.
struct SComponentSpriteMemo
{
	CComponentSpriteCache*	pcCache;
	uint64					uiState;
	SComponentSprite		sSprite;
};


void	InitComponentSpriteMemo(SComponentSpriteMemo* psMemo);
uint64	HashSpriteState(uint64 uiHash, const void* pvData, size uiSize);


//An atlas page: sprites packed in shelves, left to right, each shelf as tall as its tallest sprite.
class CComponentSpritePage : public CObject
{
CONSTRUCTABLE(CComponentSpritePage);
DESTRUCTABLE(CComponentSpritePage);
protected:
	Ptr<CImage>			mpImage;
	CArray<CImageCel>	maCels;
	int32				miShelfX;
	int32				miShelfY;
	int32				miShelfHeight;
	uint64				muiLastFrame;
	uint32				muiGeneration;  //Given by the cache, unique among all its pages ever.

public:
	void			Init(int32 iWidth, int32 iHeight, uint32 uiGeneration);
	void			Class(void) override;
	void			Free(void) override;
	bool			Save(CObjectWriter* pcFile) override;
	bool			Load(CObjectReader* pcFile) override;

	void			Reset(int32 iWidth, int32 iHeight, uint32 uiGeneration);
	bool			Place(int32 iWidth, int32 iHeight, SInt32Vec2* psPosition);
	CImageCel*		AddCel(Ptr<CImage> pSprite, SInt32Vec2 sPosition);
	void			Use(uint64 uiFrame);

	Ptr<CImage>		GetImage(void);
	uint64			GetLastFrame(void);
	uint32			GetGeneration(void);
	size			NumCels(void);
	lsize			GetBytes(void);
};


//Components' shapes rasterised once and blitted after, so a circuit's frame costs little more than copying its
//components.  A component paints its shapes, text left out, into the scratch map in pixels from its origin's pixel
//(see CComponentShapes::Paint).  What it painted is the key: the same shapes, colours, rotation, zoom and position
//within a pixel paint the same pixels, so components that look alike share a sprite and a shape that changes (a
//clock's waveform colour) is another sprite.  Text and ports, which change as the circuit runs, are drawn each paint.
//
//Sprites are cels packed onto atlas pages and kept whatever the zoom, so zooming back is free.  When the pages would
//pass the cache's size the least recently used page is reused, never one drawn from this frame (the frame's map
//points at its cels).  A sprite bigger than a page has a page of its own.  A component's memo (DrawMemo) finds its
//sprite again without painting its shapes, until its state changes or the sprite's page is reused.
class CComponentSpriteCache : public CObject
{
CONSTRUCTABLE(CComponentSpriteCache);
DESTRUCTABLE(CComponentSpriteCache);
protected:
	CMapBlock						mcSprites;  //Key to SComponentSprite.
	CArray<CComponentSpritePage>	maPages;
	size							muiCurrentPage;  //The page new sprites are packed onto.
	int32							miPageSize;
	Ptr<CGeometryMap>				mpScratch;
	Ptr<CImage>						mpRaster;  //A sprite is rasterised here then copied onto its page.
	CArrayTemplate<uint8>			mauiKey;
	uint64							muiFrame;
	lsize							muiBytes;
	lsize							muiMaxBytes;
	size							muiHits;
	size							muiMisses;
	size							muiMemoHits;
	size							muiEvictions;
	uint32							muiGenerations;  //The last page generation given; never reused, even after Clear.

public:
	void			Init(lsize uiMaxBytes = COMPONENT_SPRITE_CACHE_DEFAULT_BYTES, int32 iPageSize = COMPONENT_SPRITE_PAGE_SIZE);
	void			Class(void) override;
	void			Free(void) override;
	bool			Save(CObjectWriter* pcFile) override;
	bool			Load(CObjectReader* pcFile) override;

	void			BeginFrame(void);
	CGeometryMap*	BeginShapes(void);
	void			EndShapes(CGeometryMap* pcMap, SInt32Vec2 sOrigin, SComponentSpriteMemo* psMemo, uint64 uiState);
	bool			DrawMemo(SComponentSpriteMemo* psMemo, uint64 uiState, CGeometryMap* pcMap, SInt32Vec2 sOrigin);
	void			Clear(void);

	size			NumSprites(void);
	size			NumPages(void);
	lsize			GetBytes(void);
	size			NumHits(void);
	size			NumMisses(void);
	size			NumMemoHits(void);
	size			NumEvictions(void);
	CComponentSpritePage*	GetPage(size uiIndex);

protected:
	bool			FindOrMakeSprite(SComponentSprite* psSprite);
	void			FoldKey(void);
	bool			PlaceSprite(int32 iWidth, int32 iHeight, size* puiPage, SInt32Vec2* psPosition);
	size			NewPage(int32 iWidth, int32 iHeight);
	void			ResetPage(size uiPage, int32 iWidth, int32 iHeight);
	void			DrawSprite(SComponentSprite* psSprite, CGeometryMap* pcMap, SInt32Vec2 sOrigin);
	Ptr<CImage>		NewSpriteImage(int32 iWidth, int32 iHeight);
};


#endif // __COMPONENT_SPRITE_CACHE_H__
