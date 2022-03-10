#ifndef __LOGISIM_XOR_GATE_H__
#define __LOGISIM_XOR_GATE_H__
#include "LogisimComponent.h"


class CLogisimXORGate : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_XOR_GATE_H__

