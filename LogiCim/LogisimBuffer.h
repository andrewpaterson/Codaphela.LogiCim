#ifndef __LOGISIM_BUFFER_H__
#define __LOGISIM_BUFFER_H__
#include "LogisimFacing.h"


class CLogisimBuffer : public CLogisimFacing
{
protected:
	CChars		mszLabel;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetLabel(char* szLabel);
};


#endif // !__LOGISIM_BUFFER_H__


