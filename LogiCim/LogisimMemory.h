#ifndef __LOGISIM_MEMORY_H__
#define __LOGISIM_MEMORY_H__
#include "LogisimComponent.h"


class CLogisimMemory : public CLogisimComponent
{
protected:
	int		miAddressWidth;
	int		miDataWidth;
	CChars	mszLabel;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetAddressWidth(int iAddressWidth);
	void SetDataWidth(int iDataWidth);
	void SetLabel(char* szLabel);
};


#endif //!__LOGISIM_MEMORY_H__

