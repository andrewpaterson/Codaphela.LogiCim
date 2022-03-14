#ifndef __LOGISIM_NOT_GATE_H__
#define __LOGISIM_NOT_GATE_H__
#include "LogisimFacing.h"


class CLogisimNOTGate : public CLogisimFacing
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_NOT_GATE_H__

