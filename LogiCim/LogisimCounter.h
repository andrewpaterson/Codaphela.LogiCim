#ifndef __LOGISIM_COUNTER_H__
#define __LOGISIM_COUNTER_H__
#include "LogisimComponent.h"


class CLogisimCounter : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_COUNTER_H__


