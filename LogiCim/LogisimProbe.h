#ifndef __LOGISIM_PROBE_H__
#define __LOGISIM_PROBE_H__
#include "LogisimFacing.h"


class CLogisimProbe : public CLogisimFacing
{
protected:
	int		miRadix;
	bool	mbClassicAppearance;
	CChars	mszLabel;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetRadix(int iRadix);
	void SetClassicApearance(bool bClassicAppearance);
	void SetLabel(char* szlabel);
};


#endif // !__LOGISIM_PROBE_H__

