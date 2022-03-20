#ifndef __LOGISIM_BIT_ADDER_H__
#define __LOGISIM_BIT_ADDER_H__
#include "LogisimComponent.h"


class CLogisimBitAdder : public CLogisimComponent
{
protected:
	int		miInputs;
	int		miWidth;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetInputs(int iInputs);
	void SetWidth(int iWidth);
};


#endif // !__LOGISIM_BIT_ADDER_H__

