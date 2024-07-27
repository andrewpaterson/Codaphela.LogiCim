#ifndef __TICKABLES_H__
#define __TICKABLES_H__
#include "StandardLib\Object.h"


class CTickablePins;
class CTickables : public CObject
{
CONSTRUCTABLE(CTickables);
DESTRUCTABLE(CTickables);
protected:
public:
	void Add(Ptr<CTickablePins> pcTickablePins)
	{

	}
};


#endif // !__TICKABLES_H__

