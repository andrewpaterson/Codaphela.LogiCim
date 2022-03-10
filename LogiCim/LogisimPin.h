#ifndef __LOGISIM_PIN_H__
#define __LOGISIM_PIN_H__
#include "LogisimComponent.h"


class CLogisimPin : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_PIN_H__


