#ifndef __LOGICIM_CIRCUIT_H__
#define __LOGICIM_CIRCUIT_H__
#include "MicroprocessorLib/CircuitSimulation.h"
#include "MicroprocessorLib/FamilyVoltageConfigurationStore.h"
#include "LogicimLib/CircuitEditor.h"
#include "LogicimLib/CircuitFonts.h"


//LogiCim's editor and the circuits in it.  It starts with an empty "Main" circuit, simulated on its top view path.  A
//project loaded replaces it.
class CLogicimCircuit
{
protected:
	CFamilyVoltageConfigurationStore	mcFamilies;
	CCircuitEditor						mcEditor;
	CCircuitColours						mcColours;
	CCircuitFonts						mcFonts;

public:
	bool				Init(const char* szPropagationTimes, const char* szFontDirectory, EColourOrder eColourOrder);
	void				Kill(void);

	bool				Reset(void);
	void				Run(int64 iTimeForward);
	bool				Save(const char* szFilename);
	bool				Load(const char* szFilename);

	CCircuitEditor*		GetEditor(void);
	CCircuitColours*	GetColours(void);
	CCircuitFonts*		GetFonts(void);
	int64				GetTime(void);

protected:
	void				RemoveFontFamily(Ptr<CFontFamily> pFamily);
	bool				CreateSimulation(void);
};


#endif // __LOGICIM_CIRCUIT_H__

