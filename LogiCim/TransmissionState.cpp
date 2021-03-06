#include "TransmissionState.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static BOOL IsOutput(ETransmissionState eState)
{
	return eState == TS_Output;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static BOOL IsInput(ETransmissionState eState)
{
	return eState == TS_Input;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static BOOL IsNotSet(ETransmissionState eState)
{
	return eState == TS_NotSet;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CTransmissionState::IsImpedance(ETransmissionState eState)
{
	return eState == TS_Impedance;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
char* CTransmissionState::ToEnumString(ETransmissionState eState)
{
	if (eState == TS_Output)
	{
		return "Output";
	}
	else if (eState == TS_Input)
	{
		return "Input";
	}
	else if (eState == TS_NotSet)
	{
		return "Not Set";
	}
	else if (eState == TS_Impedance)
	{
		return "Impedance";
	}
	else
	{
		return "";
	}
}

