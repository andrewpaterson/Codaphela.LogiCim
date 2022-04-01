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
        CChars  sz;

        sz.Init();
        mpc_DEBUG_LastPortThatUpdated->GetDescription(&sz);
        gcLogger.Warning2(__METHOD__, " Trace conflict: [", sz.Text(), "] set net value [", CharToString(CTraceValue::GetCharValue(meValue)), "] but [", pcPort->getDescription(), "] set net value [", CharToString(CTraceValue::GetCharValue(eValue)), "].", NULL);
        sz.Kill();
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

