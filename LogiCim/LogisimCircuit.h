#ifndef __LOGISIM_CIRCUIT_H__
#define __LOGISIM_CIRCUIT_H__
#include "BaseLib/Chars.h"
#include "BaseLib/ListTemplate.h"
#include "LogisimWire.h"


class CLogisimCircuit
{
protected:
	CChars				mszName;

	CLogisimWireList	mlWires;

public:
	void			Init(char* szName);
	void			Kill(void);

	CLogisimWire*	AddWire(void);
};


typedef CListTemplate<CLogisimCircuit> CLogisimCircuitList;


#endif // !__LOGISIM_CIRCUIT_H__


