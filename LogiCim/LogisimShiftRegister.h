#ifndef __LOGISIM_SHIFT_REGISTER_H__
#define __LOGISIM_SHIFT_REGISTER_H__
#include "LogisimNumericType.h"
#include "LogisimTrigger.h"
#include "LogisimComponent.h"


class CLogisimShiftRegsiter : public CLogisimComponent
{
protected:
	int					miNumberOfStages;
	int					miDataBits;
	CChars				mszLabel;
	bool				mbParallelLoad;
	ELogisimTrigger		meTrigger;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetNumberOfStages(int iNumberOfStages);
	void SetDataBits(int iDataBits);
	void SetLabel(char* szLabel);
	void SetParallelLoad(bool bParallelLoad);
	void SetTrigger(ELogisimTrigger eTrigger);
};


#endif // !__LOGISIM_SHIFT_REGISTER_H__

