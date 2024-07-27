#ifndef __TRACE_VALUE_H__
#define __TRACE_VALUE_H__
#include "BaseLib/PrimitiveTypes.h"


enum ETraceValue
{
	TV_High,
	TV_Low,
	TV_HighAndLow,

	TV_Unsettled,
	TV_NotConnected,
	TV_Error
};


class CTraceValue final
{
public:
	static bool IsHigh(ETraceValue e);
	static bool IsLow(ETraceValue e);
	static bool IsUnsettled(ETraceValue e);
	static bool IsConnected(ETraceValue e);
    static bool IsError(ETraceValue e);
    static char GetCharValue(ETraceValue e);
};


#endif // !__TRACE_VALUE_H__

