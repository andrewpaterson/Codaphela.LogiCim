#ifndef __LOGISIM_ROM_H__
#define __LOGISIM_ROM_H__
#include "LogisimMemory.h"


class CLogisimROM : public CLogisimMemory
{
protected:
	uint32*	mpvMemory;
	uint32	muiSize;

public:
	void	Init(SInt2 sPosition);
	void	Kill(void);

	void	SetAddressWidth(int iAddressWidth);
	uint32*	GetMemory(void);
};


#endif // !__LOGISIM_ROM_H__

