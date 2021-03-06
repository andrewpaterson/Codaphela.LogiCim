#include "TraceValue.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CTraceValue::IsHigh(ETraceValue e)
{
	return e == TV_High;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CTraceValue::IsLow(ETraceValue e)
{
	return e == TV_Low;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CTraceValue::IsUnsettled(ETraceValue e)
{
	return e == TV_Unsettled;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CTraceValue::IsConnected(ETraceValue e)
{
	return e != TV_NotConnected;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CTraceValue::IsError(ETraceValue e)
{
	return e == TV_Error;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
char CTraceValue::GetCharValue(ETraceValue e)
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

