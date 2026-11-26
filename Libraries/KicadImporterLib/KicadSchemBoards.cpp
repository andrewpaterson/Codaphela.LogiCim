/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela KicadImporterLib

Codaphela KicadImporterLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela KicadImporterLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela KicadImporterLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include <stdlib.h>
#include "BaseLib/Atomic.h"
#include "BaseLib/ConstructorCall.h"
#include "BaseLib/PointerFunctions.h"
#include "BaseLib/FileUtil.h"
#include "BaseLib/Logger.h"
#include "KicadLib/KicadSchemFile.h"
#include "KicadLib/KicadSchemNetlist.h"
#include "KicadSchemLogicimConverter.h"
#include "KicadSchemBoards.h"


struct SKicadSchemBoardConversion
{
	CKicadSchemFile					cFile;
	CKicadSchemNetlist				cNetlist;
	CKicadSchemLogicimConverter		cConverter;
	CKicadSchemBoardImporter*		pcImporter;  //Told when the circuit has been laid out.

	void	Kill(void);
};


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemBoardConversion::Kill(void)
{
	cConverter.Kill();
	cNetlist.Kill();
	cFile.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// uiNumPins is how many pins the connector has, on a net or not.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardConnector::Init(const char* szReference, size uiNumPins)
{
	mszReference.Init(szReference);
	muiNumPins = uiNumPins;
	mcPins.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardConnector::Kill(void)
{
	mcPins.Kill();
	mszReference.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The connector's pin szNumber became the circuit pin, unless it already
// has one.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardConnector::AddPin(const char* szNumber, CPinView* pcPin)
{
	if (mcPins.Get(szNumber) == NULL)
	{
		mcPins.Put(szNumber, &pcPin);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// NULL if the connector's pin has no circuit pin.
//
//////////////////////////////////////////////////////////////////////////
CPinView* CKicadSchemBoardConnector::GetPin(const char* szNumber)
{
	CPinView**	ppcPin;

	ppcPin = mcPins.Get(szNumber);
	return (ppcPin != NULL) ? *ppcPin : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
char* CKicadSchemBoardConnector::GetReference(void) { return mszReference.Text(); }
size CKicadSchemBoardConnector::NumPins(void) const { return muiNumPins; }
size CKicadSchemBoardConnector::NumCircuitPins(void) { return mcPins.NumElements(); }


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoard::Init(const char* szSchematic)
{
	mszSchematic.Init(szSchematic);
	mpcCircuit = NULL;
	mpsConversion = NULL;
	mapcConnectors.Init();
	mbConverting = false;
	mbLaidOut = false;
	msCounts.Init();
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit belongs to the circuits it was made in.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoard::Kill(void)
{
	size						i;
	CKicadSchemBoardConnector*	pcConnector;

	for (i = 0; i < mapcConnectors.NumElements(); i++)
	{
		pcConnector = mapcConnectors.GetPtr(i);
		SafeKill(pcConnector);
	}
	mapcConnectors.Kill();
	mpcCircuit = NULL;
	mszSchematic.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemBoardConnector* CKicadSchemBoard::AddConnector(const char* szReference, size uiNumPins)
{
	CKicadSchemBoardConnector*	pcConnector;

	pcConnector = NewMalloc<CKicadSchemBoardConnector>();
	pcConnector->Init(szReference, uiNumPins);
	mapcConnectors.Add(pcConnector);
	return pcConnector;
}


//////////////////////////////////////////////////////////////////////////
//
// The connector a symbol of the board with uiNumSymbolPins pins stands
// for: the one with as many pins, or failing that the one with the most
// circuit pins (a board's audio jack is a connector too, but its edge
// connector is the board's symbol).  NULL if it has none.
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemBoardConnector* CKicadSchemBoard::FindConnector(size uiNumSymbolPins)
{
	size						i;
	CKicadSchemBoardConnector*	pcConnector;
	CKicadSchemBoardConnector*	pcMost;

	pcMost = NULL;
	for (i = 0; i < mapcConnectors.NumElements(); i++)
	{
		pcConnector = mapcConnectors.GetPtr(i);
		if (pcConnector->NumPins() == uiNumSymbolPins)
		{
			return pcConnector;
		}
		if ((pcMost == NULL) || (pcConnector->NumCircuitPins() > pcMost->NumCircuitPins()))
		{
			pcMost = pcConnector;
		}
	}
	return pcMost;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoard::SetCounts(SKicadSchemLogicimCounts* psCounts)
{
	msCounts = *psCounts;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemBoard::NumConnectors(void) const { return mapcConnectors.NumElements(); }
CKicadSchemBoardConnector* CKicadSchemBoard::GetConnector(size uiIndex) { return mapcConnectors.SafeGetPtr(uiIndex); }
void CKicadSchemBoard::SetCircuit(CCircuitView* pcCircuit) { mpcCircuit = pcCircuit; }
void CKicadSchemBoard::SetConversion(SKicadSchemBoardConversion* psConversion) { mpsConversion = psConversion; }
SKicadSchemBoardConversion* CKicadSchemBoard::GetConversion(void) const { return mpsConversion; }
void CKicadSchemBoard::SetConverting(bool bConverting) { mbConverting = bConverting; }
void CKicadSchemBoard::SetLaidOut(void) { mbLaidOut = true; }
char* CKicadSchemBoard::GetSchematic(void) { return mszSchematic.Text(); }
CCircuitView* CKicadSchemBoard::GetCircuit(void) const { return mpcCircuit; }
bool CKicadSchemBoard::IsConverting(void) const { return mbConverting; }
bool CKicadSchemBoard::IsLaidOut(void) const { return mbLaidOut; }
SKicadSchemLogicimCounts* CKicadSchemBoard::GetCounts(void) { return &msCounts; }


//////////////////////////////////////////////////////////////////////////
//
// The circuits and fonts must outlive the importer, and the variables
// its imports.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardImporter::Init(CCircuitViews* pcCircuits, CCircuitFonts* pcFonts, CKicadPathVariables* pcVariables)
{
	mpcCircuits = pcCircuits;
	mpcFonts = pcFonts;
	mpcVariables = pcVariables;
	mpcFamilies = NULL;
	mapcBoards.Init();
	mpcProgress = NULL;
	mpcJobs = NULL;
	mpcTop = NULL;
	miLayingOut = 0;
	miStop = 0;
}


//////////////////////////////////////////////////////////////////////////
//
// pcProgress is told each board's steps as it's made (see
// CKicadSchemImportProgress), NULL for none.  It isn't held.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardImporter::SetProgress(CKicadSchemImportProgress* pcProgress)
{
	mpcProgress = pcProgress;
}


//////////////////////////////////////////////////////////////////////////
//
// With pcJobs each board's circuit is laid out as a job of theirs (see the
// class); with NULL, on the thread that calls Begin.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardImporter::SetJobs(CJobs* pcJobs)
{
	mpcJobs = pcJobs;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardImporter::SetFamilies(CFamilyVoltageConfigurationStore* pcFamilies)
{
	mpcFamilies = pcFamilies;
}


//////////////////////////////////////////////////////////////////////////
//
// The circuits keep what was made.  An import begun is ended first.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardImporter::Kill(void)
{
	size				i;
	CKicadSchemBoard*	pcBoard;

	End();
	for (i = 0; i < mapcBoards.NumElements(); i++)
	{
		pcBoard = mapcBoards.GetPtr(i);
		SafeKill(pcBoard);
	}
	mapcBoards.Kill();
	mpcVariables = NULL;
	mpcFonts = NULL;
	mpcCircuits = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The board's circuit, made from its schematic with the boards it draws.
// NULL if it, or a board it draws, can't be made.
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CKicadSchemBoardImporter::Import(const char* szSchematic)
{
	Begin(szSchematic);
	return End();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static void KicadSchemBoardLayOutJob(void* pvConversion)
{
	SKicadSchemBoardConversion*	psConversion;

	psConversion = (SKicadSchemBoardConversion*)pvConversion;
	psConversion->cConverter.LayOut();
	psConversion->pcImporter->LaidOut();
}


//////////////////////////////////////////////////////////////////////////
//
// Builds the board's circuit and those of the boards it draws, and has
// every circuit built laid out: by the jobs, or with none here and now.
// False if it, or a board it draws, can't be made; the circuits that were
// built are laid out all the same.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemBoardImporter::Begin(const char* szSchematic)
{
	CSerialJobs					cSerialJobs;
	CJobs*						pcJobs;
	SKicadSchemBoardConversion*	psConversion;
	size						i;

	mpcTop = GetBoard(szSchematic, NULL);

	//Counted before any is given, so that the count isn't seen at nothing while there are more to give.
	for (i = 0; i < mapcBoards.NumElements(); i++)
	{
		if (mapcBoards.GetPtr(i)->GetConversion() != NULL)
		{
			AtomicAdd(&miLayingOut, 1);
		}
	}
	pcJobs = (mpcJobs != NULL) ? mpcJobs : &cSerialJobs;
	for (i = 0; i < mapcBoards.NumElements(); i++)
	{
		psConversion = mapcBoards.GetPtr(i)->GetConversion();
		if (psConversion != NULL)
		{
			pcJobs->Run(KicadSchemBoardLayOutJob, psConversion);
		}
	}
	return mpcTop != NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardImporter::LaidOut(void)
{
	AtomicAdd(&miLayingOut, -1);
}


//////////////////////////////////////////////////////////////////////////
//
// Every circuit given to be laid out has been.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemBoardImporter::IsDone(void)
{
	return AtomicLoad(&miLayingOut) == 0;
}


//////////////////////////////////////////////////////////////////////////
//
// The circuits being laid out, and those still waiting to be, stop at
// their next step: IsDone is true soon, though not yet, and End won't wait
// long.  Any thread may call it, one telling the progress too.  The
// importer stays cancelled.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardImporter::Cancel(void)
{
	AtomicStore(&miStop, 1);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemBoardImporter::IsCancelled(void)
{
	return AtomicLoad(&miStop) != 0;
}


//////////////////////////////////////////////////////////////////////////
//
// Waits for the circuits to be laid out, and gives the circuit of the
// board the import began with: NULL if it couldn't be made, if the import
// was cancelled, or if none was begun.
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CKicadSchemBoardImporter::End(void)
{
	size						i;
	CKicadSchemBoard*			pcBoard;
	SKicadSchemBoardConversion*	psConversion;
	CCircuitView*				pcCircuit;

	if ((mpcJobs != NULL) && !IsDone())
	{
		mpcJobs->Wait();
	}
	for (i = 0; i < mapcBoards.NumElements(); i++)
	{
		pcBoard = mapcBoards.GetPtr(i);
		psConversion = pcBoard->GetConversion();
		if (psConversion != NULL)
		{
			pcBoard->SetCounts(psConversion->cConverter.GetCounts());
			pcBoard->SetConversion(NULL);
			psConversion->Kill();
			free(psConversion);
		}
	}

	pcCircuit = ((mpcTop != NULL) && !IsCancelled()) ? mpcTop->GetCircuit() : NULL;
	mpcTop = NULL;
	return pcCircuit;
}


//////////////////////////////////////////////////////////////////////////
//
// A board already made isn't made again, nor one that failed.
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemBoard* CKicadSchemBoardImporter::GetBoard(const char* szSchematic, const char* szDrawnIn)
{
	CChars				szFullPath;
	size				i;
	CKicadSchemBoard*	pcBoard;
	CFileUtil			cFileUtil;

	if (!mpcVariables->Resolve(szSchematic, szDrawnIn, &szFullPath))
	{
		return NULL;
	}

	for (i = 0; i < mapcBoards.NumElements(); i++)
	{
		pcBoard = mapcBoards.GetPtr(i);
		if (CompareFileNamesNTFS(pcBoard->GetSchematic(), szFullPath.Text()) == 0)
		{
			szFullPath.Kill();
			if (pcBoard->IsConverting())
			{
				gcLogger.Error2(__METHOD__, " Board [", pcBoard->GetSchematic(), "] is drawn on itself.", NULL);
				return NULL;
			}
			return (pcBoard->GetCircuit() != NULL) ? pcBoard : NULL;
		}
	}

	if (!cFileUtil.Exists(szFullPath.Text()))
	{
		gcLogger.Error2(__METHOD__, " Board schematic [", szFullPath.Text(), "] (", szSchematic, ") doesn't exist.", NULL);
		szFullPath.Kill();
		return NULL;
	}
	pcBoard = Convert(szFullPath.Text());
	szFullPath.Kill();
	return pcBoard;
}


//////////////////////////////////////////////////////////////////////////
//
// Loads the schematic and builds its circuit, which builds those of the
// boards it draws first.  What it was built from is kept with the board
// until the circuit has been laid out (Begin and End).
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemBoard* CKicadSchemBoardImporter::Convert(const char* szFullPath)
{
	CKicadSchemBoard*				pcBoard;
	SKicadSchemBoardConversion*		psConversion;
	CChars							szTypeName;
	bool							bResult;

	pcBoard = NewMalloc<CKicadSchemBoard>();
	pcBoard->Init(szFullPath);
	mapcBoards.Add(pcBoard);
	pcBoard->SetConverting(true);

	GetTypeName(szFullPath, &szTypeName);
	if (mpcProgress != NULL)
	{
		mpcProgress->Progress(szTypeName.Text(), "Reading", KICAD_SCHEM_IMPORT_STEP_READ, KICAD_SCHEM_IMPORT_STEPS);
	}
	psConversion = NewMalloc<SKicadSchemBoardConversion>();
	psConversion->pcImporter = this;
	psConversion->cFile.Init();
	bResult = psConversion->cFile.Load((char*)szFullPath);
	psConversion->cNetlist.Init(&psConversion->cFile);
	bResult = bResult && psConversion->cNetlist.Build();
	psConversion->cConverter.Init(&psConversion->cFile, &psConversion->cNetlist, mpcCircuits, mpcFonts);
	psConversion->cConverter.SetFamilies(mpcFamilies);
	psConversion->cConverter.SetBoards(pcBoard, this);
	psConversion->cConverter.SetProgress(mpcProgress);
	psConversion->cConverter.SetStop(&miStop);
	bResult = bResult && psConversion->cConverter.Build(szTypeName.Text());
	if (bResult)
	{
		pcBoard->SetCircuit(psConversion->cConverter.GetCircuit());
		pcBoard->SetConversion(psConversion);
	}
	else
	{
		gcLogger.Error2(__METHOD__, " Board schematic [", szFullPath, "] couldn't be imported.", NULL);
		psConversion->Kill();
		free(psConversion);
	}
	pcBoard->SetConverting(false);

	szTypeName.Kill();
	return bResult ? pcBoard : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The schematic's file name without ".kicad_sch", with " 2", " 3" ...
// after it if another circuit already has the name.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardImporter::GetTypeName(const char* szFullPath, CChars* pszTypeName)
{
	CFileUtil	cFileUtil;
	CChars		szDirectory;
	CChars		szName;
	int32		iCopy;

	szName.Init();
	szDirectory.Init();
	cFileUtil.SplitPath(szFullPath, &szName, &szDirectory);
	szDirectory.Kill();
	if (szName.EndsWith(".kicad_sch"))
	{
		szName.RemoveFromEnd(10);
	}

	pszTypeName->Init(szName.Text());
	for (iCopy = 2; mpcCircuits->Get(pszTypeName->Text()) != NULL; iCopy++)
	{
		pszTypeName->Set(szName.Text());
		pszTypeName->Append(' ');
		pszTypeName->Append(iCopy);
	}
	szName.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemBoard* CKicadSchemBoardImporter::GetBoardForCircuit(CCircuitView* pcCircuit)
{
	size	i;

	for (i = 0; i < mapcBoards.NumElements(); i++)
	{
		if (mapcBoards.GetPtr(i)->GetCircuit() == pcCircuit)
		{
			return mapcBoards.GetPtr(i);
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemBoardImporter::NumBoards(void) const { return mapcBoards.NumElements(); }
CKicadSchemBoard* CKicadSchemBoardImporter::GetBoard(size uiIndex) { return mapcBoards.SafeGetPtr(uiIndex); }
