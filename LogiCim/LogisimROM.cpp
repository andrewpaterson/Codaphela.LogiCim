#include "LogisimROM.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimROM::Init(SInt2 sPosition)
{
	CLogisimMemory::Init(sPosition);
	mpvMemory = NULL;
	muiSize = 0;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimROM::Kill(void)
{
	CLogisimMemory::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimROM::SetAddressWidth(int iAddressWidth)
{
	CLogisimMemory::SetAddressWidth(iAddressWidth);

	SafeFree(mpvMemory);

	muiSize = 1 << iAddressWidth;
	mpvMemory = (uint8*)malloc(muiSize);
	memset(mpvMemory, 0, muiSize);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
uint8* CLogisimROM::GetMemory(void)
{
	return mpvMemory;
}

