#ifndef __LOGISIM_PULL_RESISTOR_H__
#define __LOGISIM_PULL_RESISTOR_H__
#include "LogisimComponent.h"


class CLogisimPullResistor : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_PULL_RESISTOR_H__


