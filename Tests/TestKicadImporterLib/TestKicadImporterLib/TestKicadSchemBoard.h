#ifndef __TEST_KICAD_SCHEM_BOARD_H__
#define __TEST_KICAD_SCHEM_BOARD_H__
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


#define KICAD_SCHEM_TEST_BOARD_FONTS		"Input" _FS_ "Fonts"
#define KICAD_SCHEM_TEST_BOARD_FAMILIES		"Input" _FS_ "PropagationTimes.csv"


//////////////////////////////////////////////////////////////////////////
//
// A board imported, as LogiCim imports it (with the families' input
// capacitances), and simulated, its supplies ramping from the start.
//
//////////////////////////////////////////////////////////////////////////
struct SKicadSchemTestBoard
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
		pFonts = LoadCircuitFonts(KICAD_SCHEM_TEST_BOARD_FONTS, CCO_Unknown);
		ORoot()->Add(pFonts);
		cFamilies.Init();
		AssertTrue(cFamilies.Load(KICAD_SCHEM_TEST_BOARD_FAMILIES));
		szPath.Init(szFilename);
		cFile.Init();
		AssertTrue(cFile.Load(szPath.Text()));
		szPath.Kill();
		cNetlist.Init(&cFile);
		AssertTrue(cNetlist.Build());
		cCircuits.Init();
		cConverter.Init(&cFile, &cNetlist, &cCircuits, &pFonts);
		cConverter.SetFamilies(&cFamilies);
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


#endif // __TEST_KICAD_SCHEM_BOARD_H__
