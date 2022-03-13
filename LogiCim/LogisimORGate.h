#ifndef __LOGISIM_OR_GATE_H__
#define __LOGISIM_OR_GATE_H__
#include "LogisimGate.h"


class CLogisimORGate : public CLogisimGate
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_OR_GATE_H__


