#ifndef __LOGISIM_WIRE_H__
#define __LOGISIM_WIRE_H__
#include "BaseLib/Int32Vec2.h"
#include "BaseLib/ListTemplate.h"


class CLogisimWire
{
protected:
	SInt32Vec2	msFrom;
	SInt32Vec2	msTo;

public:
	void Init(SInt32Vec2 sFrom, SInt32Vec2 sTo);
	void Kill(void);
};


typedef CListTemplate<CLogisimWire> CLogisimWireList;


#endif // __LOGISIM_WIRE_H__


