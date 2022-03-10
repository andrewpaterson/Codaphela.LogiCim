#ifndef __LOGISIM_LED_H__
#define __LOGISIM_LED_H__
#include "LogisimComponent.h"


class CLogisimLED : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_LED_H__


