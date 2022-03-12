#ifndef __LOGISIM_PIN_H__
#define __LOGISIM_PIN_H__
#include "LogisimComponent.h"


class CLogisimPin : public CLogisimComponent
{
protected:
	ELogisimFacing		meFacing; 
	int					miWidth;
	BOOL				mbOutput;
	CChars				mszLabel;
	BOOL				mbTristate;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetFacing(ELogisimFacing eFacing);
	void SetWidth(int iWidth);
	void SetOutput(BOOL bOutput);
	void SetLabel(char* szLabel);
	void SetTristate(BOOL bTristate);
};


#endif // !__LOGISIM_PIN_H__

