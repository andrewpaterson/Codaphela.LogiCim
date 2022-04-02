#include "Port.h"
#include "TraceNet.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CTraceNet> CTraceNet::Init(Ptr<CArray<CTrace>> apcConnected)
{
    macTraces.Init();
    macTraces.AddAll(apcConnected);
    meValue = TV_Unsettled;

    mpc_DEBUG_LastPortThatUpdated = NULL;

    return Ptr<CTraceNet>(this);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CTraceNet> CTraceNet::Init(Ptr<CTrace> pcTrace)
{
    macTraces.Init();
    macTraces.Add(pcTrace);
    meValue = TV_Unsettled;

    mpc_DEBUG_LastPortThatUpdated = NULL;

    return Ptr<CTraceNet>(this);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CTraceNet::Class(void)
{
    Embedded(&macTraces, "macTraces");
    UnmanagedEnum(&meValue, "meValue");
    Pointer(mpc_DEBUG_LastPortThatUpdated.This(), "mpc_DEBUG_LastPortThatUpdated");
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CTraceNet::Free(void)
{
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CTraceNet::Reset(void)
{
    meValue = TV_Unsettled;

    mpc_DEBUG_LastPortThatUpdated = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
ETraceValue CTraceNet::Update(ETraceValue eValue, Ptr<CPort> pcPort)
{
    if (meValue == eValue)
    {
        mpc_DEBUG_LastPortThatUpdated = pcPort;
        return meValue;
    }
    else if (meValue == TV_Unsettled || meValue == TV_NotConnected)
    {
        mpc_DEBUG_LastPortThatUpdated = pcPort;
        meValue = eValue;
        return meValue;
    }
    else
    {
        CChars  szDebugPort;
        CChars  szThisPort;

        szThisPort.Init();
        szDebugPort.Init();
        mpc_DEBUG_LastPortThatUpdated->GetDescription(&szDebugPort);
        pcPort->GetDescription(&szThisPort);
        gcLogger.Warning2(__METHOD__, " Trace conflict: [", szDebugPort.Text(), "] set net value [", CharToString(CTraceValue::GetCharValue(meValue)), "] but [", szThisPort.Text(), "] set net value [", CharToString(CTraceValue::GetCharValue(eValue)), "].", NULL);
        szDebugPort.Kill();
        szThisPort.Kill();
        mpc_DEBUG_LastPortThatUpdated = pcPort;
        meValue = eValue;
        return TV_Error;
    }
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
ETraceValue CTraceNet::GetValue(void)
{
    return meValue;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CPort> CTraceNet::Get_DEBUG_lastPortThatUpdated(void)
{
    return mpc_DEBUG_LastPortThatUpdated;
}

