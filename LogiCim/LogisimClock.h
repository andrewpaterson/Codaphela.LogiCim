#ifndef __LOGISIM_CLOCK_H__
#define __LOGISIM_CLOCK_H__
#include "LogisimFacing.h"


class CLogisimClock : public CLogisimFacing
{
protected:
	int		miHighDuration;
	int		miLowDuration;
	int		miPhaseOffset;
	CChars	mszLabel;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetHighDuration(int iHighDuration);
	void SetLowDuration(int iLowDuration);
	void SetPhaseOffset(int iPhaseOffset);
	void SetLabel(char* szLabel);
};


#endif // !__LOGISIM_CLOCK_H__


