#ifndef __LOGISIM_AND_GATE_H__
#define __LOGISIM_AND_GATE_H__
#include "LogisimComponent.h"


class CLogisimANDGate : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_AND_GATE_H__

