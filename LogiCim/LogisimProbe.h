#ifndef __LOGISIM_PROBE_H__
#define __LOGISIM_PROBE_H__
#include "LogisimFacing.h"


class CLogisimProbe : public CLogisimFacing
{
protected:
	int					miRadix;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetRadix(int iRadix);
};


#endif // !__LOGISIM_PROBE_H__


