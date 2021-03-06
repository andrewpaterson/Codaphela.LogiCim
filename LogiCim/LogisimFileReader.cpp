#include "BaseLib/FileUtil.h"
#include "LogisimFileReader.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::Init(char* szDirectory, char* szFileName)
{
	CFileUtil	cFileUtil;
	BOOL		bExists;

	mszFileName.Init(szFileName);
	mszDirectory.Init(szDirectory);
	mcFile.Init();

	mlCircuits.Init();
	mlLibraries.Init();
	mszMainCircuitName.Init();

	mcComponents.Init();
	mcCustomComponentList.Init();
	mcCircuitComponentList.Init();

	mszFullPathName.Init();
	cFileUtil.AppendToPath(&mszFullPathName, szDirectory);
	cFileUtil.AppendToPath(&mszFullPathName, szFileName);

	bExists = cFileUtil.Exists(mszFullPathName.Text());
	return bExists;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimFileReader::Kill(void)
{
	int							iNum;
	int							i;
	CLogisimCircuit*			pcCircuit;
	CLogisimLibrary*			pcLibrary;
	CLogisimCustomComponent*	pcCustom;
	CLogisimCircuitComponent*	pcCircuitComponent;

	iNum = mcCircuitComponentList.NumElements();
	for (i = 0; i < iNum; i++)
	{
		pcCircuitComponent = mcCircuitComponentList.Get(i);
		pcCircuitComponent->Kill();
	}
	mcCircuitComponentList.Kill();

	iNum = mcCustomComponentList.NumElements();
	for (i = 0; i < iNum; i++)
	{
		pcCustom = mcCustomComponentList.Get(i);
		pcCustom->Kill();
	}
	mcCustomComponentList.Kill();

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

	mszFullPathName.Kill();
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
char* CLogisimFileReader::GetFileName(void)
{
	return mszFullPathName.Text();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CListTemplate<CLogisimCircuit>* CLogisimFileReader::GetCircuits(void)
{
	return &mlCircuits;
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
	CLogisimComponent*	pcComponent;

	szLib = pcCompTag->GetAttribute("lib");

	szLoc = pcCompTag->GetAttribute("loc");
	if (szLoc == NULL)
	{
		return gcLogger.Error2(__METHOD__, "  Line [", IntToString(pcCompTag->GetLine() + 1), "],  Attribute [loc] not found during 'comp' conversion.", NULL);
	}

	szName = pcCompTag->GetAttribute("name");
	if (szLoc == NULL)
	{
		return gcLogger.Error2(__METHOD__, "  Line [", IntToString(pcCompTag->GetLine() + 1), "],  Attribute [name] not found during 'comp' conversion.", NULL);
	}

	bResult = ParseInt2(&sLoc, szLoc);
	ReturnOnFalse(bResult);

	pcComponent = NULL;
	if (szLib)
	{
		if (IsString(szName, "Tunnel"))
		{
			pcComponent = CreateTunnel(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Pull Resistor"))
		{
			pcComponent = CreatePullResistor(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Constant"))
		{
			pcComponent = CreateConstant(pcCompTag, sLoc);
		}
		else if (IsString(szName, "AND Gate"))
		{
			pcComponent = CreateANDGate(pcCompTag, sLoc);
		}
		else if (IsString(szName, "NAND Gate"))
		{
			pcComponent = CreateNANDGate(pcCompTag, sLoc);
		}
		else if (IsString(szName, "NOR Gate"))
		{
			pcComponent = CreateNORGate(pcCompTag, sLoc);
		}
		else if (IsString(szName, "OR Gate"))
		{
			pcComponent = CreateORGate(pcCompTag, sLoc);
		}
		else if (IsString(szName, "NOT Gate"))
		{
			pcComponent = CreateNOTGate(pcCompTag, sLoc);
		}
		else if (IsString(szName, "XOR Gate"))
		{
			pcComponent = CreateXORGate(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Clock"))
		{
			pcComponent = CreateClock(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Controlled Buffer"))
		{
			pcComponent = CreateControlledBuffer(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Counter"))
		{
			pcComponent = CreateCounter(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Decoder"))
		{
			pcComponent = CreateDecoder(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Digital Oscilloscope"))
		{
			pcComponent = CreateDigitalOscilloscope(pcCompTag, sLoc);
		}
		else if (IsString(szName, "LED"))
		{
			pcComponent = CreateLED(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Pin"))
		{
			pcComponent = CreatePin(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Probe"))
		{
			pcComponent = CreateProbe(pcCompTag, sLoc);
		}
		else if (IsString(szName, "RAM"))
		{
			pcComponent = CreateRAM(pcCompTag, sLoc);
		}
		else if (IsString(szName, "ROM"))
		{
			pcComponent = CreateROM(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Splitter"))
		{
			pcComponent = CreateSplitter(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Text"))
		{
			pcComponent = CreateText(pcCompTag, sLoc);
		}
		else if (IsString(szName, "D Flip-Flop"))
		{
			pcComponent = CreateDFlipFlop(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Random"))
		{
			pcComponent = CreateRandom(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Comparator"))
		{
			pcComponent = CreateComparator(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Shift Register"))
		{
			pcComponent = CreateShiftRegister(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Buffer"))
		{
			pcComponent = CreateBuffer(pcCompTag, sLoc);
		}
		else if (IsString(szName, "BitAdder"))
		{
			pcComponent = CreateBitAdder(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Register"))
		{
			pcComponent = CreateRegister(pcCompTag, sLoc);
		}
		else if (IsString(szName, "Multiplexer"))
		{
			pcComponent = CreateMultiplexer(pcCompTag, sLoc);
		}

		else if (IsString(szName, "1-of-8 Data Selector (F251)") ||
			IsString(szName, "2-to-4 Decoder (LVC139)") ||
			IsString(szName, "3-to-8 Decoder (LVC138)") ||
			IsString(szName, "3-to-8 Decoder (VHC238)") ||
			IsString(szName, "4-bit Line Driver (LVC125)") ||
			IsString(szName, "4-bit Line Driver (LVC126)") ||
			IsString(szName, "4-bit Multiplexer (LVC157)") ||
			IsString(szName, "4-bit Multiplexer (LVC257)") ||
			IsString(szName, "4-bit Up Counter (LVC161)") ||
			IsString(szName, "4-bit Up Counter (LVC163)") ||
			IsString(szName, "4-bit Up Counter (VHC161)") ||
			IsString(szName, "4-bit Up/Down Counter (HCT193)") ||
			IsString(szName, "8-bit Bi-latch (LVC543)") ||
			IsString(szName, "8-bit Bus Transceiver (LVC4245)") ||
			IsString(szName, "8-bit Comparator (F521)") ||
			IsString(szName, "8-bit Latch (LVC273)") ||
			IsString(szName, "8-bit Latch (LVC373)") ||
			IsString(szName, "8-bit Latch (LVC573)") ||
			IsString(szName, "8-bit Latch (LVC574)") ||
			IsString(szName, "8-bit Line Driver (LVC541)") ||
			IsString(szName, "8-bit serial in shift (LVC595)") ||
			IsString(szName, "8-bit Up Counter (HC590)") ||
			IsString(szName, "10-to-4 Encoder (HC147)") ||
			IsString(szName, "12-bit Up Counter (HC4040)") ||
			IsString(szName, "16-bit Latch (LVC16373)") ||
			IsString(szName, "16-bit Line Driver (LVC16244)") ||
			IsString(szName, "16-bit Bus Transceiver (LVC164245)") ||
			IsString(szName, "D-type Flip Flop (LVC74)") ||
			IsString(szName, "EconoReset (DS1813)") ||
			IsString(szName, "4-to-1 Multiplexer (QS3253)") ||
			IsString(szName, "8-bit parallel in shift (LV165)") ||
			IsString(szName, "8-to-3 Encoder (LS148)") ||
			IsString(szName, "8-to-1 Multiplexer (B3251)") ||
			IsString(szName, "8-to-1 Multiplexer (F251)") ||
			IsString(szName, "2-bit Translator (LSF0102)") ||
			IsString(szName, "4-bit Translator (LSF0204)") ||
			IsString(szName, "8-bit Translator (LSF0108)") ||
			IsString(szName, "4-bit Full Adder (F283)") ||
			IsString(szName, "8-bit Down Counter (HCT40103)") ||
			IsString(szName, "16-bit Latch (LVC162373)") ||
			IsString(szName, "512 Byte FIFO (IDT7201)") ||
			IsString(szName, "Helper (W65C816 Timing)") ||
			IsString(szName, "Microprocessor (W65C816)"))
		{
			pcComponent = CreateCustomComponent(pcCompTag, sLoc, szName);
		}
		else
		{
		return gcLogger.Error2(__METHOD__, "  Line [", IntToString(pcCompTag->GetLine() + 1), "],  Unknown component name [", szName, "] not found during 'comp' conversion.", NULL);
		}
	}
 	else
	{
		pcComponent = CreateCircuitComponent(pcCompTag, sLoc, szName);
	}

	if (pcComponent)
	{
		pcCircuit->AddComponent(pcComponent);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


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
	CChars			szText;
	
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
				return gcLogger.Error2(__METHOD__, "  Line [", IntToString(pcTag->GetLine() + 1), "], Component [", pcTag->GetAttribute("name"), "]:  Attribute [name] not found during 'comp.a' conversion.", NULL);
			}

			szValue = pcTag->GetAttribute("val");
			if (szValue == NULL)
			{
				szText.Init();
				pcTag->GetText(&szText);

				if (szText.Empty())
				{
					szText.Kill();
					return gcLogger.Error2(__METHOD__, "  Line [", IntToString(pcTag->GetLine() + 1), "], Component [", pcTag->GetAttribute("name"), "]:  Attribute [val] not found during 'comp.a' conversion.", NULL);
				}
				else
				{
					pcDest->Put(szName, szText.Text());
					szText.Kill();
				}
			}
			else
			{
				pcDest->Put(szName, szValue);
			}
		}
		else
		{
			return gcLogger.Error2(__METHOD__, "  Line [", IntToString(pcTag->GetLine() + 1), "], Component [", pcTag->GetAttribute("name"), "]:  Unknown tag [", szTagName, "] during 'comp' conversion.", NULL);
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
BOOL CLogisimFileReader::GetMapValueAsInt(CMarkupTag* pcTag, CMapStringString* pcMap, char* szKey, int* piValue, char* szDefault)
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
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Line [", IntToString(pcTag->GetLine() + 1), "], Component [", pcTag->GetAttribute("name"), "]:  Could not initialise text parser.", NULL);
	}

	tResult = cParser.GetInteger(piValue);
	if (NotTrue(tResult))
	{
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, "  Line [", IntToString(pcTag->GetLine() + 1), "], Component [", pcTag->GetAttribute("name"), "]:  Expected ${INTEGER} parsing Int.  Found [", szValue, "] instead.", NULL);
	}

	cParser.Kill();

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::GetMapValueAsHexLong(CMapStringString* pcMap, char* szKey, unsigned long long int* pulliValue, BOOL bInclude0x, char* szDefault)
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
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Could not initialise text parser.", NULL);
	}

	if (!bInclude0x)
	{
		tResult = cParser.GetHexadecimalPart(pulliValue, &iNumDigits);
		if (NotTrue(tResult))
		{
			cParser.Kill();
			return gcLogger.Error2(__METHOD__, " Expected ${INTEGER} parsing Int.", NULL);
		}
	}
	else
	{
		tResult = cParser.GetHexadecimal(pulliValue, &iNumDigits);
		if (NotTrue(tResult))
		{
			cParser.Kill();
			return gcLogger.Error2(__METHOD__, " Expected ${INTEGER} parsing Int.", NULL);
		}
	}

	cParser.Kill();
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
BOOL CLogisimFileReader::GetMapValueAsRGB(CMapStringString* pcMap, char* szKey, uint32* puiRGB, char* szDefault)
{
	char*			szValue;
	BOOL			bResult;
	CTextParser		cParser;
	TRISTATE		tResult;
	uint64			ulliRGB;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	bResult = cParser.Init(szValue);
	if (!bResult)
	{
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Could not initialise text parser.", NULL);
	}

	tResult = cParser.GetExactCharacter('#');
	if (NotTrue(tResult))
	{
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Expected '#' parsing RGB.", NULL);
	}

	tResult = cParser.GetHexadecimalPart(&ulliRGB, NULL, 6);
	if (NotTrue(tResult))
	{
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Expected ${INTEGER} parsing RGB.", NULL);
	}

	cParser.Kill();
	*puiRGB = (uint32)ulliRGB;

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
BOOL CLogisimFileReader::GetMapValueAsNumericType(CMapStringString* pcMap, char* szKey, ELogisimNumericType* peNumericType, char* szDefault)
{
	char*	szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (IsString(szValue, "signed"))
	{
		*peNumericType = LNT_TwosComplement;
	}
	else if (IsString(szValue, "unsigned"))
	{
		*peNumericType = LNT_Unsigned;
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [signed or unsigned] parsing Numeric Type.", NULL);
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
BOOL CLogisimFileReader::GetMapValueAsControlledBufferControl(CMapStringString* pcMap, char* szKey, ELogisimControlledBufferControl* peValue, char* szDefault)
{
	char* szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (IsString(szValue, "left"))
	{
		*peValue = LCBC_Left;
	}
	else if (IsString(szValue, "right"))
	{
		*peValue = LCBC_Right;
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [left or right] parsing Control.", NULL);
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
BOOL CLogisimFileReader::GetMapValueAsSelectLocation(CMapStringString* pcMap, char* szKey, ELogisimSelectLocation* peSelectLocation, char* szDefault)
{
	char* szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (IsString(szValue, "tr"))
	{
		*peSelectLocation = LSL_TopRight;
	}
	else if (IsString(szValue, "bl"))
	{
		*peSelectLocation = LSL_BottomLeft;
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [tr or bl] parsing Select Location.", NULL);
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
BOOL CLogisimFileReader::GetMapValueAsAlignment(CMapStringString* pcMap, char* szKey, ELogisimAlignment* peValue, char* szDefault)
{
	char*	szValue;
	BOOL	bResult;

	bResult = GetMapValue(pcMap, szKey, &szValue, szDefault);
	ReturnOnFalse(bResult);

	if (IsString(szValue, "left"))
	{
		*peValue = LA_Left;
	}
	else if (IsString(szValue, "right"))
	{
		*peValue = LA_Right;
	}
	else if (IsString(szValue, "center"))
	{
		*peValue = LA_Centered;
	}
	else if (IsString(szValue, "top"))
	{
		*peValue = LA_Top;
	}
	else if (IsString(szValue, "bottom"))
	{
		*peValue = LA_Bottom;
	}
	else if (IsString(szValue, "base"))
	{
		*peValue = LA_Base;
	}
	else
	{
		return gcLogger.Error2(__METHOD__, " Expected [left, right, center top, bottom or base] parsing Alignment.", NULL);
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
CLogisimTunnel* CLogisimFileReader::CreateTunnel(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimTunnel*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iWidth;
	char*				szLabel;
	ELogisimFacing		eFacing;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsInt(pcCompTag, &cMap, "width", &iWidth, "1");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "west");
	bResult &= GetMapValue(&cMap, "labelfont", NULL, "");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateTunnel();
	pcComp->Init(sLoc);
	pcComp->SetWidth(iWidth);
	pcComp->SetLabel(szLabel);
	pcComp->SetFacing(eFacing);

	bResult = CheckMap(pcCompTag, &cMap, "width", "label", "facing", "labelfont", NULL);
	if (!bResult)
	{
		return (CLogisimTunnel*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimPullResistor* CLogisimFileReader::CreatePullResistor(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimPullResistor*		pcComp;
	CMapStringString			cMap;
	BOOL						bResult;
	ELogisimFacing				eFacing;
	ELogisimPullResistorPull	ePull;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsPullResistorPull(&cMap, "pull", &ePull, "0");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "south");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreatePullResistor();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetPull(ePull);

	bResult = CheckMap(pcCompTag, &cMap, "pull", "facing", NULL);
	if (!bResult)
	{
		return (CLogisimPullResistor*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimConstant* CLogisimFileReader::CreateConstant(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimConstant*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iWidth;
	uint64				ulliValue;
	ELogisimFacing		eFacing;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsHexLong(&cMap, "value", &ulliValue, FALSE, "1");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "width", &iWidth, "1");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateConstant();
	pcComp->Init(sLoc);
	pcComp->SetValue(ulliValue);
	pcComp->SetWidth(iWidth);
	pcComp->SetFacing(eFacing);

	bResult = CheckMap(pcCompTag, &cMap, "value", "width", "facing", NULL);
	if (!bResult)
	{
		return (CLogisimConstant*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimANDGate* CLogisimFileReader::CreateANDGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimANDGate*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateANDGate();
	pcComp->Init(sLoc);

	bResult = PopulateGate(pcCompTag, pcComp, &cMap);
	if (!bResult)
	{
		return (CLogisimANDGate*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimNANDGate* CLogisimFileReader::CreateNANDGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimNANDGate*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateNANDGate();
	pcComp->Init(sLoc);

	bResult = PopulateGate(pcCompTag, pcComp, &cMap);
	if (!bResult)
	{
		return (CLogisimNANDGate*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimNORGate* CLogisimFileReader::CreateNORGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimNORGate*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateNORGate();
	pcComp->Init(sLoc);

	bResult = PopulateGate(pcCompTag, pcComp, &cMap);
	if (!bResult)
	{
		return (CLogisimNORGate*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimORGate* CLogisimFileReader::CreateORGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CMapStringString	cMap;
	CLogisimORGate*		pcComp;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateORGate();
	pcComp->Init(sLoc);

	bResult = PopulateGate(pcCompTag, pcComp, &cMap);
	if (!bResult)
	{
		return (CLogisimORGate*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimXORGate* CLogisimFileReader::CreateXORGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimXORGate* pcComp;
	CMapStringString	cMap;
	BOOL				bResult;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateXORGate();
	pcComp->Init(sLoc);

	bResult = PopulateGate(pcCompTag, pcComp, &cMap);
	if (!bResult)
	{
		return (CLogisimXORGate*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CLogisimFileReader::PopulateGate(CMarkupTag* pcCompTag, CLogisimGate* pcComp, CMapStringString* pcMap)
{
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

	bResult = GetMapValueAsInt(pcCompTag, pcMap, "size", &iSize, "50");
	bResult &= GetMapValueAsInt(pcCompTag, pcMap, "inputs", &iInputs, "2");
	bResult &= GetMapValue(pcMap, "label", &szLabel, "");
	bResult &= GetMapValueAsInt(pcCompTag, pcMap, "width", &iWidth, "1");
	bResult &= GetMapValueAsGateOut(pcMap, "out", &eOut, "01");
	bResult &= GetMapValueAsFacing(pcMap, "facing", &eFacing, "east");
	if (!bResult)
	{
		KillComponent(pcComp);
		return FALSE;
	}

	aszKeys.Init("size", "inputs", "label", "width", "out", "facing", NULL);

	bAnyNegate = FALSE;
	for (i = 0; i < iInputs; i++)
	{
		szNegateLabel.Init("negate");
		szNegateLabel.Append(i);
		bResult = GetMapValue(pcMap, szNegateLabel.Text(), &szNegate, "false");
		aszKeys.Add(&szNegateLabel);
		szNegateLabel.Kill();

		if (!bResult)
		{
			KillComponent(pcComp);
			aszKeys.Kill();
			return FALSE;
		}

		abNegate[i] = IsString("true", szNegate);
		if (abNegate[i])
		{
			bAnyNegate = TRUE;
		}
	}

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

	bResult = CheckMap(pcCompTag, pcMap, &aszKeys);
	aszKeys.Kill();

	if (!bResult)
	{
		KillComponent(pcComp);
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimNOTGate* CLogisimFileReader::CreateNOTGate(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimNOTGate*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	ELogisimFacing		eFacing;
	int					iSize;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "size", &iSize, "50");

	pcComp = mcComponents.CreateNOTGate();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetSize(iSize);

	bResult = CheckMap(pcCompTag, &cMap, "facing", "size", NULL);
	if (!bResult)
	{
		return (CLogisimNOTGate*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimClock* CLogisimFileReader::CreateClock(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimClock*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	ELogisimFacing		eFacing;
	int					iHighDuration;
	int					iLowDuration;
	int					iPhaseOffset;
	char*				szLabel;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "high", &iHighDuration, "1");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "low", &iLowDuration, "1");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "offset", &iPhaseOffset, "0");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");

	pcComp = mcComponents.CreateClock();
	pcComp->Init(sLoc);
	pcComp->SetHighDuration(iHighDuration);
	pcComp->SetPhaseOffset(iPhaseOffset);
	pcComp->SetLowDuration(iLowDuration);
	pcComp->SetLabel(szLabel);

	bResult = CheckMap(pcCompTag, &cMap, "facing", "high", "low", "offset", "label", NULL);
	if (!bResult)
	{
		return (CLogisimClock*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimControlledBuffer* CLogisimFileReader::CreateControlledBuffer(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimControlledBuffer*			pcComp;
	CMapStringString					cMap;
	BOOL								bResult;
	int									iWidth;
	ELogisimFacing						eFacing;
	char*								szLabel;
	ELogisimControlledBufferControl		eControl;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsInt(pcCompTag, &cMap, "width", &iWidth, "8");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValueAsControlledBufferControl(&cMap, "control", &eControl, "right");
	
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateControlledBuffer();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetWidth(iWidth);
	pcComp->SetLabel(szLabel);
	pcComp->SetControl(eControl);

	bResult = CheckMap(pcCompTag, &cMap, "width", "facing", "label", "control", NULL);
	if (!bResult)
	{
		return (CLogisimControlledBuffer*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimCounter* CLogisimFileReader::CreateCounter(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimCounter*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iWidth;
	uint64				ulliMax;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsAppearance(pcCompTag, &cMap, "appearance");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "width", &iWidth, "8");
	bResult &= GetMapValueAsHexLong(&cMap, "max", &ulliMax, TRUE, "0xff");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateCounter();
	pcComp->Init(sLoc);
	pcComp->SetWidth(iWidth);
	pcComp->SetMax(ulliMax);

	bResult = CheckMap(pcCompTag, &cMap, "appearance", "width", "max", NULL);
	if (!bResult)
	{
		return (CLogisimCounter*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimDecoder* CLogisimFileReader::CreateDecoder(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimDecoder*		pcComp;
	CMapStringString		cMap;
	BOOL					bResult;
	char*					szIncludeEnabled;
	int						iSelectBits;
	char*					szDisabledZero;
	ELogisimSelectLocation	eSelectLocation;
	char*					szTristate;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsInt(pcCompTag, &cMap, "select", &iSelectBits, "8");
	bResult &= GetMapValueAsSelectLocation(&cMap, "selloc", &eSelectLocation, "bl");
	bResult &= GetMapValue(&cMap, "tristate", &szTristate, "false");
	bResult &= GetMapValue(&cMap, "enable", &szIncludeEnabled, "true");
	bResult &= GetMapValue(&cMap, "disabled", &szDisabledZero, "0");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateDecoder();
	pcComp->Init(sLoc);
	pcComp->SetTristate(IsString(szTristate, "true"));
	pcComp->SetDisabledZero(IsString(szDisabledZero, "0"));
	pcComp->SetIncludeEnabled(IsString(szIncludeEnabled, "true"));
	pcComp->SetSelectBits(iSelectBits);
	pcComp->SetSelectLocation(eSelectLocation);

	bResult = CheckMap(pcCompTag, &cMap, "select", "selloc", "tristate", "enable", "disabled", NULL);
	if (!bResult)
	{
		return (CLogisimDecoder*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimDigitalOscilloscope* CLogisimFileReader::CreateDigitalOscilloscope(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimDigitalOscilloscope*	pcComp;
	CMapStringString				cMap;
	BOOL							bResult;
	int								iNumberOfStates;
	int								iNumberOfInputs;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsInt(pcCompTag, &cMap, "nState", &iNumberOfStates, "8");
	bResult = GetMapValueAsInt(pcCompTag, &cMap, "inputs", &iNumberOfInputs, "8");

	pcComp = mcComponents.CreateDigitalOscilloscope();
	pcComp->Init(sLoc);
	pcComp->SetNumberOfInputs(iNumberOfInputs);
	pcComp->SetNumberOfStates(iNumberOfStates);

	bResult = CheckMap(pcCompTag, &cMap, "nState", "inputs", NULL);
	if (!bResult)
	{
		return (CLogisimDigitalOscilloscope*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimLED* CLogisimFileReader::CreateLED(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimLED*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	uint32				uiColour;
	uint32				uiOffColour;
	ELogisimFacing		eFacing;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult = GetMapValueAsRGB(&cMap, "color", &uiColour, "#f00000");
	bResult = GetMapValueAsRGB(&cMap, "offcolor", &uiOffColour, "#404040");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateLED();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetColour(uiColour);
	pcComp->SetOffColour(uiOffColour);

	bResult = CheckMap(pcCompTag, &cMap, "color", "offcolor", "facing", NULL);
	if (!bResult)
	{
		return (CLogisimLED*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimPin* CLogisimFileReader::CreatePin(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimPin*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iWidth;
	int					iRadix;
	ELogisimFacing		eFacing;
	char*				szOutput;
	char*				szLabel;
	char*				szTristate;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsAppearance(pcCompTag, &cMap, "appearance");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "width", &iWidth, "8");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValue(&cMap, "labelfont", NULL, ""); 
	bResult &= GetMapValue(&cMap, "output", &szOutput, "false");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "radix", &iRadix, "2");
	bResult &= GetMapValue(&cMap, "tristate", &szTristate, "false");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreatePin();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetWidth(iWidth);
	pcComp->SetLabel(szLabel);
	pcComp->SetOutput(IsString(szOutput, "true"));
	pcComp->SetTristate(IsString(szTristate, "true"));
	pcComp->SetRadix(iRadix);

	bResult = CheckMap(pcCompTag, &cMap, "appearance", "width", "facing", "labelfont", "output", "label", "tristate", "radix", NULL);
	if (!bResult)
	{
		return (CLogisimPin*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimProbe* CLogisimFileReader::CreateProbe(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimProbe*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iRadix;
	ELogisimFacing		eFacing;
	char*				szAppearance;
	char*				szLabel;
	char*				szLabelLocation;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValue(&cMap, "appearance", &szAppearance, "classic");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "radix", &iRadix, "8");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValue(&cMap, "labelloc", &szLabelLocation, "");
	
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateProbe();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetRadix(iRadix);
	pcComp->SetClassicApearance(!IsString(szAppearance, "NewPins"));
	pcComp->SetLabel(szLabel);

	bResult = CheckMap(pcCompTag, &cMap, "appearance", "radix", "facing", "label", "labelloc", NULL);
	if (!bResult)
	{
		return (CLogisimProbe*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimRAM* CLogisimFileReader::CreateRAM(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimRAM*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iAddressWidth;
	int					iDataWidth;
	char*				szType;
	ELogisimTrigger		eTrigger;
	char*				szEnables;
	char*				szLabelVisible;
	ELogisimRAMDataBus	eDataBus;
	char*				szLabel;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsAppearance(pcCompTag , &cMap, "appearance");
	bResult &= GetMapValue(&cMap, "type", &szType, "volatile");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "addrWidth", &iAddressWidth, "8");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "dataWidth", &iDataWidth, "8");
	bResult &= GetMapValueAsTrigger(&cMap, "trigger", &eTrigger, "rising");
	bResult &= GetMapValue(&cMap, "enables", &szEnables, "byte");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValue(&cMap, "labelvisible", &szLabelVisible, "");
	bResult &= GetMapValueAsDataBus(&cMap, "databus", &eDataBus, "separate");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateRAM();
	pcComp->Init(sLoc);
	pcComp->SetAddressWidth(iAddressWidth);
	pcComp->SetDataWidth(iDataWidth);
	pcComp->SetDataBus(eDataBus);
	pcComp->SetLineEnables(IsString(szEnables, "line"));
	pcComp->SetLabel(szLabel);
	pcComp->SetTrigger(eTrigger);
	pcComp->SetVolatile(IsString(szType, "volatile"));

	bResult = CheckMap(pcCompTag, &cMap, "appearance", "type", "addrWidth", "dataWidth", "trigger", "trigger", "enables", "label", "labelvisible", "databus", NULL);
	if (!bResult)
	{
		return (CLogisimRAM*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimROM* CLogisimFileReader::CreateROM(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimROM*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	char*				szLabel;
	int					iAddressWidth;
	int					iDataWidth;
	char*				szContents;
	uint32*				pvMemory;
	CTextParser			cParser;
	TRISTATE			tResult;
	int					iContentsAddressWidth;
	int					iContentsDataWidth;

	int					iData1;
	uint64				iData2;
	int					iPosition;
	int					i;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsAppearance(pcCompTag, &cMap, "appearance");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "addrWidth", &iAddressWidth, "8");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "dataWidth", &iDataWidth, "8");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValue(&cMap, "contents", &szContents, "");
	ReturnNullOnFalse(bResult);

	cParser.Init(szContents);
	tResult = cParser.GetExactCharacterSequence("addr/data:");
	if (NotTrue(tResult))
	{
		cParser.Kill();
		gcLogger.Error2(__METHOD__, " Expected ROM contents to begin 'addr/data:'.", NULL);
		return NULL;
	}
	tResult = cParser.GetInteger(&iContentsAddressWidth);
	if (NotTrue(tResult))
	{
		cParser.Kill();
		gcLogger.Error2(__METHOD__, " Expected ${INTEGER} as ROM address width.", NULL);
		return NULL;
	}
	tResult = cParser.GetInteger(&iContentsDataWidth);
	if (NotTrue(tResult))
	{
		cParser.Kill();
		gcLogger.Error2(__METHOD__, " Expected ${INTEGER} as ROM data width.", NULL);
		return NULL;
	}
	if ((iAddressWidth != iContentsAddressWidth) || (iDataWidth != iContentsDataWidth))
	{
		cParser.Kill();
		gcLogger.Error2(__METHOD__, " Content data and address widths do not match.", NULL);
		return NULL;
	}

	pcComp = mcComponents.CreateROM();
	pcComp->Init(sLoc);
	pcComp->SetLabel(szLabel);
	pcComp->SetAddressWidth(iAddressWidth);

	pvMemory = pcComp->GetMemory();

	iPosition = 0;

	for (;;)
	{
		cParser.PushPosition();
		tResult = cParser.GetInteger(&iData1);
		if (tResult == TRITRUE)
		{
			tResult = cParser.GetExactCharacter('*', FALSE);
			if (tResult == TRITRUE)
			{
				cParser.PassPosition();
				tResult = cParser.GetHexadecimalPart(&iData2, NULL);
				for (i = 0; i < iData1; i++, iPosition++)
				{
					pvMemory[iPosition] = (uint32)iData2;
				}
			}
			else
			{
				cParser.PopPosition();
				cParser.SkipWhiteSpace();
				tResult = cParser.GetHexadecimalPart(&iData2, NULL);
				if (tResult == TRITRUE)
				{
					pvMemory[iPosition] = (uint32)iData2;
					iPosition++;
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			cParser.PopPosition();
			cParser.SkipWhiteSpace();
			tResult = cParser.GetHexadecimalPart(&iData2, NULL);
			if (tResult == TRITRUE)
			{
				pvMemory[iPosition] = (uint8)iData2;
				iPosition++;
			}
			else
			{
				break;
			}
		}
	}
	cParser.Kill();

	bResult = CheckMap(pcCompTag, &cMap, "addrWidth", "appearance", "contents", "label", NULL);
	if (!bResult)
	{
		return (CLogisimROM*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimSplitter* CLogisimFileReader::CreateSplitter(CMarkupTag* pcCompTag, SInt2 sLoc)
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
	char*						szBitOut;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsInt(pcCompTag, &cMap, "spacing", &iSpacing, "1");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "fanout", &iFanOut, "2");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "incoming", &iIncoming, "2");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValueAsSplitterAppear(&cMap, "appear", &eAppear, "left");
	ReturnNullOnFalse(bResult);

	aszKeys.Init("spacing", "fanout", "incoming", "facing", "appear", NULL);

	for (i = 0; i < iIncoming; i++)
	{
		szBitLabel.Init("bit");
		szBitLabel.Append(i);

		bResult = GetMapValue(&cMap, szBitLabel.Text(), &szBitOut, "");
		if (!bResult)
		{
			aszKeys.Kill();
			szBitLabel.Kill();
			return NULL;
		}

		if (!IsString(szBitOut, "none"))
		{
			bResult = GetMapValueAsInt(pcCompTag, &cMap, szBitLabel.Text(), &aiBitOut[i], "-1");
			aszKeys.Add(&szBitLabel);
			szBitLabel.Kill();

			if (!bResult)
			{
				aszKeys.Kill();
				return NULL;
			}

			if (aiBitOut[i] == -1)
			{
				aiBitOut[i] = i;
			}
		}
		else
		{
			aszKeys.Add(&szBitLabel);
			szBitLabel.Kill();

			aiBitOut[i] = -1;
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
	if (!bResult)
	{
		return (CLogisimSplitter*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimText* CLogisimFileReader::CreateText(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimText*			pcComp;
	CMapStringString		cMap;
	BOOL					bResult;
	char*					szText;
	char*					szFont;
	ELogisimAlignment		eHorizontalAlignment;
	ELogisimAlignment		eVerticalAlignment;
	uint32					uiColour;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValue(&cMap, "text", &szText);
	bResult &= GetMapValueAsAlignment(&cMap, "halign", &eHorizontalAlignment, "center");
	bResult &= GetMapValueAsAlignment(&cMap, "valign", &eVerticalAlignment, "base");
	bResult &= GetMapValue(&cMap, "font", &szFont, "");
	bResult &= GetMapValueAsRGB(&cMap, "color", &uiColour, "#000000");
	
	ReturnNullOnFalse(bResult);

	if ((eHorizontalAlignment != LA_Left) && (eHorizontalAlignment != LA_Right) && (eHorizontalAlignment != LA_Centered))
	{
		gcLogger.Error2(__METHOD__, " Only left, right and centered allowed for horizontal alignment.");
		return NULL;
	}

	if ((eVerticalAlignment != LA_Top) && (eVerticalAlignment != LA_Bottom) && (eVerticalAlignment != LA_Centered) && (eVerticalAlignment != LA_Base))
	{
		gcLogger.Error2(__METHOD__, " Only top, bottom, base, and centered allowed for vertical alignment.");
		return NULL;
	}

	pcComp = mcComponents.CreateText();
	pcComp->Init(sLoc);
	pcComp->SetText(szText);
	pcComp->SetHorizontalAlignment(eHorizontalAlignment);
	pcComp->SetVerticalAlignment(eVerticalAlignment);

	bResult = CheckMap(pcCompTag, &cMap, "text", "halign", "valign", "font", "color", NULL);
	if (!bResult)
	{
		return (CLogisimText*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimDTypeFlipFlop* CLogisimFileReader::CreateDFlipFlop(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimDTypeFlipFlop*	pcComp;
	CMapStringString		cMap;
	BOOL					bResult;
	char*					szLabel;
	ELogisimTrigger			eTrigger;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);
	
	bResult = GetMapValueAsAppearance(pcCompTag, &cMap, "appearance");
	bResult &= GetMapValue(&cMap, "labelfont", NULL, "");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValueAsTrigger(&cMap, "trigger", &eTrigger, "rising");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateDTypeFlipFlop();
	pcComp->Init(sLoc);
	pcComp->SetLabel(szLabel);

	bResult = CheckMap(pcCompTag, &cMap, "appearance", "labelfont", "label", "trigger", NULL);
	if (!bResult)
	{
		return (CLogisimDTypeFlipFlop*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimRandom* CLogisimFileReader::CreateRandom(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimRandom*			pcComp;
	CMapStringString		cMap;
	BOOL					bResult;
	char*					szLabel;
	int						iSeed;
	ELogisimTrigger			eTrigger;
	int						iWidth;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsAppearance(pcCompTag, &cMap, "appearance");
	bResult &= GetMapValue(&cMap, "labelfont", NULL, "");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "seed", &iSeed, "0");
	bResult &= GetMapValueAsTrigger(&cMap, "trigger", &eTrigger, "rising");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "width", &iWidth, "8");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateRandom();
	pcComp->Init(sLoc);
	pcComp->SetLabel(szLabel);
	pcComp->SetTrigger(eTrigger);
	pcComp->SetWidth(iWidth);
	pcComp->SetSeed(iSeed);

	bResult = CheckMap(pcCompTag, &cMap, "appearance", "labelfont", "label", "seed", "trigger", "width", NULL);
	if (!bResult)
	{
		return (CLogisimRandom*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimComparator* CLogisimFileReader::CreateComparator(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimComparator*		pcComp;
	CMapStringString		cMap;
	BOOL					bResult;
	ELogisimNumericType		eNumericType;
	int						iWidth;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsNumericType(&cMap, "mode", &eNumericType, "signed");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "width", &iWidth, "8");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateComparator();
	pcComp->Init(sLoc);
	pcComp->SetNumericType(eNumericType);
	pcComp->SetWidth(iWidth);

	bResult = CheckMap(pcCompTag, &cMap, "mode", "width", NULL);
	if (!bResult)
	{
		return (CLogisimComparator*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimShiftRegsiter* CLogisimFileReader::CreateShiftRegister(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimShiftRegsiter*	pcComp;
	CMapStringString		cMap;
	BOOL					bResult;
	int						iNumberOfStages;
	int						iDataBits;
	char*					szLabel;
	char*					szParallelLoad;
	ELogisimTrigger			eTrigger;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsAppearance(pcCompTag, &cMap, "appearance");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "length", &iNumberOfStages, "8");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "width", &iDataBits, "1");
	bResult &= GetMapValue(&cMap, "labelfont", NULL, "");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");
	bResult &= GetMapValue(&cMap, "parallel", &szParallelLoad, "true");
	bResult &= GetMapValueAsTrigger(&cMap, "trigger", &eTrigger, "rising");
	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateShiftRegsiter();
	pcComp->Init(sLoc);
	pcComp->SetDataBits(iDataBits);
	pcComp->SetNumberOfStages(iNumberOfStages);
	pcComp->SetLabel(szLabel);
	pcComp->SetTrigger(eTrigger);
	pcComp->SetParallelLoad(IsString(szParallelLoad, "true"));

	bResult = CheckMap(pcCompTag, &cMap, "appearance", "length", "width", "labelfont", "label", "parallel", "trigger", NULL);
	if (!bResult)
	{
		return (CLogisimShiftRegsiter*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimBuffer* CLogisimFileReader::CreateBuffer(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimBuffer*		pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	ELogisimFacing		eFacing;
	char*				szLabel;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsFacing(&cMap, "facing", &eFacing, "east");
	bResult &= GetMapValue(&cMap, "label", &szLabel, "");

	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateBuffer();
	pcComp->Init(sLoc);
	pcComp->SetFacing(eFacing);
	pcComp->SetLabel(szLabel);

	bResult = CheckMap(pcCompTag, &cMap, "facing", "label", NULL);
	if (!bResult)
	{
		return (CLogisimBuffer*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimBitAdder* CLogisimFileReader::CreateBitAdder(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimBitAdder*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iInputs;
	int					iWidth;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsInt(pcCompTag, &cMap, "inputs", &iInputs, "1");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "width", &iWidth, "8");

	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateBitAdder();
	pcComp->Init(sLoc);
	pcComp->SetWidth(iWidth);
	pcComp->SetInputs(iInputs);

	bResult = CheckMap(pcCompTag, &cMap, "inputs", "width", NULL);
	if (!bResult)
	{
		return (CLogisimBitAdder*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimRegister* CLogisimFileReader::CreateRegister(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimRegister*	pcComp;
	CMapStringString	cMap;
	BOOL				bResult;
	int					iWidth;
	ELogisimTrigger		eTrigger;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsTrigger(&cMap, "trigger", &eTrigger, "rising");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "width", &iWidth, "8");
	bResult &= GetMapValueAsAppearance(pcCompTag, &cMap, "appearance");

	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateRegister();
	pcComp->Init(sLoc);
	pcComp->SetWidth(iWidth);
	pcComp->SetTrigger(eTrigger);

	bResult = CheckMap(pcCompTag, &cMap, "trigger", "width", "appearance", NULL);
	if (!bResult)
	{
		return (CLogisimRegister*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimMultiplexer* CLogisimFileReader::CreateMultiplexer(CMarkupTag* pcCompTag, SInt2 sLoc)
{
	CLogisimMultiplexer*	pcComp;
	CMapStringString		cMap;
	BOOL					bResult;
	int						iSelectBits;
	int						iDataBits;
	ELogisimSelectLocation	eSelectLocation;
	int						iSize;
	char*					szDisabledZero;
	ELogisimFacing			eFacing;
	char*					szIncludeEnabled;

	bResult = ConvertATagsToMap(&cMap, pcCompTag);
	ReturnNullOnFalse(bResult);

	bResult = GetMapValueAsSelectLocation(&cMap, "selloc", &eSelectLocation, "bl");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "width", &iDataBits, "1");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "select", &iSelectBits, "1");
	bResult &= GetMapValueAsInt(pcCompTag, &cMap, "size", &iSize, "50");
	bResult &= GetMapValue(&cMap, "disabled", &szDisabledZero, "0");
	bResult &= GetMapValueAsFacing(&cMap, "facing", &eFacing, "west");
	bResult &= GetMapValue(&cMap, "enable", &szIncludeEnabled, "true");

	ReturnNullOnFalse(bResult);

	pcComp = mcComponents.CreateMultiplexer();
	pcComp->Init(sLoc);
	pcComp->SetDataBits(iDataBits);
	pcComp->SetDisabledZero(IsString(szDisabledZero, "0"));
	pcComp->SetIncludeEnabled(IsString(szIncludeEnabled, "true"));
	pcComp->SetSelectBits(iSelectBits);
	pcComp->SetSelectLocation(eSelectLocation);
	pcComp->SetFacing(eFacing);
	pcComp->SetSize(iSize);

	bResult = CheckMap(pcCompTag, &cMap, "selloc", "width", "select", "size", "disabled", "enable", "facing", NULL);
	if (!bResult)
	{
		return (CLogisimMultiplexer*)KillComponent(pcComp);
	}
	else
	{
		return pcComp;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimCustomComponent* CLogisimFileReader::CreateCustomComponent(CMarkupTag* pcCompTag, SInt2 sLoc, char* szName)
{
	CMarkupTag*					pcTag;
	STagIterator				sIter;
	CLogisimCustomComponent		cCustom;
	CLogisimCustomComponent*	pcCustom;

	pcTag = pcCompTag->GetTag(&sIter);
	if (pcTag)
	{
		gcLogger.Error2(__METHOD__, " Custom components should have no sub tag.", NULL);
		return NULL;
	}

	cCustom.Init(sLoc, szName);
	pcCustom = mcCustomComponentList.Add(&cCustom);

	return pcCustom;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimCircuitComponent* CLogisimFileReader::CreateCircuitComponent(CMarkupTag* pcCompTag, SInt2 sLoc, char* szName)
{
	CMarkupTag*					pcTag;
	STagIterator				sIter;
	CLogisimCircuitComponent	cCustom;
	CLogisimCircuitComponent*	pcCustom;

	pcTag = pcCompTag->GetTag(&sIter);
	if (pcTag)
	{
		gcLogger.Error2(__METHOD__, " Circuit components should have no sub tag.", NULL);
		return NULL;
	}

	cCustom.Init(sLoc, szName);
	pcCustom = mcCircuitComponentList.Add(&cCustom);

	return pcCustom;
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
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Could not initialise text parser.", NULL);
	}

	tResult = cParser.GetExactCharacter('(');
	if (NotTrue(tResult))
	{
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Expected '(' parsing Int2.", NULL);
	}
	tResult = cParser.GetInteger(&iX);
	if (NotTrue(tResult))
	{
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Expected ${INTEGER} parsing Int2.", NULL);
	}
	tResult = cParser.GetExactCharacter(',');
	if (NotTrue(tResult))
	{
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Expected ',' parsing Int2.", NULL);
	}
	tResult = cParser.GetInteger(&iY);
	if (NotTrue(tResult))
	{
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Expected ${INTEGER} parsing Int2.", NULL);
	}
	tResult = cParser.GetExactCharacter(')');
	if (NotTrue(tResult))
	{
		cParser.Kill();
		return gcLogger.Error2(__METHOD__, " Expected ')' parsing Int2.", NULL);
	}

	cParser.Kill();

	ps->Init(iX, iY);

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CLogisimComponent* CLogisimFileReader::KillComponent(CLogisimComponent* pcComp)
{
	pcComp->Kill();
	mcComponents.Remove(pcComp);
	return NULL;
}

