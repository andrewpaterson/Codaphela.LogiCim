#ifndef __INTEGRATED_CIRCUIT_H__
#define __INTEGRATED_CIRCUIT_H__
#include "StandardLib\Object.h"


class CIntegratedCircuit : public CObject
{
CONSTRUCTABLE(CIntegratedCircuit);
DESTRUCTABLE(CIntegratedCircuit);
protected:
public:
	virtual char*	GetType(void) =0;
};


#endif // __INTEGRATED_CIRCUIT_H__

