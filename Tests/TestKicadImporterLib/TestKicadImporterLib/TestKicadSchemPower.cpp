#include "BaseLib/GlobalDataTypesIO.h"
#include "BaseLib/DataMemory.h"
#include "BaseLib/StringHelper.h"
#include "StandardLib/Objects.h"
#include "MicroprocessorLib/CircuitSimulation.h"
#include "MicroprocessorLib/FamilyVoltageConfigurationStore.h"
#include "MicroprocessorLib/LongTime.h"
#include "MicroprocessorLib/PowerSource.h"
#include "MicroprocessorLib/ResetSupervisor.h"
#include "LogicimLib/CircuitFonts.h"
#include "LogicimLib/CircuitViews.h"
#include "LogicimLib/ViewPath.h"
#include "KicadLib/KicadSchemFile.h"
#include "KicadLib/KicadSchemNetlist.h"
#include "KicadImporterLib/KicadSchemLogicimConverter.h"
#include "TestLib/Assert.h"


#define KICAD_SCHEM_POWER_FONTS				"Input" _FS_ "Fonts"
#define KICAD_SCHEM_POWER_FAMILIES			"Input" _FS_ "PropagationTimes.csv"
#define KICAD_SCHEM_POWER_RESET_BOARD		"Input" _FS_ "Kicad" _FS_ "Computer" _FS_ "Main" _FS_ "Processor Reset Device" _FS_ "Processor Reset Device.kicad_sch"


//////////////////////////////////////////////////////////////////////////
//
// A board imported and simulated, its supplies ramping from the start.
//
//////////////////////////////////////////////////////////////////////////
struct SKicadSchemPowerBoard
{
	CKicadSchemFile						cFile;
	CKicadSchemNetlist					cNetlist;
	Ptr<CCircuitFonts>					pFonts;
	CFamilyVoltageConfigurationStore	cFamilies;
	CCircuitViews						cCircuits;
	CKicadSchemLogicimConverter			cConverter;
	CViewPaths							cPaths;
	CViewPath*							pcPath;
	Ptr<CCircuitSimulation>				pSimulation;

	void Init(const char* szFilename, const char* szName)
	{
		CChars	szPath;

		ObjectsInit();
		pFonts = LoadCircuitFonts(KICAD_SCHEM_POWER_FONTS, CCO_Unknown);
		ORoot()->Add(pFonts);
		cFamilies.Init();
		AssertTrue(cFamilies.Load(KICAD_SCHEM_POWER_FAMILIES));
		szPath.Init(szFilename);
		cFile.Init();
		AssertTrue(cFile.Load(szPath.Text()));
		szPath.Kill();
		cNetlist.Init(&cFile);
		AssertTrue(cNetlist.Build());
		cCircuits.Init();
		cConverter.Init(&cFile, &cNetlist, &cCircuits, &pFonts);
		AssertTrue(cConverter.Convert(szName));
		cPaths.Init();
		pcPath = cPaths.AddTop(cConverter.GetCircuit());
		pSimulation = OMalloc<CCircuitSimulation>(szName);
		ORoot()->Add(pSimulation);
		AssertTrue(cConverter.GetCircuit()->CreateSimulation(pcPath, &pSimulation, &cFamilies));
	}

	void Kill(void)
	{
		ORoot()->Remove(pSimulation);
		pSimulation = NULL;
		cConverter.Kill();
		cCircuits.Kill();
		cPaths.Kill();
		cNetlist.Kill();
		cFile.Kill();
		cFamilies.Kill();
		pFonts = NULL;
		ORoot()->RemoveAll();
		ObjectsFlush();
		ObjectsKill(false);
	}

	//The component made for the view named szName.
	CCircuitComponent* Get(const char* szName)
	{
		size	i;

		for (i = 0; i < cConverter.GetCircuit()->NumComponents(); i++)
		{
			if (StringCompare(cConverter.GetCircuit()->GetComponent(i)->GetName(), szName) == 0)
			{
				return cConverter.GetCircuit()->GetComponent(i)->GetComponent(pcPath, &pSimulation);
			}
		}
		return NULL;
	}

	float32 Voltage(CPowerSource* pcSupply)
	{
		return pcSupply->GetPowerOutPort()->GetVoltageOut(pSimulation->GetTime());
	}

	void RunTo(int64 iNanoseconds)
	{
		pSimulation->RunToTime(NanosecondsToTime(iNanoseconds) - pSimulation->GetTime());
	}
};


