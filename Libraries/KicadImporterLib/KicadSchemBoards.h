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
#include "BaseLib/Chars.h"
#include "BaseLib/ArrayChars.h"
#include "BaseLib/ArrayTemplatePtr.h"
#include "LogicimLib/CircuitViews.h"
#include "LogicimLib/CircuitFonts.h"
#include "LogicimLib/ValueView.h"
#include "KicadPathVariables.h"


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
	CArrayChars					maszNumbers;
	CArrayTemplatePtr<CPinView>	mapcPins;

public:
	void		Init(const char* szReference, size uiNumPins);
	void		Kill(void);

	void		AddPin(const char* szNumber, CPinView* pcPin);
	CPinView*	GetPin(const char* szNumber);

	char*		GetReference(void);
	size		NumPins(void);
	size		NumCircuitPins(void);
};


//A board made into a Logicim circuit from its own schematic, and its connectors.  Another board that has it drawn
//as a symbol joins the symbol's pins to the circuit's pins of the connector the symbol stands for, by pin number.
class CKicadSchemBoard
{
protected:
	CChars									mszSchematic;  //The schematic's full path.
	CCircuitView*							mpcCircuit;
	CArrayTemplatePtr<CKicadSchemBoardConnector>	mapcConnectors;
	bool									mbConverting;
	bool									mbLaidOut;
	size									muiUnconnected;
	size									muiOverlaps;
	size									muiPlaceholders;
	size									muiBoards;

public:
	void						Init(const char* szSchematic);
	void						Kill(void);

	CKicadSchemBoardConnector*	AddConnector(const char* szReference, size uiNumPins);
	CKicadSchemBoardConnector*	FindConnector(size uiNumSymbolPins);
	size						NumConnectors(void);
	CKicadSchemBoardConnector*	GetConnector(size uiIndex);

	void						SetCircuit(CCircuitView* pcCircuit);
	void						SetConverting(bool bConverting);
	void						SetLaidOut(void);
	void						SetCounts(size uiUnconnected, size uiOverlaps, size uiPlaceholders, size uiBoards);

	char*						GetSchematic(void);
	CCircuitView*				GetCircuit(void);
	bool						IsConverting(void);
	bool						IsLaidOut(void);
	size						NumUnconnected(void);
	size						NumOverlaps(void);
	size						NumPlaceholders(void);
	size						NumBoards(void);
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
class CKicadSchemBoardImporter : public CKicadSchemBoardSource
{
protected:
	CCircuitViews*						mpcCircuits;
	CCircuitFonts*						mpcFonts;
	CKicadPathVariables*				mpcVariables;
	CArrayTemplatePtr<CKicadSchemBoard>	mapcBoards;

public:
	void				Init(CCircuitViews* pcCircuits, CCircuitFonts* pcFonts, CKicadPathVariables* pcVariables);
	void				Kill(void);

	CCircuitView*		Import(const char* szSchematic);
	CKicadSchemBoard*	GetBoard(const char* szSchematic, const char* szDrawnIn) override;

	size				NumBoards(void);
	CKicadSchemBoard*	GetBoard(size uiIndex);
	CKicadSchemBoard*	GetBoardForCircuit(CCircuitView* pcCircuit);

protected:
	CKicadSchemBoard*	Convert(const char* szFullPath);
	void				GetTypeName(const char* szFullPath, CChars* pszTypeName);
};


#endif // __KICAD_SCHEM_BOARDS_H__
