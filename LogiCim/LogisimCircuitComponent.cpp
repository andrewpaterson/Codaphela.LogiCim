#include "LogisimCircuitComponent.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimCircuitComponent::Init(SInt2 sPosition, char* szCircuitName)
{
	CLogisimComponent::Init(sPosition);
	mszCircuitName.Init(szCircuitName);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimCircuitComponent::Kill(void)
{
	mszCircuitName.Kill();
	CLogisimComponent::Kill();
}

