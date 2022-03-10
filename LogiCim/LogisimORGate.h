#ifndef __LOGISIM_OR_GATE_H__
#define __LOGISIM_OR_GATE_H__
#include "LogisimComponent.h"


class CLogisimORGate : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_OR_GATE_H__


