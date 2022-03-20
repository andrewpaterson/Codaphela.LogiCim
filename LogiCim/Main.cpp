#include "LogisimFileReader.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int __cdecl main(void)
{
	CLogisimFileReader					cReader;
	CMarkupTag*							pcRoot;
	BOOL								bFileExists;
	BOOL								bResult;

	gcLogger.Init();
	gcLogger.SetBreakOnWarning(FALSE);

	bFileExists = cReader.Init("D:\\Work\\658-Computer\\Logisim", "658_Computer.circ");
	if (bFileExists)
	{
		pcRoot = cReader.Read();
		bResult = cReader.Convert(pcRoot);
		if (bResult)
		{
			CListTemplate<CLogisimCircuit>*		plCircuits;
			int									i;
			int									iNumElements;
			CLogisimCircuit*					pcCircuit;
			CChars								sz;

			plCircuits = cReader.GetCircuits();
			iNumElements = plCircuits->NumElements();
			for (i = 0; i < iNumElements; i++)
			{
				pcCircuit = plCircuits->Get(i);

				sz.Init();
				sz.Append(pcCircuit->GetName());
				sz.AppendNewLine();
				sz.DumpKill();
			}
		}
		else
		{
			gcLogger.Error2(__METHOD__, " Failed to read [", cReader.GetFileName(), "].", NULL);
		}
	}
	else
	{
		gcLogger.Error2(__METHOD__, " File [", cReader.GetFileName(), "] does not exist.", NULL);
	}
	cReader.Kill();

	gcLogger.Kill();

	return 0;
}

