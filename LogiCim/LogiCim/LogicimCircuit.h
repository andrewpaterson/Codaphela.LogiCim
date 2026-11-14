/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela LogiCim

Codaphela LogiCim is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela LogiCim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Codaphela LogiCim.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#ifndef __LOGICIM_CIRCUIT_H__
#define __LOGICIM_CIRCUIT_H__
#include "MicroprocessorLib/CircuitSimulation.h"
#include "MicroprocessorLib/FamilyVoltageConfigurationStore.h"
#include "LogicimLib/CircuitEditor.h"
#include "LogicimLib/CircuitFonts.h"
#include "BaseLib/ArrayTemplate.h"


//Where a project starts in the editor's undo stack (made, loaded or imported), its file and the state it was saved
//in.  The snapshots after it, up to the next project's start, are its.
struct SLogicimProjectStart
{
	size	uiId;		//The undo snapshot the project started at.
	size	uiSavedId;	//The undo snapshot of its last change when it was saved (or started), MAX_SIZE if it never was.
	CChars	szFilename;	//Its file, empty if it hasn't one.
};


typedef CArrayTemplate<SLogicimProjectStart>	CArrayLogicimProjectStart;


//LogiCim's editor and the circuits in it.  It starts with an empty "Main" circuit, simulated on its top view path.  A
//project loaded, or a KiCad schematic imported (see CKicadSchemLogicimConverter), replaces it.  It is an object held by the window's
//content; Setup makes the editor.  It knows the project's file: the one it was loaded from or saved to, or after an import the
//schematic's name as an .xml file beside it; empty until there's one.  Undo goes back past a project's start to the
//project before it, so each project's start is kept (masProjects) and the project, its file and whether it has changes
//are those of the start the current snapshot comes after.
class CLogicimCircuit : public CObject
{
CONSTRUCTABLE(CLogicimCircuit);
DESTRUCTABLE(CLogicimCircuit);
protected:
	CFamilyVoltageConfigurationStore	mcFamilies;
	Ptr<CCircuitEditor>					mpEditor;  //It holds the circuit fonts.
	CCircuitColours						mcColours;
	CArrayLogicimProjectStart			masProjects;  //The projects in the undo stack, the oldest first.

public:
	void				Init(void);
	void				Class(void) override;
	void				Free(void) override;

	bool				Save(CObjectWriter* pcFile) override;
	bool				Load(CObjectReader* pcFile) override;

	bool				Setup(const char* szPropagationTimes, const char* szFontDirectory, EColourOrder eColourOrder);

	bool				Reset(void);
	bool				Run(int64 iTimeForward);
	bool				Save(const char* szFilename);
	bool				Load(const char* szFilename);
	bool				ImportKicadSchematic(const char* szFilename);
	bool				NewProject(void);

	CCircuitEditor*		GetEditor(void);
	CCircuitColours*	GetColours(void);
	CCircuitFonts*		GetFonts(void);
	char*				GetFilename(void);
	bool				IsModified(void);
	int64				GetTime(void);

protected:
	bool				CreateSimulation(void);
	bool				LoadCircuits(CCircuitViews* pcCircuits, CCircuitView* pcTop, const char* szSource);
	void				StartProject(const char* szFilename);
	SLogicimProjectStart*	GetProject(void);
	size				GetChangeId(void);
	void				KillProjects(void);
};


#endif // __LOGICIM_CIRCUIT_H__

