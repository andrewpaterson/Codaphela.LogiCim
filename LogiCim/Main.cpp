#include "LogisimFileReader.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int __cdecl main(void)
{
	CLogisimFileReader	cReader;
	CMarkupTag*			pcRoot;
	BOOL				bFileExists;

	gcLogger.Init();
	gcLogger.SetBreakOnWarning(FALSE);

	bFileExists = cReader.Init("D:\\Work\\658 - Computer\\logi65816", "Test.circ");
	//bFileExists = cReader.Init("D:\\Work\\658-Computer\\Logisim", "658_Computer.bak.circ");
	if (bFileExists)
	{
		pcRoot = cReader.Read();
		cReader.Convert(pcRoot);
	}
	else
	{
		gcLogger.Error2(__METHOD__, " File [", cReader.GetFileName(), "] does not exist.", NULL);
	}
	cReader.Kill();

	gcLogger.Kill();

	return 0;
}

