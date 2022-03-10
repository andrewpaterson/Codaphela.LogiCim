#ifndef __LOGISIM_DIGITAL_OSCILLOSCOPE_H__
#define __LOGISIM_DIGITAL_OSCILLOSCOPE_H__
#include "LogisimComponent.h"


class CLogisimDigitalOscilloscope : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_DIGITAL_OSCILLOSCOPE_H__


