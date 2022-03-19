#include "LogisimShiftRegister.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimShiftRegsiter::Init(SInt2 sPosition)
{
	CLogisimComponent::Init(sPosition);
	miNumberOfStages = 0;
	miDataBits = 0;
	mszLabel.Init();
	mbParallelLoad = FALSE;
	meTrigger = LT_NotSet;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimShiftRegsiter::Kill(void)
{
	mszLabel.Kill();
	CLogisimComponent::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimShiftRegsiter::SetNumberOfStages(int iNumberOfStages) { miNumberOfStages = iNumberOfStages; }
void CLogisimShiftRegsiter::SetDataBits(int iDataBits) { miDataBits = iDataBits; }
void CLogisimShiftRegsiter::SetLabel(char* szLabel) { mszLabel.Set(szLabel); }
void CLogisimShiftRegsiter::SetParallelLoad(BOOL bParallelLoad) { mbParallelLoad = bParallelLoad; }
void CLogisimShiftRegsiter::SetTrigger(ELogisimTrigger eTrigger) { meTrigger = eTrigger; }

