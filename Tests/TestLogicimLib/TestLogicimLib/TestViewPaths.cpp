#include "BaseLib/ConstructorCall.h"
#include "BaseLib/GlobalDataTypesIO.h"
#include "BaseLib/Logger.h"
#include "StandardLib/Objects.h"
#include "MicroprocessorLib/CircuitSimulation.h"
#include "MicroprocessorLib/FamilyVoltageConfigurationStore.h"
#include "MicroprocessorLib/Trace.h"
#include "MicroprocessorLib/Event.h"
#include "MicroprocessorLib/Timeline.h"
#include "MicroprocessorLib/Voltage.h"
#include "LogicimLib/BufferView.h"
#include "LogicimLib/PowerSourceView.h"
#include "LogicimLib/CircuitView.h"
#include "LogicimLib/ViewPath.h"
#include "TestLib/Assert.h"


//////////////////////////////////////////////////////////////////////////
//
// A circuit is the only view on its top path.  The path has a subcircuit
// simulation for each circuit simulation it's simulated in.
//
//////////////////////////////////////////////////////////////////////////
void TestViewPathTop(void)
{
	CViewPaths				cPaths;
	CCircuitView			cCircuit;
	CCircuitView			cOther;
	Ptr<CCircuitSimulation>	pFirst;
	Ptr<CCircuitSimulation>	pSecond;
	CViewPath*				pcPath;
	CViewPath*				pcOtherPath;
	CChars					sz;
	SLogConfig				sLogConfig;

	ObjectsInit();
	CViewPath::ResetNextID();
	cCircuit.Init();
	cOther.Init();
	cPaths.Init();
	pFirst = OMalloc<CCircuitSimulation>("First");
	ORoot()->Add(pFirst);
	pSecond = OMalloc<CCircuitSimulation>("Second");
	ORoot()->Add(pSecond);

	pcPath = cPaths.AddTop(&cCircuit);
	AssertNotNull(pcPath);
	AssertTrue(pcPath->IsTop());
	AssertSize(1, pcPath->Length());
	AssertTrue(pcPath->GetFirst() == &cCircuit);
	AssertTrue(pcPath->GetLast() == &cCircuit);
	AssertTrue(pcPath->GetCircuitView() == &cCircuit);
	AssertNull(pcPath->GetPrevious());
	AssertLong(1, pcPath->GetID());
	sz.Init();
	pcPath->AppendDescription(&sz);
	AssertString("Circuit", sz.Text());
	sz.Kill();

	pcOtherPath = cPaths.AddTop(&cOther);
	AssertLong(2, pcOtherPath->GetID());
	AssertSize(2, cPaths.NumViewPaths());
	AssertTrue(cPaths.GetTop(&cCircuit) == pcPath);
	AssertTrue(cPaths.GetTop(&cOther) == pcOtherPath);

	gcLogger.GetConfig(&sLogConfig);
	gcLogger.SetSilent();
	AssertNull(cPaths.AddTop(&cCircuit));
	gcLogger.SetConfig(&sLogConfig);
	AssertSize(2, cPaths.NumViewPaths());

	AssertSize(0, pcPath->NumCircuitSimulations());
	AssertFalse(pcPath->ContainsCircuitSimulation(&pFirst));
	AssertNull(pcPath->GetSubcircuitSimulation(&pFirst));
	AssertTrue(pcPath->AddSubcircuitSimulation(pFirst->GetSubcircuitTopSimulation()));
	AssertTrue(pcPath->AddSubcircuitSimulation(pSecond->GetSubcircuitTopSimulation()));
	gcLogger.SetSilent();
	AssertFalse(pcPath->AddSubcircuitSimulation(pFirst->GetSubcircuitTopSimulation()));
	gcLogger.SetConfig(&sLogConfig);
	AssertSize(2, pcPath->NumCircuitSimulations());
	AssertTrue(pcPath->GetCircuitSimulation(0) == &pFirst);
	AssertTrue(pcPath->GetCircuitSimulation(1) == &pSecond);
	AssertTrue(pcPath->GetSubcircuitSimulation(&pSecond) == pSecond->GetSubcircuitTopSimulation());
	AssertFalse(pcOtherPath->ContainsCircuitSimulation(&pFirst));

	AssertTrue(pcPath->RemoveSubcircuitSimulation(&pFirst));
	AssertFalse(pcPath->RemoveSubcircuitSimulation(&pFirst));
	AssertFalse(pcPath->ContainsCircuitSimulation(&pFirst));
	AssertTrue(pcPath->ContainsCircuitSimulation(&pSecond));
	AssertTrue(pcPath->GetCircuitSimulation(0) == &pSecond);

	ORoot()->Remove(pSecond);
	pSecond = NULL;
	ORoot()->Remove(pFirst);
	pFirst = NULL;
	cPaths.Kill();
	cOther.Kill();
	cCircuit.Kill();
	ObjectsKill();
}


