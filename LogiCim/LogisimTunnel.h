#ifndef __LOGISIM_TUNNEL_H__
#define __LOGISIM_TUNNEL_H__
#include "LogisimComponent.h"


class CLogisimTunnel : public CLogisimComponent
{
protected:
	int				miWidth;
	CChars			mszLabel;
	ELogisimFacing	meFacing;

public:
	void	Init(SInt2 sPosition);
	void	Kill(void);

	void	SetWidth(int iWidth);
	void	SetLabel(char* szLabel);
	void	SetFacing(ELogisimFacing eFacing);
};


#endif // !__LOGISIM_TUNNEL_H__

