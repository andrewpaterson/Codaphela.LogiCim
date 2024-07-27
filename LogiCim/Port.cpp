#include "Port.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CPort> CPort::Init(Ptr<CTickablePins> pcTickable, char* szName)
{
    mpcTickable = pcTickable;
    mszName.Init(szName);
    meState = TS_NotSet;
    mpcTickable->AddPort(this);

    return Ptr<CPort>(this);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPort::Class(void)
{
    Pointer(mpcTickable.This(), "mpcTickable");
    UnmanagedString(&mszName, "mszName");
    UnmanagedEnum(&meState, "meState");
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPort::Free(void)
{
    mszName.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
char* CPort::GetName(void)
{
    return mszName.Text();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CPort::ResetConnections()
{
    meState = TS_NotSet;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
ETraceValue CPort::ReadStates(Ptr<CArray<CPort>> pacPorts)
{
    bool bHigh = false;
    bool bLow = false;
    bool bError = false;
    bool bUnsettled = false;
    bool bConnected = false;

    int iNumElements = pacPorts->NumElements();

    for (int i = 0; i < iNumElements; i++)
    {
        Ptr<CPort> pcPort = pacPorts->Get(i);
        ETraceValue eValue = pcPort->Read();
        if (CTraceValue::IsConnected(eValue))
        {
            bConnected = true;
        }

        if (CTraceValue::IsError(eValue))
        {
            bError = true;
        }
        else if (CTraceValue::IsUnsettled(eValue))
        {
            bUnsettled = true;
        }
        else if (CTraceValue::IsHigh(eValue))
        {
            bHigh = true;
        }
        else if (CTraceValue::IsLow(eValue))
        {
            bLow = true;
        }
    }

    return TranslatePortValue(bHigh, bLow, bError, bUnsettled, bConnected);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
ETraceValue CPort::TranslatePortValue(bool bHigh, bool bLow, bool bError, bool bUnsettled, bool bConnected)
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


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
char* CPort::GetPortTransmissionStateAsString(CChars* pszDest)
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


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
char* CPort::GetDescription(CChars* pszDest)
{
    GetTickable()->GetDescription(pszDest);
    pszDest->Append(".");
    pszDest->Append(&mszName);
    return pszDest->Text();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CTickablePins> CPort::GetTickable(void)
{
    return mpcTickable;
}

