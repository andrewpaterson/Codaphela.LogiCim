#ifndef __LOGISIM_LIBRARY_H__
#define __LOGISIM_LIBRARY_H__
#include "BaseLib/Chars.h"
#include "BaseLib/ListTemplate.h"


class CLogisimLibrary
{
protected:
	CChars	mszName;
	CChars	mszDescription;

public:
	void Init(char* szName, char* szDescription);
	void Kill(void);
};


typedef CListTemplate<CLogisimLibrary> CLogisimLibraryList;


#endif // !__LOGISIM_LIBRARY_H__


