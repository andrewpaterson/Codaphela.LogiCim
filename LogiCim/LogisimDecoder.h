#ifndef __LOGISIM_DECODER_H__
#define __LOGISIM_DECODER_H__
#include "LogisimSelectLocation.h"
#include "LogisimComponent.h"


class CLogisimDecoder : public CLogisimComponent
{
protected:
	BOOL					mbIncludeEnabled;
	int						miSelectBits;
	BOOL					mbDisabledZero;
	ELogisimSelectLocation	meSelectLocation;
	BOOL					mbTristate;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetIncludeEnabled(BOOL bIncludeEnabled);
	void SetSelectBits(int iSelectBits);
	void SetDisabledZero(BOOL bDisabledZero);
	void SetSelectLocation(ELogisimSelectLocation eSelectLocation);
	void SetTristate(BOOL bTristate);
};


#endif // !__LOGISIM_DECODER_H__

