#include "LogisimDFlipFlop.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimDTypeFlipFlop::Init(SInt2 sPosition)
{
	CLogisimComponent::Init(sPosition);
	mszLabel.Init();
	meTrigger = LT_NotSet;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimDTypeFlipFlop::Kill(void)
{
	mszLabel.Kill();
	CLogisimComponent::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimDTypeFlipFlop::SetLabel(char* szLabel) { mszLabel.Set(szLabel); }
void CLogisimDTypeFlipFlop::SetTrigger(ELogisimTrigger eTrigger) { meTrigger = eTrigger; }
