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
			Ptr<CTrace>				Init(void);
			void					Free(void);

			void					Connect(Ptr<CTrace> pcTrace);

protected:
			Ptr<CArray<CTrace>>		GetTraces(void);
			void					SetNet(Ptr<CTraceNet> pcNet);
			Ptr<CArray<CTrace>>		FindConnected(void);
	static	void					Recurse(Ptr<CSet<CTrace>> papcAllConnections, Ptr<CArray<CTrace>> papcStack);
			ETraceValue				GetValue(void);
			ETraceValue				UpdateNetValue(ETraceValue value, Ptr<CPort> pcPort);
			Ptr<CTraceNet>			GetNet(void);
			Ptr<CString>			ToString(void);
			char					GetStringChar(void);
			Ptr<CPort>				Get_DEBUG_lastPortThatUpdated(void);
			bool					IsHigh(void);
			bool					IsError(void);
			bool					IsLow(void);
			bool					IsUnsettled(void);
			bool					IsNotConnected(void);

protected:
			void					Class(void);
};


#endif // !__TRACE_H__

