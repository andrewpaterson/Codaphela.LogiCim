#include "MicroprocessorLib/Voltage.h"
#include "KicadImporterLib/KicadSchemPassives.h"
#include "TestKicadSchemBoard.h"


#define KICAD_SCHEM_PASSIVES_SHEET		"Input" _FS_ "Passives.kicad_sch"


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
	AssertSize(1, psCounts->uiSlowRCs);
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

	AssertSize(1, sBoard.cConverter.NumSlowRCs());
	psRC = sBoard.cConverter.GetSlowRC(0);
	AssertString("R2", psRC->sResistor.pcSymbol->GetReference());
	AssertDouble(1e-9, psRC->fCapacitance, 15);
	AssertString("B_{s}", psRC->sResistor.apcNets[psRC->iLoad]->GetName());
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

	DataMemoryKill();
	DataIOKill();
	TestStatistics();
}
