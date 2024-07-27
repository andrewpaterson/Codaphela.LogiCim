#ifndef __TRANSMISSION_STATE_H__
#define __TRANSMISSION_STATE_H__
#include "BaseLib/PrimitiveTypes.h"


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
	static bool IsOutput(ETransmissionState eState);
	static bool IsInput(ETransmissionState eState);
	static bool IsNotSet(ETransmissionState eState);
	static bool IsImpedance(ETransmissionState eState);
	static char* ToEnumString(ETransmissionState eState);
};


#endif // !__TRANSMISSION_STATE_H__

