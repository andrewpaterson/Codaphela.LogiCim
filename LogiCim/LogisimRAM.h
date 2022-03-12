#ifndef __LOGISIM_RAM_H__
#define __LOGISIM_RAM_H__
#include "LogisimTrigger.h"
#include "LogisimComponent.h"


enum ELogisimRAMDataBus
{
	LRDB_Separate,
	LRDB_Bidirectional,
	LRDB_NotSet
};


class CLogisimRAM : public CLogisimComponent
{
protected:
	int					miAddressWidth;
	BOOL				mbVolatile;
	ELogisimTrigger		meTrigger;
	BOOL				mbLineEnables;
	ELogisimRAMDataBus	meDataBus;

public:
	void Init(SInt2 sPosition);
	void Kill(void);

	void SetAddressWidth(int iAddressWidth);
	void SetVolatile(BOOL bVolatile);
	void SetTrigger(ELogisimTrigger eTrigger);
	void SetLineEnables(BOOL bLineEnables);
	void SetDataBus(ELogisimRAMDataBus	eDataBus);
};


#endif // !__LOGISIM_RAM_H__


