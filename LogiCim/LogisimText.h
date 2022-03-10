#ifndef __LOGISIM_TEXT_H__
#define __LOGISIM_TEXT_H__
#include "LogisimComponent.h"


class CLogisimText : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_TEXT_H__


