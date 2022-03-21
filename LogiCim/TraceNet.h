#ifndef __TRACE_NET_H__
#define __TRACE_NET_H__
#include "StandardLib/Object.h"
#include "StandardLib/Array.h"
#include "TraceValue.h"
#include "Port.h"


class CTrace;
class CTraceNet : public CObject
{
CONSTRUCTABLE(CTraceNet);
DESTRUCTABLE(CTraceNet);
protected:
    CArray<CTrace>  macTraces;
    ETraceValue     meValue;

    Ptr<CPort>      mpc_DEBUG_LastPortThatUpdated;

public:
    Ptr<CTraceNet> Init(Ptr<CArray<CTrace>> apcConnected)
    {
        macTraces.Init();
        macTraces.AddAll(apcConnected);
        meValue = TV_Unsettled;

        mpc_DEBUG_LastPortThatUpdated = NULL;
    }

    Ptr<CTraceNet> Init(Ptr<CTrace> pcTrace)
    {
        macTraces.Init();
        macTraces.Add(pcTrace);
        meValue = TV_Unsettled;

        mpc_DEBUG_LastPortThatUpdated = NULL;
    }

    void Reset(void)
    {
        meValue = TV_Unsettled;

        mpc_DEBUG_LastPortThatUpdated = NULL;
    }

    ETraceValue Update(ETraceValue eValue, Ptr<CPort> pcPort)
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
            gcLogger.Warning2(__METHOD__, " Trace conflict: [", mpc_DEBUG_LastPortThatUpdated.getDescription(), "] set net value [", meValue.getStringValue(), "] but [", pcPort.getDescription(), "] set net value [", value.getStringValue(), "].", NULL);
            mpc_DEBUG_LastPortThatUpdated = pcPort;
            meValue = eValue;
            return TV_Error;
        }
    }

    ETraceValue GetValue(void)
    {
        return meValue;
    }

    Ptr<CPort> Get_DEBUG_lastPortThatUpdated(void)
    {
        return mpc_DEBUG_LastPortThatUpdated;
    }
};


#endif // !__TRACE_NET_H__