//////////////////////////////////////////////////////////////////////////
//
// A ground on an inverter's input, as TestCircuitViewGroundDrivesInverters.
//
//////////////////////////////////////////////////////////////////////////
static void AddGroundedInverter(CCircuitView* pcCircuit, CInverterView** ppcInverter, CTraceView** ppcOut)
{
	CGroundView*	pcGround;

	pcGround = NewMalloc<CGroundView>();
	pcGround->Init(-4, 3, ROT_North, "");
	pcCircuit->Add(pcGround);
	*ppcInverter = NewMalloc<CInverterView>();
	(*ppcInverter)->Init(-3, 3, ROT_East, "", "LVC", false, 1, 1);
	pcCircuit->Add(*ppcInverter);
	*ppcOut = pcCircuit->AddTrace(-1, 3, 6, 3);
}


//////////////////////////////////////////////////////////////////////////
//
// One circuit's views simulated in two circuit simulations at once, as
// Logicim's editor can: each simulation has its own components and traces,
// and forgetting one leaves the other running.
//
//////////////////////////////////////////////////////////////////////////
void TestViewPathTwoSimulations(void)
{
	CFamilyVoltageConfigurationStore	cFamilies;
	CFamilyVoltageConfiguration*		pcFamily;
	Ptr<CCircuitSimulation>				pFirst;
	Ptr<CCircuitSimulation>				pSecond;
	CCircuitView						cCircuit;
	CViewPaths							cPaths;
	CViewPath*							pcPath;
	CInverterView*						pcInverter;
	CTraceView*							pcOut;
	float32								fVcc;

	ObjectsInit();
	CEvent::ResetNextID();
	CTrace::ResetNextID();
	cFamilies.Init();
	AssertTrue(cFamilies.Load("Input" _FS_ "PropagationTimes.csv"));
	pcFamily = cFamilies.Get("LVC");
	fVcc = pcFamily->GetDefaultVoltageConfiguration(DEFAULT_LOGIC_LEVEL)->GetVcc();
	pFirst = OMalloc<CCircuitSimulation>("First");
	ORoot()->Add(pFirst);
	pSecond = OMalloc<CCircuitSimulation>("Second");
	ORoot()->Add(pSecond);
	cCircuit.Init();
	cPaths.Init();
	pcPath = cPaths.AddTop(&cCircuit);
	AddGroundedInverter(&cCircuit, &pcInverter, &pcOut);

	AssertTrue(cCircuit.CreateSimulation(pcPath, &pFirst, &cFamilies));
	AssertTrue(cCircuit.CreateSimulation(pcPath, &pSecond, &cFamilies));
	AssertTrue(pcPath->GetSubcircuitSimulation(&pFirst) == pFirst->GetSubcircuitTopSimulation());
	AssertTrue(pcPath->GetSubcircuitSimulation(&pSecond) == pSecond->GetSubcircuitTopSimulation());
	AssertSize(2, pcInverter->NumComponents());
	AssertNotNull(pcInverter->GetComponent(pcPath, &pFirst));
	AssertNotNull(pcInverter->GetComponent(pcPath, &pSecond));
	AssertTrue(pcInverter->GetComponent(pcPath, &pFirst) != pcInverter->GetComponent(pcPath, &pSecond));
	AssertTrue(pcInverter->GetComponent(pcPath, &pFirst)->GetContainingSubcircuitSimulation() == pFirst->GetSubcircuitTopSimulation());
	AssertTrue(pcInverter->GetComponent(pcPath, &pSecond)->GetContainingSubcircuitSimulation() == pSecond->GetSubcircuitTopSimulation());
	AssertSize(1, pcOut->NumTraces(pcPath, &pFirst));
	AssertSize(1, pcOut->NumTraces(pcPath, &pSecond));
	AssertTrue(pcOut->GetTrace(pcPath, &pFirst, 0) != pcOut->GetTrace(pcPath, &pSecond, 0));
	AssertTrue(pcInverter->GetPortView("Output")->GetPort(pcPath, &pFirst, 0)->GetTrace() == pcOut->GetTrace(pcPath, &pFirst, 0));
	AssertTrue(pcInverter->GetPortView("Output")->GetPort(pcPath, &pSecond, 0)->GetTrace() == pcOut->GetTrace(pcPath, &pSecond, 0));

	//Only the first is run; the second's output hasn't been driven yet.
	pFirst->GetTimeline()->Run();
	AssertInt(TV_High, pcFamily->GetValue(pcOut->GetTrace(pcPath, &pFirst, 0)->GetVoltage(pFirst->GetTime()), fVcc));
	AssertFalse(IsVoltage(pcOut->GetTrace(pcPath, &pSecond, 0)->GetVoltage(pSecond->GetTime())));

	cCircuit.DisconnectSimulation(pcPath, &pFirst);
	AssertFalse(pcPath->ContainsCircuitSimulation(&pFirst));
	AssertNull(pcInverter->GetComponent(pcPath, &pFirst));
	AssertSize(0, pcOut->NumTraces(pcPath, &pFirst));
	AssertSize(1, pcInverter->NumComponents());
	AssertNotNull(pcInverter->GetComponent(pcPath, &pSecond));

	pSecond->GetTimeline()->Run();
	AssertInt(TV_High, pcFamily->GetValue(pcOut->GetTrace(pcPath, &pSecond, 0)->GetVoltage(pSecond->GetTime()), fVcc));

	cCircuit.Kill();
	cPaths.Kill();
	ORoot()->Remove(pSecond);
	pSecond = NULL;
	ORoot()->Remove(pFirst);
	pFirst = NULL;
	cFamilies.Kill();
	ObjectsKill();
}


