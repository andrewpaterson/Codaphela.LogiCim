#ifndef __LOGISIM_RANDOM_H__
#define __LOGISIM_RANDOM_H__
#include "LogisimTrigger.h"
#include "LogisimComponent.h"


class CLogisimRandom : public CLogisimComponent
{
protected:
	CChars				mszLabel;
	int					miSeed;
	ELogisimTrigger		meTrigger;
	int					miWidth;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetLabel(char* szLabel);
	void SetSeed(int iSeed);
	void SetTrigger(ELogisimTrigger eTrigger);
	void SetWidth(int iWidth);
};


#endif // !__LOGISIM_RANDOM_H__

