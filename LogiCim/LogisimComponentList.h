#ifndef __LOGISIM_COMPONENT_LIST_H__
#define __LOGISIM_COMPONENT_LIST_H__
#include "BaseLib/ListVariable.h"
#include "LogisimTunnel.h"
#include "LogisimPullResistor.h"
#include "LogisimConstant.h"
#include "LogisimANDGate.h"			
#include "LogisimNANDGate.h"			
#include "LogisimNORGate.h"			
#include "LogisimORGate.h"				
#include "LogisimNOTGate.h"			
#include "LogisimXORGate.h"			
#include "LogisimClock.h"				
#include "LogisimControlledBuffer.h"	
#include "LogisimCounter.h"			
#include "LogisimDecoder.h"			
#include "LogisimDigitalOscilloscope.h"
#include "LogisimLED.h"				
#include "LogisimPin.h"				
#include "LogisimProbe.h"				
#include "LogisimRAM.h"				
#include "LogisimROM.h"				
#include "LogisimSplitter.h"			
#include "LogisimText.h"				
#include "LogisimDFlipFlop.h"			
#include "LogisimRandom.h"
#include "LogisimComparator.h"
#include "LogisimShiftRegister.h"
#include "LogisimBuffer.h"


class CLogisimComponentList
{
protected:
	CListVariable	mcList;

public:
	void							Init(void);
	void							Kill(void);

	CLogisimPullResistor*			CreatePullResistor(void);
	CLogisimTunnel*					CreateTunnel(void);
	CLogisimConstant*				CreateConstant(void);
	CLogisimANDGate*				CreateANDGate(void);
	CLogisimNANDGate*				CreateNANDGate(void);
	CLogisimNORGate*				CreateNORGate(void);
	CLogisimORGate*					CreateORGate(void);
	CLogisimNOTGate*				CreateNOTGate(void);
	CLogisimXORGate*				CreateXORGate(void);
	CLogisimClock*					CreateClock(void);
	CLogisimControlledBuffer*		CreateControlledBuffer(void);
	CLogisimCounter*				CreateCounter(void);
	CLogisimDecoder*				CreateDecoder(void);
	CLogisimDigitalOscilloscope*	CreateDigitalOscilloscope(void);
	CLogisimLED*					CreateLED(void);
	CLogisimPin*					CreatePin(void);
	CLogisimProbe*					CreateProbe(void);
	CLogisimRAM*					CreateRAM(void);
	CLogisimROM*					CreateROM(void);
	CLogisimSplitter*				CreateSplitter(void);
	CLogisimText*					CreateText(void);
	CLogisimDTypeFlipFlop*			CreateDTypeFlipFlop(void);
	CLogisimRandom*					CreateRandom(void);
	CLogisimComparator*				CreateComparator(void);
	CLogisimShiftRegsiter*			CreateShiftRegsiter(void);
	CLogisimBuffer*					CreateBuffer(void);

	BOOL							Remove(CLogisimComponent* pcComp);
};


#endif // !__LOGISIM_COMPONENT_LIST_H__


