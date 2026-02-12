#ifndef __LOGISIM_PIN_H__
#define __LOGISIM_PIN_H__
#include "BaseLib/Chars.h"
#include "LogisimFacing.h"


class CLogisimPin : public CLogisimFacing
{
protected:
	int		miWidth;
	CChars	mszLabel;
	bool	mbOutput;
	bool	mbTristate;
	int		miRadix;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetWidth(int iWidth);
	void SetLabel(char* szLabel);
	void SetOutput(bool bOutput);
	void SetTristate(bool bTristate);
	void SetRadix(int iRadix);
};


#endif // !__LOGISIM_PIN_H__

