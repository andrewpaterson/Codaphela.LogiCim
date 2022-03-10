#ifndef __LOGISIM_SPLITTER_H__
#define __LOGISIM_SPLITTER_H__
#include "LogisimComponent.h"


enum CLogisimSplitterAppearance
{
	LSA_Left,
	LSA_Right,
	LSA_Centered,
	LSA_Legacy,
	LSA_NotSet,
};


class CLogisimSplitter : public CLogisimComponent
{
protected:
	int							miSpacing;
	int							miFanOut;
	int							miIncoming;
	ELogisimFacing				meFacing;
	CLogisimSplitterAppearance	meAppear;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetSpacing(int iSpacing);
	void SetFanOut(int miFanOut);
	void SetIncoming(int iIncoming);
	void SetFacing(ELogisimFacing eFacing);
	void SetAppear(CLogisimSplitterAppearance eAppear);
};


#endif // !__LOGISIM_SPLITTER_H__


