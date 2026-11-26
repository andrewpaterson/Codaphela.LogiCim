#include <string.h>
#include "BaseLib/GlobalDataTypesIO.h"
#include "BaseLib/DataMemory.h"
#include "BaseLib/Chars.h"
#include "StandardLib/Objects.h"
#include "LogicimLib/CircuitFonts.h"
#include "KicadImporterLib/KicadSchemLogicimDecode.h"
#include "TestLib/Assert.h"


#define KICAD_SCHEM_DECODE_FONTS			"Input" _FS_ "Fonts"
#define KICAD_SCHEM_DECODE_FAMILIES			"Input" _FS_ "PropagationTimes.csv"
#define KICAD_SCHEM_DECODE_ADDRESS_BOARD	"Input" _FS_ "Kicad" _FS_ "Computer" _FS_ "Processor" _FS_ "HCP65 MPU Address Decode" _FS_ "HCP65 MPU Address Decode.kicad_sch"


//A board with address lines A0 and A1 and a mode on its connector J1.  ~{Sel} is A0 NAND Mode (U1) through a series
//resistor (R1); ~{High} is A1 OR an input tied low by a GND label (U2).  R2 pulls Mode up to 3.3V, which mustn't join
//them.
char gszKicadSchemDecodeTestFile[] =
	"(kicad_sch (version 20250114) (generator \"eeschema\") (uuid \"root\") (paper \"A4\")\n"
	"	(lib_symbols\n"
	"		(symbol \"Test:Conn\" (property \"Reference\" \"J\") (property \"Value\" \"Conn\")\n"
	"			(symbol \"Conn_1_1\"\n"
	"				(pin passive line (at 5.08 3.81 180) (length 2.54) (name \"\") (number \"1\"))\n"
	"				(pin passive line (at 5.08 1.27 180) (length 2.54) (name \"\") (number \"2\"))\n"
	"				(pin passive line (at 5.08 -1.27 180) (length 2.54) (name \"\") (number \"3\"))\n"
	"				(pin passive line (at 5.08 -3.81 180) (length 2.54) (name \"\") (number \"4\"))\n"
	"				(pin passive line (at 5.08 -6.35 180) (length 2.54) (name \"\") (number \"5\"))))\n"
	"		(symbol \"Nexperia:74LVC1G00\" (property \"Reference\" \"U\") (property \"Value\" \"74LVC1G00\")\n"
	"			(symbol \"74LVC1G00_1_1\"\n"
	"				(pin input line (at -5.08 1.27 0) (length 2.54) (name \"A\") (number \"1\"))\n"
	"				(pin input line (at -5.08 -1.27 0) (length 2.54) (name \"B\") (number \"2\"))\n"
	"				(pin output line (at 5.08 0 180) (length 2.54) (name \"Y\") (number \"4\"))\n"
	"				(pin power_in line (at 0 5.08 270) (length 2.54) (hide yes) (name \"VCC\") (number \"5\"))\n"
	"				(pin power_in line (at 0 -5.08 90) (length 2.54) (hide yes) (name \"GND\") (number \"3\"))))\n"
	"		(symbol \"Nexperia:74LVC1G32\" (property \"Reference\" \"U\") (property \"Value\" \"74LVC1G32\")\n"
	"			(symbol \"74LVC1G32_1_1\"\n"
	"				(pin input line (at -5.08 1.27 0) (length 2.54) (name \"A\") (number \"1\"))\n"
	"				(pin input line (at -5.08 -1.27 0) (length 2.54) (name \"B\") (number \"2\"))\n"
	"				(pin output line (at 5.08 0 180) (length 2.54) (name \"Y\") (number \"4\"))\n"
	"				(pin power_in line (at 0 5.08 270) (length 2.54) (hide yes) (name \"VCC\") (number \"5\"))\n"
	"				(pin power_in line (at 0 -5.08 90) (length 2.54) (hide yes) (name \"GND\") (number \"3\"))))\n"
	"		(symbol \"Device:R\" (property \"Reference\" \"R\") (property \"Value\" \"R\")\n"
	"			(symbol \"R_1_1\"\n"
	"				(pin passive line (at 0 3.81 270) (length 1.27) (name \"~\") (number \"1\"))\n"
	"				(pin passive line (at 0 -3.81 90) (length 1.27) (name \"~\") (number \"2\")))))\n"
	"	(symbol (lib_id \"Test:Conn\") (at 50 50 0) (unit 1) (uuid \"j1\") (property \"Reference\" \"J1\") (property \"Value\" \"Conn\"))\n"
	"	(symbol (lib_id \"Nexperia:74LVC1G00\") (at 100 50 0) (unit 1) (uuid \"u1\") (property \"Reference\" \"U1\") (property \"Value\" \"74LVC1G00\"))\n"
	"	(symbol (lib_id \"Nexperia:74LVC1G32\") (at 100 80 0) (unit 1) (uuid \"u2\") (property \"Reference\" \"U2\") (property \"Value\" \"74LVC1G32\"))\n"
	"	(symbol (lib_id \"Device:R\") (at 120 50 0) (unit 1) (uuid \"r1\") (property \"Reference\" \"R1\") (property \"Value\" \"22R\"))\n"
	"	(symbol (lib_id \"Device:R\") (at 140 50 0) (unit 1) (uuid \"r2\") (property \"Reference\" \"R2\") (property \"Value\" \"10k\"))\n"
	"	(label \"A0\" (at 55.08 46.19 0) (uuid \"l1\"))\n"
	"	(label \"A1\" (at 55.08 48.73 0) (uuid \"l2\"))\n"
	"	(label \"Mode\" (at 55.08 51.27 0) (uuid \"l3\"))\n"
	"	(label \"~{Sel}\" (at 55.08 53.81 0) (uuid \"l4\"))\n"
	"	(label \"~{High}\" (at 55.08 56.35 0) (uuid \"l5\"))\n"
	"	(label \"A0\" (at 94.92 48.73 0) (uuid \"l6\"))\n"
	"	(label \"Mode\" (at 94.92 51.27 0) (uuid \"l7\"))\n"
	"	(label \"~{Sel}_{I}\" (at 105.08 50 0) (uuid \"l8\"))\n"
	"	(label \"A1\" (at 94.92 78.73 0) (uuid \"l9\"))\n"
	"	(label \"GND\" (at 94.92 81.27 0) (uuid \"l10\"))\n"
	"	(label \"~{High}\" (at 105.08 80 0) (uuid \"l11\"))\n"
	"	(label \"~{Sel}_{I}\" (at 120 46.19 0) (uuid \"l12\"))\n"
	"	(label \"~{Sel}\" (at 120 53.81 0) (uuid \"l13\"))\n"
	"	(label \"3.3V\" (at 140 46.19 0) (uuid \"l14\"))\n"
	"	(label \"Mode\" (at 140 53.81 0) (uuid \"l15\"))\n"
	")\n";


