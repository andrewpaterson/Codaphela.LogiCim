#ifndef __LOGISIM_COMPONENT_LIST_H__
#define __LOGISIM_COMPONENT_LIST_H__
#include "BaseLib/ListVariable.h"
#include "LogisimTunnel.h"
#include "LogisimPullResistor.h"


class CLogisimComponentList
{
protected:
	CListVariable	mcList;
public:
	void					Init(void);
	void					Kill(void);
	CLogisimPullResistor*	CreatePullResistor(void);
	CLogisimTunnel*			CreateTunnel(void);
};


#endif // !__LOGISIM_COMPONENT_LIST_H__


