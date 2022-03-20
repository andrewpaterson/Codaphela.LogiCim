#ifndef __LOGISIM_CIRCUIT_H__
#define __LOGISIM_CIRCUIT_H__
#include "BaseLib/Chars.h"
#include "BaseLib/ListTemplate.h"
#include "BaseLib/ArrayTemplatePtr.h"
#include "LogisimWire.h"
#include "LogisimComponent.h"


typedef CArrayTemplatePtr<CLogisimComponent> CLogisimComponentPtrArray;


class CLogisimCircuit
{
protected:
	CChars						mszName;

	CLogisimWireList			mlWires;
	CLogisimComponentPtrArray	mapComponents;

public:
	void			Init(char* szName);
	void			Kill(void);

	CLogisimWire*	AddWire(void);
	void			AddComponent(CLogisimComponent* pcComponent);
	char*			GetName(void);
};


typedef CListTemplate<CLogisimCircuit> CLogisimCircuitList;


#endif // !__LOGISIM_CIRCUIT_H__


