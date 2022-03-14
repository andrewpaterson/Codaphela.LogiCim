#ifndef __LOGISIM_CLOCK_H__
#define __LOGISIM_CLOCK_H__
#include "LogisimFacing.h"


class CLogisimClock : public CLogisimFacing
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_CLOCK_H__


