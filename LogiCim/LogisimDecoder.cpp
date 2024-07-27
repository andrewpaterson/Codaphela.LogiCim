#include "LogisimDecoder.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimDecoder::Init(SInt2 sPosition)
{
	CLogisimComponent::Init(sPosition);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimDecoder::Kill(void)
{
	CLogisimComponent::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimDecoder::SetIncludeEnabled(bool bIncludeEnabled) { mbIncludeEnabled = bIncludeEnabled; }
void CLogisimDecoder::SetSelectBits(int iSelectBits) { miSelectBits = iSelectBits; }
void CLogisimDecoder::SetDisabledZero(bool bDisabledZero) { mbDisabledZero = bDisabledZero; }
void CLogisimDecoder::SetSelectLocation(ELogisimSelectLocation eSelectLocation) { meSelectLocation = eSelectLocation; }
void CLogisimDecoder::SetTristate(bool bTristate) { mbTristate = bTristate; }

