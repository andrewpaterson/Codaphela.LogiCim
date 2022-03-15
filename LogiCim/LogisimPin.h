#ifndef __LOGISIM_PIN_H__
#define __LOGISIM_PIN_H__
#include "LogisimFacing.h"


class CLogisimPin : public CLogisimFacing
{
protected:
	int					miWidth;
	CChars				mszLabel;
	BOOL				mbOutput;
	BOOL				mbTristate;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetWidth(int iWidth);
	void SetLabel(char* szLabel);
	void SetOutput(BOOL bOutput);
	void SetTristate(BOOL bTristate);
};


#endif // !__LOGISIM_PIN_H__

