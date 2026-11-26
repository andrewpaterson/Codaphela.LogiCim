#include "MicroprocessorLib/Voltage.h"
#include "MicroprocessorLib/RCFilter.h"
#include "MicroprocessorLib/AnalogComparator.h"
#include "MicroprocessorLib/Constant.h"
#include "MicroprocessorLib/Pin.h"
#include "KicadImporterLib/KicadSchemPassives.h"
#include "TestKicadSchemBoard.h"


#define KICAD_SCHEM_PASSIVES_SHEET			"Input" _FS_ "Passives.kicad_sch"
#define KICAD_SCHEM_PASSIVES_CLOCK_DOUBLER	"Input" _FS_ "Kicad" _FS_ "Computer" _FS_ "Component Boards" _FS_ "Clock Doubler" _FS_ "Clock Doubler.kicad_sch"
#define KICAD_SCHEM_PASSIVES_PS2_TIMING		"Input" _FS_ "Kicad" _FS_ "Computer" _FS_ "PS2" _FS_ "PS2 Timing" _FS_ "PS2 Timing.kicad_sch"


//////////////////////////////////////////////////////////////////////////
//
// Every way HCP65/Kicad writes a resistor's or a capacitor's value
// (2026-11-26), and what can't be read.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemPassiveValues(void)
{
	AssertDouble(22.0, GetKicadSchemResistance("22Ω"), 6);
	AssertDouble(220.0, GetKicadSchemResistance("220"), 6);
	AssertDouble(1000.0, GetKicadSchemResistance("1.0K"), 6);
	AssertDouble(1000.0, GetKicadSchemResistance("1.0KΩ"), 6);
	AssertDouble(4700.0, GetKicadSchemResistance("4.7KΩ"), 6);
	AssertDouble(10000.0, GetKicadSchemResistance("10K"), 6);
	AssertDouble(1000000.0, GetKicadSchemResistance("1MΩ"), 6);
	AssertDouble(10.0, GetKicadSchemResistance("CAY16-100J4LF"), 6);
	AssertDouble(220.0, GetKicadSchemResistance("CAY16-221J4LF"), 6);
	AssertDouble(2200.0, GetKicadSchemResistance("CAY16-222F4LF"), 6);
	AssertDouble(1000.0, GetKicadSchemResistance("4609X-101-102LF"), 6);
	AssertDouble(1000000.0, GetKicadSchemResistance("4609X-101-105LF"), 6);
	AssertDouble(47.0, GetKicadSchemResistance("4816P-1-470LF"), 6);
	AssertFalse(IsVoltage(GetKicadSchemResistance("R_0805")));
	AssertFalse(IsVoltage(GetKicadSchemResistance("R_Small")));
	AssertFalse(IsVoltage(GetKicadSchemResistance("ERJ-P06F49R9V")));
	AssertFalse(IsVoltage(GetKicadSchemResistance(NULL)));

	AssertDouble(5e-12, GetKicadSchemCapacitance("5pF"), 18);
	AssertDouble(16e-12, GetKicadSchemCapacitance("16pF"), 18);
	AssertDouble(1e-9, GetKicadSchemCapacitance("1nF"), 15);
	AssertDouble(1e-7, GetKicadSchemCapacitance("0.1μF"), 13);
	AssertDouble(1e-6, GetKicadSchemCapacitance("1μF"), 12);
	AssertDouble(1e-6, GetKicadSchemCapacitance("1.0µF"), 12);
	AssertDouble(2.2e-6, GetKicadSchemCapacitance("2.2µF"), 12);
	AssertDouble(1e-3, GetKicadSchemCapacitance("1000μF"), 9);
	AssertDouble(10e-12, GetKicadSchemCapacitance("CA0612KRX7R9BB100"), 18);
	AssertFalse(IsVoltage(GetKicadSchemCapacitance("C_0805")));
	AssertFalse(IsVoltage(GetKicadSchemCapacitance("C_Polarized")));
	AssertFalse(IsVoltage(GetKicadSchemCapacitance("885012207033")));
}


