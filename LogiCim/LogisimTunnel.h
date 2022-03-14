#ifndef __LOGISIM_TUNNEL_H__
#define __LOGISIM_TUNNEL_H__
#include "LogisimFacing.h"


class CLogisimTunnel : public CLogisimFacing
{
protected:
	int				miWidth;
	CChars			mszLabel;

public:
	void	Init(SInt2 sPosition);
	void	Kill(void);

	void	SetWidth(int iWidth);
	void	SetLabel(char* szLabel);
};


#endif // !__LOGISIM_TUNNEL_H__

