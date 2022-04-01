#ifndef __TRACE_VALUE_H__
#define __TRACE_VALUE_H__
#include "BaseLib/Bool.h"


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
	static BOOL IsHigh(ETraceValue e);
	static BOOL IsLow(ETraceValue e);
	static BOOL IsUnsettled(ETraceValue e);
    static BOOL IsError(ETraceValue e);
    static char GetCharValue(ETraceValue e);
};


#endif // !__TRACE_VALUE_H__

