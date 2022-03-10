#ifndef __LOGISIM_COMPONENT_H__
#define __LOGISIM_COMPONENT_H__
#include "BaseLib/Int2.h"
#include "LogisimFacing.h"


class CLogisimComponent
{
protected:
	SInt2	msPosition;
public:
			void	Init(SInt2 sPosition);
	virtual void	Kill(void);
};


#endif // !__LOGISIM_COMPONENT_H__


