#ifndef __PORT_H__
#define __PORT_H__
#include "StandardLib\Object.h"
#include "StandardLib\Pointer.h"
#include "StandardLib\Collection.h"
#include "StandardLib\Array.h"
#include "Trace.h"
#include "TransmissionState.h"
#include "TickablePins.h"


class CPort : public CObject
{
CONSTRUCTABLE(CPort);
DESTRUCTABLE(CPort);
protected:
    Ptr<CTickablePins>  mpcTickable;
    CChars              mszName;
    ETransmissionState  meState;

public:
            Ptr<CPort>          Init(Ptr<CTickablePins> pcTickable, char* szName);
            void                Free(void);

            char*               GetName(void);
            void                ResetConnections(void);
    virtual void                AddTraceValues(Ptr<CArray<CTraceValue>> pacTraceValues) =0;
    virtual void                UpdateConnection(void) =0;
    virtual ETraceValue         Read(void) =0;
    static  ETraceValue         ReadStates(Ptr<CArray<CPort>> pacPorts);
    static  ETraceValue         TranslatePortValue(bool bHigh, bool bLow, bool bError, bool bUnsettled, bool bConnected);
            char*               GetPortTransmissionStateAsString(CChars* pszDest);
            char*               GetDescription(CChars* pszDest);
            Ptr<CTickablePins>  GetTickable(void);
    virtual void                GetConnections(Ptr<CArray<CTrace>> pcDest) =0;
    virtual char*               GetTraceValuesAsString(CChars* pszDest) =0;
    virtual char*               GetWireValuesAsString(CChars* pszDest) =0;
    virtual char*               GetConnectionValuesAsString(CChars* pszDest) =0;

protected:
             void			    Class(void);
};


#endif // !__PORT_H__

