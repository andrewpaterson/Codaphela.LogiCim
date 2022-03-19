#ifndef __LOGISIM_SPLITTER_H__
#define __LOGISIM_SPLITTER_H__
#include "LogisimFacing.h"


enum ELogisimSplitterAppearance
{
	LSA_Left,
	LSA_Right,
	LSA_Centered,
	LSA_Legacy,
	LSA_NotSet,
};


class CLogisimSplitter : public CLogisimFacing
{
protected:
	int							miSpacing;
	int							miFanOut;
	int							miIncoming;
	ELogisimSplitterAppearance	meAppear;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetSpacing(int iSpacing);
	void SetFanOut(int miFanOut);
	void SetIncoming(int iIncoming);
	void SetAppear(ELogisimSplitterAppearance eAppear);
};


#endif // !__LOGISIM_SPLITTER_H__

