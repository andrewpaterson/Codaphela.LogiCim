#ifndef __LOGISIM_D_TYPE_FLIP_FLOP_H__
#define __LOGISIM_D_TYPE_FLIP_FLOP_H__
#include"LogisimTrigger.h"
#include "LogisimComponent.h"


class CLogisimDTypeFlipFlop : public CLogisimComponent
{
protected:
	CChars				mszLabel;
	ELogisimTrigger		meTrigger;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetLabel(char* szLabel);
	void SetTrigger(ELogisimTrigger eTrigger);
};


#endif // !__LOGISIM_D_TYPE_FLIP_FLOP_H__

