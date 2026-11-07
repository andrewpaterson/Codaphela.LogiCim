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
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "ComponentSpriteCache.h"


//////////////////////////////////////////////////////////////////////////
//
// A memo that remembers nothing.
//
//////////////////////////////////////////////////////////////////////////
void InitComponentSpriteMemo(SComponentSpriteMemo* psMemo)
{
	psMemo->pcCache = NULL;
	psMemo->uiState = 0;
	psMemo->sSprite.uiPage = COMPONENT_SPRITE_NO_PAGE;
	psMemo->sSprite.uiGeneration = 0;
	psMemo->sSprite.pcCel = NULL;
	psMemo->sSprite.sOffset.Init(0, 0);
}


//////////////////////////////////////////////////////////////////////////
//
// FNV-1a: start from COMPONENT_SPRITE_STATE_START and add each thing the
// state depends on.
//
//////////////////////////////////////////////////////////////////////////
uint64 HashSpriteState(uint64 uiHash, const void* pvData, size uiSize)
{
	const uint8*	puiData;
	size			i;

	puiData = (const uint8*)pvData;
	for (i = 0; i < uiSize; i++)
	{
		uiHash ^= puiData[i];
		uiHash *= (uint64)1099511628211ULL;
	}
	return uiHash;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpritePage::Init(int32 iWidth, int32 iHeight, uint32 uiGeneration)
{
	PreInit();
	maCels.Init();
	Reset(iWidth, iHeight, uiGeneration);
	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpritePage::Class(void)
{
	M_Pointer(mpImage);
	M_Embedded(maCels);
	U_Int32(miShelfX);
	U_Int32(miShelfY);
	U_Int32(miShelfHeight);
	U_UInt64(muiLastFrame);
	U_UInt32(muiGeneration);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpritePage::Free(void)
{
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CComponentSpritePage::Save(CObjectWriter* pcFile) { return false; }
bool CComponentSpritePage::Load(CObjectReader* pcFile) { return false; }


//////////////////////////////////////////////////////////////////////////
//
// Empties the page, at iWidth by iHeight, for sprites again.  Cels still
// on it are freed; the new generation tells memos their sprites have
// gone.
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpritePage::Reset(int32 iWidth, int32 iHeight, uint32 uiGeneration)
{
	maCels.RemoveAll();
	if (mpImage.IsNull() || (mpImage->GetWidth() != (uint32)iWidth) || (mpImage->GetHeight() != (uint32)iHeight))
	{
		mpImage = OMalloc<CImage>(iWidth, iHeight, CFT_RGBA, CCO_RGB, CRGB_24bit, ARGB_8bit);
	}
	memset(mpImage->GetData(), 0, (size_t)GetBytes());
	miShelfX = 0;
	miShelfY = 0;
	miShelfHeight = 0;
	muiLastFrame = 0;
	muiGeneration = uiGeneration;
}


//////////////////////////////////////////////////////////////////////////
//
// Finds room for a sprite: beside the last on its shelf, or on a new
// shelf under it.  Returns false if the page is full.
//
//////////////////////////////////////////////////////////////////////////
bool CComponentSpritePage::Place(int32 iWidth, int32 iHeight, SInt32Vec2* psPosition)
{
	int32	iPageWidth;
	int32	iPageHeight;

	iPageWidth = (int32)mpImage->GetWidth();
	iPageHeight = (int32)mpImage->GetHeight();
	if ((iWidth > iPageWidth) || (iHeight > iPageHeight))
	{
		return false;
	}

	if (miShelfX + iWidth > iPageWidth)
	{
		miShelfY += miShelfHeight;
		miShelfX = 0;
		miShelfHeight = 0;
	}
	if (miShelfY + iHeight > iPageHeight)
	{
		return false;
	}

	psPosition->Init(miShelfX, miShelfY);
	miShelfX += iWidth;
	if (iHeight > miShelfHeight)
	{
		miShelfHeight = iHeight;
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Copies the sprite, its top left of pSprite, onto the page at sPosition
// and returns its cel.  The page and sprite images are both premultiplied
// RGBA bytes.
//
//////////////////////////////////////////////////////////////////////////
CImageCel* CComponentSpritePage::AddCel(Ptr<CImage> pSprite, SInt32Vec2 sPosition)
{
	Ptr<CImageCel>	pCel;
	uint8*			puiSource;
	uint8*			puiDest;
	size			uiStride;
	lsize			uiSourceRow;
	lsize			uiDestRow;
	int32			y;
	int32			iWidth;
	int32			iHeight;

	iWidth = (int32)pSprite->GetWidth();
	iHeight = (int32)pSprite->GetHeight();
	uiStride = mpImage->GetPixelByteStride();
	uiSourceRow = (lsize)uiStride * pSprite->GetWidth();
	uiDestRow = (lsize)uiStride * mpImage->GetWidth();
	puiSource = (uint8*)pSprite->GetData();
	puiDest = (uint8*)mpImage->GetData() + (lsize)sPosition.y * uiDestRow + (size)sPosition.x * uiStride;
	for (y = 0; y < iHeight; y++)
	{
		memcpy(puiDest, puiSource, (size_t)((lsize)uiStride * iWidth));
		puiSource += uiSourceRow;
		puiDest += uiDestRow;
	}

	pCel = OMalloc<CImageCel>(mpImage, sPosition.x, sPosition.y, sPosition.x + iWidth, sPosition.y + iHeight);
	maCels.Add(pCel);
	return &pCel;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpritePage::Use(uint64 uiFrame) { muiLastFrame = uiFrame; }
Ptr<CImage> CComponentSpritePage::GetImage(void) { return mpImage; }
uint64 CComponentSpritePage::GetLastFrame(void) { return muiLastFrame; }
uint32 CComponentSpritePage::GetGeneration(void) { return muiGeneration; }
size CComponentSpritePage::NumCels(void) { return maCels.NumElements(); }
lsize CComponentSpritePage::GetBytes(void) { return (lsize)mpImage->GetPixelByteStride() * mpImage->GetWidth() * mpImage->GetHeight(); }


//////////////////////////////////////////////////////////////////////////
//
// uiMaxBytes is how much the atlas pages can grow to before the least
// recently used is reused.  Pages are iPageSize pixels square.
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpriteCache::Init(lsize uiMaxBytes, int32 iPageSize)
{
	PreInit();
	mcSprites.Init();
	maPages.Init();
	muiCurrentPage = COMPONENT_SPRITE_NO_PAGE;
	miPageSize = iPageSize;
	mpScratch = OMalloc<CGeometryMap>();
	mpRaster = NULL;
	mauiKey.Init();
	muiFrame = 1;
	muiBytes = 0;
	muiMaxBytes = uiMaxBytes;
	muiHits = 0;
	muiMisses = 0;
	muiMemoHits = 0;
	muiEvictions = 0;
	muiGenerations = 0;
	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpriteCache::Class(void)
{
	U_Unknown(CMapBlock, mcSprites);
	M_Embedded(maPages);
	U_Size(muiCurrentPage);
	U_Int32(miPageSize);
	M_Pointer(mpScratch);
	M_Pointer(mpRaster);
	U_Unknown(CArrayTemplate<uint8>, mauiKey);
	U_UInt64(muiFrame);
	U_LSize(muiBytes);
	U_LSize(muiMaxBytes);
	U_Size(muiHits);
	U_Size(muiMisses);
	U_Size(muiMemoHits);
	U_Size(muiEvictions);
	U_UInt32(muiGenerations);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpriteCache::Free(void)
{
	mauiKey.Kill();
	mcSprites.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CComponentSpriteCache::Save(CObjectWriter* pcFile) { return false; }
bool CComponentSpriteCache::Load(CObjectReader* pcFile) { return false; }


//////////////////////////////////////////////////////////////////////////
//
// Before a frame is painted.  Pages drawn from in this frame aren't
// reused until a later one.
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpriteCache::BeginFrame(void)
{
	muiFrame++;
}


//////////////////////////////////////////////////////////////////////////
//
// The map a component's shapes are painted into, in pixels from its
// origin's pixel.
//
//////////////////////////////////////////////////////////////////////////
CGeometryMap* CComponentSpriteCache::BeginShapes(void)
{
	mpScratch->Clear();
	return &mpScratch;
}


//////////////////////////////////////////////////////////////////////////
//
// Blits the sprite of the shapes painted since BeginShapes into pcMap,
// for a component whose origin is in pixel sOrigin, making it if no
// shapes like them have been painted before.  psMemo, if not NULL,
// remembers it with uiState.
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpriteCache::EndShapes(CGeometryMap* pcMap, SInt32Vec2 sOrigin, SComponentSpriteMemo* psMemo, uint64 uiState)
{
	SComponentSprite	sSprite;

	sSprite.uiPage = COMPONENT_SPRITE_NO_PAGE;
	sSprite.uiGeneration = 0;
	sSprite.pcCel = NULL;
	sSprite.sOffset.Init(0, 0);
	if (mpScratch->NumCommands() > 0)
	{
		if (!FindOrMakeSprite(&sSprite))
		{
			if (psMemo)
			{
				InitComponentSpriteMemo(psMemo);
			}
			return;
		}
		DrawSprite(&sSprite, pcMap, sOrigin);
	}

	if (psMemo)
	{
		psMemo->pcCache = this;
		psMemo->uiState = uiState;
		psMemo->sSprite = sSprite;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Blits the memo's sprite if it was made by this cache for uiState and
// its page hasn't been reused.  Returns false if the shapes must be
// painted to find their sprite.
//
//////////////////////////////////////////////////////////////////////////
bool CComponentSpriteCache::DrawMemo(SComponentSpriteMemo* psMemo, uint64 uiState, CGeometryMap* pcMap, SInt32Vec2 sOrigin)
{
	SComponentSprite*		psSprite;
	CComponentSpritePage*	pcPage;

	if ((psMemo->pcCache != this) || (psMemo->uiState != uiState))
	{
		return false;
	}

	psSprite = &psMemo->sSprite;
	if (psSprite->uiPage != COMPONENT_SPRITE_NO_PAGE)
	{
		if (psSprite->uiPage >= maPages.NumElements())
		{
			return false;
		}
		pcPage = &maPages.Get(psSprite->uiPage);
		if (pcPage->GetGeneration() != psSprite->uiGeneration)
		{
			return false;
		}
		DrawSprite(psSprite, pcMap, sOrigin);
	}
	muiMemoHits++;
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpriteCache::DrawSprite(SComponentSprite* psSprite, CGeometryMap* pcMap, SInt32Vec2 sOrigin)
{
	maPages.Get(psSprite->uiPage)->Use(muiFrame);
	pcMap->AddSprite(psSprite->pcCel, sOrigin.x + psSprite->sOffset.x, sOrigin.y + psSprite->sOffset.y);
}


//////////////////////////////////////////////////////////////////////////
//
// The sprite of the scratch map's shapes.  A new sprite is the shapes
// rasterised onto transparent black, which leaves them premultiplied,
// then copied onto a page.
//
//////////////////////////////////////////////////////////////////////////
bool CComponentSpriteCache::FindOrMakeSprite(SComponentSprite* psSprite)
{
	SComponentSprite*	psFound;
	CInt32Rect			cBounds;
	int32				iWidth;
	int32				iHeight;
	int32				y;
	size				uiPage;
	SInt32Vec2			sPosition;
	CComponentSpritePage*	pcPage;
	uint8*				puiRow;
	lsize				uiRowStride;

	mauiKey.ReInit();
	mpScratch->AppendKey(&mauiKey);
	FoldKey();
	psFound = (SComponentSprite*)mcSprites.Get(mauiKey.GetData(), mauiKey.NumElements());
	if (psFound)
	{
		muiHits++;
		*psSprite = *psFound;
		return true;
	}

	muiMisses++;
	if (!mpScratch->GetBounds(&cBounds) || (cBounds.GetWidth() <= 0) || (cBounds.GetHeight() <= 0))
	{
		return false;
	}
	iWidth = cBounds.GetWidth();
	iHeight = cBounds.GetHeight();

	//The raster is only ever grown; the part the sprite is drawn into is cleared first.
	if (mpRaster.IsNull() || ((int32)mpRaster->GetWidth() < iWidth) || ((int32)mpRaster->GetHeight() < iHeight))
	{
		mpRaster = NewSpriteImage(mpRaster.IsNull() ? iWidth : ((int32)mpRaster->GetWidth() > iWidth ? (int32)mpRaster->GetWidth() : iWidth),
								  mpRaster.IsNull() ? iHeight : ((int32)mpRaster->GetHeight() > iHeight ? (int32)mpRaster->GetHeight() : iHeight));
	}
	uiRowStride = (lsize)mpRaster->GetPixelByteStride() * mpRaster->GetWidth();
	puiRow = (uint8*)mpRaster->GetData();
	for (y = 0; y < iHeight; y++)
	{
		memset(puiRow, 0, (size_t)((lsize)mpRaster->GetPixelByteStride() * iWidth));
		puiRow += uiRowStride;
	}
	mpScratch->SetViewport(mpRaster);
	mpScratch->Blit(&cBounds, false);
	mpScratch->SetViewport(NULL);

	if (!PlaceSprite(iWidth, iHeight, &uiPage, &sPosition))
	{
		return false;
	}
	pcPage = &maPages.Get(uiPage);

	//The page wants a sprite exactly the sprite's size; the raster can be bigger.
	if (((int32)mpRaster->GetWidth() == iWidth) && ((int32)mpRaster->GetHeight() == iHeight))
	{
		psSprite->pcCel = pcPage->AddCel(mpRaster, sPosition);
	}
	else
	{
		Ptr<CImage>	pSprite;
		uint8*		puiDest;
		lsize		uiDestRow;

		pSprite = NewSpriteImage(iWidth, iHeight);
		uiDestRow = (lsize)pSprite->GetPixelByteStride() * iWidth;
		puiRow = (uint8*)mpRaster->GetData();
		puiDest = (uint8*)pSprite->GetData();
		for (y = 0; y < iHeight; y++)
		{
			memcpy(puiDest, puiRow, (size_t)uiDestRow);
			puiRow += uiRowStride;
			puiDest += uiDestRow;
		}
		psSprite->pcCel = pcPage->AddCel(pSprite, sPosition);
	}
	psSprite->uiPage = uiPage;
	psSprite->uiGeneration = pcPage->GetGeneration();
	psSprite->sOffset = cBounds.msTopLeft;
	mcSprites.Put(mauiKey.GetData(), mauiKey.NumElements(), psSprite, sizeof(SComponentSprite));
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// A key longer than the map takes (a big component's shapes, such as a
// board with many pins) keeps its first bytes, then the hash of all of it
// and its length, so it fits.
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpriteCache::FoldKey(void)
{
	uint64	uiHash;
	uint64	uiLength;
	size	uiKept;

	if (mauiKey.NumElements() <= MAX_KEY_SIZE)
	{
		return;
	}
	uiLength = (uint64)mauiKey.NumElements();
	uiHash = HashSpriteState(COMPONENT_SPRITE_STATE_START, mauiKey.GetData(), mauiKey.NumElements());
	uiKept = MAX_KEY_SIZE - sizeof(uint64) - sizeof(uint64);
	mauiKey.SetUsedElements(uiKept);
	memcpy(mauiKey.InsertNumAt(sizeof(uint64), mauiKey.NumElements()), &uiHash, sizeof(uint64));
	memcpy(mauiKey.InsertNumAt(sizeof(uint64), mauiKey.NumElements()), &uiLength, sizeof(uint64));
}


//////////////////////////////////////////////////////////////////////////
//
// Room for a sprite: on the page sprites are being packed onto, or a new
// one.  A sprite bigger than a page has one of its own.
//
//////////////////////////////////////////////////////////////////////////
bool CComponentSpriteCache::PlaceSprite(int32 iWidth, int32 iHeight, size* puiPage, SInt32Vec2* psPosition)
{
	size	uiPage;
	int32	iPageWidth;
	int32	iPageHeight;

	if ((iWidth <= miPageSize) && (iHeight <= miPageSize))
	{
		if ((muiCurrentPage != COMPONENT_SPRITE_NO_PAGE) && maPages.Get(muiCurrentPage)->Place(iWidth, iHeight, psPosition))
		{
			*puiPage = muiCurrentPage;
			return true;
		}
		iPageWidth = miPageSize;
		iPageHeight = miPageSize;
	}
	else
	{
		iPageWidth = iWidth;
		iPageHeight = iHeight;
	}

	uiPage = NewPage(iPageWidth, iPageHeight);
	if (!maPages.Get(uiPage)->Place(iWidth, iHeight, psPosition))
	{
		return false;
	}
	if ((iPageWidth == miPageSize) && (iPageHeight == miPageSize))
	{
		muiCurrentPage = uiPage;
	}
	*puiPage = uiPage;
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// An empty page.  Past the cache's size the least recently used page not
// drawn from this frame is reused; if every page was, another is made.
//
//////////////////////////////////////////////////////////////////////////
size CComponentSpriteCache::NewPage(int32 iWidth, int32 iHeight)
{
	size					i;
	size					uiOldest;
	uint64					uiOldestFrame;
	CComponentSpritePage*	pcPage;
	Ptr<CComponentSpritePage>	pPage;

	if (muiBytes + (lsize)iWidth * iHeight * 4 > muiMaxBytes)
	{
		uiOldest = COMPONENT_SPRITE_NO_PAGE;
		uiOldestFrame = muiFrame;
		for (i = 0; i < maPages.NumElements(); i++)
		{
			pcPage = &maPages.Get(i);
			if (pcPage->GetLastFrame() < uiOldestFrame)
			{
				uiOldestFrame = pcPage->GetLastFrame();
				uiOldest = i;
			}
		}
		if (uiOldest != COMPONENT_SPRITE_NO_PAGE)
		{
			ResetPage(uiOldest, iWidth, iHeight);
			return uiOldest;
		}
	}

	muiGenerations++;
	pPage = OMalloc<CComponentSpritePage>(iWidth, iHeight, muiGenerations);
	maPages.Add(pPage);
	muiBytes += pPage->GetBytes();
	return maPages.NumElements() - 1;
}


//////////////////////////////////////////////////////////////////////////
//
// Forgets the page's sprites and empties it at the new size.
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpriteCache::ResetPage(size uiPage, int32 iWidth, int32 iHeight)
{
	CComponentSpritePage*	pcPage;
	SMapIterator			sIterator;
	void*					pvKey;
	size					uiKeySize;
	void*					pvData;
	size					uiDataSize;
	bool					bExists;
	CArrayTemplate<uint8>	auiKeys;
	CArrayTemplate<size>	auiKeySizes;
	size					i;
	uint8*					puiKey;

	//The page's sprites' keys are found first; the map can't be changed while it is iterated.
	auiKeys.Init();
	auiKeySizes.Init();
	bExists = mcSprites.StartIteration(&sIterator, &pvKey, &uiKeySize, &pvData, &uiDataSize);
	while (bExists)
	{
		if (((SComponentSprite*)pvData)->uiPage == uiPage)
		{
			auiKeys.InsertBlockAfterEnd((uint8*)pvKey, uiKeySize);
			auiKeySizes.Add(&uiKeySize);
		}
		bExists = mcSprites.Iterate(&sIterator, &pvKey, &uiKeySize, &pvData, &uiDataSize);
	}
	puiKey = auiKeys.GetData();
	for (i = 0; i < auiKeySizes.NumElements(); i++)
	{
		mcSprites.Remove(puiKey, *auiKeySizes.Get(i));
		puiKey += *auiKeySizes.Get(i);
	}
	auiKeySizes.Kill();
	auiKeys.Kill();

	pcPage = &maPages.Get(uiPage);
	muiBytes -= pcPage->GetBytes();
	muiGenerations++;
	pcPage->Reset(iWidth, iHeight, muiGenerations);
	muiBytes += pcPage->GetBytes();
	if (uiPage == muiCurrentPage)
	{
		muiCurrentPage = COMPONENT_SPRITE_NO_PAGE;
	}
	muiEvictions++;
}


//////////////////////////////////////////////////////////////////////////
//
// Premultiplied RGBA bytes, cleared to transparent black.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CImage> CComponentSpriteCache::NewSpriteImage(int32 iWidth, int32 iHeight)
{
	Ptr<CImage>		pImage;

	pImage = OMalloc<CImage>(iWidth, iHeight, CFT_RGBA, CCO_RGB, CRGB_24bit, ARGB_8bit);
	memset(pImage->GetData(), 0, (size_t)((lsize)pImage->GetPixelByteStride() * iWidth * iHeight));
	return pImage;
}


//////////////////////////////////////////////////////////////////////////
//
// Frees every sprite and page.  Not while a frame that drew them is being
// painted.
//
//////////////////////////////////////////////////////////////////////////
void CComponentSpriteCache::Clear(void)
{
	mcSprites.Kill();
	mcSprites.Init();
	maPages.RemoveAll();
	muiCurrentPage = COMPONENT_SPRITE_NO_PAGE;
	muiBytes = 0;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CComponentSpriteCache::NumSprites(void) { return mcSprites.NumElements(); }
size CComponentSpriteCache::NumPages(void) { return maPages.NumElements(); }
lsize CComponentSpriteCache::GetBytes(void) { return muiBytes; }
size CComponentSpriteCache::NumHits(void) { return muiHits; }
size CComponentSpriteCache::NumMisses(void) { return muiMisses; }
size CComponentSpriteCache::NumMemoHits(void) { return muiMemoHits; }
size CComponentSpriteCache::NumEvictions(void) { return muiEvictions; }
CComponentSpritePage* CComponentSpriteCache::GetPage(size uiIndex) { return &maPages.Get(uiIndex); }
