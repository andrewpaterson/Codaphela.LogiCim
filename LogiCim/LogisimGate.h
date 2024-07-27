#ifndef __LOGISIM_GATE_H__
#define __LOGISIM_GATE_H__
#include "BaseLib/ArrayBit.h"
#include "LogisimFacing.h"


enum ELogisimGateOut
{
	LGO_ZeroOne,
	LGO_ZeroFloating,
	LGO_FloatingOne,
	LGO_NotSet
};


class CLogisimGate : public CLogisimFacing
{
protected:
	int					miInputs;
	ELogisimGateOut		meOut;
	int					miSize;
	int					miWidth;
	CChars				mszLabel;
	CArrayBit			mabNegate;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetInputs(int iInputs);
	void SetOut(ELogisimGateOut eOut);
	void SetSize(int iSize);
	void SetWidth(int iWidth);
	void SetLabel(char* szLabel);
	void SetNegtate(int iInput, bool bNegate);
};


#endif // !__LOGISIM_GATE_H__

