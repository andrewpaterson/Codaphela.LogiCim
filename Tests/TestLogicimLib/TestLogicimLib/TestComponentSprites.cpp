#include "BaseLib/GlobalDataTypesIO.h"
#include "StandardLib/Objects.h"
#include "SupportLib/Image.h"
#include "SupportLib/GeometryMap.h"
#include "SupportLib/SpriteMap.h"
#include "MicroprocessorLib/CircuitSimulation.h"
#include "MicroprocessorLib/FamilyVoltageConfigurationStore.h"
#include "MicroprocessorLib/Trace.h"
#include "MicroprocessorLib/Event.h"
#include "MicroprocessorLib/Timeline.h"
#include "LogicimLib/ValueView.h"
#include "LogicimLib/FlipFlopView.h"
#include "LogicimLib/CounterView.h"
#include "LogicimLib/LabelView.h"
#include "LogicimLib/CircuitView.h"
#include "LogicimLib/ComponentSpriteCache.h"
#include "TestLib/Assert.h"


void LoadTestCircuitFonts(CCircuitFonts* pcFonts);
void KillTestCircuitFonts(CCircuitFonts* pcFonts);


//////////////////////////////////////////////////////////////////////////
//
// The circuit drawn as the circuit canvas draws it, with its components'
// shapes from pcCache if there is one.  The viewport is scrolled by
// sScroll grid units, so positions needn't be on whole pixels.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CImage> RenderWithCache(CCircuitView* pcCircuit, CCircuitColours* pcColours, int32 iWidth, int32 iHeight, float64 fZoom, float64 fScrollX, float64 fScrollY, CViewPath* pcViewPath, CCircuitSimulation* pcSimulation, CCircuitFonts* pcFonts, CComponentSpriteCache* pcCache)
{
	Ptr<CImage>			pImage;
	Ptr<CGeometryMap>	pMap;
	Ptr<CSpriteMap>		pSprites;
	CCircuitViewport	cViewport;
	CCircuitPainter		cPainter;

	pImage = OMalloc<CImage>(iWidth, iHeight, PT_uint8, IMAGE_DIFFUSE_RED, IMAGE_DIFFUSE_GREEN, IMAGE_DIFFUSE_BLUE, CHANNEL_STOP);
	pMap = OMalloc<CGeometryMap>();
	pSprites = OMalloc<CSpriteMap>();

	cViewport.Init(iWidth, iHeight);
	cViewport.SetZoom(fZoom);
	cViewport.Scroll(fScrollX, fScrollY);
	cPainter.InitImage(pImage, &cViewport, &pMap, &pSprites, pcColours, pcFonts, pcViewPath, pcSimulation);
	if (pcCache)
	{
		pcCache->BeginFrame();
		cPainter.SetSpriteCache(pcCache);
	}
	PaintCircuitGrid(&cPainter);
	pcCircuit->Paint(&cPainter);
	AssertTrue(cPainter.BlitImage());

	cPainter.Kill();
	cViewport.Kill();
	return pImage;
}


//////////////////////////////////////////////////////////////////////////
//
// The most any channel of any pixel differs, -1 if the images aren't the
// same size.
//
//////////////////////////////////////////////////////////////////////////
static int32 MaxImageDifference(Ptr<CImage> pA, Ptr<CImage> pB)
{
	uint8*	puiA;
	uint8*	puiB;
	lsize	i;
	lsize	uiBytes;
	int32	iDifference;
	int32	iMax;

	if ((pA->GetWidth() != pB->GetWidth()) || (pA->GetHeight() != pB->GetHeight()) || (pA->GetPixelByteStride() != pB->GetPixelByteStride()))
	{
		return -1;
	}
	puiA = (uint8*)pA->GetData();
	puiB = (uint8*)pB->GetData();
	uiBytes = (lsize)pA->GetPixelByteStride() * pA->GetWidth() * pA->GetHeight();
	iMax = 0;
	for (i = 0; i < uiBytes; i++)
	{
		iDifference = (int32)puiA[i] - (int32)puiB[i];
		if (iDifference < 0)
		{
			iDifference = -iDifference;
		}
		if (iDifference > iMax)
		{
			iMax = iDifference;
		}
	}
	return iMax;
}


