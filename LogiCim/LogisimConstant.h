#ifndef __LOGISIM_CONSTANT_H__
#define __LOGISIM_CONSTANT_H__
#include "LogisimComponent.h"


class CLogisimConstant : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_CONSTANT_H__


