#ifndef __LOGISIM_DIGITAL_OSCILLOSCOPE_H__
#define __LOGISIM_DIGITAL_OSCILLOSCOPE_H__
#include "LogisimComponent.h"


class CLogisimDigitalOscilloscope : public CLogisimComponent
{
protected:
	int		miNumberOfStates;
	int		miNumberOfInputs;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetNumberOfStates(int iNumberOfStates);
	void SetNumberOfInputs(int iNumberOfInputs);
};


#endif // !__LOGISIM_DIGITAL_OSCILLOSCOPE_H__


