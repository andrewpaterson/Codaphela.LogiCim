#include "LogisimFileReader.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int __cdecl main(void)
{
	CLogisimFileReader	cReader;
	CMarkupTag*			pcRoot;

	gcLogger.Init();
	gcLogger.SetBreakOnWarning(FALSE);

	cReader.Init("D:\\Work\\658-Computer\\Logisim", "658_Computer.bak.circ");
	pcRoot = cReader.Read();
	cReader.Convert(pcRoot);

	cReader.Kill();

	gcLogger.Kill();

	return 0;
}

