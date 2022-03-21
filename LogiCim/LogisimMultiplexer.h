#ifndef __LOGISIM_MULTIPLEXER_H__
#define __LOGISIM_MULTIPLEXER_H__
#include "LogisimSelectLocation.h"
#include "LogisimFacing.h"


class CLogisimMultiplexer : public CLogisimFacing
{
protected:
	int						miDataBits;
	BOOL					mbIncludeEnabled;
	int						miSelectBits;
	BOOL					mbDisabledZero;
	ELogisimSelectLocation	meSelectLocation;
	int						miSize;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetDataBits(int iDataBits);
	void SetDisabledZero(BOOL bDisabledZero);
	void SetSelectLocation(ELogisimSelectLocation eSelectLocation);
	void SetSelectBits(int iSelectBits);
	void SetIncludeEnabled(BOOL bIncludeEnabled);
	void SetSize(int iSize);
};


#endif // !__LOGISIM_MULTIPLEXER_H__

