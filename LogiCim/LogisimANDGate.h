#ifndef __LOGISIM_AND_GATE_H__
#define __LOGISIM_AND_GATE_H__
#include "LogisimGate.h"


class CLogisimANDGate : public CLogisimGate
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_AND_GATE_H__

