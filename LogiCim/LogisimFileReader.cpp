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
		if (IsString(szTagName, "circuit"))
		{
			bResult = ConvertCircuit(pcTag);
			ReturnOnFalse(bResult);
		}
		else if (IsString(szTagName, "main"))
		{
			bResult = ConvertMain(pcTag);
			ReturnOnFalse(bResult);
		}
		else if (IsString(szTagName, "lib"))
		{
			bResult = ConvertLibrary(pcTag);
			ReturnOnFalse(bResult);
		}
		else if (IsString(szTagName, "options"))
		{
		}
		else if (IsString(szTagName, "mappings"))
		{
		}
		else if (IsString(szTagName, "toolbar"))
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
		if (IsString(szTagName, "wire"))
		{
			bResult = ConvertWire(pcTag, pcCircuit);
			ReturnOnFalse(bResult);
		}
		else if (IsString(szTagName, "comp"))
		{
			bResult = ConvertComponent(pcTag, pcCircuit);
			ReturnOnFalse(bResult);
		}
		else if (IsString(szTagName, "appear"))
		{
		}
		else if (IsString(szTagName, "a"))
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

	if (IsString(szName, "Tunnel"))
	{
		return CreateTunnel(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Pull Resistor"))
	{
		return CreatePullResistor(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Constant"))
	{
		return CreateConstant(pcCompTag, sLoc);
	}
	else if (IsString(szName, "AND Gate"))
	{
		return CreateANDGate(pcCompTag, sLoc);
	}
	else if (IsString(szName, "NAND Gate"))
	{
		return CreateNANDGate(pcCompTag, sLoc);
	}
	else if (IsString(szName, "NOR Gate"))
	{
		return CreateNORGate(pcCompTag, sLoc);
	}
	else if (IsString(szName, "OR Gate"))
	{
		return CreateORGate(pcCompTag, sLoc);
	}
	else if (IsString(szName, "NOT Gate"))
	{
		return CreateNOTGate(pcCompTag, sLoc);
	}
	else if (IsString(szName, "XOR Gate"))
	{
		return CreateXORGate(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Clock"))
	{
		return CreateClock(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Controlled Buffer"))
	{
		return CreateControlledBuffer(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Counter"))
	{
		return CreateCounter(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Decoder"))
	{
		return CreateDecoder(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Digital Oscilloscope"))
	{
		return CreateDigitalOscilloscope(pcCompTag, sLoc);
	}
	else if (IsString(szName, "LED"))
	{
		return CreateLED(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Pin"))
	{
		return CreatePin(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Probe"))
	{
		return CreateProbe(pcCompTag, sLoc);
	}
	else if (IsString(szName, "RAM"))
	{
		return CreateRAM(pcCompTag, sLoc);
	}
	else if (IsString(szName, "ROM"))
	{
		return CreateROM(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Splitter"))
	{
		return CreateSplitter(pcCompTag, sLoc);
	}
	else if (IsString(szName, "Text"))
	{
		return CreateText(pcCompTag, sLoc);
	}
	else if (IsString(szName, "D Flip-Flop"))
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
		if (IsString(szTagName, "a"))
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
		pcTag = pcCompTag->GetNextTag(&sIter);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CheckMap(CMarkupTag* pcTag, CMapStringString* pcMap, const char* szFirstKey, ...)
{
	va_list		vaMarker;
	char*		szKey;

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
		return UnknownTagError(pcTag, pcMap);
	}

	pcMap->Kill();
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CheckMap(CMarkupTag* pcTag, CMapStringString* pcMap, CArrayChars* paszKeys)
{
	int			i;
	CChars*		psz;
	
	for (i = 0; i < paszKeys->NumElements(); i++)
	{
		psz = paszKeys->Get(i);
		pcMap->Remove(psz->Text());
	}

	if (pcMap->NumElements() != 0)
	{
		return UnknownTagError(pcTag, pcMap);
	}

	pcMap->Kill();
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::UnknownTagError(CMarkupTag* pcTag, CMapStringString* pcMap)
{
	CChars	szUnknownKeys;
	char* szComponentName;

	szComponentName = pcTag->GetAttribute("name");

	szUnknownKeys.Init();
	pcMap->GetKeysAsString(&szUnknownKeys, ", ");
	gcLogger.Error2(__METHOD__, " Line [", IntToString(pcTag->GetLine() + 1),  "]:  Unknown 'a' tag name [", szUnknownKeys.Text(), "] creating ", szComponentName, ".", NULL);
	szUnknownKeys.Kill();
	pcMap->Kill();
	return FALSE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::IsString(char* szLeft, char* szRight)
{
	return StringCompare(szLeft, szRight) == 0;
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

	if (IsString(szValue, "east"))
	{
		*peFacing = LF_East;
	}
	else if (IsString(szValue, "west"))
	{
		*peFacing = LF_West;
	}
	else if (IsString(szValue, "north"))
	{
		*peFacing = LF_North;
	}
	else if (IsString(szValue, "south"))
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

	if (IsString(szValue, "high"))
	{
		*peTrigger = LT_High;
	}
	else if (IsString(szValue, "low"))
	{
		*peTrigger = LT_Low;
	}
	else if (IsString(szValue, "rising"))
	{
		*peTrigger = LT_Rising;
	}
	else if (IsString(szValue, "falling"))
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
BOOL CLogisimFileReader::GetMapValueAsAppearance(CMarkupTag* pcTag, CMapStringString* pcMap, char* szKey)
{
	char*	szValue;
	BOOL	bResult;
	char*	szComponentName;

	bResult = GetMapValue(pcMap, szKey, &szValue, "classic");
	ReturnOnFalse(bResult);

	if (IsString(szValue, "classic"))
	{
	}
	else
	{
		szComponentName = pcTag->GetAttribute("name");
		return gcLogger.Error2(__METHOD__, " Component [", szComponentName, "], Line [", IntToString(pcTag->GetLine() + 1), "]:  Expected [classic] parsing Appearance.", NULL);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValueAsSplitterAppear(CMapStringString* pcMap, char* szKey, ELogisimSplitterAppearance* peValue, char* szDefault)
{
	char* szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (IsString(szValue, "center"))
	{
		*peValue = LSA_Centered;
	}
	else if (IsString(szValue, "left"))
	{
		*peValue = LSA_Left;
	}
	else if (IsString(szValue, "right"))
	{
		*peValue = LSA_Right;
	}
	else if (IsString(szValue, "legacy"))
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
BOOL CLogisimFileReader::GetMapValueAsPullResistorPull(CMapStringString* pcMap, char* szKey, ELogisimPullResistorPull* pePull, char* szDefault)
{
	char*	szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (IsString(szValue, "0"))
	{
		*pePull = LPRP_Zero;
	}
	else if (IsString(szValue, "1"))
	{
		*pePull = LPRP_One;
	}
	else if (IsString(szValue, "error"))
	{
		*pePull = LPRP_Error;
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [0, 1, or 1] parsing Pull.", NULL);
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValueAsGateOut(CMapStringString* pcMap, char* szKey, ELogisimGateOut* peOut, char* szDefault)
{
	char*	szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (IsString(szValue, "01"))
	{
		*peOut = LGO_ZeroOne;
	}
	else if (IsString(szValue, "0Z"))
	{
		*peOut = LGO_ZeroFloating;
	}
	else if (IsString(szValue, ""))
	{
		*peOut = LGO_FloatingOne;
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [01, 0Z or Z1] parsing Out.", NULL);
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

	if (IsString(szValue, "bidir"))
	{
		*peValue = LRDB_Bidirectional;
	}
	else if (IsString(szValue, "separate"))
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

	return CheckMap(pcCompTag, &cMap, "width", "label", "facing", "labelfont", NULL);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::CreatePullResistor(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimPullResistor*		pcComp;
	CMapStringString			cMap;
	BOOL						bResult;
	ELogisimFacing				eFacing;
	ELogisimPullResistorPull	ePull;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValueAsPullResistorPull(&cMap, "pull", &ePull, "0");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "south");
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreatePullResistor();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetPull(ePull);

	return CheckMap(pcCompTag, &cMap, "pull", "facing", NULL);
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
	ELogisimFacing		eFacing;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValueAsHexLong(&cMap, "value", &ulliValue, "1");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValueAsInt(&cMap, "width", &iWidth, "1");
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateConstant();
	pcComp->Init(sLoc);
	pcComp->SetValue(ulliValue);
	pcComp->SetWidth(iWidth);
	pcComp->SetFacing(eFacing);

	return CheckMap(pcCompTag, &cMap, "value", "width", "facing", NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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
	int					iSize;
	int					iInputs;
	char*				szLabel;
	int					iWidth;
	ELogisimGateOut		eOut;
	ELogisimFacing		eFacing;
	BOOL				abNegate[64];
	int					i;
	CChars				szNegateLabel;
	char*				szNegate;
	BOOL				bAnyNegate;
	CArrayChars			aszKeys;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValueAsInt(&cMap, "size", &iSize, "50");
	bResult &= GetMapValueAsInt(&cMap, "inputs", &iInputs, "2");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValueAsInt(&cMap, "width", &iWidth, "1");
	bResult &= GetMapValueAsGateOut(&cMap, "out", &eOut, "01");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east"); 
	ReturnOnFalse(bResult);

	aszKeys.Init("size", "inputs", "label", "width", "out", "facing", NULL);

	bAnyNegate = FALSE;
	for (i = 0; i < iInputs; i++)
	{
		szNegateLabel.Init("negate");
		szNegateLabel.Append(i);
		bResult = GetMapValue(&cMap, szNegateLabel.Text(), &szNegate, "false");
		aszKeys.Add(&szNegateLabel);
		szNegateLabel.Kill();

		if (!bResult)
		{
			aszKeys.Kill();
			return FALSE;
		}

		abNegate[i] = IsString("true", szNegate);
		if (abNegate[i])
		{
			bAnyNegate = TRUE;
		}
	}

	pcComp = mcComponents.CreateORGate();
	pcComp->Init(sLoc);
	pcComp->SetSize(iSize);
	pcComp->SetInputs(iInputs);
	pcComp->SetWidth(iWidth);
	pcComp->SetOut(eOut);
	pcComp->SetFacing(eFacing);
	if (bAnyNegate)
	{
		for (i = 0; i < iInputs; i++)
		{
			pcComp->SetNegtate(i, abNegate[i]);
		}
	}

	bResult = CheckMap(pcCompTag, &cMap, &aszKeys);
	aszKeys.Kill();
	return bResult;
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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
	int					iWidth;
	ELogisimFacing		eFacing;
	char*				szOutput;
	char*				szLabel;
	char*				szTristate;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValueAsAppearance(pcCompTag, &cMap, "appearance");
	bResult &= GetMapValueAsInt(&cMap, "width", &iWidth, "8");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValue(&cMap, "labelfont", NULL, ""); 
	bResult &= GetMapValue(&cMap, "output", &szOutput, "false");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValue(&cMap, "tristate", &szTristate, "false");
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreatePin();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetWidth(iWidth);
	pcComp->SetLabel(szLabel);
	pcComp->SetOutput(IsString(szOutput, "true"));
	pcComp->SetTristate(IsString(szTristate, "true"));

	return CheckMap(pcCompTag, &cMap, "appearance", "width", "facing", "labelfont", "output", "label", "tristate", NULL);
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
	char*				szAppearance;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValue(&cMap, "appearance", &szAppearance, "classic");
	bResult &= GetMapValueAsInt(&cMap, "radix", &iRadix, "8");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	ReturnOnFalse(bResult);

	pcComp = mcComponents.CreateProbe();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetRadix(iRadix);
	pcComp->SetClassicApearance(!IsString(szAppearance, "NewPins"));

	return CheckMap(pcCompTag, &cMap, "appearance", "radix", "facing", NULL);
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

	bResult = GetMapValueAsAppearance(pcCompTag , &cMap, "appearance");
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
	pcComp->SetLineEnables(IsString(szEnables, "line"));
	pcComp->SetTrigger(eTrigger);
	pcComp->SetVolatile(IsString(szType, "volatile"));

	return CheckMap(pcCompTag, &cMap, "appearance", "type", "addrWidth", "trigger", "trigger", "enables", "labelvisible", "databus", NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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
	ELogisimSplitterAppearance	eAppear;
	CArrayChars					aszKeys;
	int							i;
	CChars						szBitLabel;
	int							aiBitOut[64];

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnOnFalse(bResult);

	bResult = GetMapValueAsInt(&cMap, "spacing", &iSpacing, "1");
	bResult &= GetMapValueAsInt(&cMap, "fanout", &iFanOut, "2");
	bResult &= GetMapValueAsInt(&cMap, "incoming", &iIncoming, "2");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValueAsSplitterAppear(&cMap, "appear", &eAppear, "left");
	ReturnOnFalse(bResult);

	aszKeys.Init("spacing", "fanout", "incoming", "facing", "appear", NULL);

	for (i = 0; i < iIncoming; i++)
	{
		szBitLabel.Init("bit");
		szBitLabel.Append(i);
		bResult = GetMapValueAsInt(&cMap, szBitLabel.Text(), &aiBitOut[i], "-1");
		aszKeys.Add(&szBitLabel);
		szBitLabel.Kill();

		if (!bResult)
		{
			aszKeys.Kill();
			return FALSE;
		}

		if (aiBitOut[i] == -1)
		{
			aiBitOut[i] = i;
		}
	}
	pcComp = mcComponents.CreateSplitter();
	pcComp->Init(sLoc);
	pcComp->SetAppear(eAppear);
	pcComp->SetFacing(eFacing);
	pcComp->SetFanOut(iFanOut);
	pcComp->SetIncoming(iIncoming);
	pcComp->SetSpacing(iSpacing);

	bResult = CheckMap(pcCompTag,  &cMap, &aszKeys);
	aszKeys.Kill();
	return bResult;
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

	return CheckMap(pcCompTag, &cMap, (char*)NULL);
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

