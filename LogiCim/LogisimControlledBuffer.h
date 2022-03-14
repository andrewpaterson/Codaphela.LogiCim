#ifndef __LOGISIM_CONTROLLED_BUFFER_H__
#define __LOGISIM_CONTROLLED_BUFFER_H__
#include "LogisimFacing.h"


class CLogisimControlledBuffer : public CLogisimFacing
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_CONTROLLED_BUFFER_H__


