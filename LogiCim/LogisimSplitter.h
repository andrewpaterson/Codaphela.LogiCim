#ifndef __LOGISIM_SPLITTER_H__
#define __LOGISIM_SPLITTER_H__
#include "LogisimComponent.h"


class CLogisimSplitter : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_SPLITTER_H__


