#ifndef __LOGISIM_COMPARATOR_H__
#define __LOGISIM_COMPARATOR_H__
#include "LogisimNumericType.h"
#include "LogisimComponent.h"


class CLogisimComparator : public CLogisimComponent
{
protected:
	ELogisimNumericType		meNumericType;
	int						miWidth;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetNumericType(ELogisimNumericType eNumericType);
	void SetWidth(int iWidth);
};


#endif // !__LOGISIM_COMPARATOR_H__

