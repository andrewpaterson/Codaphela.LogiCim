#ifndef __LOGISIM_NOT_GATE_H__
#define __LOGISIM_NOT_GATE_H__
#include "LogisimComponent.h"


class CLogisimNOTGate : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_NOT_GATE_H__

