#include "LogisimFileReader.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int __cdecl main(void)
{
	CLogisimFileReader cReader;

	gcLogger.Init();
	gcLogger.SetBreakOnWarning(FALSE);

	cReader.Init();
	cReader.Kill();

	gcLogger.Kill();

	return 0;
}

