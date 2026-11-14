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
void CLogicimWindowContent::GetPlace(SCircuitPlace* psDest)
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
bool CLogicimWindowContent::ShowPlace(SCircuitPlace* psPlace)
{
	CCircuitEditor*							pcEditor;
	CCircuitView*							pcTop;
	CArrayTemplatePtr<CCircuitInstanceView>	apcPath;
	SCircuitPlace							sHere;
	bool									bPath;
	bool									bMoved;

	pcEditor = mpLogicimCircuit->GetEditor();
	pcTop = (pcEditor->GetCircuits() != NULL) ? pcEditor->GetCircuits()->Get(psPlace->szCircuit.Text()) : NULL;
	if (pcTop == NULL)
	{
		return false;
	}
	apcPath.Init();
	bPath = psPlace->szPath.Empty() || (FindInstancePath(pcTop, psPlace->szPath.Text(), &apcPath) != NULL);
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
void CLogicimWindowContent::RecordLeft(SCircuitPlace* psLeft)
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
	SCircuitPlace	sHere;
	SCircuitPlace	sPlace;
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


//////////////////////////////////////////////////////////////////////////
//
// View, Go To: a dialog can't be shown from the keys or a menu item's
// input action, so it's shown at the next tick.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::RequestGoTo(void)
{
	mpWindow->Post(this, &CLogicimWindowContent::ShowGoToDialog);
}


//////////////////////////////////////////////////////////////////////////
//
// Go To, as the HCP65 IDE's: Components, Nets and Subcircuits tabs, the
// result chosen gone to.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowGoToDialog(void)
{
	mpGoToDialog = OMalloc<CGoToDialog>(mpWindow, &msDialogLook, "Go To", &mcGoTo, mpFontColours->GetFont(mpTheme->GetColour(TC_BrightText)));
	mpGoToDialog->AddTab("Components", mpCommands->Find("View.GoTo"));
	mpGoToDialog->AddTab("Nets");
	mpGoToDialog->AddTab("Subcircuits");
	mpGoToDialog->Start(LGT_Components, "");
	if (mpGoToDialog->Show())
	{
		GoToTarget(mcGoTo.GetTarget(mpGoToDialog->GetChosen()));
	}
	mpGoToDialog = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// What Go To's tab finds for the pattern, best first; the targets are
// the searcher's until it next searches (GetGoToTarget).
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::SearchGoTo(size uiTab, const char* szPattern, CGoToItems* pcDest)
{
	CNameMatcher	cMatcher;

	cMatcher.Init(szPattern);
	mcGoTo.GoToSearch(uiTab, &cMatcher, pcDest);
	pcDest->Sort();
	cMatcher.Kill();
}


SLogicimGoToTarget* CLogicimWindowContent::GetGoToTarget(SGoToItem* psItem) { return mcGoTo.GetTarget(psItem); }


//////////////////////////////////////////////////////////////////////////
//
// The editor goes to a Go To result (a step for Back): a component is
// selected and centred on, in the instance it was found in; a subcircuit
// is edited at its top.  False if it's gone.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::GoToTarget(SLogicimGoToTarget* psTarget)
{
	SCircuitPlace	sPlace;
	CCircuitEditor*	pcEditor;
	bool			bResult;

	if (psTarget == NULL)
	{
		return false;
	}
	sPlace.Init(psTarget->szCircuit.Text(), psTarget->szPath.Text());
	bResult = ShowPlace(&sPlace);
	sPlace.Kill();
	pcEditor = mpLogicimCircuit->GetEditor();
	if (bResult && (psTarget->uiComponent != ARRAY_ELEMENT_NOT_FOUND))
	{
		bResult = psTarget->uiComponent < pcEditor->GetCircuit()->NumComponents();
		if (bResult)
		{
			pcEditor->SelectOnly(pcEditor->GetCircuit()->GetComponent(psTarget->uiComponent));
			CentreOnSelection();
		}
	}
	UpdateInfo();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// The canvas scrolls, its zoom as it is, so the selection is in its
// middle.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::CentreOnSelection(void)
{
	SFloat64Bounds		sBox;
	CCircuitViewport*	pcViewport;
	float64				fScale;

	if (mpLogicimCircuit->GetEditor()->GetSelection()->GetBoundingBox(&sBox))
	{
		pcViewport = mpCircuit->GetCircuitDraw()->GetViewport();
		fScale = pcViewport->GetPixelsPerGrid();
		pcViewport->SetPosition(-(sBox.sMinimum.x + sBox.sMaximum.x) / 2 * fScale, -(sBox.sMinimum.y + sBox.sMaximum.y) / 2 * fScale);
		mpCircuit->Invalidate();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Undo or redo pressed: through the revealing undo, which shows a change
// out of view before undoing it, saying so in the status bar with the
// keys that undo it.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::PressUndo(bool bRedo, bool bStep)
{
	CChars			szKeys;
	CChars			szMessage;
	Ptr<CCommand>	pCommand;

	pCommand = mpCommands->Find(bStep ? (bRedo ? "Edit.StepRedo" : "Edit.StepUndo") : (bRedo ? "Edit.Redo" : "Edit.Undo"));
	szKeys.Init();
	if (pCommand.IsNotNull())
	{
		pCommand->AppendKeysText(&szKeys, mpWindow->GetComponentInput());
	}
	szMessage.Init();
	mbUndoStep = bStep;
	mcUndo.Press(bRedo, szKeys.Text(), &szMessage);
	mpMessageInfo->SetText(szMessage.Text());
	szMessage.Kill();
	szKeys.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The change the next undo (or redo) would undo is in view if it was made
// in the place edited, or if that place is gone (then there's nowhere to
// show it, and it's undone as it is).
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::IsUndoShown(bool bRedo)
{
	SCircuitPlace	sChange;
	SCircuitPlace	sHere;
	CCircuitView*	pcTop;
	CChars			szPath;
	bool			bShown;

	sChange.Init();
	sHere.Init();
	bShown = true;
	if (mpLogicimCircuit->GetEditor()->GetUndoPlace(bRedo, mbUndoStep, &sChange))
	{
		GetPlace(&sHere);
		pcTop = mpLogicimCircuit->GetEditor()->GetCircuits()->Get(sChange.szCircuit.Text());
		szPath.Init();
		bShown = sHere.IsSameView(&sChange) || (pcTop == NULL) || !ResolveInstancePath(pcTop, sChange.szPath.Text(), &szPath);
		szPath.Kill();
	}
	sHere.Kill();
	sChange.Kill();
	return bShown;
}


//////////////////////////////////////////////////////////////////////////
//
// The editor goes to where the change was made (a step for Back).
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ShowUndo(bool bRedo)
{
	SCircuitPlace	sChange;
	bool			bResult;

	sChange.Init();
	bResult = mpLogicimCircuit->GetEditor()->GetUndoPlace(bRedo, mbUndoStep, &sChange) && ShowPlace(&sChange);
	sChange.Kill();
	return bResult;
}


bool CLogicimWindowContent::DoUndo(bool bRedo)
{
	bool	bResult;

	bResult = bRedo ? mpLogicimCircuit->GetEditor()->Redo(mbUndoStep) : mpLogicimCircuit->GetEditor()->Undo(mbUndoStep);
	UpdateInfo();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// The undo history's position and the place edited: a second press is
// only a second press if neither has changed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::AppendUndoState(CChars* pszDest)
{
	SCircuitPlace	sHere;

	pszDest->Append((int64)mpLogicimCircuit->GetEditor()->GetUndoStack()->GetCurrentId());
	pszDest->Append(' ');
	sHere.Init();
	GetPlace(&sHere);
	sHere.Print(pszDest);
	sHere.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// "in Board in U2": as the change's place's tab would be titled.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::AppendUndoWhere(bool bRedo, CChars* pszDest)
{
	SCircuitPlace	sChange;

	sChange.Init();
	if (mpLogicimCircuit->GetEditor()->GetUndoPlace(bRedo, mbUndoStep, &sChange))
	{
		pszDest->Append("in ");
		AppendTabTitle(&sChange, pszDest);
	}
	sChange.Kill();
}


char* CLogicimWindowContent::GetMessage(void) { return mpMessageInfo->GetText(); }

