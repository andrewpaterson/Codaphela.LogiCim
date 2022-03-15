#include "LogisimControlledBuffer.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimControlledBuffer::Init(SInt2 sPosition)
{
	CLogisimFacing::Init(sPosition);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimControlledBuffer::Kill(void)
{
	CLogisimFacing::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimControlledBuffer::SetWidth(int iWidth) { miWidth = iWidth; }
void CLogisimControlledBuffer::SetLabel(char* szLabel) { mszLabel.Set(szLabel); }
void CLogisimControlledBuffer::SetControl(ELogisimControlledBufferControl eControl) { meControl = eControl; }

