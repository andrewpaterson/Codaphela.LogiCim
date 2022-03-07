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
	//<comp lib="0" loc="(2200,3500)" name="Tunnel">
	//  <a name="label" val="CH_3_D"/>
	//  <a name="labelfont" val="SansSerif bold 10"/>
	//  <a name="width" val="8"/>
	//</comp>

	return TRUE;
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

	cParser.Init(sz);

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

