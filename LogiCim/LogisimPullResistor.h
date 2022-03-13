#ifndef __LOGISIM_PULL_RESISTOR_H__
#define __LOGISIM_PULL_RESISTOR_H__
#include "LogisimComponent.h"


enum ELogisimPullResistorPull
{
	LPRP_Zero,
	LPRP_One,
	LPRP_Error,
	LPRP_Notset
};


class CLogisimPullResistor : public CLogisimComponent
{
protected:
	ELogisimFacing				meFacing;
	ELogisimPullResistorPull	mePull;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetFacing(ELogisimFacing eFacing);
	void SetPull(ELogisimPullResistorPull ePull);
};


#endif // !__LOGISIM_PULL_RESISTOR_H__