//////////////////////////////////////////////////////////////////////////
//
// The decode of a sheet given as text.
//
//////////////////////////////////////////////////////////////////////////
bool DecodeKicadSchemText(char* szText, const char* szName, CChars* pszDest)
{
	CKicadSchemFile						cFile;
	CKicadSchemNetlist					cNetlist;
	CFamilyVoltageConfigurationStore	cFamilies;
	Ptr<CCircuitFonts>					pFonts;
	CKicadSchemLogicimDecode			cDecode;
	bool								bResult;

	pFonts = LoadCircuitFonts(KICAD_SCHEM_DECODE_FONTS, CCO_Unknown);
	ORoot()->Add(pFonts);
	cFamilies.Init();
	AssertTrue(cFamilies.Load(KICAD_SCHEM_DECODE_FAMILIES));
	cFile.Init();
	AssertTrue(cFile.Import(szText, strlen(szText), (char*)szName));
	cNetlist.Init(&cFile);
	AssertTrue(cNetlist.Build());

	cDecode.Init(&cFile, &cNetlist, &pFonts, &cFamilies);
	bResult = cDecode.Decode(szName, pszDest);
	cDecode.Kill();

	cNetlist.Kill();
	cFile.Kill();
	cFamilies.Kill();
	ORoot()->Remove(pFonts);
	pFonts = NULL;
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// The small board's ranges: ~{Sel} at the odd addresses in Mode, a range
// repeating every 2; ~{High} at the addresses with A1 low, which it reads
// only if the GND label drives its other input.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemDecodeSmall(void)
{
	CChars	szRanges;

	ObjectsInit();
	szRanges.Init();
	AssertTrue(DecodeKicadSchemText(gszKicadSchemDecodeTestFile, "Decode Test", &szRanges));
	AssertString("Mode=0\t~{High}\t$0\t$1\n"
				 "Mode=1\t~{Sel}\t$1\t$1\t$2\t$3\n"
				 "Mode=1\t~{High}\t$0\t$1\n", szRanges.Text());
	szRanges.Kill();
	ORoot()->RemoveAll();
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
// The HCP65's address decode board, as the site's check reads it: in
// kernel mode after the native latch (RAM Swap high) and out of reset.
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemDecodeAddressBoard(void)
{
	CChars	szRanges;

	ObjectsInit();
	szRanges.Init();
	AssertTrue(DecodeKicadSchemFile(KICAD_SCHEM_DECODE_ADDRESS_BOARD, KICAD_SCHEM_DECODE_FAMILIES, KICAD_SCHEM_DECODE_FONTS, &szRanges));
	AssertTrue(szRanges.Contains("~{Reset}=1,Kernal Mode=1,RAM Swap=1\t~{RAM}\t$000000\t$07FFFF\n"));
	AssertTrue(szRanges.Contains("~{Reset}=1,Kernal Mode=1,RAM Swap=1\t~{ROM}\t$080000\t$0FFFFF\n"));
	AssertTrue(szRanges.Contains("~{Reset}=1,Kernal Mode=1,RAM Swap=0\t~{ROM}\t$000000\t$07FFFF\n"));
	AssertTrue(szRanges.Contains("~{Reset}=1,Kernal Mode=1,RAM Swap=1\t~{Registers}\t$100000\t$107FFF\n"));
	AssertTrue(szRanges.Contains("~{Reset}=1,Kernal Mode=1,RAM Swap=1\t~{Register5}\t$100005\t$100005\t$000010\t$107FF5\n"));
	AssertTrue(szRanges.Contains("~{Reset}=1,Kernal Mode=1,RAM Swap=1\t~{Device7}\t$138000\t$13FFFF\n"));
	AssertTrue(szRanges.Contains("~{Reset}=1,Kernal Mode=1,RAM Swap=1\t~{Device16}\t$180000\t$19FFFF\n"));
	AssertTrue(szRanges.Contains("~{Reset}=1,Kernal Mode=1,RAM Swap=1\t~{Main Memory}\t$200000\t$FFFFFF\n"));
	AssertTrue(szRanges.Contains("~{Reset}=1,Kernal Mode=0,RAM Swap=1\t~{Main Memory}\t$000000\t$FFFFFF\n"));
	AssertFalse(szRanges.Contains("Kernal Mode=0,RAM Swap=1\t~{RAM}"));
	AssertFalse(szRanges.Contains("~{Reset}=0,Kernal Mode=1,RAM Swap=1\t~{Device1}"));
	szRanges.Kill();
	ORoot()->RemoveAll();
	ObjectsFlush();
	ObjectsKill(false);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestKicadSchemDecode(void)
{
	BeginTests();
	DataIOInit();
	DataMemoryInit();

	TestKicadSchemDecodeSmall();
	TestKicadSchemDecodeAddressBoard();

	DataMemoryKill();
	DataIOKill();
	TestStatistics();
}