//What the tests draw: values with text that changes, flip flops turned two ways, a counter, a label in a box.
struct SSpriteTestCircuit
{
	CFamilyVoltageConfigurationStore	cFamilies;
	CCircuitSimulation					cSimulation;
	CCircuitView						cCircuit;
	CViewPaths							cPaths;
	CViewPath*							pcPath;
	CCircuitFonts						cFonts;
	CLabelView*							pcLabel;
};


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static void InitSpriteTestCircuit(SSpriteTestCircuit* psTest)
{
	CConstantView*			pcConstant;
	CProbeView*				pcProbe;
	CDTypeFlipFlopView*		pcFlipFlop;
	CCounterView*			pcCounter;

	CEvent::ResetNextID();
	CTrace::ResetNextID();
	LoadTestCircuitFonts(&psTest->cFonts);
	psTest->cFamilies.Init();
	AssertTrue(psTest->cFamilies.Load("Input" _FS_ "PropagationTimes.csv"));
	psTest->cSimulation.Init("Sprites");
	psTest->cCircuit.Init();
	psTest->cPaths.Init();
	psTest->pcPath = psTest->cPaths.AddTop(&psTest->cCircuit);

	pcConstant = NewMalloc<CConstantView>();
	pcConstant->Init(-38, -8, ROT_East, "", "LVC", false, 8, RADIX_Hexadecimal, &psTest->cFonts);
	psTest->cCircuit.Add(pcConstant);
	pcProbe = NewMalloc<CProbeView>();
	pcProbe->Init(-24, -8, ROT_West, "", "LVC", false, 8, RADIX_Hexadecimal, &psTest->cFonts);
	psTest->cCircuit.Add(pcProbe);
	psTest->cCircuit.AddTrace(-38, -8, -24, -8);

	pcFlipFlop = NewMalloc<CDTypeFlipFlopView>();
	pcFlipFlop->Init(-2, 0, ROT_East, "", "LVC", false);
	psTest->cCircuit.Add(pcFlipFlop);
	pcFlipFlop = NewMalloc<CDTypeFlipFlopView>();
	pcFlipFlop->Init(11, 0, ROT_North, "", "LVC", false);
	psTest->cCircuit.Add(pcFlipFlop);
	pcCounter = NewMalloc<CCounterView>();
	pcCounter->Init(31, 0, ROT_East, "", "LVC", false, 4, 15);
	psTest->cCircuit.Add(pcCounter);

	psTest->pcLabel = NewMalloc<CLabelView>();
	psTest->pcLabel->Init(-18, 5, ROT_East, "Sprites", TA_Left, false, true, true, &psTest->cFonts);
	psTest->cCircuit.Add(psTest->pcLabel);

	AssertTrue(psTest->cCircuit.CreateSimulation(psTest->pcPath, &psTest->cSimulation, &psTest->cFamilies));
	psTest->cSimulation.GetTimeline()->Run();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static void KillSpriteTestCircuit(SSpriteTestCircuit* psTest)
{
	psTest->cCircuit.Kill();
	psTest->cPaths.Kill();
	psTest->cSimulation.Kill();
	psTest->cFamilies.Kill();
	KillTestCircuitFonts(&psTest->cFonts);
}


//////////////////////////////////////////////////////////////////////////
//
// Drawn from sprites the circuit looks as it does drawn shape by shape,
// but for rounding, at any zoom and position within a pixel, in either
// colours.  A sprite's translucent shapes are composited premultiplied
// rather than blended one by one, which can round two apart.
//
//////////////////////////////////////////////////////////////////////////
void TestComponentSpritesMatchShapes(void)
{
	SSpriteTestCircuit			sTest;
	CCircuitColours				cColours;
	Ptr<CComponentSpriteCache>	pCache;
	Ptr<CImage>					pShapes;
	Ptr<CImage>					pSprites;
	float64						afZooms[] = { 0.75, 1.5, 2.25 };
	float64						afScrolls[] = { 0.0, 0.37 };
	size						i;
	size						j;
	size						k;

	ObjectsInit();
	InitSpriteTestCircuit(&sTest);
	pCache = OMalloc<CComponentSpriteCache>();

	for (k = 0; k < 2; k++)
	{
		if (k == 0)
		{
			cColours.InitLogicim();
		}
		else
		{
			cColours.InitDark();
		}
		for (i = 0; i < 3; i++)
		{
			for (j = 0; j < 2; j++)
			{
				pShapes = RenderWithCache(&sTest.cCircuit, &cColours, 900, 300, afZooms[i], afScrolls[j], afScrolls[j] * 1.5, sTest.pcPath, &sTest.cSimulation, &sTest.cFonts, NULL);
				pSprites = RenderWithCache(&sTest.cCircuit, &cColours, 900, 300, afZooms[i], afScrolls[j], afScrolls[j] * 1.5, sTest.pcPath, &sTest.cSimulation, &sTest.cFonts, &pCache);
				AssertTrue((MaxImageDifference(pShapes, pSprites) >= 0) && (MaxImageDifference(pShapes, pSprites) <= 2));
			}
		}
		cColours.Kill();
	}

	pSprites = NULL;
	pShapes = NULL;
	pCache = NULL;
	KillSpriteTestCircuit(&sTest);
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// Drawn from pcCache's sprites the circuit is within rounding of being
// drawn shape by shape.  Returns the sprites' drawing.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CImage> AssertMatchesShapes(SSpriteTestCircuit* psTest, CCircuitColours* pcColours, float64 fZoom, float64 fScrollX, float64 fScrollY, CViewPath* pcPath, CCircuitSimulation* pcSimulation, CComponentSpriteCache* pcCache)
{
	Ptr<CImage>	pShapes;
	Ptr<CImage>	pSprites;
	int32		iDifference;

	pSprites = RenderWithCache(&psTest->cCircuit, pcColours, 900, 300, fZoom, fScrollX, fScrollY, pcPath, pcSimulation, &psTest->cFonts, pcCache);
	pShapes = RenderWithCache(&psTest->cCircuit, pcColours, 900, 300, fZoom, fScrollX, fScrollY, pcPath, pcSimulation, &psTest->cFonts, NULL);
	iDifference = MaxImageDifference(pShapes, pSprites);
	AssertTrue((iDifference >= 0) && (iDifference <= 2));
	return pSprites;
}


//////////////////////////////////////////////////////////////////////////
//
// Painted again, every component and port draws the sprite its memo
// remembers, without painting its shapes.  Components that look alike
// share sprites: more of the same flip flop, on whole pixels, make none.
// Other colours make more.
//
//////////////////////////////////////////////////////////////////////////
void TestComponentSpritesShared(void)
{
	SSpriteTestCircuit			sTest;
	CCircuitColours				cColours;
	Ptr<CComponentSpriteCache>	pCache;
	size						uiSprites;
	size						uiMisses;
	size						uiHits;
	size						uiLookups;
	CDTypeFlipFlopView*			pcFlipFlop;
	size						i;

	ObjectsInit();
	InitSpriteTestCircuit(&sTest);
	cColours.InitLogicim();
	pCache = OMalloc<CComponentSpriteCache>();

	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	uiSprites = pCache->NumSprites();
	uiMisses = pCache->NumMisses();
	uiLookups = pCache->NumHits() + pCache->NumMisses();
	AssertTrue(uiSprites > 0);
	AssertSize(uiSprites, uiMisses);
	AssertTrue(uiSprites < uiLookups);
	AssertSize(0, pCache->NumMemoHits());

	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertSize(uiSprites, pCache->NumSprites());
	AssertSize(uiMisses, pCache->NumMisses());
	AssertSize(uiLookups, pCache->NumHits() + pCache->NumMisses());
	AssertSize(uiLookups, pCache->NumMemoHits());

	//Three more flip flops like the first, unsimulated so their ports are disconnected: their sprites are the first's.
	for (i = 0; i < 3; i++)
	{
		pcFlipFlop = NewMalloc<CDTypeFlipFlopView>();
		pcFlipFlop->Init(-2 + 10 * (int32)i, -12, ROT_East, "", "LVC", false);
		sTest.cCircuit.Add(pcFlipFlop);
	}
	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, NULL, NULL, &pCache);
	uiSprites = pCache->NumSprites();
	uiMisses = pCache->NumMisses();
	uiHits = pCache->NumHits();
	for (i = 0; i < 3; i++)
	{
		pcFlipFlop = NewMalloc<CDTypeFlipFlopView>();
		pcFlipFlop->Init(-2 + 10 * (int32)i, 12, ROT_East, "", "LVC", false);
		sTest.cCircuit.Add(pcFlipFlop);
	}
	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, NULL, NULL, &pCache);
	AssertSize(uiSprites, pCache->NumSprites());
	AssertSize(uiMisses, pCache->NumMisses());
	AssertTrue(pCache->NumHits() > uiHits);

	//Other colours are other sprites.
	cColours.Kill();
	cColours.InitDark();
	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, NULL, NULL, &pCache);
	AssertTrue(pCache->NumSprites() > uiSprites);
	AssertTrue(pCache->NumMisses() > uiMisses);

	cColours.Kill();
	pCache = NULL;
	KillSpriteTestCircuit(&sTest);
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// Text isn't in the sprites, it can change every tick: a label's new text
// is drawn over the same sprite, which its memo still finds, so nothing
// is looked up or rasterised.  Hiding its box is a change to its shapes:
// the memo is forgotten.
//
//////////////////////////////////////////////////////////////////////////
void TestComponentSpritesText(void)
{
	SSpriteTestCircuit			sTest;
	CCircuitColours				cColours;
	Ptr<CComponentSpriteCache>	pCache;
	Ptr<CImage>					pSprites;
	Ptr<CImage>					pBefore;
	size						uiSprites;
	size						uiMisses;
	size						uiHits;

	ObjectsInit();
	InitSpriteTestCircuit(&sTest);
	cColours.InitLogicim();
	pCache = OMalloc<CComponentSpriteCache>();

	pBefore = AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	uiSprites = pCache->NumSprites();
	uiMisses = pCache->NumMisses();
	uiHits = pCache->NumHits();

	sTest.pcLabel->GetShapes()->SetText(1, "Changed");
	pSprites = AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertSize(uiSprites, pCache->NumSprites());
	AssertSize(uiMisses, pCache->NumMisses());
	AssertSize(uiHits, pCache->NumHits());
	AssertTrue(MaxImageDifference(pBefore, pSprites) > 2);

	sTest.pcLabel->GetShapes()->SetVisible(0, false);
	pBefore = pSprites;
	pSprites = AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertTrue(MaxImageDifference(pBefore, pSprites) > 2);
	AssertSize(uiHits, pCache->NumHits());

	cColours.Kill();
	pBefore = NULL;
	pSprites = NULL;
	pCache = NULL;
	KillSpriteTestCircuit(&sTest);
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// Sprites are kept whatever the zoom: zooming back finds them without
// rasterising.  Scrolling part of a pixel needs other sprites; scrolling
// back finds the first ones.
//
//////////////////////////////////////////////////////////////////////////
void TestComponentSpritesZoomsAndScrolls(void)
{
	SSpriteTestCircuit			sTest;
	CCircuitColours				cColours;
	Ptr<CComponentSpriteCache>	pCache;
	size						uiMisses;
	size						uiHits;

	ObjectsInit();
	InitSpriteTestCircuit(&sTest);
	cColours.InitLogicim();
	pCache = OMalloc<CComponentSpriteCache>();

	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	uiMisses = pCache->NumMisses();
	AssertMatchesShapes(&sTest, &cColours, 1.5, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertTrue(pCache->NumMisses() > uiMisses);
	uiMisses = pCache->NumMisses();
	uiHits = pCache->NumHits();
	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertSize(uiMisses, pCache->NumMisses());
	AssertTrue(pCache->NumHits() > uiHits);
	AssertSize(0, pCache->NumEvictions());

	//A twentieth of a grid unit at twenty pixels a unit is a pixel; a fortieth is half of one.
	AssertMatchesShapes(&sTest, &cColours, 2, 0.025, 0.025, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertTrue(pCache->NumMisses() > uiMisses);
	uiMisses = pCache->NumMisses();
	uiHits = pCache->NumHits();
	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertSize(uiMisses, pCache->NumMisses());
	AssertTrue(pCache->NumHits() > uiHits);

	cColours.Kill();
	pCache = NULL;
	KillSpriteTestCircuit(&sTest);
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// Sprites are packed onto atlas pages; one bigger than a page has a page
// of its own.  Drawn from small pages the circuit looks the same.
//
//////////////////////////////////////////////////////////////////////////
void TestComponentSpritesPages(void)
{
	SSpriteTestCircuit			sTest;
	CCircuitColours				cColours;
	Ptr<CComponentSpriteCache>	pCache;
	size						i;
	size						uiCels;
	bool						bOwnPage;
	Ptr<CImage>					pPage;

	ObjectsInit();
	InitSpriteTestCircuit(&sTest);
	cColours.InitLogicim();

	pCache = OMalloc<CComponentSpriteCache>();
	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertTrue((pCache->NumPages() >= 1) && (pCache->NumPages() < pCache->NumSprites()));
	AssertLSize((lsize)pCache->NumPages() * COMPONENT_SPRITE_PAGE_SIZE * COMPONENT_SPRITE_PAGE_SIZE * 4, pCache->GetBytes());

	//Pages of 32 pixels: the ports share pages, the bodies are each on one their own size.
	pCache = OMalloc<CComponentSpriteCache>((lsize)COMPONENT_SPRITE_CACHE_DEFAULT_BYTES, (int32)32);
	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertTrue(pCache->NumPages() > 1);
	uiCels = 0;
	bOwnPage = false;
	for (i = 0; i < pCache->NumPages(); i++)
	{
		uiCels += pCache->GetPage(i)->NumCels();
		pPage = pCache->GetPage(i)->GetImage();
		if ((pPage->GetWidth() > 32) || (pPage->GetHeight() > 32))
		{
			bOwnPage = true;
			AssertSize(1, pCache->GetPage(i)->NumCels());
		}
	}
	AssertTrue(bOwnPage);
	AssertSize(pCache->NumSprites(), uiCels);
	AssertTrue(pCache->NumPages() < pCache->NumSprites());

	cColours.Kill();
	pPage = NULL;
	pCache = NULL;
	KillSpriteTestCircuit(&sTest);
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// Past the cache's size the least recently used page is reused, but never
// one drawn from in the frame being painted.  A memo of a sprite on a
// reused page is forgotten.
//
//////////////////////////////////////////////////////////////////////////
void TestComponentSpritesEviction(void)
{
	SSpriteTestCircuit			sTest;
	CCircuitColours				cColours;
	Ptr<CComponentSpriteCache>	pCache;
	size						uiPages;
	size						uiMisses;

	ObjectsInit();
	InitSpriteTestCircuit(&sTest);
	cColours.InitLogicim();

	//Room for one page of 64 pixels.
	pCache = OMalloc<CComponentSpriteCache>((lsize)64 * 64 * 4, (int32)64);
	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	uiPages = pCache->NumPages();
	AssertTrue(uiPages > 1);
	AssertSize(0, pCache->NumEvictions());

	//Another zoom's sprites reuse the first zoom's pages; they weren't drawn from this frame.
	AssertMatchesShapes(&sTest, &cColours, 1.5, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertTrue(pCache->NumEvictions() > 0);

	//Back at the first zoom the memos of sprites on reused pages are forgotten and the sprites made again.
	uiMisses = pCache->NumMisses();
	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);
	AssertTrue(pCache->NumMisses() > uiMisses);

	//Cleared.
	pCache->Clear();
	AssertSize(0, pCache->NumSprites());
	AssertSize(0, pCache->NumPages());
	AssertLSize(0, pCache->GetBytes());
	AssertMatchesShapes(&sTest, &cColours, 2, 0, 0, sTest.pcPath, &sTest.cSimulation, &pCache);

	cColours.Kill();
	pCache = NULL;
	KillSpriteTestCircuit(&sTest);
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestComponentSprites(void)
{
	BeginTests();
	DataIOInit();

	TestComponentSpritesMatchShapes();
	TestComponentSpritesShared();
	TestComponentSpritesText();
	TestComponentSpritesZoomsAndScrolls();
	TestComponentSpritesPages();
	TestComponentSpritesEviction();

	DataIOKill();
	TestStatistics();
}