//////////////////////////////////////////////////////////////////////////
//
// The net named szName, as the netlist has it.
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemNet* TestKicadSchemPassivesNet(SKicadSchemTestBoard* psBoard, const char* szName)
{
	CKicadSchemNet*	pcNet;

	pcNet = psBoard->cNetlist.GetNet((char*)szName);
	AssertNotNull(pcNet);
	return pcNet;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the nets named szName and szOther are one, joined by a resistor.
//
//////////////////////////////////////////////////////////////////////////
bool TestKicadSchemPassivesJoined(SKicadSchemTestBoard* psBoard, const char* szName, const char* szOther)
{
	CKicadSchemLogicimModel*	pcModel;

	pcModel = psBoard->cConverter.GetModel();
	return pcModel->GetGroup(TestKicadSchemPassivesNet(psBoard, szName)) == pcModel->GetGroup(TestKicadSchemPassivesNet(psBoard, szOther));
}


//////////////////////////////////////////////////////////////////////////
//
// The test sheet's elements: a resistor is one, an isolated network one a
// pair of its pins, a bussed network none.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemPassiveElements(void)
{
	SKicadSchemTestBoard			sBoard;
	CArrayKicadSchemPassiveElement	asResistors;
	CArrayKicadSchemPassiveElement	asCapacitors;
	SKicadSchemPassiveElement*		psElement;
	size							i;
	size							uiNetwork;

	sBoard.Init(KICAD_SCHEM_PASSIVES_SHEET, "Passives");
	asResistors.Init();
	GetKicadSchemPassiveElements(&sBoard.cFile, &sBoard.cNetlist, true, &asResistors);
	AssertSize(8, asResistors.NumElements());
	uiNetwork = 0;
	for (i = 0; i < asResistors.NumElements(); i++)
	{
		psElement = asResistors.Get(i);
		AssertNotNull(psElement->apcNets[0]);
		AssertNotNull(psElement->apcNets[1]);
		AssertFalse(StringCompare(psElement->pcSymbol->GetReference(), "RN2") == 0);
		if (StringCompare(psElement->pcSymbol->GetReference(), "RN1") == 0)
		{
			AssertString(psElement->apcPins[0]->GetName(), psElement->apcPins[1]->GetName());
			AssertDouble(220.0, psElement->fValue, 6);
			uiNetwork++;
		}
	}
	AssertSize(4, uiNetwork);

	asCapacitors.Init();
	GetKicadSchemPassiveElements(&sBoard.cFile, &sBoard.cNetlist, false, &asCapacitors);
	AssertSize(2, asCapacitors.NumElements());
	asCapacitors.Kill();
	asResistors.Kill();
	sBoard.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The test sheet imported: R1 (22 ohm into C1's 10pF, 0.22ns) joins A to
// A_{s}; R2 (1k into 1nF, 1us) is an RC filter, left apart; RN1's four
// 220 ohms into the wiring's 3pF join D0 to D3 to their D_{s}s; the
// bussed RN2 joins nothing; R3's value can't be read, so it joins, warned;
// R4 is a pull to 3V3, not a series resistor.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemPassiveJoins(void)
{
	SKicadSchemTestBoard		sBoard;
	SKicadSchemLogicimRC*		psRC;
	SKicadSchemLogicimCounts*	psCounts;

	sBoard.Init(KICAD_SCHEM_PASSIVES_SHEET, "Passives");
	psCounts = sBoard.cConverter.GetCounts();
	AssertSize(6, psCounts->uiSeriesResistors);
	AssertSize(1, psCounts->uiRCFilters);
	AssertSize(1, psCounts->uiUnreadResistors);

	AssertTrue(TestKicadSchemPassivesJoined(&sBoard, "A", "A_{s}"));
	AssertTrue(TestKicadSchemPassivesJoined(&sBoard, "D0", "D0_{s}"));
	AssertTrue(TestKicadSchemPassivesJoined(&sBoard, "D3", "D3_{s}"));
	AssertFalse(TestKicadSchemPassivesJoined(&sBoard, "D0", "D1"));
	AssertTrue(TestKicadSchemPassivesJoined(&sBoard, "F", "F_{s}"));
	AssertFalse(TestKicadSchemPassivesJoined(&sBoard, "B", "B_{s}"));
	AssertFalse(TestKicadSchemPassivesJoined(&sBoard, "E", "E0"));
	AssertFalse(TestKicadSchemPassivesJoined(&sBoard, "E0", "E1"));
	AssertFalse(TestKicadSchemPassivesJoined(&sBoard, "G", "3V3"));
	AssertString(sBoard.cConverter.GetModel()->GetNetName(TestKicadSchemPassivesNet(&sBoard, "A")), sBoard.cConverter.GetModel()->GetNetName(TestKicadSchemPassivesNet(&sBoard, "A_{s}")));

	AssertSize(1, sBoard.cConverter.NumRCFilters());
	psRC = sBoard.cConverter.GetRCFilter(0);
	AssertString("R2", psRC->sResistor.pcSymbol->GetReference());
	AssertDouble(1e-9, psRC->fCapacitance, 15);
	AssertString("B_{s}", psRC->sResistor.apcNets[psRC->iLoad]->GetName());
	sBoard.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The trace of the board's pin named szPin.
//
//////////////////////////////////////////////////////////////////////////
CTrace* TestKicadSchemPassivesPinTrace(SKicadSchemTestBoard* psBoard, const char* szPin)
{
	CPin*	pcPin;

	pcPin = (CPin*)psBoard->Get(szPin);
	AssertNotNull(pcPin);
	return pcPin->GetTracePort(0)->GetTrace();
}


//////////////////////////////////////////////////////////////////////////
//
// A one bit constant of szFamily at fVcc, on supplies of its own, driving
// the board's pin named szPin as another board would, from bValue.
//
//////////////////////////////////////////////////////////////////////////
CLogicPort* TestKicadSchemPassivesDrive(SKicadSchemTestBoard* psBoard, const char* szPin, const char* szFamily, float32 fVcc, bool bValue)
{
	CSubcircuitSimulation*	pcTop;
	CTimeline*				pcTimeline;
	CPowerSource*			pcVcc;
	CPowerSource*			pcGnd;
	CConstant*				pcConstant;
	CTrace*					pcVccTrace;
	CTrace*					pcGndTrace;

	pcTop = psBoard->pSimulation->GetSubcircuitTopSimulation();
	pcTimeline = psBoard->pSimulation->GetTimeline();
	pcVccTrace = psBoard->pSimulation->GetCircuit()->AddTrace();
	pcGndTrace = psBoard->pSimulation->GetCircuit()->AddTrace();
	pcVcc = &OMalloc<CPowerSource>(pcTop, "Driver VCC", fVcc);
	pcGnd = &OMalloc<CPowerSource>(pcTop, "Driver GND", 0.0f);
	AssertTrue(pcVcc->GetPowerOutPort()->ConnectLive(pcVccTrace, pcTimeline));
	AssertTrue(pcGnd->GetPowerOutPort()->ConnectLive(pcGndTrace, pcTimeline));
	pcConstant = &OMalloc<CConstant>(pcTop, "Driver", OMalloc<CConstantPins>(psBoard->cFamilies.Get(szFamily), 1), bValue ? 1 : 0);
	AssertTrue(pcConstant->GetPins()->GetVoltageCommon()->ConnectLive(pcVccTrace, pcTimeline));
	AssertTrue(pcConstant->GetPins()->GetVoltageGround()->ConnectLive(pcGndTrace, pcTimeline));
	AssertTrue(pcConstant->GetConstantPins()->GetOutput(0)->ConnectLive(TestKicadSchemPassivesPinTrace(psBoard, szPin), pcTimeline));
	psBoard->pSimulation->GetTimeline()->Run();
	return pcConstant->GetConstantPins()->GetOutput(0);
}


//////////////////////////////////////////////////////////////////////////
//
// The Clock Doubler: Fast CLK through its 74LVC1G17 (a Schmitt buffer) to
// Clean Fast CLK, R1 and C1 (750 ohm into 16pF, 12ns) to the MAX9010's
// IN-, against 1.6V on IN+, and the comparator's OUT XORed with Clean
// Fast CLK.  Each edge of Fast CLK gives a low pulse on 2x Fast CLK until
// the RC passes 1.6V, 12 x ln(2.9 / 1.5) = 7.9ns, and the comparator
// (2.5ns before it moves) and the XOR follow: about 12ns wide.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemPassiveClockDoubler(void)
{
	SKicadSchemTestBoard			sBoard;
	CRCFilter*						pcFilter;
	CIntegratedCircuit*				pcBuffer;
	CLogicPort*						pcClock;
	CTrace*							pcDoubled;
	int64							iEdge;
	int64							iFall;
	int64							iRise;
	int64							iTime;
	size							i;
	float32							fVoltage;

	sBoard.Init(KICAD_SCHEM_PASSIVES_CLOCK_DOUBLER, "Clock Doubler");
	AssertSize(1, sBoard.cConverter.GetCounts()->uiRCFilters);
	pcFilter = (CRCFilter*)sBoard.Get("R1");
	AssertNotNull(pcFilter);
	AssertString(RC_FILTER_TYPE, pcFilter->GetType());
	AssertTrue((pcFilter->GetRiseTimeConstant() >= NanosecondsToTime((int64)12) - 2) && (pcFilter->GetRiseTimeConstant() <= NanosecondsToTime((int64)12) + 2));
	AssertString(ANALOG_COMPARATOR_TYPE, ((CIntegratedCircuit*)sBoard.Get("IC2"))->GetType());
	pcBuffer = (CIntegratedCircuit*)sBoard.Get("IC4");
	AssertNotNull(pcBuffer);
	AssertString("LVC 1G17", ((CFamilyVoltageConfiguration*)pcBuffer->GetVoltageConfigurationSource())->GetFamily());

	sBoard.RunTo(3000000);
	pcClock = TestKicadSchemPassivesDrive(&sBoard, "Fast CLK", "LVC", 3.3f, false);
	pcDoubled = TestKicadSchemPassivesPinTrace(&sBoard, "2x Fast CLK");
	AssertTrue(pcDoubled->GetVoltage(sBoard.pSimulation->GetTime()) > 3.0f);

	for (i = 0; i < 2; i++)
	{
		iEdge = sBoard.pSimulation->GetTime();
		pcClock->Write(sBoard.pSimulation->GetTimeline(), i == 0);
		iFall = -1;
		iRise = -1;
		for (iTime = iEdge; iTime < iEdge + NanosecondsToTime((int64)60); iTime += NanosecondsToTime(0.25))
		{
			sBoard.pSimulation->RunToTime(iTime - sBoard.pSimulation->GetTime());
			fVoltage = pcDoubled->GetVoltage(iTime);
			if ((iFall < 0) && (fVoltage < 1.65f))
			{
				iFall = iTime;
			}
			else if ((iFall >= 0) && (iRise < 0) && (fVoltage > 1.65f))
			{
				iRise = iTime;
			}
		}
		AssertTrue(iFall >= 0);
		AssertTrue(iRise >= 0);
		AssertTrue(iRise - iFall > NanosecondsToTime((int64)9));
		AssertTrue(iRise - iFall < NanosecondsToTime((int64)15));
		sBoard.pSimulation->GetTimeline()->Run();
		AssertTrue(pcDoubled->GetVoltage(sBoard.pSimulation->GetTime()) > 3.0f);
	}

	sBoard.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The PS2 Timing: Clock through a 74VHCT14 (Schmitt, at 5V) to ~{Clock},
// R3 (4.7k) and D1's first diode into C3 (0.1uF) on RC Packet Pulse, then
// two more inverters to Packet Pulse.  A burst of eight 40us clocks
// charges C3 through the diode on each low and R3 lets only a little go
// on each high, so Packet Pulse stays high through the burst.  After the
// last, C3 falls along R3 x C3, 470us, from about 4.5V to VT- (1.075V at
// 5V), 470 x ln(4.4 / 0.975) = 708us, and Packet Pulse falls then.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemPassivePS2Timing(void)
{
	SKicadSchemTestBoard	sBoard;
	CRCFilter*				pcFilter;
	CLogicPort*				pcClock;
	CTrace*					pcPacketPulse;
	CTrace*					pcRCPacketPulse;
	CPortOutput*			pcCurve;
	float32					fThreshold;
	int64					iLast;
	int64					iCrossing;
	size					i;

	sBoard.Init(KICAD_SCHEM_PASSIVES_PS2_TIMING, "PS2 Timing");
	pcFilter = (CRCFilter*)sBoard.Get("R3");
	AssertNotNull(pcFilter);
	AssertTrue((pcFilter->GetFallTimeConstant() >= NanosecondsToTime((int64)470000) - 50) && (pcFilter->GetFallTimeConstant() <= NanosecondsToTime((int64)470000) + 50));
	AssertTrue((pcFilter->GetRiseTimeConstant() >= NanosecondsToTime((int64)2500) - 5) && (pcFilter->GetRiseTimeConstant() <= NanosecondsToTime((int64)2500) + 5));
	pcFilter = (CRCFilter*)sBoard.Get("R1");
	AssertNotNull(pcFilter);
	AssertTrue((pcFilter->GetFallTimeConstant() >= NanosecondsToTime((int64)5000000) - 500) && (pcFilter->GetFallTimeConstant() <= NanosecondsToTime((int64)5000000) + 500));
	AssertNull(sBoard.Get("R2"));
	pcFilter = (CRCFilter*)sBoard.Get("R3");

	sBoard.RunTo(3000000);
	pcClock = TestKicadSchemPassivesDrive(&sBoard, "Clock", "VHCT", 5.0f, true);
	sBoard.RunTo(4000000);
	pcPacketPulse = TestKicadSchemPassivesPinTrace(&sBoard, "Packet Pulse");
	pcRCPacketPulse = TestKicadSchemPassivesPinTrace(&sBoard, "RC Packet Pulse");
	AssertTrue(pcPacketPulse->GetVoltage(sBoard.pSimulation->GetTime()) < 0.5f);

	for (i = 0; i < 8; i++)
	{
		pcClock->Write(sBoard.pSimulation->GetTimeline(), false);
		sBoard.pSimulation->RunToTime(NanosecondsToTime((int64)40000));
		AssertTrue(pcPacketPulse->GetVoltage(sBoard.pSimulation->GetTime()) > 4.0f);
		pcClock->Write(sBoard.pSimulation->GetTimeline(), true);
		sBoard.pSimulation->RunToTime(NanosecondsToTime((int64)40000));
		AssertTrue(pcPacketPulse->GetVoltage(sBoard.pSimulation->GetTime()) > 4.0f);
		AssertTrue(pcRCPacketPulse->GetVoltage(sBoard.pSimulation->GetTime()) > 4.0f);
	}
	iLast = sBoard.pSimulation->GetTime() - NanosecondsToTime((int64)40000);

	pcCurve = pcFilter->GetRCFilterPins()->GetOut()->GetOutput();
	AssertTrue(pcCurve->IsCurve());
	AssertTrue(pcCurve->GetTime() - iLast < NanosecondsToTime((int64)30));
	fThreshold = sBoard.cFamilies.GetPart("VHCT", "14")->GetLowVoltageIn(5.0f);
	AssertFloat(1.075f, fThreshold, 3);
	iCrossing = pcCurve->GetTime() + pcCurve->CalculateTimeAtVoltage(pcCurve->GetStartVoltage(), fThreshold);
	AssertTrue(iCrossing - iLast > NanosecondsToTime((int64)700000));
	AssertTrue(iCrossing - iLast < NanosecondsToTime((int64)716000));
	sBoard.pSimulation->RunToTime(iCrossing - NanosecondsToTime((int64)1000) - sBoard.pSimulation->GetTime());
	AssertTrue(pcPacketPulse->GetVoltage(sBoard.pSimulation->GetTime()) > 4.0f);
	sBoard.pSimulation->RunToTime(NanosecondsToTime((int64)2000));
	AssertTrue(pcPacketPulse->GetVoltage(sBoard.pSimulation->GetTime()) < 0.5f);

	sBoard.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemPassives(void)
{
	BeginTests();
	DataIOInit();
	DataMemoryInit();

	TestKicadSchemPassiveValues();
	TestKicadSchemPassiveElements();
	TestKicadSchemPassiveJoins();
	TestKicadSchemPassiveClockDoubler();
	TestKicadSchemPassivePS2Timing();

	DataMemoryKill();
	DataIOKill();
	TestStatistics();
}
