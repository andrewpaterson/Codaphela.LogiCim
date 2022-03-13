#include "LogisimGate.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimGate::Init(SInt2 sPosition)
{
	CLogisimComponent::Init(sPosition);
	meFacing = LF_NotSet;
	miInputs = 0;
	meOut = LGO_NotSet;
	miSize = 0;
	miWidth = 0;
	mszLabel.Init();
	mabNegate.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimGate::Kill(void)
{
	CLogisimComponent::Kill();
	mabNegate.Kill();
	mszLabel.Kill();
}

//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimGate::SetWidth(int iWidth) 
{ 
	miWidth = iWidth; 
	
	mabNegate.ReInit();
	mabNegate.GrowBy(iWidth);
	mabNegate.Zero();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimGate::SetFacing(ELogisimFacing eFacing) { meFacing = eFacing; }
void CLogisimGate::SetInputs(int iInputs) { miInputs = iInputs; }
void CLogisimGate::SetOut(ELogisimGateOut eOut) { meOut = eOut; }
void CLogisimGate::SetSize(int iSize) { miSize = iSize; }
void CLogisimGate::SetLabel(char* szLabel) { mszLabel.Set(szLabel); }
void CLogisimGate::SetNegtate(int iInput, BOOL bNegate) { mabNegate.Set(iInput, bNegate); }

