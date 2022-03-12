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
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimRAM::Kill(void)
{
	CLogisimComponent::Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogisimRAM::SetAddressWidth(int iAddressWidth) { miAddressWidth = iAddressWidth; }
void CLogisimRAM::SetVolatile(BOOL bVolatile) { mbVolatile = bVolatile; }
void CLogisimRAM::SetTrigger(ELogisimTrigger eTrigger) { meTrigger = eTrigger; }
void CLogisimRAM::SetLineEnables(BOOL bLineEnables) { mbLineEnables = bLineEnables; }
void CLogisimRAM::SetDataBus(ELogisimRAMDataBus	eDataBus) { meDataBus = eDataBus; }

