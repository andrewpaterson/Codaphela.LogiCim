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
    void Init(Ptr<CTickablePins> pcTickable, char* szName)
    {
        mpcTickable = pcTickable;
        mszName.Init(szName);
        meState = TS_NotSet;
        mpcTickable->AddPort(this);
    }

    char* GetName(void)
    {
        return mszName.Text();
    }

    void ResetConnections()
    {
        meState = TS_NotSet;
    }

    virtual void AddTraceValues(Ptr<CArray<CTraceValue>> pacTraceValues) =0;

    virtual void UpdateConnection(void) =0;

    virtual ETraceValue Read(void) =0;

    static ETraceValue ReadStates(Ptr<CArray<CPort>> pacPorts)
    {
        BOOL bHigh = FALSE;
        BOOL bLow = FALSE;
        BOOL bError = FALSE;
        BOOL bUnsettled = FALSE;
        BOOL bConnected = FALSE;

        int iNumElements = pacPorts->NumElements();

        for (int i = 0; i < iNumElements; i++)
        {
            Ptr<CPort> pcPort = pacPorts->Get(i);
            ETraceValue eValue = pcPort->Read();
            if (CTraceValue::IsConnected(eValue))
            {
                bConnected = TRUE;
            }

            if (CTraceValue::IsError(eValue))
            {
                bError = TRUE;
            }
            else if (CTraceValue::IsUnsettled(eValue))
            {
                bUnsettled = TRUE;
            }
            else if (CTraceValue::IsHigh(eValue))
            {
                bHigh = TRUE;
            }
            else if (CTraceValue::IsLow(eValue))
            {
                bLow = TRUE;
            }
        }

        return TranslatePortValue(bHigh, bLow, bError, bUnsettled, bConnected);
    }

    static ETraceValue TranslatePortValue(BOOL bHigh, BOOL bLow, BOOL bError, BOOL bUnsettled, BOOL bConnected)
    {
        if (bConnected)
        {
            if (bError)
            {
                return TV_Error;
            }
            else if (bUnsettled)
            {
                return TV_Unsettled;
            }
            else if (bHigh && bLow)
            {
                return TV_HighAndLow;
            }
            else if (bHigh)
            {
                return TV_High;
            }
            else if (bLow)
            {
                return TV_Low;
            }
        }
        return TV_NotConnected;
    }

    char* GetPortTransmissionStateAsString(CChars* pszDest)
    {
        pszDest->Append(&mszName);
        pszDest->Append("  [");
        if (CTransmissionState::IsInput(meState))
        {
            pszDest->Append("<-");
        }
        else if (CTransmissionState::IsOutput(meState))
        {
            pszDest->Append("->");
        }
        else if (CTransmissionState::IsNotSet(meState))
        {
            pszDest->Append("..");
        }
        else if (CTransmissionState::IsImpedance(meState))
        {
            pszDest->Append("xx");
        }
        pszDest->Append("]");
        return pszDest->Text();
    }

    char* GetDescription(CChars* pszDest)
    {
        GetTickable()->GetDescription(pszDest);
        pszDest->Append(".");
        pszDest->Append(&mszName);
        return pszDest->Text();
    }

    Ptr<CTickablePins> GetTickable(void)
    {
        return mpcTickable;
    }

    virtual void GetConnections(Ptr<CArray<CTrace>> pcDest);

    virtual char* GetTraceValuesAsString(CChars* pszDest) =0;

    virtual char* getWireValuesAsString(CChars* pszDest) =0;

    virtual char* getConnectionValuesAsString(CChars* pszDest) =0;
};


#endif // !__PORT_H__

