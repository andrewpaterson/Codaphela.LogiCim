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
#include <stdlib.h>
#include "BaseLib/Timer.h"
#include "StandardLib/ClassDefines.h"
#include "WindowLib/Window.h"
#include "WindowLib/ComponentInput.h"
#include "LogicimLib/CircuitEditor.h"
#include "LogicimWindowContent.h"
#include "CircuitCanvas.h"


#define CIRCUIT_CANVAS_DOUBLE_CLICK_MILLISECONDS	500  //As Logicim's when the desktop doesn't say.
#define CIRCUIT_CANVAS_DOUBLE_CLICK_PIXELS			4
#define CIRCUIT_CANVAS_WHEEL_ZOOM					(4.0 / 3.0)  //A third more zoom for each notch in.


static EComponentInputCommand	gaeCircuitCanvasCommands[] =
{
	CIC_ActivateComponent, CIC_ExecuteComponent, CIC_StartDragSelect, CIC_ContDragSelect, CIC_StopDragSelect,
	CIC_StartDragScroll, CIC_ContDragScroll, CIC_Delete, CIC_ClearSelection, CIC_Undo, CIC_Redo,
};


//////////////////////////////////////////////////////////////////////////
//
// The canvas takes focus when clicked, so the keys come to it.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::Init(Ptr<CWindow> pWindow, Ptr<CCircuitCanvasDraw> pDraw, CLogicimWindowContent* pcContent, CCircuitEditor* pcEditor)
{
	PreInit();
	CCanvas::Init(pWindow, pDraw);
	mpCircuitDraw = pDraw;
	mpcContent = pcContent;
	mpcEditor = pcEditor;
	msScroll.Init(0, 0);
	msPointer.Init(0, 0);
	miLastRelease = 0;
	msLastRelease.Init(0, 0);
	SetCanGetFocus(true);
	AddInputActions(gaeCircuitCanvasCommands, this, &CCircuitCanvas::CommandAction);
	AddInputAction(IAF_Printable, NULL, this, &CCircuitCanvas::PrintableAction);
	AddInputAction(IAF_Wheel, NULL, this, &CCircuitCanvas::WheelAction);

	//Before the context menu's own action (see CLogicimWindowContent::CreateCanvasMenu), so its items are ready when it opens.
	AddInputAction(CIC_ContextMenu, this, &CCircuitCanvas::ContextMenuAction);
	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
// The input actions of gaeCircuitCanvasCommands.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::CommandAction(SComponentInputEvent* psEvent)
{
	switch (psEvent->eCommand)
	{
	case CIC_ActivateComponent:	ActivateComponent(psEvent->x, psEvent->y);	break;
	case CIC_ExecuteComponent:	ExecuteComponent(psEvent->x, psEvent->y);	break;
	case CIC_StartDragSelect:	StartDragSelect(psEvent->x, psEvent->y);	break;
	case CIC_ContDragSelect:	ContDragSelect(psEvent->x, psEvent->y);		break;
	case CIC_StopDragSelect:	StopDragSelect(psEvent->x, psEvent->y);		break;
	case CIC_StartDragScroll:	StartDragScroll(psEvent->x, psEvent->y);	break;
	case CIC_ContDragScroll:	ContDragScroll(psEvent->x, psEvent->y);		break;
	case CIC_Delete:			Delete();									break;
	case CIC_ClearSelection:	ClearSelection();							break;
	case CIC_Undo:				Undo();										break;
	case CIC_Redo:				Redo();										break;
	default:					break;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::PrintableAction(SComponentInputEvent* psEvent)
{
	Printable(psEvent->cPrintable, psEvent->bInsert);
}


//////////////////////////////////////////////////////////////////////////
//
// The canvas only zooms, so the wheel sideways goes on to its parent.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::WheelAction(SComponentInputEvent* psEvent)
{
	if (!Wheel(psEvent->iRight, psEvent->iDown))
	{
		psEvent->bPass = true;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The right button: the window content selects what it's on and fills
// the canvas's menu, which then opens if it has any items.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::ContextMenuAction(SComponentInputEvent* psEvent)
{
	mpcContent->PrepareCanvasMenu(ToGrid(psEvent->x, psEvent->y));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::Class(void)
{
	CCanvas::Class();
	M_Pointer(mpCircuitDraw);
	U_Pointer(mpcContent);
	U_Pointer(mpcEditor);
	U_Data(SInt32Vec2, msScroll);
	U_Data(SInt32Vec2, msPointer);
	U_Int64(miLastRelease);
	U_Data(SInt32Vec2, msLastRelease);
}


//////////////////////////////////////////////////////////////////////////
//
// x and y are relative to the canvas' top left.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::MovedOver(int x, int y)
{
	msPointer.Init(x, y);
	mpcContent->PointerOverCircuit(x, y);
	UpdateModifiers();
	mpcEditor->PointerMoved(ToGrid(x, y));
}


//////////////////////////////////////////////////////////////////////////
//
// Each notch up zooms in by a third and each notch down out by as much
// (back to where a notch up came from), keeping the circuit under the
// pointer where it is.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitCanvas::Wheel(int iRight, int iDown)
{
	float64	fZoom;
	int		i;

	if (iDown != 0)
	{
		fZoom = mpCircuitDraw->GetZoom();
		for (i = 0; i < abs(iDown); i++)
		{
			fZoom = (iDown < 0) ? fZoom * CIRCUIT_CANVAS_WHEEL_ZOOM : fZoom / CIRCUIT_CANVAS_WHEEL_ZOOM;
		}
		mpCircuitDraw->GetViewport()->ZoomAround(msPointer.x, msPointer.y, fZoom);
		mpcContent->Zoom(mpCircuitDraw->GetZoom());
		return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Button one pressed.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::ActivateComponent(int x, int y)
{
	UpdateModifiers();
	mpcEditor->ButtonPressed(ToGrid(x, y));
}


//////////////////////////////////////////////////////////////////////////
//
// Button one released.  Released twice in half a second within a few
// pixels it's a double click, which enters the subcircuit instance it's
// on (as KiCad enters a sheet) or else edits the properties of the
// component it's on, as Logicim's componentViewDoubleClicked.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::ExecuteComponent(int x, int y)
{
	int64				iNow;
	CComponentView*		pcComponent;

	UpdateModifiers();
	mpcEditor->ButtonReleased(ToGrid(x, y));

	iNow = GetDebugTickCount();
	if ((miLastRelease != 0) && (iNow - miLastRelease <= CIRCUIT_CANVAS_DOUBLE_CLICK_MILLISECONDS) && (abs(x - msLastRelease.x) <= CIRCUIT_CANVAS_DOUBLE_CLICK_PIXELS) && (abs(y - msLastRelease.y) <= CIRCUIT_CANVAS_DOUBLE_CLICK_PIXELS))
	{
		miLastRelease = 0;
		pcComponent = mpcEditor->GetComponentAt(ToGrid(x, y));
		if ((pcComponent != NULL) && (pcComponent->GetCircuitInstanceView() != NULL))
		{
			mpcContent->EnterSubcircuit(pcComponent);
		}
		else
		{
			mpcContent->EditProperties();
		}
	}
	else
	{
		miLastRelease = iNow;
		msLastRelease.Init(x, y);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Button one pressed with shift or control held.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::StartDragSelect(int x, int y)
{
	ActivateComponent(x, y);
}


//////////////////////////////////////////////////////////////////////////
//
// The drag goes on outside the canvas.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::ContDragSelect(int x, int y)
{
	UpdateModifiers();
	mpcEditor->PointerMoved(ToGrid(x, y));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::StopDragSelect(int x, int y)
{
	ExecuteComponent(x, y);
}


//////////////////////////////////////////////////////////////////////////
//
// Button three drags the circuit about, as Logicim's middle button.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::StartDragScroll(int x, int y)
{
	msScroll.Init(x, y);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::ContDragScroll(int x, int y)
{
	mpCircuitDraw->GetViewport()->Scroll(msScroll.x - x, msScroll.y - y);
	msScroll.Init(x, y);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::Delete(void)
{
	mpcEditor->Delete();
}


//////////////////////////////////////////////////////////////////////////
//
// Escape.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::ClearSelection(void)
{
	mpcEditor->StopEdit();
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's keys that are printable.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::Printable(char c, bool bInsert)
{
	const char*	szXMLName;

	switch (c)
	{
	case 'r':
		mpcEditor->RotateLeft();
		return;
	case 'R':
		mpcEditor->RotateRight();
		return;
	case 'x':
		mpcEditor->FlipHorizontally();
		return;
	case 'y':
		mpcEditor->FlipVertically();
		return;
	case 'e':
	case 'E':
		mpcContent->EditProperties();
		return;
	case 'm':
		mpcEditor->StartMove();
		return;
	case 'g':
		szXMLName = "Ground";
		break;
	case 'v':
		szXMLName = "PositivePower";
		break;
	case 'p':
		szXMLName = "Pin";
		break;
	case 'c':
		szXMLName = "Constant";
		break;
	case 'A':
		szXMLName = "AndGate";
		break;
	case 'O':
		szXMLName = "OrGate";
		break;
	case 'X':
		szXMLName = "XorGate";
		break;
	case 'N':
		szXMLName = "Inverter";
		break;
	case 'C':
		szXMLName = "Clock";
		break;
	case 'P':
		szXMLName = "Probe";
		break;
	case 'S':
		szXMLName = "Splitter";
		break;
	case 'T':
		szXMLName = "Tunnel";
		break;
	case 'L':
		szXMLName = "Label";
		break;
	case 'F':
		szXMLName = "DTypeFlipFlop";
		break;
	default:
		return;
	}
	mpcEditor->StartPlaceComponent(szXMLName);
}


//////////////////////////////////////////////////////////////////////////
//
// Ctrl+Z and Ctrl+Shift+Z.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::Undo(void) { mpcContent->Undo(false); }
void CCircuitCanvas::Redo(void) { mpcContent->Redo(false); }


//////////////////////////////////////////////////////////////////////////
//
// x and y are relative to the canvas' top left.
//
//////////////////////////////////////////////////////////////////////////
SFloat64Vec2 CCircuitCanvas::ToGrid(int x, int y)
{
	SFloat64Vec2	sScreen;
	SFloat64Vec2	sGrid;

	sScreen.Init(x, y);
	mpCircuitDraw->GetViewport()->ScreenToGrid(&sGrid, &sScreen);
	return sGrid;
}


//////////////////////////////////////////////////////////////////////////
//
// The selection mode follows control and shift.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitCanvas::UpdateModifiers(void)
{
	CComponentInput*	pcInput;

	pcInput = mpWindow->GetComponentInput();
	if (pcInput != NULL)
	{
		mpcEditor->SetModifiers(pcInput->IsControl(), pcInput->IsShift());
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CCircuitCanvasDraw> CCircuitCanvas::GetCircuitDraw(void) { return mpCircuitDraw; }

