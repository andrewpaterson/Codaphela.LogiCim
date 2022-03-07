#ifndef __LOGISIM_CIRCUIT_H__
#define __LOGISIM_CIRCUIT_H__
#include "BaseLib/Chars.h"
#include "BaseLib/ListTemplate.h"


class CLogisimCircuit
{
protected:
	CChars	mszName;

public:
	void Init(char* szName);
	void Kill(void);
};


typedef CListTemplate<CLogisimCircuit> CLogisimCircuitList;


#endif // !__LOGISIM_CIRCUIT_H__