//////////////////////////////////////////////////////////////////////////
//
// A component is made once for each path and simulation, and only for a
// path that ends in the circuit and is simulated.
//
//////////////////////////////////////////////////////////////////////////
void TestViewPathCreateErrors(void)
{
	CFamilyVoltageConfigurationStore	cFamilies;
	Ptr<CCircuitSimulation>				pSimulation;
	CCircuitView						cCircuit;
	CCircuitView						cOther;
	CViewPaths							cPaths;
	CViewPath*							pcPath;
	CViewPath*							pcOtherPath;
	CInverterView*						pcInverter;
	CTraceView*							pcOut;
	SLogConfig							sLogConfig;

	ObjectsInit();
	cFamilies.Init();
	AssertTrue(cFamilies.Load("Input" _FS_ "PropagationTimes.csv"));
	pSimulation = OMalloc<CCircuitSimulation>("Errors");
	ORoot()->Add(pSimulation);
	cCircuit.Init();
	cOther.Init();
	cPaths.Init();
	pcPath = cPaths.AddTop(&cCircuit);
	pcOtherPath = cPaths.AddTop(&cOther);
	AddGroundedInverter(&cCircuit, &pcInverter, &pcOut);

	gcLogger.GetConfig(&sLogConfig);
	gcLogger.SetSilent();
	AssertFalse(cCircuit.CreateSimulation(pcOtherPath, &pSimulation, &cFamilies));
	AssertSize(0, pcInverter->NumComponents());
	AssertNull(pcInverter->CreateComponent(pcPath, &pSimulation, &cFamilies));
	gcLogger.SetConfig(&sLogConfig);

	AssertTrue(cCircuit.CreateSimulation(pcPath, &pSimulation, &cFamilies));
	gcLogger.SetSilent();
	AssertNull(pcInverter->CreateComponent(pcPath, &pSimulation, &cFamilies));
	gcLogger.SetConfig(&sLogConfig);
	AssertSize(1, pcInverter->NumComponents());

	cCircuit.Kill();
	cOther.Kill();
	cPaths.Kill();
	ORoot()->Remove(pSimulation);
	pSimulation = NULL;
	cFamilies.Kill();
	ObjectsKill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestViewPaths(void)
{
	BeginTests();
	DataIOInit();

	TestViewPathTop();
	TestViewPathTwoSimulations();
	TestViewPathCreateErrors();

	DataIOKill();
	TestStatistics();
}

