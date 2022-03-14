#include "LogisimFacing.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimFacing::Init(SInt2 sPosition)
{
	CLogisimComponent::Init(sPosition);
	meFacing = LF_NotSet;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimFacing::Kill(void)
{
	CLogisimComponent::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimFacing::SetFacing(ELogisimFacing eFacing) 
{ 
	meFacing = eFacing; 
}
