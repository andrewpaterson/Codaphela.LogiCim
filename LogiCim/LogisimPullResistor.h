#ifndef __LOGISIM_PULL_RESISTOR_H__
#define __LOGISIM_PULL_RESISTOR_H__
#include "LogisimFacing.h"


enum ELogisimPullResistorPull
{
	LPRP_Zero,
	LPRP_One,
	LPRP_Error,
	LPRP_Notset
};


class CLogisimPullResistor : public CLogisimFacing
{
protected:
	ELogisimPullResistorPull	mePull;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetPull(ELogisimPullResistorPull ePull);
};


#endif // !__LOGISIM_PULL_RESISTOR_H__

