#include "LogisimRAM.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimRAM::Init(SInt2 sPosition)
{
	CLogisimComponent::Init(sPosition);
	miAddressWidth = 0;
	mbVolatile = TRUE;
	meTrigger = LT_NotSet;
	mbLineEnables = FALSE;
	meDataBus = LRDB_NotSet;
	mszLabel.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimRAM::Kill(void)
{
	mszLabel.Kill();
	CLogisimComponent::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimRAM::SetVolatile(BOOL bVolatile) { mbVolatile = bVolatile; }
void CLogisimRAM::SetTrigger(ELogisimTrigger eTrigger) { meTrigger = eTrigger; }
void CLogisimRAM::SetLineEnables(BOOL bLineEnables) { mbLineEnables = bLineEnables; }
void CLogisimRAM::SetDataBus(ELogisimRAMDataBus	eDataBus) { meDataBus = eDataBus; }

