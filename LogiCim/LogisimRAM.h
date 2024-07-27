#ifndef __LOGISIM_RAM_H__
#define __LOGISIM_RAM_H__
#include "LogisimMemory.h"
#include "LogisimTrigger.h"


enum ELogisimRAMDataBus
{
	LRDB_Separate,
	LRDB_Bidirectional,
	LRDB_NotSet
};


class CLogisimRAM : public CLogisimMemory
{
protected:
	bool				mbVolatile;
	ELogisimTrigger		meTrigger;
	bool				mbLineEnables;
	ELogisimRAMDataBus	meDataBus;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetVolatile(bool bVolatile);
	void SetTrigger(ELogisimTrigger eTrigger);
	void SetLineEnables(bool bLineEnables);
	void SetDataBus(ELogisimRAMDataBus	eDataBus);
};


#endif // !__LOGISIM_RAM_H__

