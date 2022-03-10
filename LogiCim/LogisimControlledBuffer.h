#ifndef __LOGISIM_CONTROLLED_BUFFER_H__
#define __LOGISIM_CONTROLLED_BUFFER_H__
#include "LogisimComponent.h"


class CLogisimControlledBuffer : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_CONTROLLED_BUFFER_H__


