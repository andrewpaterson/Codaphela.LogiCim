#ifndef __LOGISIM_COMPONENT_H__
#define __LOGISIM_COMPONENT_H__
#include "BaseLib/Chars.h"
#include "BaseLib/Int32Vec2.h"


class CLogisimComponent
{
protected:
	SInt32Vec2	msPosition;

public:
			void	Init(SInt32Vec2 sPosition);
	virtual void	Kill(void);
};


#endif // __LOGISIM_COMPONENT_H__


