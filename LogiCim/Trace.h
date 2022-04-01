#ifndef __TRACE_H__
#define __TRACE_H__
#include "BaseLib/ArrayTemplatePtr.h"
#include "StandardLib/Object.h"
#include "StandardLib/Objects.h"
#include "StandardLib/Array.h"
#include "StandardLib/Set.h"
#include "StandardLib/String.h"
#include "TraceNet.h"


class CTrace : public CObject
{
CONSTRUCTABLE(CTrace);
DESTRUCTABLE(CTrace);
protected:
    CArray<CTrace>  mapcTraces;
    Ptr<CTraceNet>  mpcNet;

public:
	void Init(void)
	{
		mapcTraces.Init();
		mpcNet = OMalloc<CTraceNet>(Ptr<CTrace>(this));
	}

	void Connect(Ptr<CTrace> pcTrace)
	{
		mapcTraces.Add(pcTrace);
		pcTrace->mapcTraces.Add(this);

		Ptr<CArray<CTrace>> pacConnected = FindConnected();
		
		int iNumElements = pacConnected->NumElements();
		for (int i = 0; i < iNumElements; i++)
		{
			Ptr<CTrace> pcConnection = pacConnected->Get(i);
			pcConnection->SetNet(mpcNet);
		}
	}

protected:
	Ptr<CArray<CTrace>> GetTraces(void)
	{
		return Ptr<CArray<CTrace>>(&mapcTraces);
	}

	void SetNet(Ptr<CTraceNet> pcNet)
	{
		mpcNet = pcNet;
	}

	Ptr<CArray<CTrace>> FindConnected()
	{
		CSet<CTrace> apcAllConnections;
		apcAllConnections.Init();

		CArray<CTrace> apcStack;
		apcStack.Init();

		apcStack.Add(Ptr<CTrace>(this));
		apcAllConnections.Add(Ptr<CTrace>(this));

		Recurse(Ptr<CSet<CTrace>>(&apcAllConnections), Ptr<CArray<CTrace>>(&apcStack));

		Ptr<CArray<CTrace>> p = OMalloc<CArray<CTrace>>();
		p->AddAll(&apcAllConnections);
		return p;
	}

	static void Recurse(Ptr<CSet<CTrace>> papcAllConnections, Ptr<CArray<CTrace>> papcStack)
	{
		while (!papcStack->IsEmpty())
		{
			Ptr<CTrace> pcTrace = papcStack->Get(papcStack->Size() - 1);
			papcStack->RemoveAt(papcStack->Size() - 1);

			Ptr<CArray<CTrace>> papcTrces = pcTrace->GetTraces();
			int iNumElements = papcTrces->NumElements();
			for (int i = 0; i < iNumElements; i++)
			{
				Ptr<CTrace> pcConnection = papcTrces->Get(i);
				BOOL bAdded = papcAllConnections->Add(pcConnection);
				if (bAdded)
				{
					papcStack->Add(pcConnection);
				}
			}
		}
	}

	ETraceValue GetValue(void)
	{
		if (mpcNet.IsNotNull())
		{
			return mpcNet->GetValue();
		}
		else
		{
			return TV_Unsettled;
		}
	}

	ETraceValue UpdateNetValue(ETraceValue value, Ptr<CPort> pcPort)
	{
		return mpcNet->Update(value, pcPort);
	}

	Ptr<CTraceNet> GetNet()
	{
		return mpcNet;
	}

	Ptr<CString> ToString()
	{
		ETraceValue value = mpcNet->GetValue();
		return  NULL;//StringUtil.toEnumString(value);
	}

	char GetStringChar(void)
	{
		return CTraceValue::GetCharValue(mpcNet->GetValue());
	}

	Ptr<CPort> Get_DEBUG_lastPortThatUpdated()
	{
		if (mpcNet.IsNotNull())
		{
			return mpcNet->Get_DEBUG_lastPortThatUpdated();
		}
		else
		{
			return NULL;
		}
	}

	BOOL isHigh()
	{
		return mpcNet.IsNotNull() && CTraceValue::IsHigh(mpcNet->GetValue());
	}

	BOOL isError()
	{
		return mpcNet.IsNotNull() && CTraceValue::IsError(mpcNet->GetValue());
	}

	BOOL isLow()
	{
		return mpcNet.IsNotNull() && CTraceValue::IsLow(mpcNet->GetValue());
	}

	BOOL isUnsettled()
	{
		return mpcNet.IsNotNull() && CTraceValue::IsUnsettled(mpcNet->GetValue());
	}

	BOOL isNotConnected()
	{
		return mpcNet.IsNull();
	}
};


#endif // !__TRACE_H__

