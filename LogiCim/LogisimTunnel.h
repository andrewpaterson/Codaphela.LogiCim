#ifndef __LOGISIM_TUNNEL_H__
#define __LOGISIM_TUNNEL_H__
#include "LogisimComponent.h"


class CLogisimTunnel : public CLogisimComponent
{
protected:
public:
	void	Init(SInt2 sPosition);
	void	Kill(void);
};


#endif // !__LOGISIM_TUNNEL_H__


