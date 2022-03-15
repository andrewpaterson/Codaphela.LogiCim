#ifndef __LOGISIM_COUNTER_H__
#define __LOGISIM_COUNTER_H__
#include "LogisimComponent.h"


class CLogisimCounter : public CLogisimComponent
{
protected:
	int		miWidth;
	uint64	mulliMax;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetWidth(int iWidth);
	void SetMax(uint64 ulliMax);
};


#endif // !__LOGISIM_COUNTER_H__

