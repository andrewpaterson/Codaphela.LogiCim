#include "BaseLib/FileUtil.h"
#include "BaseLib/TextFile.h"
#include "BaseLib/GlobalDataTypesIO.h"
#include "BaseLib/DataMemory.h"
#include "BaseLib/Chars.h"
#include "BaseLib/ArrayInt.h"
#include "StandardLib/Objects.h"
#include "LogicimLib/CircuitFile.h"
#include "LogicimLib/CircuitFonts.h"
#include "LogicimLib/TunnelView.h"
#include "LogicimLib/SubcircuitInstanceView.h"
#include "KicadLib/KicadSchemSubnetGraph.h"
#include "KicadImporterLib/KicadSchemLogicimConverter.h"
#include "KicadImporterLib/KicadSchemBoards.h"
#include "KicadImporterLib/KicadSchemSupplyNames.h"
#include "TestLib/Assert.h"
#include "TestLib/AssertFile.h"


#define KICAD_SCHEM_LOGICIM_FONTS		"Input" _FS_ "Fonts"
#define KICAD_SCHEM_LOGICIM_TEST_BITS	64


extern char gszKicadSchemTestFile[];
extern char gszKicadSchemBusTestFile[];
extern char gszKicadSchemNetlistTestFile[];


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemLogicFunction(void)
{
	CChars	szFamily;
	CChars	szPartType;

	szFamily.Init();
	AssertInt(KSLF_And, ParseKicadSchemLogicFunction("Nexperia:74LVC08APW,112_Multi", &szFamily));
	AssertString("LVC", szFamily.Text());
	AssertInt(KSLF_And, ParseKicadSchemLogicFunction("Nexperia:74LVC1G08GV,125", &szFamily));
	AssertInt(KSLF_Inverter, ParseKicadSchemLogicFunction("74LVC2G04GW-Q100H_Multi", &szFamily));
	AssertInt(KSLF_Inverter, ParseKicadSchemLogicFunction("Texas_Instruments:SN74LVC14APWRG4 _Multi", &szFamily));
	AssertInt(KSLF_Nand, ParseKicadSchemLogicFunction("74HC00", &szFamily));
	AssertString("LVC", szFamily.Text());
	AssertInt(KSLF_Nand, ParseKicadSchemLogicFunction("Texas_Instruments:SN74F30DR", &szFamily));
	AssertString("F", szFamily.Text());
	AssertInt(KSLF_Or, ParseKicadSchemLogicFunction("74AHCT32", &szFamily));
	AssertString("AHCT", szFamily.Text());
	AssertInt(KSLF_DFlipFlop, ParseKicadSchemLogicFunction("Nexperia:74LVC74APW,118_Multi", &szFamily));
	AssertInt(KSLF_Nor, ParseKicadSchemLogicFunction("74LVC02APW,118", &szFamily));
	AssertInt(KSLF_Xor, ParseKicadSchemLogicFunction("74VHCT86", &szFamily));
	AssertString("VHCT", szFamily.Text());
	AssertInt(KSLF_None, ParseKicadSchemLogicFunction("Nexperia:74LVC574APW,118", &szFamily));
	AssertInt(KSLF_None, ParseKicadSchemLogicFunction("Texas_Instruments:CD74HCT40105M", &szFamily));
	AssertInt(KSLF_None, ParseKicadSchemLogicFunction("Device:R", &szFamily));
	AssertInt(KSLF_None, ParseKicadSchemLogicFunction("W65C02S", &szFamily));
	AssertInt(KSLF_None, ParseKicadSchemLogicFunction("1748", &szFamily));
	AssertInt(KSLF_None, ParseKicadSchemLogicFunction(NULL, &szFamily));
	szFamily.Kill();

	szFamily.Init();
	szPartType.Init();
	AssertInt(KSLF_Inverter, ParseKicadSchemLogicFunction("Nexperia:74LVC2G04GW-Q100H_Multi", &szFamily, &szPartType));
	AssertString("2G04", szPartType.Text());
	AssertInt(KSLF_Inverter, ParseKicadSchemLogicFunction("SN74LVC14APWR", &szFamily, &szPartType));
	AssertString("14", szPartType.Text());
	AssertString("LVC", szFamily.Text());
	szPartType.Kill();
	szFamily.Kill();

	AssertString("Nand", KicadSchemLogicFunctionName(KSLF_Nand));
	AssertTrue(IsKicadSchemLogicimFamily("HCT"));
	AssertFalse(IsKicadSchemLogicimFamily("HC"));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemLogicimNames(void)
{
	AssertTrue(IsKicadSchemGroundName("GND"));
	AssertTrue(IsKicadSchemGroundName("gnda"));
	AssertTrue(IsKicadSchemGroundName("VSS"));
	AssertFalse(IsKicadSchemGroundName("VCC"));
	AssertFalse(IsKicadSchemGroundName("+5V"));

	AssertFloat(5.0f, GetKicadSchemSupplyVoltage("+5V"), 4);
	AssertFloat(3.3f, GetKicadSchemSupplyVoltage("+3V3"), 4);
	AssertFloat(3.3f, GetKicadSchemSupplyVoltage("3.3V"), 4);
	AssertFloat(12.0f, GetKicadSchemSupplyVoltage("12VOut"), 4);
	AssertFloat(5.0f, GetKicadSchemSupplyVoltage("VCC"), 4);

	AssertInt(ROT_East, GetKicadSchemRotation(ROT_East, 0));
	AssertInt(ROT_North, GetKicadSchemRotation(ROT_East, 90));
	AssertInt(ROT_West, GetKicadSchemRotation(ROT_East, 180));
	AssertInt(ROT_South, GetKicadSchemRotation(ROT_East, 270));
	AssertInt(ROT_South, GetKicadSchemRotation(ROT_East, -90));
	AssertInt(ROT_West, GetKicadSchemRotation(ROT_North, 90));
	AssertInt(ROT_East, GetKicadSchemRotation(ROT_South, 90));
}


//////////////////////////////////////////////////////////////////////////
//
// An L of two wires from (0, 0) right to (10, 0) and down to (10, 20),
// with a pin on its middle at (10, 5).
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemSubnetGraph(void)
{
	CKicadSchemSubnet		cSubnet;
	CKicadSchemWire			cAcross;
	CKicadSchemWire			cDown;
	CKicadSchemSubnetGraph	cGraph;
	SFloat64Vec2			sStart;
	SFloat64Vec2			sCorner;
	SFloat64Vec2			sEnd;
	SFloat64Vec2			sMiddle;
	size					uiStart;
	size					uiEnd;
	size					uiMiddle;
	CArrayInt				aiTargets;
	CArrayInt				aiPath;
	CArrayFloat64Vec2		asPath;
	size					i;
	bool					bHorizontal;

	sStart.Init(0, 0);
	sCorner.Init(10, 0);
	sEnd.Init(10, 20);
	sMiddle.Init(10, 5);
	cAcross.Init(&sStart, &sCorner, false);
	cDown.Init(&sCorner, &sEnd, false);
	cSubnet.Init();
	cSubnet.AddWire(&cAcross);
	cSubnet.AddWire(&cDown);

	cGraph.Init(&cSubnet);
	uiStart = cGraph.AddPoint(&sStart);
	uiEnd = cGraph.AddPoint(&sEnd);
	uiMiddle = cGraph.AddPoint(&sMiddle);
	AssertSize(uiStart, cGraph.AddPoint(&sStart));
	cGraph.Build();
	AssertSize(4, cGraph.NumPoints());
	AssertSize(3, cGraph.NumEdges());

	aiTargets.Init();
	aiTargets.Add((int)uiEnd);
	aiPath.Init();
	AssertTrue(cGraph.FindPath(uiStart, &aiTargets, &aiPath));
	AssertSize(4, aiPath.NumElements());
	AssertInt((int)uiStart, aiPath.GetValue(0));
	AssertInt((int)uiMiddle, aiPath.GetValue(2));
	AssertInt((int)uiEnd, aiPath.GetValue(3));

	asPath.Init();
	for (i = 0; i < aiPath.NumElements(); i++)
	{
		asPath.Add(cGraph.GetPoint(aiPath.GetValue(i)));
	}
	AssertDouble(1.0, CKicadSchemSubnetGraph::GetBendFraction(&asPath, &bHorizontal), 6);
	AssertTrue(bHorizontal);
	asPath.Kill();

	//Back up from the end: down (up the page) first, turning at the top, a whole way up.
	aiTargets.ReInit();
	aiTargets.Add((int)uiStart);
	aiPath.ReInit();
	AssertTrue(cGraph.FindPath(uiEnd, &aiTargets, &aiPath));
	asPath.Init();
	for (i = 0; i < aiPath.NumElements(); i++)
	{
		asPath.Add(cGraph.GetPoint(aiPath.GetValue(i)));
	}
	AssertDouble(1.0, CKicadSchemSubnetGraph::GetBendFraction(&asPath, &bHorizontal), 6);
	AssertFalse(bHorizontal);
	asPath.Kill();

	//A Z from (0, 0) to (20, 10) turning down at x 5.
	asPath.Init();
	sStart.Init(0, 0);
	asPath.Add(&sStart);
	sStart.Init(5, 0);
	asPath.Add(&sStart);
	sStart.Init(5, 10);
	asPath.Add(&sStart);
	sStart.Init(20, 10);
	asPath.Add(&sStart);
	AssertDouble(0.25, CKicadSchemSubnetGraph::GetBendFraction(&asPath, &bHorizontal), 6);
	AssertTrue(bHorizontal);
	asPath.Kill();

	asPath.Init();
	asPath.Add(&sStart);
	AssertDouble(0.5, CKicadSchemSubnetGraph::GetBendFraction(&asPath, &bHorizontal), 6);
	asPath.Kill();

	aiPath.Kill();
	aiTargets.Kill();
	cGraph.Kill();
	cSubnet.Kill();
	cDown.Kill();
	cAcross.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The port as "owner.port (x, y)".
//
//////////////////////////////////////////////////////////////////////////
void AppendKicadSchemLogicimPort(CChars* psz, CPortView* pcPortView)
{
	psz->Append(pcPortView->GetOwner()->GetName());
	psz->Append('.');
	psz->Append(pcPortView->GetPortName(0));
	psz->Append(" (");
	psz->Append(pcPortView->GetGridPosition().x);
	psz->Append(", ");
	psz->Append(pcPortView->GetGridPosition().y);
	psz->Append(')');
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit's ports joined as Logicim's wiring joins them (see
// CCircuitWiring::JoinLevel): wires that touch, ports on wires, ports at
// the same place and ports of tunnels with the same name; and their bits
// where splitters join them.  Every subnet's terminals must be joined, and
// no bit can be of two different nets.
//
//////////////////////////////////////////////////////////////////////////
void AssertKicadSchemLogicimWired(CKicadSchemLogicimConverter* pcConverter)
{
	CCircuitView*					pcCircuit;
	CKicadSchemLogicimModel*		pcModel;
	CArrayPortViewPtr				apcPorts;
	CArrayInt						aiParents;
	CArrayTemplatePtr<CKicadSchemNet>	apcGroupNets;
	CArrayInt						aiSubnetGroups;
	size							uiTraces;
	size							i;
	size							j;
	const char*						szName;
	const char*						szOtherName;
	SKicadSchemLogicimTerminal*		psTerminal;
	int								iGroup;
	CKicadSchemNet*					pcNet;
	size							uiSubnet;
	CChars							szFirst;
	CChars							szThis;
	CArrayInt						aiBits;
	CArrayPortBitJoin				asJoins;
	SPortBitJoin*					psJoin;
	int								iBit;

	pcCircuit = pcConverter->GetCircuit();
	pcModel = pcConverter->GetModel();
	apcPorts.Init();
	pcCircuit->CollectPortViews(&apcPorts);
	uiTraces = pcCircuit->NumTraces();
	aiParents.Init();
	for (i = 0; i < uiTraces + apcPorts.NumElements(); i++)
	{
		aiParents.Add((int)i);
	}
	for (i = 0; i < uiTraces; i++)
	{
		for (j = i + 1; j < uiTraces; j++)
		{
			if (pcCircuit->GetTrace(i)->Touches(pcCircuit->GetTrace(j)))
			{
				JoinKicadSchemRoots(&aiParents, (int)i, (int)j);
			}
		}
	}
	for (i = 0; i < apcPorts.NumElements(); i++)
	{
		for (j = 0; j < uiTraces; j++)
		{
			if (pcCircuit->GetTrace(j)->Contains(apcPorts.GetPtr(i)->GetGridPosition()))
			{
				JoinKicadSchemRoots(&aiParents, (int)(uiTraces + i), (int)j);
			}
		}
		szName = apcPorts.GetPtr(i)->GetOwner()->GetConnectionName();
		for (j = i + 1; j < apcPorts.NumElements(); j++)
		{
			szOtherName = apcPorts.GetPtr(j)->GetOwner()->GetConnectionName();
			if ((apcPorts.GetPtr(i)->GetGridPosition() == apcPorts.GetPtr(j)->GetGridPosition()) ||
				((szName != NULL) && (szOtherName != NULL) && (StringCompare(szName, szOtherName) == 0)))
			{
				JoinKicadSchemRoots(&aiParents, (int)(uiTraces + i), (int)(uiTraces + j));
			}
		}
	}

	//The bits of each group, joined where components (splitters) join them.
	aiBits.Init();
	for (i = 0; i < aiParents.NumElements() * KICAD_SCHEM_LOGICIM_TEST_BITS; i++)
	{
		aiBits.Add((int)i);
	}
	for (i = 0; i < pcCircuit->NumComponents(); i++)
	{
		asJoins.Init();
		pcCircuit->GetComponent(i)->GetBitJoins(&asJoins);
		for (j = 0; j < asJoins.NumElements(); j++)
		{
			psJoin = asJoins.Get(j);
			AssertTrue((psJoin->uiFromBit < KICAD_SCHEM_LOGICIM_TEST_BITS) && (psJoin->uiToBit < KICAD_SCHEM_LOGICIM_TEST_BITS));
			JoinKicadSchemRoots(&aiBits,
				FindKicadSchemRoot(&aiParents, (int)(uiTraces + apcPorts.Find(psJoin->pcFrom))) * KICAD_SCHEM_LOGICIM_TEST_BITS + (int)psJoin->uiFromBit,
				FindKicadSchemRoot(&aiParents, (int)(uiTraces + apcPorts.Find(psJoin->pcTo))) * KICAD_SCHEM_LOGICIM_TEST_BITS + (int)psJoin->uiToBit);
		}
		asJoins.Kill();
	}

	apcGroupNets.Init();
	for (i = 0; i < aiBits.NumElements(); i++)
	{
		apcGroupNets.Add(NULL);
	}
	aiSubnetGroups.Init();
	for (i = 0; i < pcModel->NumTerminals(); i++)
	{
		aiSubnetGroups.Add(-1);
	}
	for (i = 0; i < pcModel->NumTerminals(); i++)
	{
		psTerminal = pcModel->GetTerminal(i);
		iGroup = FindKicadSchemRoot(&aiParents, (int)(uiTraces + apcPorts.Find(psTerminal->pcPortView)));

		//Nothing of another net, bit for bit (a bus's own terminals are many nets).
		pcNet = psTerminal->pcSubnet->GetNet();
		iBit = FindKicadSchemRoot(&aiBits, iGroup * KICAD_SCHEM_LOGICIM_TEST_BITS);
		if (!psTerminal->bBus)
		{
			if (apcGroupNets.GetPtr(iBit) == NULL)
			{
				apcGroupNets.SetPtr(iBit, pcNet);
			}
		}
		if ((!psTerminal->bBus) && (apcGroupNets.GetPtr(iBit) != pcNet))
		{
			//Names the two nets that are joined.
			szFirst.Init(pcConverter->GetCircuit()->GetTypeName());
			szFirst.Append(": ");
			szFirst.Append(apcGroupNets.GetPtr(iBit)->GetName());
			szThis.Init(pcNet->GetName());
			szThis.Append(" at ");
			AppendKicadSchemLogicimPort(&szThis, psTerminal->pcPortView);
			AssertString(szFirst.Text(), szThis.Text());
		}

		//All of its subnet: the first terminal of the subnet's group.
		for (j = 0; j < i; j++)
		{
			if (pcModel->GetTerminal(j)->pcSubnet == psTerminal->pcSubnet)
			{
				break;
			}
		}
		uiSubnet = j;
		if (uiSubnet == i)
		{
			aiSubnetGroups.SetValue(i, iGroup);
		}
		else if (aiSubnetGroups.GetValue(uiSubnet) != iGroup)
		{
			//Names the two terminals that aren't wired together.
			szFirst.Init(pcConverter->GetCircuit()->GetTypeName());
			szFirst.Append(": ");
			AppendKicadSchemLogicimPort(&szFirst, pcModel->GetTerminal(uiSubnet)->pcPortView);
			szThis.Init();
			AppendKicadSchemLogicimPort(&szThis, psTerminal->pcPortView);
			AssertString(szFirst.Text(), szThis.Text());
		}
	}

	aiSubnetGroups.Kill();
	apcGroupNets.Kill();
	aiBits.Kill();
	aiParents.Kill();
	apcPorts.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CComponentView* FindKicadSchemLogicimComponent(CCircuitView* pcCircuit, const char* szName)
{
	size	i;

	for (i = 0; i < pcCircuit->NumComponents(); i++)
	{
		if (StringCompare(pcCircuit->GetComponent(i)->GetName(), szName) == 0)
		{
			return pcCircuit->GetComponent(i);
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The converted Output/name.xml is the same, byte for byte, as the
// expected Input/Logicim/name.xml.
//
//////////////////////////////////////////////////////////////////////////
void AssertKicadSchemLogicimExpected(const char* szName)
{
	CChars	szExpected;
	CChars	szActual;

	szExpected.Init("Input" _FS_ "Logicim" _FS_);
	szExpected.Append(szName);
	szExpected.Append(".xml");
	szActual.Init("Output" _FS_);
	szActual.Append(szName);
	szActual.Append(".xml");
	AssertFile(szExpected.Text(), szActual.Text());
	szActual.Kill();
	szExpected.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Converts the schematic, checks its wiring and saves it as
// Output/name.xml, which must be the expected file and load back.
//
//////////////////////////////////////////////////////////////////////////
void AssertKicadSchemLogicimConverts(char* szName, size uiMinimumGates, size uiMinimumPlaceholders, const char* szFirstGate, char* szText = NULL, size uiPowers = 0, size uiSplitters = 0, size uiBoardPins = 0)
{
	CKicadSchemFile					cFile;
	CKicadSchemNetlist				cNetlist;
	Ptr<CCircuitFonts>				pFonts;
	CCircuitViews					cCircuits;
	CCircuitViews					cLoaded;
	CKicadSchemLogicimConverter		cConverter;
	CChars							szPath;
	CFileUtil						cFileUtil;
	CCircuitView*					pcLoaded;
	CChars							szUnconnected;
	size							i;
	SKicadSchemLogicimConnection*	psConnection;
	SKicadSchemLogicimCounts*		psCounts;
	CKicadSchemLogicimModel*		pcModel;

	ObjectsInit();
	pFonts = LoadCircuitFonts(KICAD_SCHEM_LOGICIM_FONTS, CCO_Unknown);
	AssertTrue(pFonts->HasFamilies());
	ORoot()->Add(pFonts);

	szPath.Init("Input" _FS_);
	szPath.Append(szName);
	szPath.Append(".kicad_sch");
	cFile.Init();
	if (szText == NULL)
	{
		AssertTrue(cFile.Load(szPath.Text()));
	}
	else
	{
		AssertTrue(cFile.Import(szText, strlen(szText), szPath.Text()));
	}
	cNetlist.Init(&cFile);
	AssertTrue(cNetlist.Build());

	cCircuits.Init();
	cConverter.Init(&cFile, &cNetlist, &cCircuits, &pFonts);
	AssertTrue(cConverter.Convert(szName));
	psCounts = cConverter.GetCounts();
	pcModel = cConverter.GetModel();
	AssertTrue(psCounts->uiGates >= uiMinimumGates);
	AssertTrue(psCounts->uiPlaceholders >= uiMinimumPlaceholders);
	AssertSize(uiBoardPins, psCounts->uiBoardPins);
	if (szText == NULL)
	{
		AssertTrue(pcModel->NumConnections() > 0);
	}
	AssertSize(0, psCounts->uiOverlaps);
	AssertSize(uiPowers, psCounts->uiPowers);
	AssertSize(uiSplitters, psCounts->uiSplitters);
	AssertTrue(psCounts->uiExcessKnotsAfter <= psCounts->uiExcessKnotsBefore);
	if (szFirstGate != NULL)
	{
		AssertNotNull(FindKicadSchemLogicimComponent(cConverter.GetCircuit(), szFirstGate));
	}

	cFileUtil.MakeDir("Output");
	szPath.Kill();
	szPath.Init("Output" _FS_);
	szPath.Append(szName);
	szPath.Append(".xml");
	AssertTrue(SaveCircuitsFile(&cCircuits, cConverter.GetCircuit(), NULL, szPath.Text()));
	AssertKicadSchemLogicimExpected(szName);

	szUnconnected.Init();
	for (i = 0; i < pcModel->NumConnections(); i++)
	{
		psConnection = pcModel->GetConnection(i);
		if (!psConnection->bConnected)
		{
			AppendKicadSchemLogicimPort(&szUnconnected, pcModel->GetTerminal(psConnection->uiFrom)->pcPortView);
			szUnconnected.Append(" to ");
			AppendKicadSchemLogicimPort(&szUnconnected, pcModel->GetTerminal(psConnection->uiTo)->pcPortView);
			szUnconnected.Append("; ");
		}
	}
	AssertString("", szUnconnected.Text());
	szUnconnected.Kill();
	AssertSize(0, psCounts->uiUnconnected);
	AssertKicadSchemLogicimWired(&cConverter);

	cLoaded.Init();
	AssertTrue(LoadCircuitsFile(&cLoaded, szName, NULL, NULL, szPath.Text()));
	AssertSize(cCircuits.NumCircuits(), cLoaded.NumCircuits());
	pcLoaded = cLoaded.Get(szName);
	AssertNotNull(pcLoaded);
	AssertSize(cConverter.GetCircuit()->NumComponents(), pcLoaded->NumComponents());
	AssertSize(cConverter.GetCircuit()->NumTraces(), pcLoaded->NumTraces());
	cLoaded.Kill();

	cConverter.Kill();
	cCircuits.Kill();
	cNetlist.Kill();
	cFile.Kill();
	szPath.Kill();

	ORoot()->Remove(pFonts);
	pFonts = NULL;
	ORoot()->RemoveAll();
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// How many times szText is in the saved circuit.
//
//////////////////////////////////////////////////////////////////////////
void AssertKicadSchemLogicimAttributes(char* szName, char* szText, int iExpected)
{
	CTextFile	cFile;
	CChars		szPath;
	int			iCount;
	char*		sz;

	szPath.Init("Output" _FS_);
	szPath.Append(szName);
	szPath.Append(".xml");
	cFile.Init();
	AssertTrue(cFile.Read(szPath.Text()));
	iCount = 0;
	for (sz = strstr(cFile.Text(), szText); sz != NULL; sz = strstr(sz + 1, szText))
	{
		iCount++;
	}
	AssertInt(iExpected, iCount);
	cFile.Kill();
	szPath.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// How many of the element the converted Output/name.xml has.
//
//////////////////////////////////////////////////////////////////////////
void AssertKicadSchemLogicimElements(char* szName, char* szElement, int iExpected)
{
	CTextFile	cFile;
	CChars		szPath;
	CChars		szTag;
	int			iCount;
	char*		sz;

	szPath.Init("Output" _FS_);
	szPath.Append(szName);
	szPath.Append(".xml");
	cFile.Init();
	AssertTrue(cFile.Read(szPath.Text()));
	szTag.Init("<");
	szTag.Append(szElement);
	szTag.Append(' ');
	iCount = 0;
	for (sz = strstr(cFile.Text(), szTag.Text()); sz != NULL; sz = strstr(sz + 1, szTag.Text()))
	{
		iCount++;
	}
	AssertInt(iExpected, iCount);
	szTag.Kill();
	cFile.Kill();
	szPath.Kill();
}


//A 5V regulator whose output is labelled VCC, so it reaches the hidden VCC pin of a 74LVC1G04, with a ground on only the
//regulator's ground pin.
char gszKicadSchemRegulatorTestFile[] =
	"(kicad_sch (version 20250114) (generator \"eeschema\") (uuid \"root\") (paper \"A4\")\n"
	"	(lib_symbols\n"
	"		(symbol \"Regulator_Linear:Test5V\" (property \"Reference\" \"U\") (property \"Value\" \"Test5V\")\n"
	"			(symbol \"Test5V_1_1\"\n"
	"				(pin power_in line (at -7.62 0 0) (length 2.54) (name \"VIn\") (number \"1\"))\n"
	"				(pin power_out line (at 7.62 0 180) (length 2.54) (name \"5VOut\") (number \"3\"))\n"
	"				(pin power_in line (at 0 -7.62 90) (length 2.54) (name \"GND\") (number \"2\"))))\n"
	"		(symbol \"Nexperia:74LVC1G04\" (property \"Reference\" \"U\") (property \"Value\" \"74LVC1G04\")\n"
	"			(symbol \"74LVC1G04_1_1\"\n"
	"				(pin input line (at -5.08 0 0) (length 2.54) (name \"A\") (number \"2\"))\n"
	"				(pin output line (at 5.08 0 180) (length 2.54) (name \"Y\") (number \"4\"))\n"
	"				(pin power_in line (at 0 5.08 270) (length 2.54) (hide yes) (name \"VCC\") (number \"5\"))\n"
	"				(pin power_in line (at 0 -5.08 90) (length 2.54) (hide yes) (name \"GND\") (number \"3\"))))\n"
	"		(symbol \"power:GND\" (power) (property \"Reference\" \"#PWR\") (property \"Value\" \"GND\")\n"
	"			(symbol \"GND_1_1\" (pin power_in line (at 0 0 270) (length 0) (hide yes) (name \"GND\") (number \"1\")))))\n"
	"	(symbol (lib_id \"Regulator_Linear:Test5V\") (at 50.8 50.8 0) (unit 1) (uuid \"u1\") (property \"Reference\" \"U1\") (property \"Value\" \"Test5V\"))\n"
	"	(symbol (lib_id \"Nexperia:74LVC1G04\") (at 101.6 50.8 0) (unit 1) (uuid \"u2\") (property \"Reference\" \"U2\") (property \"Value\" \"74LVC1G04\"))\n"
	"	(symbol (lib_id \"power:GND\") (at 50.8 58.42 0) (unit 1) (uuid \"p1\") (property \"Reference\" \"#PWR01\") (property \"Value\" \"GND\"))\n"
	"	(wire (pts (xy 58.42 50.8) (xy 66.04 50.8)) (uuid \"w1\"))\n"
	"	(label \"VCC\" (at 66.04 50.8 0) (uuid \"l1\"))\n"
	")\n";


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemLogicimConvert(void)
{
	//Its ground is on no wire, only the hidden power pins of the 74HC00 (which has implied power), so it isn't made.
	AssertKicadSchemLogicimConverts("Importer Test", 2, 0, "U1", gszKicadSchemTestFile, 0);
	//Its VCC symbol is on no wire, only U1's hidden VCC pin, so it isn't made.
	AssertKicadSchemLogicimConverts("Netlist Test", 0, 1, NULL, gszKicadSchemNetlistTestFile, 0);
	AssertKicadSchemLogicimConverts("Bus Test", 0, 2, NULL, gszKicadSchemBusTestFile, 0, 3);
	AssertKicadSchemLogicimConverts("Clock Oscillator", 0, 1, NULL, NULL, 0, 0, 4);
	AssertKicadSchemLogicimConverts("Clock Hold V1 to V2", 0, 2, NULL, NULL, 0, 1, 25);
	AssertKicadSchemLogicimConverts("LSF0204", 0, 1, NULL);
	AssertKicadSchemLogicimConverts("LED Display", 0, 2, NULL);
	//Its L7805 isn't drawn (a regulator gives its voltage to the ICs it powers).
	AssertKicadSchemLogicimConverts("LCD Dual 7 Segment Tester", 0, 1, NULL, NULL, 0, 0, 9);
	//Its three regulators aren't drawn.
	AssertKicadSchemLogicimConverts("Power and Reset", 0, 1, NULL, NULL, 0, 0, 32);
	//Its AP7365 isn't drawn.
	AssertKicadSchemLogicimConverts("HCP65 MPU NMI", 10, 0, "IC8", NULL, 0, 0, 25);
	//Its AP7365 isn't drawn.
	AssertKicadSchemLogicimConverts("Triple MPU Clock Generator", 8, 1, NULL, NULL, 0, 0, 53);
	//Its AP7365 isn't drawn.
	AssertKicadSchemLogicimConverts("Flags Enable Interrupt", 0, 0, NULL, NULL, 0, 4, 26);
	//Its LD1117S50C isn't drawn.
	AssertKicadSchemLogicimConverts("UART Board", 0, 1, NULL, NULL, 0, 10, 90);

	//A 74 series part drawn as units is units, and drawn whole is a package.
	AssertKicadSchemLogicimElements("HCP65 MPU NMI", "Logic74Unit", 15);
	AssertKicadSchemLogicimElements("HCP65 MPU NMI", "Logic74Package", 6);
	AssertKicadSchemLogicimElements("Flags Enable Interrupt", "Logic74Unit", 11);
	AssertKicadSchemLogicimElements("Flags Enable Interrupt", "Logic74Package", 3);
	AssertKicadSchemLogicimElements("Triple MPU Clock Generator", "Logic74Package", 3);
	AssertKicadSchemLogicimElements("Triple MPU Clock Generator", "Logic74Unit", 14);

	//Voltage regulators (an LD1117S33C, an LD1117S50C and an L7812) aren't drawn.
	AssertKicadSchemLogicimElements("Power and Reset", "PositivePower", 0);

	//A regulator isn't drawn: the IC its output powers gets its voltage as its implied Vcc.  The ground on only the
	//regulator's pin, and the VCC label's tunnel on a wire to nothing drawn, aren't made, so neither is the wire.
	AssertKicadSchemLogicimConverts("Regulator Test", 1, 0, NULL, gszKicadSchemRegulatorTestFile, 0);
	AssertKicadSchemLogicimElements("Regulator Test", "PositivePower", 0);
	AssertKicadSchemLogicimElements("Regulator Test", "Ground", 0);
	AssertKicadSchemLogicimElements("Regulator Test", "Tunnel", 0);
	AssertKicadSchemLogicimElements("Regulator Test", "Trace", 0);
	AssertKicadSchemLogicimAttributes("Regulator Test", "impliedVCC=\"5", 1);
}


//////////////////////////////////////////////////////////////////////////
//
// The board's circuit pin for its connector's pin, and that it's the
// instance's pin szPinName.
//
//////////////////////////////////////////////////////////////////////////
void AssertKicadSchemBoardPin(CKicadSchemBoard* pcBoard, size uiSymbolPins, const char* szConnector, const char* szNumber, CSubcircuitInstanceView* pcInstance, const char* szPinName)
{
	CKicadSchemBoardConnector*	pcConnector;
	CPinView*					pcPin;

	pcConnector = pcBoard->FindConnector(uiSymbolPins);
	AssertNotNull(pcConnector);
	AssertString(szConnector, pcConnector->GetReference());
	pcPin = pcConnector->GetPin(szNumber);
	AssertNotNull(pcPin);
	AssertString(szPinName, pcPin->GetName());
	AssertNotNull(pcInstance->GetPin(pcPin));
}


//////////////////////////////////////////////////////////////////////////
//
// The Audio Board, with the boards drawn on it made from their own
// schematics (by their symbols' Schematic properties) and joined by pin
// number, each made once.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemLogicimBoards(void)
{
	Ptr<CCircuitFonts>			pFonts;
	CKicadPathVariables			cVariables;
	CCircuitViews				cCircuits;
	CCircuitViews				cLoaded;
	CKicadSchemBoardImporter	cImporter;
	CCircuitView*				pcTop;
	CKicadSchemBoard*			pcBoard;
	CComponentView*				pcComponent;
	CSubcircuitInstanceView*	pcInstance;
	size						i;
	size						uiInstances;
	CFileUtil					cFileUtil;
	CChars						szProblems;

	ObjectsInit();
	pFonts = LoadCircuitFonts(KICAD_SCHEM_LOGICIM_FONTS, CCO_Unknown);
	ORoot()->Add(pFonts);

	cVariables.Init();
	cVariables.Set("_658_COMPUTER_", "Input");
	cCircuits.Init();
	cImporter.Init(&cCircuits, &pFonts, &cVariables);
	pcTop = cImporter.Import("${_658_COMPUTER_}/Kicad/Computer/Audio/Audio Board/Audio Board.kicad_sch");
	AssertNotNull(pcTop);
	AssertString("Audio Board", pcTop->GetTypeName());

	//The Audio Board and the 12 boards on it, each once though Selector 12bit, Memory 32KB and Comparator 16bit are drawn twice.
	AssertSize(13, cImporter.NumBoards());
	szProblems.Init();
	for (i = 0; i < cImporter.NumBoards(); i++)
	{
		pcBoard = cImporter.GetBoard(i);
		AssertNotNull(pcBoard->GetCircuit());
		if ((pcBoard->GetCounts()->uiOverlaps != 0) || (pcBoard->GetCounts()->uiUnconnected != 0))
		{
			szProblems.Append(pcBoard->GetCircuit()->GetTypeName());
			szProblems.Append(": ");
			szProblems.Append(pcBoard->GetCounts()->uiOverlaps);
			szProblems.Append(" overlaps, ");
			szProblems.Append(pcBoard->GetCounts()->uiUnconnected);
			szProblems.Append(" unconnected; ");
		}
	}
	AssertString("", szProblems.Text());
	szProblems.Kill();
	pcBoard = cImporter.GetBoardForCircuit(pcTop);
	AssertSize(15, pcBoard->GetCounts()->uiBoards);

	uiInstances = 0;
	for (i = 0; i < pcTop->NumComponents(); i++)
	{
		pcComponent = pcTop->GetComponent(i);
		if (StringCompare(pcComponent->GetXMLName(), "Subcircuit") == 0)
		{
			pcInstance = (CSubcircuitInstanceView*)pcComponent;
			if (cImporter.GetBoardForCircuit(pcInstance->GetInstanceCircuitView()) != NULL)
			{
				uiInstances++;
			}
		}
	}
	AssertSize(15, uiInstances);

	//Joined by number: the Audio Board's symbol calls pin 7 Clk_Sel_{S1}, the Audio Clock's net is "Clock Select _{S1}".
	pcInstance = (CSubcircuitInstanceView*)FindKicadSchemLogicimComponent(pcTop, "B13");
	AssertNotNull(pcInstance);
	AssertString("Audio Clock", pcInstance->GetInstanceCircuitView()->GetTypeName());
	pcBoard = cImporter.GetBoardForCircuit(pcInstance->GetInstanceCircuitView());
	AssertKicadSchemBoardPin(pcBoard, 32, "J2", "7", pcInstance, "Clock Select _{S1}");
	AssertKicadSchemBoardPin(pcBoard, 32, "J2", "6", pcInstance, "~{Reset}");
	AssertNull(pcBoard->FindConnector(32)->GetPin("1"));  //5V.
	AssertNull(pcBoard->FindConnector(32)->GetPin("5"));  //GND.

	//The Audio DAC's symbol is its 32 pin edge connector, J1, not its audio jack, J2.
	pcInstance = (CSubcircuitInstanceView*)FindKicadSchemLogicimComponent(pcTop, "B9");
	AssertNotNull(pcInstance);
	pcBoard = cImporter.GetBoardForCircuit(pcInstance->GetInstanceCircuitView());
	AssertKicadSchemBoardPin(pcBoard, 32, "J1", "4", pcInstance, "~{Enable}");

	//Both Selector 12bits are instances of one circuit.
	AssertTrue(((CSubcircuitInstanceView*)FindKicadSchemLogicimComponent(pcTop, "B3"))->GetInstanceCircuitView() == ((CSubcircuitInstanceView*)FindKicadSchemLogicimComponent(pcTop, "B5"))->GetInstanceCircuitView());

	cFileUtil.MakeDir("Output");
	AssertTrue(SaveCircuitsFile(&cCircuits, pcTop, NULL, "Output" _FS_ "Audio Board.xml"));
	AssertKicadSchemLogicimExpected("Audio Board");
	cLoaded.Init();
	AssertTrue(LoadCircuitsFile(&cLoaded, "Audio Board", NULL, NULL, "Output" _FS_ "Audio Board.xml"));
	AssertSize(cCircuits.NumCircuits(), cLoaded.NumCircuits());
	AssertSize(pcTop->NumComponents(), cLoaded.Get("Audio Board")->NumComponents());
	AssertSize(pcTop->NumTraces(), cLoaded.Get("Audio Board")->NumTraces());
	cLoaded.Kill();

	//A board that isn't there is a placeholder.
	AssertNull(cImporter.Import("${_658_COMPUTER_}/Kicad/Computer/Nothing/Nothing.kicad_sch"));
	AssertNull(cImporter.Import("${_NOT_SET_}/Nothing.kicad_sch"));

	cImporter.Kill();
	cCircuits.Kill();
	cVariables.Kill();

	ORoot()->Remove(pFonts);
	pFonts = NULL;
	ORoot()->RemoveAll();
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// KiCad's path variables, from kicad_common.json's text.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadPathVariables(void)
{
	CKicadPathVariables	cVariables;
	CChars				szPath;
	CChars				szRoot;
	CChars				szExpected;
	CFileUtil			cFileUtil;

	cVariables.Init();
	AssertTrue(cVariables.ReadKicadCommon("{ \"api\": { \"enable_server\": false }, \"environment\": { \"show_warning_dialog\": true, \"vars\": { \"KICAD_TEST_A\": \"D:\\\\Boards\", \"KICAD_TEST_B\": \"\\u0041/B\" } }, \"x\": null }"));
	AssertString("D:\\Boards", cVariables.Get("KICAD_TEST_A"));
	AssertString("A/B", cVariables.Get("KICAD_TEST_B"));
	AssertTrue(cVariables.Get("KICAD_TEST_C") == NULL);
	cVariables.Set("KICAD_TEST_A", "Here");
	AssertString("Here", cVariables.Get("KICAD_TEST_A"));
	AssertTrue(cVariables.ReadKicadCommon("{ \"environment\": { \"vars\": null } }"));
	AssertFalse(cVariables.ReadKicadCommon("{ \"environment\": { \"vars\": [ 1 ] } }"));
	AssertFalse(cVariables.ReadKicadCommon("{ }"));

	//Put in, collapsed, and relative to the schematic that names it.
	szRoot.Init();
	cFileUtil.CurrentDirectory(&szRoot);
	cVariables.Set("KICAD_TEST_ROOT", szRoot.Text());
	AssertTrue(cVariables.Resolve("${KICAD_TEST_ROOT}/A/../B/C.kicad_sch", NULL, &szPath));
	szExpected.Init(szRoot);
	szExpected.Append(_FS_ "B" _FS_ "C.kicad_sch");
	AssertString(szExpected.Text(), szPath.Text());
	szExpected.Kill();
	szPath.Kill();
	szExpected.Init(szRoot);
	szExpected.Append(_FS_ "B" _FS_ "C.kicad_sch");
	AssertTrue(cVariables.Resolve("../D/E.kicad_sch", szExpected.Text(), &szPath));
	szExpected.Set(szRoot);
	szExpected.Append(_FS_ "D" _FS_ "E.kicad_sch");
	AssertString(szExpected.Text(), szPath.Text());
	szExpected.Kill();
	szPath.Kill();
	AssertFalse(cVariables.Resolve("${KICAD_TEST_UNSET}/A.kicad_sch", NULL, &szPath));
	szRoot.Kill();
	cVariables.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemLogicim(void)
{
	BeginTests();
	DataIOInit();
	DataMemoryInit();

	TestKicadSchemLogicFunction();
	TestKicadSchemLogicimNames();
	TestKicadSchemSubnetGraph();
	TestKicadSchemLogicimConvert();
	TestKicadPathVariables();
	TestKicadSchemLogicimBoards();

	DataMemoryKill();
	DataIOKill();
	TestStatistics();
}

