#include "LogisimMemory.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimMemory::Init(SInt2 sPosition)
{
	CLogisimComponent::Init(sPosition);
	miAddressWidth = 0;
	mszLabel.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimMemory::Kill(void)
{
	mszLabel.Kill();
	CLogisimComponent::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimMemory::SetAddressWidth(int iAddressWidth) { miAddressWidth = iAddressWidth; }
void CLogisimMemory::SetLabel(char* szLabel) { mszLabel.Set(szLabel); }

