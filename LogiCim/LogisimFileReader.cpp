#include "LogisimFileReader.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimFileReader::Init(char* szDirectory, char* szFileName)
{
	mszFileName.Init(szFileName);
	mszDirectory.Init(szDirectory);
	mcFile.Init();

	mlCircuits.Init();
	mlLibraries.Init();
	mszMainCircuitName.Init();

	mcComponents.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimFileReader::Kill(void)
{
	int					iNum;
	int					i;
	CLogisimCircuit*	pcCircuit;
	CLogisimLibrary*	pcLibrary;

	mcComponents.Kill();

	mszMainCircuitName.Kill();

	iNum = mlCircuits.NumElements();
	for (i = 0; i < iNum; i++)
	{
		pcCircuit = mlCircuits.Get(i);
		pcCircuit->Kill();
	}
	mlCircuits.Kill();

	iNum = mlLibraries.NumElements();
	for (i = 0; i < iNum; i++)
	{
		pcLibrary = mlLibraries.Get(i);
		pcLibrary->Kill();
	}
	mlLibraries.Kill();

	mcFile.Kill();
	mszDirectory.Kill();
	mszFileName.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CMarkupTag* CLogisimFileReader::Read(void)
{
	CMarkupTag*		pcRoot;
	BOOL			bResult;

	bResult = mcFile.Read(mszFileName.Text(), mszDirectory.Text());
	if (bResult)
	{ 
		pcRoot = mcFile.mcMarkup.GetRootTag();
		return pcRoot;
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::Convert(CMarkupTag* pcContainer)
{
	STagIterator	sIter;
	CMarkupTag*		pcTag;
	char*			szTagName;
	BOOL			bResult;

	pcTag = pcContainer->GetTag(&sIter);
	while (pcTag)
	{
		szTagName = pcTag->GetName();
		if (StringCompare(szTagName, "circuit") == 0)
		{
			bResult = ConvertCircuit(pcTag);
			ReturnOnFalse(bResult);
		}
		else if (StringCompare(szTagName, "main") == 0)
		{
			bResult = ConvertMain(pcTag);
			ReturnOnFalse(bResult);
		}
		else if (StringCompare(szTagName, "lib") == 0)
		{
			bResult = ConvertLibrary(pcTag);
			ReturnOnFalse(bResult);
		}
		else if (StringCompare(szTagName, "options") == 0)
		{
		}
		else if (StringCompare(szTagName, "mappings") == 0)
		{
		}
		else if (StringCompare(szTagName, "toolbar") == 0)
		{
		}
		else
		{
			return gcLogger.Error2(__METHOD__, " Unknown tag [", szTagName, "] during conversion.", NULL);
		}
		pcTag = pcContainer->GetNextTag(&sIter);
	}
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::ConvertCircuit(CMarkupTag* pcCircuitTag)
{
	STagIterator		sIter;
	CMarkupTag*			pcTag;
	char*				szTagName;
	CLogisimCircuit*	pcCircuit;
	char*				szCircuitName;
	BOOL				bResult;

	szCircuitName = pcCircuitTag->GetAttribute("name");
	if (szCircuitName == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Attribute [name] not found during 'circuit' conversion.", NULL);
	}

	pcCircuit = mlCircuits.Add();
	pcCircuit->Init(szCircuitName);

	pcTag = pcCircuitTag->GetTag(&sIter);
	while (pcTag)
	{
		szTagName = pcTag->GetName();
		if (StringCompare(szTagName, "wire") == 0)
		{
			bResult = ConvertWire(pcTag, pcCircuit);
			ReturnOnFalse(bResult);
		}
		else if (StringCompare(szTagName, "comp") == 0)
		{
			bResult = ConvertComponent(pcTag, pcCircuit);
			ReturnOnFalse(bResult);
		}
		else if (StringCompare(szTagName, "appear") == 0)
		{
		}
		else if (StringCompare(szTagName, "a") == 0)
		{
		}
		else
		{
			return gcLogger.Error2(__METHOD__, " Unknown tag [", szTagName, "] during 'circuit' conversion.", NULL);
		}
		pcTag = pcCircuitTag->GetNextTag(&sIter);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::ConvertMain(CMarkupTag* pcMainTag)
{
	char*	szCircuitName;

	szCircuitName = pcMainTag->GetAttribute("name");
	if (szCircuitName == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Attribute [name] not found during 'main' conversion.", NULL);
	}

	mszMainCircuitName.Set(szCircuitName);

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::ConvertLibrary(CMarkupTag* pcMainTag)
{
	char*				szLibraryName;
	char*				szLibraryDesc;
	CLogisimLibrary*	pcLibrary;

	szLibraryName = pcMainTag->GetAttribute("name");
	if (szLibraryName == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Attribute [name] not found during 'library' conversion.", NULL);
	}

	szLibraryDesc = pcMainTag->GetAttribute("desc");
	if (szLibraryDesc == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Attribute [desc] not found during 'library' conversion.", NULL);
	}

	pcLibrary = mlLibraries.Add();
	pcLibrary->Init(szLibraryName, szLibraryDesc);

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::ConvertWire(CMarkupTag* pcWireTag, CLogisimCircuit* pcCircuit)
{
	CLogisimWire*	pcWire;
	char*			szFromPair;
	char*			szToPair;
	SInt2			sFrom;
	SInt2			sTo;
	BOOL			bResult;

	szFromPair = pcWireTag->GetAttribute("from");
	if (szFromPair == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Attribute [from] not found during 'wire' conversion.", NULL);
	}

	szToPair = pcWireTag->GetAttribute("to");
	if (szToPair == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Attribute [to] not found during 'wire' conversion.", NULL);
	}

	bResult = ParseInt2(&sFrom, szFromPair);
	ReturnOnFalse(bResult);

	bResult = ParseInt2(&sTo, szToPair);
	ReturnOnFalse(bResult);

	pcWire = pcCircuit->AddWire();
	pcWire->Init(sFrom, sTo);

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::ConvertComponent(CMarkupTag* pcCompTag, CLogisimCircuit* pcCircuit)
{
	char*				szLib;
	char*				szLoc;
	char*				szName;
	SInt2				sLoc;
	BOOL				bResult;

	szLib = pcCompTag->GetAttribute("lib");

	szLoc = pcCompTag->GetAttribute("loc");
	if (szLoc == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Attribute [loc] not found during 'comp' conversion.", NULL);
	}

	szName = pcCompTag->GetAttribute("name");
	if (szLoc == NULL)
	{
		return gcLogger.Error2(__METHOD__, " Attribute [name] not found during 'comp' conversion.", NULL);
	}

	bResult = ParseInt2(&sLoc, szLoc);
	ReturnOnFalse(bResult);

	if (StringCompare(szName, "Tunnel") == 0)
	{
		return CreateTunnel(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Pull Resistor") == 0)
	{
		return CreatePullResistor(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Constant") == 0)
	{
		return CreateConstant(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "AND Gate") == 0)
	{
		return CreateANDGate(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "NAND Gate") == 0)
	{
		return CreateNANDGate(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "NOR Gate") == 0)
	{
		return CreateNORGate(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "OR Gate") == 0)
	{
		return CreateORGate(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "NOT Gate") == 0)
	{
		return CreateNOTGate(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "XOR Gate") == 0)
	{
		return CreateXORGate(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Clock") == 0)
	{
		return CreateClock(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Controlled Buffer") == 0)
	{
		return CreateControlledBuffer(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Counter") == 0)
	{
		return CreateCounter(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Decoder") == 0)
	{
		return CreateDecoder(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Digital Oscilloscope") == 0)
	{
		return CreateDigitalOscilloscope(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "LED") == 0)
	{
		return CreateLED(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Pin") == 0)
	{
		return CreatePin(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Probe") == 0)
	{
		return CreateProbe(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "RAM") == 0)
	{
		return CreateRAM(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "ROM") == 0)
	{
		return CreateROM(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Splitter") == 0)
	{
		return CreateSplitter(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "Text") == 0)
	{
		return CreateText(pcCompTag, sLoc);
	}
	else if (StringCompare(szName, "D Flip-Flop") == 0)
	{
		return CreateDFlipFlop(pcCompTag, sLoc);
	}
	else
	{
		if (szLib != NULL)
		{
			return gcLogger.Error2(__METHOD__, " Unknown component name [", szName, "] not found during 'comp' conversion.", NULL);
		}
	}

	
	return TRUE;
}


/*
1-of-8 Data Selector(F251)
10-to-4 Encoder(HC147)
12-bit Up Counter(HC4040)
16-bit Latch(LVC16373)
16-bit Line Driver(LVC16244)
2-to-4 Decoder(LVC139)
3-to-8 Decoder(LVC138)
4-bit Line Driver(LVC125)
4-bit Multiplexer(LVC157)
4-bit Multiplexer(LVC257)
4-bit Up Counter(LVC161)
4-bit Up Counter(LVC163)
4-bit Up Counter(VHC161)
4-bit Up / Down Counter(HCT193)
8-bit Bi-latch(LVC543)
8-bit Bus Transceiver(LVC4245)
8-bit Comparator(F521)
8-bit Latch(LVC273)
8-bit Latch(LVC573)
8-bit Latch(LVC574)
8-bit Line Driver(LVC541)
8-bit Up Counter(HC590)
8-bit serial in shift(LVC595)
D-type Flip Flop(LVC74)
EconoReset(DS1813)
Helper(W65C816 Timing)
Microprocessor(W65C816)
*/


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::ConvertATagsToMap(CMapStringString* pcDest, CMarkupTag* pcCompTag)
{
	STagIterator	sIter;
	char*			szTagName;
	char*			szName;
	char*			szValue;
	CMarkupTag*		pcTag;

	pcDest->Init();
	pcTag = pcCompTag->GetTag(&sIter);
	while (pcTag)
	{
		szTagName = pcTag->GetName();
		if (StringCompare(szTagName, "a") == 0)
		{
			szName = pcTag->GetAttribute("name");
			if (szName == NULL)
			{
				return gcLogger.Error2(__METHOD__, " Attribute [name] not found during 'comp.a' conversion.", NULL);
			}

			szValue = pcTag->GetAttribute("val");
			if (szValue == NULL)
			{
				return gcLogger.Error2(__METHOD__, " Attribute [val] not found during 'comp.a' conversion.", NULL);
			}

			pcDest->Put(szName, szValue);
		}
		else
		{
			return gcLogger.Error2(__METHOD__, " Unknown tag [", szTagName, "] during 'comp' conversion.", NULL);
		}
		pcTag = pcTag->GetNextTag(&sIter);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CheckMap(char* szComponentName, CMapStringString* pcMap, const char* szFirstKey, ...)
{
	va_list		vaMarker;
	char*		szKey;
	CChars		szUnknownKeys;

	if (szFirstKey != NULL)
	{
		pcMap->Remove(szFirstKey);

		va_start(vaMarker, szFirstKey);
		szKey = va_arg(vaMarker, char*);
		while (szKey != NULL)
		{
			pcMap->Remove(szKey);
			szKey = va_arg(vaMarker, char*);
		}
		va_end(vaMarker);
	}

	if (pcMap->NumElements() != 0)
	{
		szUnknownKeys.Init();
		pcMap->GetKeysAsString(&szUnknownKeys, ", ");
		gcLogger.Error2(__METHOD__, " Unknown 'a' tag name [", szUnknownKeys.Text(), "] creating ", szComponentName, ".", NULL);
		szUnknownKeys.Kill();
		pcMap->Kill();
		return FALSE;
	}

	pcMap->Kill();
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValueAsInt(CMapStringString* pcMap, char* szKey, int* piValue, char* szDefault)
{
	char* szValue;
	BOOL			bResult;
	CTextParser		cParser;
	TRISTATE		tResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	bResult = cParser.Init(szValue);
	if (!bResult)
	{
		return gcLogger.Error2(__METHOD__, " Could not initialise text parser.", NULL);
	}

	tResult = cParser.GetInteger(piValue);
	if (NotTrue(tResult))
	{
		return gcLogger.Error2(__METHOD__, " Expected ${INTEGER} parsing Int.", NULL);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValueAsHexLong(CMapStringString* pcMap, char* szKey, unsigned long long int* pulliValue, char* szDefault)
{
	char* szValue;
	BOOL			bResult;
	CTextParser		cParser;
	TRISTATE		tResult;
	int				iNumDigits;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	bResult = cParser.Init(szValue);
	if (!bResult)
	{
		return gcLogger.Error2(__METHOD__, " Could not initialise text parser.", NULL);
	}

	tResult = cParser.GetHexadecimalPart(pulliValue, &iNumDigits);
	if (NotTrue(tResult))
	{
		return gcLogger.Error2(__METHOD__, " Expected ${INTEGER} parsing Int.", NULL);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValueAsFacing(CMapStringString* pcMap, char* szKey, ELogisimFacing* peFacing, char* szDefault)
{
	char*	szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (StringCompare(szValue, "east") == 0)
	{
		*peFacing = LF_East;
	}
	else if (StringCompare(szValue, "west") == 0)
	{
		*peFacing = LF_West;
	}
	else if (StringCompare(szValue, "north") == 0)
	{
		*peFacing = LF_North;
	}
	else if (StringCompare(szValue, "south") == 0)
	{
		*peFacing = LF_South;
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [north, east, south or west] parsing Facing.", NULL);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValueAsTrigger(CMapStringString* pcMap, char* szKey, ELogisimTrigger* peTrigger, char* szDefault)
{
	char* szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (StringCompare(szValue, "high") == 0)
	{
		*peTrigger = LT_High;
	}
	else if (StringCompare(szValue, "low") == 0)
	{
		*peTrigger = LT_Low;
	}
	else if (StringCompare(szValue, "rising") == 0)
	{
		*peTrigger = LT_Rising;
	}
	else if (StringCompare(szValue, "falling") == 0)
	{
		*peTrigger = LT_Falling;
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [high, low, rising or falling] parsing Trigger.", NULL);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValueAsAppearance(CMapStringString* pcMap, char* szKey)
{
	char* szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, "classic");
	ReturnOnFalse(bResult);

	if (StringCompare(szValue, "classic") == 0)
	{
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [classic] parsing Appearance.", NULL);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValueAsSplitterAppear(CMapStringString* pcMap, char* szKey, CLogisimSplitterAppearance* peValue, char* szDefault)
{
	char* szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (StringCompare(szValue, "center") == 0)
	{
		*peValue = LSA_Centered;
	}
	else if (StringCompare(szValue, "left") == 0)
	{
		*peValue = LSA_Left;
	}
	else if (StringCompare(szValue, "right") == 0)
	{
		*peValue = LSA_Right;
	}
	else if (StringCompare(szValue, "legacy") == 0)
	{
		*peValue = LSA_Legacy;
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [center, left, right or legacy] parsing Appear.", NULL);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValueAsDataBus(CMapStringString* pcMap, char* szKey, ELogisimRAMDataBus* peValue, char* szDefault)
{
	char* szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (StringCompare(szValue, "bidir") == 0)
	{
		*peValue = LRDB_Bidirectional;
	}
	else if (StringCompare(szValue, "separate") == 0)
	{
		*peValue = LRDB_Separate;
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [bidir or separate] parsing Appear.", NULL);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValue(CMapStringString* pcMap, char* szKey, char** pszValue, char* szDefault)
{
	char* szValue;

	szValue = pcMap->Get(szKey);
	if (szDefault == NULL)
	{
		if (szValue)
		{
			SafeAssign(pszValue, szValue);
		}
		else
		{
			return gcLogger.Error2(__METHOD__, " Map value [", szKey, "] not found.", NULL);
		}
	}
	else
	{
		if (szValue)
		{
			SafeAssign(pszValue, szValue);
		}
		else
		{
			SafeAssign(pszValue, szDefault);
		}
	}
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateTunnel(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimTunnel*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iWidth;
	char*				szLabel;
	ELogisimFacing		eFacing;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValueAsInt(&cMap, "width", &iWidth, "1");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "west");
	bResult &= GetMapValue(&cMap, "labelfont", NULL, "");
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateTunnel();
	pcComp->Init(sLoc);

	pcComp->SetWidth(iWidth);
	pcComp->SetLabel(szLabel);
	pcComp->SetFacing(eFacing);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, "width", "label", "facing", "labelfont", NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreatePullResistor(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimPullResistor*	pcComp;
	CMapStringString		cMap;
	BOOL					bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreatePullResistor();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateConstant(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimConstant*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iWidth;
	uint64				ulliValue;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValueAsHexLong(&cMap, "value", &ulliValue, "1");
	bResult &= GetMapValueAsInt(&cMap, "width", &iWidth, "1");
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateConstant();
	pcComp->Init(sLoc);
	pcComp->SetValue(ulliValue);
	pcComp->SetWidth(iWidth);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, "value", "width", NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateANDGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimANDGate*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateANDGate();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateNANDGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimNANDGate*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateNANDGate();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateNORGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimNORGate*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateNORGate();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateORGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimORGate*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateORGate();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateNOTGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimNOTGate*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateNOTGate();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateXORGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimXORGate*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateXORGate();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateClock(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimClock*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateClock();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateControlledBuffer(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimControlledBuffer*	pcComp;
	CMapStringString			cMap;
	BOOL						bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateControlledBuffer();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateCounter(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimCounter*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateCounter();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateDecoder(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimDecoder*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateDecoder();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateDigitalOscilloscope(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimDigitalOscilloscope*	pcComp;
	CMapStringString				cMap;
	BOOL							bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateDigitalOscilloscope();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateLED(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimLED*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateLED();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreatePin(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimPin*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreatePin();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateProbe(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimProbe*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iRadix;
	ELogisimFacing		eFacing;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValueAsAppearance(&cMap, "appearance");
	bResult &= GetMapValueAsInt(&cMap, "radix", &iRadix, "8");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateProbe();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetRadix(iRadix);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, "appearance", "radix", "facing", NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateRAM(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimRAM*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iAddressWidth;
	char*				szType;
	ELogisimTrigger		eTrigger;
	char*				szEnables;
	char*				szLabelVisible;
	ELogisimRAMDataBus	eDataBus;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValueAsAppearance(&cMap, "appearance");
	bResult &= GetMapValue(&cMap, "type", &szType, "volatile");
	bResult &= GetMapValueAsInt(&cMap, "addrWidth", &iAddressWidth, "8");
	bResult &= GetMapValueAsTrigger(&cMap, "trigger", &eTrigger, "rising");
	bResult &= GetMapValue(&cMap, "enables", &szEnables, "byte");
	bResult &= GetMapValue(&cMap, "labelvisible", &szLabelVisible, "");
	bResult &= GetMapValueAsDataBus(&cMap, "databus", &eDataBus, "separate");
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateRAM();
	pcComp->Init(sLoc);
	pcComp->SetAddressWidth(iAddressWidth);
	pcComp->SetDataBus(eDataBus);
	pcComp->SetLineEnables(StringCompare(szEnables, "line") == 0);
	pcComp->SetTrigger(eTrigger);
	pcComp->SetVolatile(StringCompare(szType, "volatile") == 0);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, "appearance", "type", "addrWidth", "trigger", "trigger", "enables", "labelvisible", "databus", NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateROM(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimROM*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateROM();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateSplitter(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimSplitter*			pcComp;
	CMapStringString			cMap;
	BOOL						bResult;
	int							iSpacing;
	int							iFanOut;
	int							iIncoming;
	ELogisimFacing				eFacing;
	CLogisimSplitterAppearance	eAppear;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValueAsInt(&cMap, "spacing", &iSpacing, "1");
	bResult &= GetMapValueAsInt(&cMap, "fanout", &iFanOut, "2");
	bResult &= GetMapValueAsInt(&cMap, "incoming", &iIncoming, "2");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValueAsSplitterAppear(&cMap, "appear", &eAppear, "left");
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateSplitter();
	pcComp->Init(sLoc);
	pcComp->SetAppear(eAppear);
	pcComp->SetFacing(eFacing);
	pcComp->SetFanOut(iFanOut);
	pcComp->SetIncoming(iIncoming);
	pcComp->SetSpacing(iSpacing);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, "spacing", "fanout", "incoming", "facing", "appear", NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateText(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimText*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateText();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreateDFlipFlop(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimDTypeFlipFlop*	pcComp;
	CMapStringString		cMap;
	BOOL					bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateDTypeFlipFlop();
	pcComp->Init(sLoc);

	return CheckMap(pcCompTag->GetAttribute("name"), &cMap, NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::ParseInt2(SInt2* ps, char* sz)
{
	TRISTATE		tResult;
	int				iX;
	int				iY;
	CTextParser		cParser;
	BOOL			bResult;
	
	bResult = cParser.Init(sz);
	if (!bResult)
	{
		return gcLogger.Error2(__METHOD__, " Could not initialise text parser.", NULL);
	}

	tResult = cParser.GetExactCharacter('(');
	if (NotTrue(tResult))
	{
		return gcLogger.Error2(__METHOD__, " Expected '(' parsing Int2.", NULL);
	}
	tResult = cParser.GetInteger(&iX);
	if (NotTrue(tResult))
	{
		return gcLogger.Error2(__METHOD__, " Expected ${INTEGER} parsing Int2.", NULL);
	}
	tResult = cParser.GetExactCharacter(',');
	if (NotTrue(tResult))
	{
		return gcLogger.Error2(__METHOD__, " Expected ',' parsing Int2.", NULL);
	}
	tResult = cParser.GetInteger(&iY);
	if (NotTrue(tResult))
	{
		return gcLogger.Error2(__METHOD__, " Expected ${INTEGER} parsing Int2.", NULL);
	}
	tResult = cParser.GetExactCharacter(')');
	if (NotTrue(tResult))
	{
		return gcLogger.Error2(__METHOD__, " Expected ')' parsing Int2.", NULL);
	}

	cParser.Kill();

	ps->Init(iX, iY);

	return TRUE;
}

