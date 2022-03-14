#ifndef __LOGISIM_CUSTOM_COMPONENT_H__
#define __LOGISIM_CUSTOM_COMPONENT_H__
#include "BaseLib/ListTemplate.h"
#include "LogisimComponent.h"


class CLogisimCustomComponent : public CLogisimComponent
{
protected:
	CChars	mszComponentType;

public:
	void	Init(SInt2 sPosition, char* szComponentType);
	void	Kill(void);
};


typedef CListTemplate<CLogisimCustomComponent> CLogisimCustomComponentList;


#endif //!__LOGISIM_CUSTOM_COMPONENT_H__

