#ifndef __LOGISIM_NOT_GATE_H__
#define __LOGISIM_NOT_GATE_H__
#include "LogisimFacing.h"


class CLogisimNOTGate : public CLogisimFacing
{
protected:
	int		miSize;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetSize(int iSize);
};


#endif // !__LOGISIM_NOT_GATE_H__

