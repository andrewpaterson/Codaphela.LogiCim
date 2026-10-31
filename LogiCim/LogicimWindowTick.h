#ifndef __LOGICIM_WINDOW_TICK_H__
#define __LOGICIM_WINDOW_TICK_H__
#include "WindowLib/WindowTick.h"
#include "LogicimWindowData.h"


class CLogicimWindowTick : public CWindowTick
{
CONSTRUCTABLE(CLogicimWindowTick);
DESTRUCTABLE(CLogicimWindowTick);
protected:
	SLogicimWindowData*	mpcData;
	CChars				mszSnapshot;
	int					miSnapshotTicks;

public:
	void	Init(SLogicimWindowData* pcData);
	void	Class(void) override;
	void	Free(void) override;
	void	SetSnapshot(const char* szFilename, int iTicks);
	void	Tick(Ptr<CWindow> pWindow, int64 iUpdateTimeInMillieconds, int64 iTotalTimeInMillieconds) override;
};


#endif // __LOGICIM_WINDOW_TICK_H__

