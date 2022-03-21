#ifndef __LOGISIM_REGISTER_H__
#define __LOGISIM_REGISTER_H__
#include "LogisimTrigger.h"
#include "LogisimComponent.h"


class CLogisimRegister : public CLogisimComponent
{
protected:
	int					miWidth;
	ELogisimTrigger		meTrigger;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetWidth(int iWidth);
	void SetTrigger(ELogisimTrigger eTrigger);
};


#endif // !__LOGISIM_REGISTER_H__

