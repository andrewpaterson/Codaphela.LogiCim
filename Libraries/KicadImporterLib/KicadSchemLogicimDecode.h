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
#ifndef __KICAD_SCHEM_LOGICIM_DECODE_H__
#define __KICAD_SCHEM_LOGICIM_DECODE_H__
#include "BaseLib/ArrayTemplate.h"
#include "BaseLib/ArrayInt.h"
#include "BaseLib/ArrayChar.h"
#include "BaseLib/ArrayChars.h"
#include "BaseLib/UnionFind.h"
#include "StandardLib/Pointer.h"
#include "MicroprocessorLib/CircuitSimulation.h"
#include "MicroprocessorLib/FamilyVoltageConfigurationStore.h"
#include "MicroprocessorLib/PowerSource.h"
#include "LogicimLib/ViewPath.h"
#include "LogicimLib/ValueView.h"
#include "KicadSchemLogicimConverter.h"


//Grid units between the components of a decoded circuit, so no two components' ports are in the same place.
#define KICAD_SCHEM_DECODE_SPACING		1000


//A board pin the decode drives: an address line (A0 is bit 0) or a mode (iBit -1), driven low or high by connecting one
//of its two supplies to it.
struct SKicadSchemDecodeInput
{
	CPinView*		pcPinView;
	int				iBit;
	CPowerSource*	pcLow;
	CPowerSource*	pcHigh;
	bool			bHigh;
};


//What an output is for one combination of the address lines.
enum EKicadSchemDecodeState
{
	KSDS_Inactive,
	KSDS_Active,
	KSDS_Unsettled,
};


//A board pin the decode reads, active low when its name is overlined (KiCad's "~{RAM}"), its state for each
//combination of the address lines in the mode being decoded (EKicadSchemDecodeState), and the ranges it's active over
//in the mode, a first and last address each, while they're found (iStart is where the open one started, -1 for none).
struct SKicadSchemDecodeOutput
{
	CPinView*		pcPinView;
	bool			bActiveLow;
	CArrayChar		acStates;
	CArrayInt		aiRanges;
	int				iStart;
};


typedef CArrayTemplate<SKicadSchemDecodeInput>	CArrayKicadSchemDecodeInput;
typedef CArrayTemplate<SKicadSchemDecodeOutput>	CArrayKicadSchemDecodeOutput;


//Reads a board's address decode by simulating it: what each chip select is for every address, in each mode.
//
//The sheet's circuit is built by CKicadSchemLogicimConverter (Build: its parts, with no placing or routing) and wired
//by its netlist instead of by wires: every component is moved apart, and a tunnel named as its net goes on each port
//the converter made for a pin, label or supply.  A resistor between two nets that aren't supplies (a series
//resistor, as on chip selects leaving the board) joins them, though the converter leaves resistors out.  Supplies are
//the converter's: its power symbols and the labels named as supplies ("GND", "3.3V").
//
//The board's pins (its edge connector's nets) are its inputs where only inputs are on their nets, and its outputs
//where an output is.  An input named A and a number is that address line, any other input is a mode.  Every
//combination of the address lines is driven in each combination of the modes, the simulation is run until it
//settles, and each output is read: active, inactive or neither (an error).
//
//Decode writes a line for each range of addresses an output is active over in each mode, the address lines the board
//hasn't being any value:
//
//	<mode>\t<output>\t<first>\t<last>
//	<mode>\t<output>\t<first>\t<last>\t<every>\t<through>
//
//The mode is each mode pin's name and level ("Kernal Mode=1,RAM Swap=0"), addresses are $ and hexadecimal, as wide
//as the highest address line, and the second form is ranges as long as the first, every <every> bytes up to
//<through> (a register decoded by A0 to A3 alone, repeating every 16 bytes).
class CKicadSchemLogicimDecode
{
protected:
	CKicadSchemFile*					mpcFile;
	CKicadSchemNetlist*					mpcNetlist;
	CCircuitFonts*						mpcFonts;
	CFamilyVoltageConfigurationStore*	mpcFamilies;
	CCircuitViews						mcCircuits;
	CKicadSchemLogicimConverter			mcConverter;
	CViewPaths							mcPaths;
	CViewPath*							mpcPath;
	Ptr<CCircuitSimulation>				mpSimulation;
	CArrayKicadSchemDecodeInput			masAddresses;
	CArrayKicadSchemDecodeInput			masModes;
	CArrayKicadSchemDecodeOutput		masOutputs;
	int									miAddressWidth;

public:
	void	Init(CKicadSchemFile* pcFile, CKicadSchemNetlist* pcNetlist, CCircuitFonts* pcFonts, CFamilyVoltageConfigurationStore* pcFamilies);
	void	Kill(void);

	bool	Decode(const char* szTypeName, CChars* pszDest);

protected:
	bool	Build(const char* szTypeName);
	void	AddNetTunnels(void);
	bool	Simulate(void);
	bool	FindPins(void);
	void	AddInput(CPinView* pcPinView);
	bool	DecodeMode(size uiMode, CChars* pszDest);
	void	SetInputs(CArrayKicadSchemDecodeInput* pasInputs, size uiValue);
	char	ReadOutput(SKicadSchemDecodeOutput* psOutput);
	void	WriteRanges(size uiMode, CChars* pszDest);
	void	MakeAddressLookups(CArrayInt* paiLow, CArrayInt* paiHigh, int iLowBits);
	void	FindRows(CArrayInt* paiRows);
	void	FindRanges(CArrayInt* paiLow, CArrayInt* paiHigh, int iLowBits, CArrayInt* paiRows);
	void	SetActive(SKicadSchemDecodeOutput* psOutput, int iAddress, bool bActive);
	void	WriteOutputRanges(const char* szMode, SKicadSchemDecodeOutput* psOutput, CChars* pszDest);
	void	AppendMode(size uiMode, CChars* psz);
	void	AppendAddress(int iAddress, CChars* psz);
	size	FindNet(CKicadSchemNet* pcNet);
	size	GetNetGroup(CKicadSchemNet* pcNet);
	CTrace*	GetPortTrace(CPortView* pcPortView);
};


bool DecodeKicadSchemFile(const char* szSchematic, const char* szPropagationTimes, const char* szFontDirectory, CChars* pszDest);


#endif // __KICAD_SCHEM_LOGICIM_DECODE_H__