//////////////////////////////////////////////////////////////////////////
//
// Power and Reset switched on: the 18V input rises over 1ms, its L7812
// (dropout 2V) comes on at 14V and rises over 1ms, its LD1117S50C
// (dropout 1.1V) on at 12V's 6.1V, so 5V passes the DS1813's 4.35V at
// about 2.2ms, and the DS1813 lets its reset go 150ms after that.  Its
// 74LVC1G17 (IC4), on 5V, buffers the reset onto ~{Reset}_{5V}.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemPowerAndReset(void)
{
	SKicadSchemPowerBoard	sBoard;
	CPowerSource*			pc18V;
	CPowerSource*			pc12V;
	CPowerSource*			pc5V;
	CPowerSource*			pc3V3;
	CResetSupervisor*		pcSupervisor;
	CIntegratedCircuit*		pcBuffer;
	CLogicPort*				pcReset5V;

	sBoard.Init("Input" _FS_ "Power and Reset.kicad_sch", "Power and Reset");
	pcBuffer = (CIntegratedCircuit*)sBoard.Get("IC4");
	AssertNotNull(pcBuffer);
	pcReset5V = (CLogicPort*)pcBuffer->GetPort("Output");
	AssertNotNull(pcReset5V);
	pc18V = (CPowerSource*)sBoard.Get("18V");
	pc12V = (CPowerSource*)sBoard.Get("IC1");
	pc5V = (CPowerSource*)sBoard.Get("IC2");
	pc3V3 = (CPowerSource*)sBoard.Get("IC5");
	pcSupervisor = (CResetSupervisor*)sBoard.Get("IC3");
	AssertNotNull(pc18V);
	AssertNotNull(pc12V);
	AssertNotNull(pc5V);
	AssertNotNull(pc3V3);
	AssertNotNull(pcSupervisor);
	AssertNotNull(pc12V->GetFeedPort());
	AssertTrue(pc12V->GetFeedPort()->GetTrace() == pc18V->GetPowerOutPort()->GetTrace());
	AssertTrue(pc5V->GetFeedPort()->GetTrace() == pc12V->GetPowerOutPort()->GetTrace());
	AssertTrue(pcSupervisor->GetPins()->GetVoltageCommon()->GetTrace() == pc5V->GetPowerOutPort()->GetTrace());
	AssertTrue(sBoard.pSimulation->IsPoweringUp());
	AssertFloat(0.0f, sBoard.Voltage(pc18V), 3);

	sBoard.RunTo(700000);
	AssertFalse(pc12V->IsOn());
	sBoard.RunTo(900000);
	AssertTrue(pc12V->IsOn());
	AssertFalse(pc5V->IsOn());
	sBoard.RunTo(1400000);
	AssertTrue(pc5V->IsOn());
	AssertTrue(pc3V3->IsOn());
	sBoard.RunTo(3000000);
	AssertFloat(18.0f, sBoard.Voltage(pc18V), 3);
	AssertFloat(12.0f, sBoard.Voltage(pc12V), 3);
	AssertFloat(5.0f, sBoard.Voltage(pc5V), 3);
	AssertFloat(3.3f, sBoard.Voltage(pc3V3), 3);
	AssertTrue(pcSupervisor->IsAsserting());
	AssertTrue(sBoard.pSimulation->IsPoweringUp());
	AssertInt(TV_Low, pcReset5V->Read(sBoard.pSimulation->GetTimeline()).eValue);

	sBoard.RunTo(150000000);
	AssertTrue(pcSupervisor->IsAsserting());
	sBoard.RunTo(153000000);
	AssertFalse(pcSupervisor->IsAsserting());
	AssertFalse(sBoard.pSimulation->IsPoweringUp());
	AssertInt(TV_High, pcReset5V->Read(sBoard.pSimulation->GetTimeline()).eValue);

	sBoard.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The Processor Reset Device's 5V comes in on its connector's labels, its
// AP7365 makes 3.3V from it (on at 3.6V, 200us to rise), its DS1813 holds
// ~{Reset Switch} low for 150ms after 5V passes 4.35V, and its 74LVC14
// (IC7), on 3.3V, inverts it onto Reset_1: high while reset is held, low
// after.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemProcessorResetDevice(void)
{
	SKicadSchemPowerBoard	sBoard;
	CPowerSource*			pc3V3;
	CResetSupervisor*		pcSupervisor;
	CIntegratedCircuit*		pcInverters;
	CLogicPort*				pcReset1;

	sBoard.Init(KICAD_SCHEM_POWER_RESET_BOARD, "Processor Reset Device");
	pc3V3 = (CPowerSource*)sBoard.Get("IC6");
	pcSupervisor = (CResetSupervisor*)sBoard.Get("IC2");
	pcInverters = (CIntegratedCircuit*)sBoard.Get("IC7");
	AssertNotNull(pc3V3);
	AssertNotNull(pcSupervisor);
	AssertNotNull(pcInverters);
	AssertNotNull(pc3V3->GetFeedPort());
	AssertTrue(pcInverters->GetPins()->GetVoltageCommon()->GetTrace() == pc3V3->GetPowerOutPort()->GetTrace());
	pcReset1 = (CLogicPort*)pcInverters->GetPort("1Y");
	AssertNotNull(pcReset1);

	sBoard.RunTo(3000000);
	AssertFloat(3.3f, sBoard.Voltage(pc3V3), 3);
	AssertTrue(pcInverters->IsPowered(sBoard.pSimulation->GetTime()));
	AssertTrue(pcSupervisor->IsAsserting());
	AssertInt(TV_High, pcReset1->Read(sBoard.pSimulation->GetTimeline()).eValue);

	sBoard.RunTo(155000000);
	AssertFalse(pcSupervisor->IsAsserting());
	AssertInt(TV_Low, pcReset1->Read(sBoard.pSimulation->GetTimeline()).eValue);
	AssertFalse(sBoard.pSimulation->IsPoweringUp());

	sBoard.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemPower(void)
{
	BeginTests();
	DataIOInit();
	DataMemoryInit();

	TestKicadSchemPowerAndReset();
	TestKicadSchemProcessorResetDevice();

	DataMemoryKill();
	DataIOKill();
	TestStatistics();
}
