#ifndef __LOGISIM_CONSTANT_H__
#define __LOGISIM_CONSTANT_H__
#include "LogisimComponent.h"


class CLogisimConstant : public CLogisimComponent
{
protected:
	int		miWidth;
	int64	mlliValue;

public:
	void	Init(SInt2 sPosition);
	void	Kill(void);

	void	SetWidth(int iWidth);
	void	SetValue(int64 lliValue);
};


#endif // !__LOGISIM_CONSTANT_H__

