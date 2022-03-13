#ifndef __LOGISIM_NOR_GATE_H__
#define __LOGISIM_NOR_GATE_H__
#include "LogisimGate.h"


class CLogisimNORGate : public CLogisimGate
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_NOR_GATE_H__


