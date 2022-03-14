#ifndef __LOGISIM_NAND_GATE_H__
#define __LOGISIM_NAND_GATE_H__
#include "LogisimGate.h"


class CLogisimNANDGate : public CLogisimGate
{
protected:
public:
	void Init(SInt2 sPosition);
	void Kill(void);
};


#endif // !__LOGISIM_NAND_GATE_H__

