#ifndef __LOGISIM_WIRE_H__
#define __LOGISIM_WIRE_H__
#include "BaseLib/Int2.h"
#include "BaseLib/ListTemplate.h"


class CLogisimWire
{
protected:
	SInt2	msFrom;
	SInt2	msTo;

public:
	void Init(SInt2 sFrom, SInt2 sTo);
	void Kill(void);
};


typedef CListTemplate<CLogisimWire> CLogisimWireList;


#endif // !__LOGISIM_WIRE_H__


