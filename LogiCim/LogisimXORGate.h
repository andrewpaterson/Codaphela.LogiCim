#ifndef __LOGISIM_XOR_GATE_H__
#define __LOGISIM_XOR_GATE_H__
#include "LogisimGate.h"


class CLogisimXORGate : public CLogisimGate
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_XOR_GATE_H__

