#ifndef __LOGICIM_WINDOW_DATA_H__
#define __LOGICIM_WINDOW_DATA_H__
#include "BaseLib/PrimitiveTypes.h"


//Dialogs are shown from the window's tick, not from the listener of the button or menu item that asked for them,
//because a modal dialog reads the input itself.
enum ELogicimDialog
{
	LDG_None,
	LDG_Properties,
	LDG_ChooseComponent,
	LDG_NewSubcircuit,
	LDG_NewSimulation,
	LDG_LoadProject,
	LDG_SaveProject,
};


class CLogicimWindowContent;
struct SLogicimWindowData
{
	CLogicimWindowContent*	pcContent;
	ELogicimDialog			eDialog;  //The dialog to show next tick.
};


#endif // __LOGICIM_WINDOW_DATA_H__

