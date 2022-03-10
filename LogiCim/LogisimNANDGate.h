#ifndef __LOGISIM_NAND_GATE_H__
#define __LOGISIM_NAND_GATE_H__
#include "LogisimComponent.h"


class CLogisimNANDGate : public CLogisimComponent
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_NAND_GATE_H__


