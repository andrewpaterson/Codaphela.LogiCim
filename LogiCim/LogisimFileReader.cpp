#include "LogisimFileReader.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimFileReader::Init(void)
{
	CXMLFile		cFile;
	CMarkupTag*		pcRoot;
	SInt3			x;
	BOOL			bResult;

	x.Init(0, 0, 0);

	cFile.Init();

	bResult = cFile.Read("658_Computer.bak.circ", "D:\\Work\\658-Computer\\Logisim");
	if (bResult)
	{ 
		pcRoot = cFile.mcMarkup.GetRootTag();
		pcRoot->GetName();
		pcRoot->GetTag()->GetName();
		

		STagIterator	sIter;
		CMarkupTag*		pcTag;
		CChars			sz;

		pcTag = pcRoot->GetTag(&sIter);
		while (pcTag)
		{
			sz.Init(pcTag->GetName());
			sz.AppendNewLine();
			sz.DumpKill();
			pcTag = pcRoot->GetNextTag(&sIter);
		}
		
		//pcRoot->Dump();
	}

	cFile.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimFileReader::Kill(void)
{

}


