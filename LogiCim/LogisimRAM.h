#ifndef __LOGISIM_RAM_H__
#define __LOGISIM_RAM_H__
#include "LogisimComponent.h"


class CLogisimRAM : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_RAM_H__


