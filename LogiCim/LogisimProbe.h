#ifndef __LOGISIM_PROBE_H__
#define __LOGISIM_PROBE_H__
#include "LogisimFacing.h"


class CLogisimProbe : public CLogisimFacing
{
protected:
	int		miRadix;
	BOOL	mbClassicAppearance;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetRadix(int iRadix);
	void SetClassicApearance(BOOL bClassicAppearance);
};


#endif // !__LOGISIM_PROBE_H__

