#include "LogisimCounter.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimCounter::Init(SInt2 sPosition)
{
	CLogisimComponent::Init(sPosition);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimCounter::Kill(void)
{
	CLogisimComponent::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimCounter::SetWidth(int iWidth) { miWidth = iWidth; }
void CLogisimCounter::SetMax(uint64 ulliMax) { mulliMax = ulliMax; }

