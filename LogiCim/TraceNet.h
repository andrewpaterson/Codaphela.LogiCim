#ifndef __TRACE_NET_H__
#define __TRACE_NET_H__
#include "StandardLib/Object.h"
#include "StandardLib/Array.h"
#include "TraceValue.h"


class CPort;
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
    Ptr<CTraceNet>  Init(Ptr<CArray<CTrace>> apcConnected);
    Ptr<CTraceNet>  Init(Ptr<CTrace> pcTrace);
    void            Free(void);

    void            Reset(void);
    ETraceValue     Update(ETraceValue eValue, Ptr<CPort> pcPort);
    ETraceValue     GetValue(void);
    Ptr<CPort>      Get_DEBUG_lastPortThatUpdated(void);

protected:
    void			Class(void);
};


#endif // !__TRACE_NET_H__

