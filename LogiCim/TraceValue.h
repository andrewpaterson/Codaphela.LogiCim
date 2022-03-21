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
	static BOOL IsHigh(ETraceValue e)
	{
		return e == TV_High;
	}

	static BOOL IsLow(ETraceValue e)
	{
		return e == TV_Low;
	}

	static BOOL IsUnsettled(ETraceValue e)
	{
		return e == TV_Unsettled;
	}

	static BOOL IsError(ETraceValue e)
	{
		return e == TV_Error;
	}

	static char GetStringValue(ETraceValue e)
	{
        if (IsHigh(e))
        {
            return '1';
        }
        else if (IsLow(e))
        {
            return '0';
        }
        else if (IsUnsettled(e))
        {
            return '.';
        }
        else if (IsError(e))
        {
            return 'E';
        }
        return ' ';
	}
};


#endif // !__TRACE_VALUE_H__

