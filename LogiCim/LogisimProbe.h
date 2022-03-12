#ifndef __LOGISIM_PROBE_H__
#define __LOGISIM_PROBE_H__
#include "LogisimComponent.h"


class CLogisimProbe : public CLogisimComponent
{
protected:
	int					miRadix;
	ELogisimFacing		meFacing;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetRadix(int iRadix);
	void SetFacing(ELogisimFacing eFacing);
};


#endif // !__LOGISIM_PROBE_H__


