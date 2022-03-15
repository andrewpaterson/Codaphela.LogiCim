#ifndef __LOGISIM_CONTROLLED_BUFFER_H__
#define __LOGISIM_CONTROLLED_BUFFER_H__
#include "LogisimFacing.h"


enum ELogisimControlledBufferControl
{
	LCBC_Left,
	LCBC_Right,
	LCBC_NotSet
};


class CLogisimControlledBuffer : public CLogisimFacing
{
protected:
	int									miWidth;
	CChars								mszLabel;
	ELogisimControlledBufferControl		meControl;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetWidth(int iWidth);
	void SetLabel(char* szLabel);
	void SetControl(ELogisimControlledBufferControl eControl);
};


#endif // !__LOGISIM_CONTROLLED_BUFFER_H__


