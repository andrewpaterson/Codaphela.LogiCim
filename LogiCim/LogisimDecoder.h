#ifndef __LOGISIM_DECODER_H__
#define __LOGISIM_DECODER_H__
#include "LogisimSelectLocation.h"
#include "LogisimComponent.h"


class CLogisimDecoder : public CLogisimComponent
{
protected:
	bool					mbIncludeEnabled;
	int						miSelectBits;
	bool					mbDisabledZero;
	ELogisimSelectLocation	meSelectLocation;
	bool					mbTristate;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetIncludeEnabled(bool bIncludeEnabled);
	void SetSelectBits(int iSelectBits);
	void SetDisabledZero(bool bDisabledZero);
	void SetSelectLocation(ELogisimSelectLocation eSelectLocation);
	void SetTristate(bool bTristate);
};


#endif // !__LOGISIM_DECODER_H__

