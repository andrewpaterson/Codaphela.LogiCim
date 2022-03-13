#include "LogisimSplitter.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimSplitter::Init(SInt2 sPosition)
{
	CLogisimComponent::Init(sPosition);
	miSpacing = 0;
	miFanOut = 0;
	miIncoming = 0;
	meFacing = LF_NotSet;
	meAppear = LSA_NotSet;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimSplitter::Kill(void)
{
	CLogisimComponent::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimSplitter::SetSpacing(int iSpacing) { miSpacing = iSpacing; }
void CLogisimSplitter::SetFanOut(int miFanOut) { miFanOut = miFanOut; }
void CLogisimSplitter::SetIncoming(int iIncoming) { miIncoming = iIncoming; }
void CLogisimSplitter::SetFacing(ELogisimFacing eFacing) { meFacing = eFacing; }
void CLogisimSplitter::SetAppear(ELogisimSplitterAppearance eAppear) { meAppear = eAppear; }

