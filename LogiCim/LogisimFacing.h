#ifndef __LOGISIM_FACING_H__
#define __LOGISIM_FACING_H__
#include "LogisimComponent.h"


enum ELogisimFacing
{
	LF_North,
	LF_East,
	LF_South,
	LF_West,
	LF_NotSet,
};


class CLogisimFacing : public CLogisimComponent
{
protected:
	ELogisimFacing	meFacing;

public:
	void	Init(SInt2 sPosition);
	void	Kill(void);

	void	SetFacing(ELogisimFacing eFacing);
};


#endif // !__LOGISIM_FACING_H__

