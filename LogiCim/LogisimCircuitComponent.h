#ifndef __LOGISIM_CIRCUIT_COMPONENT_H__
#define __LOGISIM_CIRCUIT_COMPONENT_H__
#include "BaseLib/ListTemplate.h"
#include "LogisimComponent.h"


class CLogisimCircuitComponent : public CLogisimComponent
{
protected:
	CChars	mszCircuitName;

public:
	void Init(SInt2 sPosition, char* szCircuitName);
	void Kill(void);
};


typedef CListTemplate<CLogisimCircuitComponent> CLogisimCircuitComponentList;


#endif // !__LOGISIM_CIRCUIT_COMPONENT_H__

