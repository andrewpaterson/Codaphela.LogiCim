#ifndef __LOGISIM_DECODER_H__
#define __LOGISIM_DECODER_H__
#include "LogisimComponent.h"


class CLogisimDecoder : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_DECODER_H__


