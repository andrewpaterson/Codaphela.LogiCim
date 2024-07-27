#ifndef __LOGISIM_MULTIPLEXER_H__
#define __LOGISIM_MULTIPLEXER_H__
#include "LogisimSelectLocation.h"
#include "LogisimFacing.h"


class CLogisimMultiplexer : public CLogisimFacing
{
protected:
	int						miDataBits;
	bool					mbIncludeEnabled;
	int						miSelectBits;
	bool					mbDisabledZero;
	ELogisimSelectLocation	meSelectLocation;
	int						miSize;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetDataBits(int iDataBits);
	void SetDisabledZero(bool bDisabledZero);
	void SetSelectLocation(ELogisimSelectLocation eSelectLocation);
	void SetSelectBits(int iSelectBits);
	void SetIncludeEnabled(bool bIncludeEnabled);
	void SetSize(int iSize);
};


#endif // !__LOGISIM_MULTIPLEXER_H__

