#include "LogisimCircuit.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimCircuit::Init(char* szName)
{
	mszName.Init(szName);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimCircuit::Kill(void)
{
	mszName.Kill();
}

