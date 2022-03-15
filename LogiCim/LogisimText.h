#ifndef __LOGISIM_TEXT_H__
#define __LOGISIM_TEXT_H__
#include "LogisimAlignment.h"
#include "LogisimComponent.h"


class CLogisimText : public CLogisimComponent
{
protected:
	CChars				mszText;
	ELogisimAlignment	meHorizontalAlignment;
	ELogisimAlignment	meVerticalAlignment;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetText(char* szText);
	void SetHorizontalAlignment(ELogisimAlignment eHorizontalAlignment);
	void SetVerticalAlignment(ELogisimAlignment eVerticalAlignment);
};


#endif // !__LOGISIM_TEXT_H__


