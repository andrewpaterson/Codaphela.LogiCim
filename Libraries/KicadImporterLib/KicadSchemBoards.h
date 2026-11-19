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
#ifndef __KICAD_SCHEM_BOARDS_H__
#define __KICAD_SCHEM_BOARDS_H__
#include "KicadSchemImportProgress.h"
#include "BaseLib/Chars.h"
#include "BaseLib/ArrayChars.h"
#include "BaseLib/Jobs.h"
#include "BaseLib/ArrayTemplatePtr.h"
#include "BaseLib/MapStringTemplate.h"
#include "LogicimLib/CircuitViews.h"
#include "LogicimLib/CircuitFonts.h"
#include "LogicimLib/ValueView.h"
#include "KicadLib/KicadPathVariables.h"
#include "KicadSchemLogicimCounts.h"


//The property of a board's symbol that names the board's own schematic, as
//"${_658_COMPUTER_}/Kicad/Computer/Audio/Audio Clock/Audio Clock.kicad_sch".
#define KICAD_SCHEM_BOARD_SCHEMATIC_PROPERTY	"Schematic"


//One of a board's connectors ("J1") and the circuit's pins its pins became, by the connector's pin numbers (see
//CKicadSchemLogicimConverter::AddBoardPins).  A pin on a supply's net, or on no net, has no circuit pin.  Two of the
//connector's pins on the same net share one.
class CKicadSchemBoardConnector
{
protected:
	CChars						mszReference;
	size						muiNumPins;
	CMapStringTemplate<CPinView*>	mcPins;  //By the connector's pin number.

public:
	void		Init(const char* szReference, size uiNumPins);
	void		Kill(void);

	void		AddPin(const char* szNumber, CPinView* pcPin);
	CPinView*	GetPin(const char* szNumber);

	char*		GetReference(void);
	size		NumPins(void) const;
	size		NumCircuitPins(void);
};


//A board's schematic, its netlist and the converter making its circuit of them, kept from when the circuit is built
//to when it's laid out.
struct SKicadSchemBoardConversion;


//A board made into a Logicim circuit from its own schematic, and its connectors.  Another board that has it drawn
//as a symbol joins the symbol's pins to the circuit's pins of the connector the symbol stands for, by pin number.
class CKicadSchemBoard
{
protected:
	CChars											mszSchematic;  //The schematic's full path.
	CCircuitView*									mpcCircuit;
	SKicadSchemBoardConversion*						mpsConversion;  //Until its circuit is laid out.
	CArrayTemplatePtr<CKicadSchemBoardConnector>	mapcConnectors;
	bool											mbConverting;
	bool											mbLaidOut;
	SKicadSchemLogicimCounts						msCounts;  //What converting it made.

public:
	void						Init(const char* szSchematic);
	void						Kill(void);

	CKicadSchemBoardConnector*	AddConnector(const char* szReference, size uiNumPins);
	CKicadSchemBoardConnector*	FindConnector(size uiNumSymbolPins);
	size						NumConnectors(void) const;
	CKicadSchemBoardConnector*	GetConnector(size uiIndex);

	void						SetCircuit(CCircuitView* pcCircuit);
	void						SetConversion(SKicadSchemBoardConversion* psConversion);
	SKicadSchemBoardConversion*	GetConversion(void) const;
	void						SetConverting(bool bConverting);
	void						SetLaidOut(void);
	void						SetCounts(SKicadSchemLogicimCounts* psCounts);

	char*						GetSchematic(void);
	CCircuitView*				GetCircuit(void) const;
	bool						IsConverting(void) const;
	bool						IsLaidOut(void) const;
	SKicadSchemLogicimCounts*	GetCounts(void);
};


//What the converter asks for the board a symbol stands for (see CKicadSchemLogicimConverter::AddBoard).
class CKicadSchemBoardSource
{
public:
	//The board made from the schematic, the symbol's Schematic property, which may use path variables and be
	//relative to the directory of the schematic it's drawn in.  NULL if it can't be made.
	virtual CKicadSchemBoard*	GetBoard(const char* szSchematic, const char* szDrawnIn) = 0;
};


//Imports a board's schematic into Logicim circuits with every board drawn on it (a symbol with a Schematic
//property), and every board drawn on those, each made once however many times it's drawn.  Each board is a circuit
//named as its schematic's file ("Audio Clock"), with its connectors' pins as the circuit's pins, so each symbol drawn
//for it is an instance of it.  Boards are made before the boards that draw them; a board that draws itself, however
//far down, fails.
//
//An import is in two parts.  Begin builds every board's circuit (CKicadSchemLogicimConverter::Build), in that order,
//on the caller's thread: it is quick, and it is all of an import that asks the other circuits, the boards and the
//fonts for anything, or changes them.  Then each circuit is laid out (LayOut), which is nearly all of an import's time
//and reads and changes nothing but the circuit: with jobs (SetJobs) each board's is a job, so as many are laid out at
//once as the jobs have threads, and what's imported is the same whichever finishes first.  IsDone says when every one
//has been, and End waits for that and gives the top board's circuit.  Import is Begin and End.
//
//An import begun can be cancelled (Cancel): each circuit's layout stops at its next step (see
//CKicadSchemLogicimConverter), which is soon, and End gives no circuit, then or after.  What was made is in the
//circuits all the same, some of it not laid out: they, and the importer, are for whoever owns them to throw away.
//
//With jobs a board's layout steps are told (SetProgress) by the thread that lays it out, so several boards' steps
//come at once, from threads that aren't the caller's: what's told must be ready for that.
class CKicadSchemBoardImporter : public CKicadSchemBoardSource
{
protected:
	CCircuitViews*						mpcCircuits;
	CCircuitFonts*						mpcFonts;
	CKicadPathVariables*				mpcVariables;
	CArrayTemplatePtr<CKicadSchemBoard>	mapcBoards;
	CKicadSchemImportProgress*			mpcProgress;  //Told each board's steps, NULL for none.
	CJobs*								mpcJobs;  //What lays the circuits out, NULL for the caller's thread.
	CKicadSchemBoard*					mpcTop;  //The board Begin was given, NULL if it couldn't be built.
	int32								miLayingOut;  //Circuits given to be laid out that haven't been yet.  The jobs' threads count it down.
	int32								miStop;  //Not zero once the import is cancelled.  The jobs' threads read it.

public:
	void				Init(CCircuitViews* pcCircuits, CCircuitFonts* pcFonts, CKicadPathVariables* pcVariables);
	void				Kill(void);
	void				SetProgress(CKicadSchemImportProgress* pcProgress);
	void				SetJobs(CJobs* pcJobs);  //They outlive the import.

	CCircuitView*		Import(const char* szSchematic);
	bool				Begin(const char* szSchematic);
	bool				IsDone(void);
	void				Cancel(void);
	bool				IsCancelled(void);
	CCircuitView*		End(void);
	void				LaidOut(void);  //A job's: its circuit is laid out.
	CKicadSchemBoard*	GetBoard(const char* szSchematic, const char* szDrawnIn) override;

	size				NumBoards(void) const;
	CKicadSchemBoard*	GetBoard(size uiIndex);
	CKicadSchemBoard*	GetBoardForCircuit(CCircuitView* pcCircuit);

protected:
	CKicadSchemBoard*	Convert(const char* szFullPath);
	void				GetTypeName(const char* szFullPath, CChars* pszTypeName);
};


#endif // __KICAD_SCHEM_BOARDS_H__
