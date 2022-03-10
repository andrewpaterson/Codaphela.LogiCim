#ifndef __LOGISIM_CLOCK_H__
#define __LOGISIM_CLOCK_H__
#include "LogisimComponent.h"


class CLogisimClock : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_CLOCK_H__


