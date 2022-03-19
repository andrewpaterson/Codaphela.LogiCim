#ifndef __LOGISIM_LED_H__
#define __LOGISIM_LED_H__
#include "LogisimFacing.h"


class CLogisimLED : public CLogisimFacing
{
protected:
	uint32				muiColour;
	uint32				muiOffColour;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetColour(uint32 uiColour);
	void SetOffColour(uint32 uiOffColour);
};


#endif // !__LOGISIM_LED_H__

