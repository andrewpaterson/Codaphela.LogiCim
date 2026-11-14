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
#include "BaseLib/StringHelper.h"
#include "StandardLib/Objects.h"
#include "LogicimLib/CircuitEditor.h"
#include "LogicimWindowContent.h"


//////////////////////////////////////////////////////////////////////////
//
// Where the editor is and how the canvas shows it.  Nowhere (no circuit)
// before a project is set up.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::GetPlace(SLogicimPlace* psDest)
{
	CCircuitEditor*		pcEditor;
	CCircuitViewport*	pcViewport;

	psDest->szCircuit.Clear();
	psDest->szPath.Clear();
	pcEditor = mpLogicimCircuit->GetEditor();
	if (pcEditor->GetTopCircuit() != NULL)
	{
		psDest->szCircuit.Append(pcEditor->GetTopCircuit()->GetTypeName());
		pcEditor->GetInstancePathText(&psDest->szPath);
	}
	pcViewport = mpCircuit->GetCircuitDraw()->GetViewport();
	psDest->sPosition = pcViewport->GetPosition();
	psDest->fZoom = pcViewport->GetZoom();
}


//////////////////////////////////////////////////////////////////////////
//
// The editor goes to the place: its circuit simulated, if it isn't (as
// choosing it in Subcircuits does), and the path into it entered; the
// canvas shows it as the place says.  False, nothing changed, if the
// place is gone (its circuit, or an instance on its path).
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ShowPlace(SLogicimPlace* psPlace)
{
	CCircuitEditor*							pcEditor;
	CCircuitView*							pcTop;
	CArrayTemplatePtr<CCircuitInstanceView>	apcPath;
	SLogicimPlace							sHere;
	bool									bPath;
	bool									bMoved;

	pcEditor = mpLogicimCircuit->GetEditor();
	pcTop = (pcEditor->GetCircuits() != NULL) ? pcEditor->GetCircuits()->Get(psPlace->szCircuit.Text()) : NULL;
	if (pcTop == NULL)
	{
		return false;
	}
	apcPath.Init();
	bPath = psPlace->szPath.Empty() || (pcEditor->FindInstancePath(pcTop, psPlace->szPath.Text(), &apcPath) != NULL);
	apcPath.Kill();
	if (!bPath)
	{
		return false;
	}

	sHere.Init();
	GetPlace(&sHere);
	bMoved = !sHere.IsSameView(psPlace);
	if (bMoved)
	{
		if (((pcEditor->GetTopCircuit() != pcTop) && !pcEditor->Simulate(psPlace->szCircuit.Text())) || !pcEditor->EnterInstancePath(psPlace->szPath.Text()))
		{
			sHere.Kill();
			UpdateInfo();
			return false;
		}
		RecordLeft(&sHere);
	}
	sHere.Kill();

	if (psPlace->fZoom > 0)
	{
		mpCircuit->GetCircuitDraw()->SetZoom((float32)psPlace->fZoom);
		mpCircuit->GetCircuitDraw()->GetViewport()->SetPosition(psPlace->sPosition.x, psPlace->sPosition.y);
		mpCircuit->Invalidate();
		ShowZoom();
	}
	else if (bMoved)
	{
		ZoomToFit(false);
	}
	UpdateInfo();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// A move left psLeft, for Back, unless Back or Forward made it.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::RecordLeft(SLogicimPlace* psLeft)
{
	if (!mbNavigating && !psLeft->szCircuit.Empty())
	{
		mcHistory.Record(psLeft);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// View, Back and Forward: to the latest place left, or gone back from,
// that's still there, as it was shown, the place left kept for going the
// other way.  False if there's none.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::Go(bool bBack)
{
	SLogicimPlace	sHere;
	SLogicimPlace	sPlace;
	bool			bShown;

	sHere.Init();
	sPlace.Init();
	GetPlace(&sHere);
	bShown = false;
	mbNavigating = true;
	while (!bShown && mcHistory.Take(bBack, &sPlace))
	{
		bShown = ShowPlace(&sPlace);
	}
	mbNavigating = false;
	if (bShown)
	{
		mcHistory.Put(!bBack, &sHere);
	}
	sPlace.Kill();
	sHere.Kill();
	return bShown;
}


bool CLogicimWindowContent::GoBack(void) { return Go(true); }
bool CLogicimWindowContent::GoForward(void) { return Go(false); }
void CLogicimWindowContent::NavigateBack(void) { Go(true); }
void CLogicimWindowContent::NavigateForward(void) { Go(false); }
bool CLogicimWindowContent::CanGoBack(void) { return mcHistory.Num(true) > 0; }
bool CLogicimWindowContent::CanGoForward(void) { return mcHistory.Num(false) > 0; }
CLogicimNavigationHistory* CLogicimWindowContent::GetHistory(void) { return &mcHistory; }

