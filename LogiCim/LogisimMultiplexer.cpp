#include "LogisimMultiplexer.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimMultiplexer::Init(SInt2 sPosition)
{
	CLogisimFacing::Init(sPosition);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimMultiplexer::Kill(void)
{
	CLogisimFacing::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimMultiplexer::SetDataBits(int iDataBits) { miDataBits = iDataBits; }
void CLogisimMultiplexer::SetDisabledZero(BOOL bDisabledZero) { mbDisabledZero = bDisabledZero; }
void CLogisimMultiplexer::SetSelectLocation(ELogisimSelectLocation eSelectLocation) { meSelectLocation = eSelectLocation; }
void CLogisimMultiplexer::SetSelectBits(int iSelectBits) { miSelectBits = iSelectBits; }
void CLogisimMultiplexer::SetIncludeEnabled(BOOL bIncludeEnabled) { mbIncludeEnabled = bIncludeEnabled; }
void CLogisimMultiplexer::SetSize(int iSize) { miSize = iSize; }

