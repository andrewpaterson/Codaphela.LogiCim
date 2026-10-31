#include "StandardLib/ClassDefines.h"
#include "SupportLib/ColourARGB32.h"
#include "SupportLib/PNGWriter.h"
#include "WindowLib/Window.h"
#include "LogicimWindowContent.h"
#include "LogicimWindowTick.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowTick::Init(SLogicimWindowData* pcData)
{
    PreInit();

    CWindowTick::Init();
    mpcData = pcData;
    mszSnapshot.Init();
    miSnapshotTicks = -1;

    PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowTick::Class(void)
{
    CWindowTick::Class();
    U_Pointer(mpcData);
    U_String(mszSnapshot);
    U_Int32(miSnapshotTicks);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowTick::Free(void)
{
    mszSnapshot.Kill();
    CWindowTick::Free();
}


//////////////////////////////////////////////////////////////////////////
//
// Saves the window as a PNG after iTicks ticks and closes it, to check how
// it draws without looking at the screen.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowTick::SetSnapshot(const char* szFilename, int iTicks)
{
    mszSnapshot.Set(szFilename);
    miSnapshotTicks = iTicks;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowTick::Tick(Ptr<CWindow> pWindow, int64 iUpdateTimeInMillieconds, int64 iTotalTimeInMillieconds)
{
    ELogicimDialog   eDialog;

    if (mpcData->pcContent)
    {
        mpcData->pcContent->Tick(iUpdateTimeInMillieconds);
    }
    pWindow->Paint();

    if (miSnapshotTicks >= 0)
    {
        if (miSnapshotTicks == 0)
        {
            SavePNG(pWindow->GetCanvas()->GetImageOrNull(), mszSnapshot.Text());
            pWindow->Stop();
        }
        miSnapshotTicks--;
    }

    if ((mpcData->eDialog != LDG_None) && mpcData->pcContent)
    {
        eDialog = mpcData->eDialog;
        mpcData->eDialog = LDG_None;
        mpcData->pcContent->ShowDialog(eDialog);
    }
}

