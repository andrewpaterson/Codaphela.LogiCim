#include "LogisimComparator.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimComparator::Init(SInt2 sPosition)
{
	CLogisimComponent::Init(sPosition);
	miWidth = 0;
	meNumericType = LNT_NotSet;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimComparator::Kill(void)
{
	CLogisimComponent::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimComparator::SetWidth(int iWidth) { miWidth = iWidth; }
void CLogisimComparator::SetNumericType(ELogisimNumericType eNumericType) { meNumericType = eNumericType; }
