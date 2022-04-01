#ifndef __TRANSMISSION_STATE_H__
#define __TRANSMISSION_STATE_H__
#include "BaseLib/Bool.h"


enum ETransmissionState
{
	TS_Input,
	TS_Output,
	TS_Impedance,
	TS_NotSet
};


class CTransmissionState
{
public:
	static BOOL IsOutput(ETransmissionState eState);
	static BOOL IsInput(ETransmissionState eState);
	static BOOL IsNotSet(ETransmissionState eState);
	static BOOL IsImpedance(ETransmissionState eState);
	static char* ToEnumString(ETransmissionState eState);
};


#endif // !__TRANSMISSION_STATE_H__

