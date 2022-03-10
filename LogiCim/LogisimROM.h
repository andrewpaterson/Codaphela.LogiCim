#ifndef __LOGISIM_ROM_H__
#define __LOGISIM_ROM_H__
#include "LogisimComponent.h"


class CLogisimROM : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_ROM_H__

