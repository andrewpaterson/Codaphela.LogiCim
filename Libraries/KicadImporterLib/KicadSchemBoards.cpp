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
#include "BaseLib/ConstructorCall.h"
#include "BaseLib/PointerFunctions.h"
#include "BaseLib/FileUtil.h"
#include "BaseLib/Logger.h"
#include "KicadLib/KicadSchemFile.h"
#include "KicadLib/KicadSchemNetlist.h"
#include "KicadSchemLogicimConverter.h"
#include "KicadSchemBoards.h"


//////////////////////////////////////////////////////////////////////////
//
// uiNumPins is how many pins the connector has, on a net or not.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardConnector::Init(const char* szReference, size uiNumPins)
{
	mszReference.Init(szReference);
	muiNumPins = uiNumPins;
	maszNumbers.Init();
	mapcPins.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardConnector::Kill(void)
{
	mapcPins.Kill();
	maszNumbers.Kill();
	mszReference.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardConnector::AddPin(const char* szNumber, CPinView* pcPin)
{
	if (maszNumbers.GetIndex(szNumber) == ARRAY_ELEMENT_NOT_FOUND)
	{
		maszNumbers.Add(szNumber);
		mapcPins.Add(pcPin);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// NULL if the connector's pin has no circuit pin.
//
//////////////////////////////////////////////////////////////////////////
CPinView* CKicadSchemBoardConnector::GetPin(const char* szNumber)
{
	size	uiIndex;

	uiIndex = maszNumbers.GetIndex(szNumber);
	if (uiIndex == ARRAY_ELEMENT_NOT_FOUND)
	{
		return NULL;
	}
	return mapcPins.GetPtr(uiIndex);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
char* CKicadSchemBoardConnector::GetReference(void) { return mszReference.Text(); }
size CKicadSchemBoardConnector::NumPins(void) { return muiNumPins; }
size CKicadSchemBoardConnector::NumCircuitPins(void) { return maszNumbers.NumElements(); }


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoard::Init(const char* szSchematic)
{
	mszSchematic.Init(szSchematic);
	mpcCircuit = NULL;
	mapcConnectors.Init();
	mbConverting = false;
	mbLaidOut = false;
	muiUnconnected = 0;
	muiOverlaps = 0;
	muiPlaceholders = 0;
	muiBoards = 0;
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
void CKicadSchemBoard::SetCounts(size uiUnconnected, size uiOverlaps, size uiPlaceholders, size uiBoards)
{
	muiUnconnected = uiUnconnected;
	muiOverlaps = uiOverlaps;
	muiPlaceholders = uiPlaceholders;
	muiBoards = uiBoards;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemBoard::NumConnectors(void) { return mapcConnectors.NumElements(); }
CKicadSchemBoardConnector* CKicadSchemBoard::GetConnector(size uiIndex) { return mapcConnectors.SafeGetPtr(uiIndex); }
void CKicadSchemBoard::SetCircuit(CCircuitView* pcCircuit) { mpcCircuit = pcCircuit; }
void CKicadSchemBoard::SetConverting(bool bConverting) { mbConverting = bConverting; }
void CKicadSchemBoard::SetLaidOut(void) { mbLaidOut = true; }
char* CKicadSchemBoard::GetSchematic(void) { return mszSchematic.Text(); }
CCircuitView* CKicadSchemBoard::GetCircuit(void) { return mpcCircuit; }
bool CKicadSchemBoard::IsConverting(void) { return mbConverting; }
bool CKicadSchemBoard::IsLaidOut(void) { return mbLaidOut; }
size CKicadSchemBoard::NumUnconnected(void) { return muiUnconnected; }
size CKicadSchemBoard::NumOverlaps(void) { return muiOverlaps; }
size CKicadSchemBoard::NumPlaceholders(void) { return muiPlaceholders; }
size CKicadSchemBoard::NumBoards(void) { return muiBoards; }


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
	mapcBoards.Init();
}


//////////////////////////////////////////////////////////////////////////
//
// The circuits keep what was made.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemBoardImporter::Kill(void)
{
	size				i;
	CKicadSchemBoard*	pcBoard;

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
	CKicadSchemBoard*	pcBoard;

	pcBoard = GetBoard(szSchematic, NULL);
	return (pcBoard != NULL) ? pcBoard->GetCircuit() : NULL;
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
// Loads the schematic and makes its circuit, which makes the boards it
// draws first.
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemBoard* CKicadSchemBoardImporter::Convert(const char* szFullPath)
{
	CKicadSchemBoard*				pcBoard;
	CKicadSchemFile					cFile;
	CKicadSchemNetlist				cNetlist;
	CKicadSchemLogicimConverter		cConverter;
	CChars							szTypeName;
	bool							bResult;

	pcBoard = NewMalloc<CKicadSchemBoard>();
	pcBoard->Init(szFullPath);
	mapcBoards.Add(pcBoard);
	pcBoard->SetConverting(true);

	cFile.Init();
	bResult = cFile.Load((char*)szFullPath);
	cNetlist.Init(&cFile);
	bResult = bResult && cNetlist.Build();
	GetTypeName(szFullPath, &szTypeName);
	cConverter.Init(&cFile, &cNetlist, mpcCircuits, mpcFonts);
	cConverter.SetBoards(pcBoard, this);
	bResult = bResult && cConverter.Convert(szTypeName.Text());
	if (bResult)
	{
		pcBoard->SetCircuit(cConverter.GetCircuit());
		pcBoard->SetCounts(cConverter.NumUnconnected(), cConverter.NumOverlaps(), cConverter.NumPlaceholders(), cConverter.NumBoards());
	}
	else
	{
		gcLogger.Error2(__METHOD__, " Board schematic [", szFullPath, "] couldn't be imported.", NULL);
	}
	pcBoard->SetConverting(false);

	cConverter.Kill();
	szTypeName.Kill();
	cNetlist.Kill();
	cFile.Kill();
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
size CKicadSchemBoardImporter::NumBoards(void) { return mapcBoards.NumElements(); }
CKicadSchemBoard* CKicadSchemBoardImporter::GetBoard(size uiIndex) { return mapcBoards.SafeGetPtr(uiIndex); }
