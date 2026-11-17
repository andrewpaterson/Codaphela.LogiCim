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
// A tab: a canvas of its own (its own zoom and position) in a document
// pane in the dock's well.  The first is the "Circuit" pane, which can't
// be closed; the others can.  psPlace is where it goes when it's first
// active (NULL for the first, which shows the editor from the start).
//
//////////////////////////////////////////////////////////////////////////
Ptr<CDockPane> CLogicimWindowContent::CreateTab(SCircuitPlace* psPlace)
{
	Ptr<CCircuitCanvas>	pCanvas;
	Ptr<CDockPane>		pPane;
	SDockPlace			sPlace;
	CChars				szID;
	CChars				szTitle;
	bool				bFirst;

	bFirst = maTabPanes.NumElements() == 0;
	pCanvas = OMalloc<CCircuitCanvas>(mpWindow, OMalloc<CCircuitCanvasDraw>(&mpLogicimCircuit), this, mpLogicimCircuit->GetEditor());
	pCanvas->SetAlignment(CSH_Stretch, CSV_Stretch);

	//The circuit's draw reads the circuit, which its edits, the simulation and the view change without telling the canvas.
	pCanvas->SetDrawnEveryPaint(true);
	pCanvas->SetContextMenu(mpCanvasMenu);
	pCanvas->GetCircuitDraw()->SetOwnPlace(psPlace);

	szID.Init(LOGICIM_CIRCUIT_PANE);
	if (!bFirst)
	{
		szID.Append(' ');
		szID.Append((int64)++muiTabsMade);
	}
	szTitle.Init();
	if (psPlace != NULL)
	{
		AppendTabTitle(psPlace, &szTitle);
	}

	//A tab's close cross is made as it's shown, so the first is added hidden, made unclosable, then shown.
	sPlace.InWell();
	pPane = mpDock->AddPane(szID.Text(), szTitle.Text(), Ptr<CImageCel>(), pCanvas, DPK_Document, &sPlace, false);
	pPane->SetClosable(!bFirst);
	maTabPanes.Add(pPane);
	maTabCanvases.Add(pCanvas);
	mpDock->Show(pPane);
	szTitle.Kill();
	szID.Kill();
	return pPane;
}


//////////////////////////////////////////////////////////////////////////
//
// The tab of a pane, ARRAY_ELEMENT_NOT_FOUND for a tool's.
//
//////////////////////////////////////////////////////////////////////////
size CLogicimWindowContent::FindTab(CDockPane* pcPane)
{
	size	i;

	for (i = 0; i < maTabPanes.NumElements(); i++)
	{
		if (&maTabPanes.Get(i) == pcPane)
		{
			return i;
		}
	}
	return ARRAY_ELEMENT_NOT_FOUND;
}


//////////////////////////////////////////////////////////////////////////
//
// The tab made active, or shown in front of its group, is the one
// edited.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ActivePaneChanged(CDockPane* pcPane) { SwitchToTab(FindTab(pcPane)); }
void CLogicimWindowContent::DocumentSelected(CDockPane* pcPane) { SwitchToTab(FindTab(pcPane)); }


//////////////////////////////////////////////////////////////////////////
//
// Tab uiTab is the one edited: the tab left keeps where it was, drawn
// there, and the editor goes to the new tab's place (as a step for Back).
// A tab whose place is gone shows where the editor already is.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::SwitchToTab(size uiTab)
{
	SCircuitPlace			sLeft;
	SCircuitPlace			sPlace;
	Ptr<CCircuitCanvasDraw>	pDraw;

	if ((uiTab >= maTabCanvases.NumElements()) || (uiTab == muiActiveTab))
	{
		return;
	}
	sLeft.Init();
	sPlace.Init();
	GetPlace(&sLeft);
	mpCircuit->GetCircuitDraw()->SetOwnPlace(&sLeft);

	MakeTabActive(uiTab);
	pDraw = mpCircuit->GetCircuitDraw();
	if (pDraw->GetOwnPlace() != NULL)
	{
		sPlace.Copy(pDraw->GetOwnPlace());
	}
	pDraw->SetOwnPlace(NULL);

	RecordLeft(&sLeft);
	mbNavigating = true;
	if (sPlace.szCircuit.Empty() || !ShowPlace(&sPlace))
	{
		UpdateInfo();
	}
	mbNavigating = false;
	sPlace.Kill();
	sLeft.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// A new tab at psPlace, made the one edited.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::OpenInNewTab(SCircuitPlace* psPlace)
{
	mpDock->Activate(CreateTab(psPlace));
	SwitchToTab(maTabPanes.NumElements() - 1);
}


//////////////////////////////////////////////////////////////////////////
//
// Window, New Tab: the place edited, in a tab of its own.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::NewTab(void)
{
	SCircuitPlace	sPlace;

	sPlace.Init();
	GetPlace(&sPlace);
	OpenInNewTab(&sPlace);
	sPlace.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The first tab stays open.  A tab closed goes; if it was the one
// edited, the first is edited.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::PaneClosing(CDockPane* pcPane)
{
	return FindTab(pcPane) != 0;
}


void CLogicimWindowContent::PaneClosed(CDockPane* pcPane)
{
	size	uiTab;

	uiTab = FindTab(pcPane);
	if ((uiTab == ARRAY_ELEMENT_NOT_FOUND) || (uiTab == 0))
	{
		return;
	}
	maTabPanes.RemoveAt(uiTab);
	maTabCanvases.RemoveAt(uiTab);
	if (uiTab == muiActiveTab)
	{
		//The tab closed is gone, so where it was isn't kept.
		MakeTabActive(0);
		ShowTabPlace(0);
		mpDock->Activate(mpCircuitPane);
	}
	else if (uiTab < muiActiveTab)
	{
		muiActiveTab--;
	}
	UpdateInfo();
}


//////////////////////////////////////////////////////////////////////////
//
// The editor goes to tab uiTab's own place, which it then shows (see
// SwitchToTab), without a step for Back.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ShowTabPlace(size uiTab)
{
	SCircuitPlace			sPlace;
	Ptr<CCircuitCanvasDraw>	pDraw;

	pDraw = maTabCanvases.Get(uiTab)->GetCircuitDraw();
	if (pDraw->GetOwnPlace() == NULL)
	{
		return;
	}
	sPlace.Init();
	sPlace.Copy(pDraw->GetOwnPlace());
	pDraw->SetOwnPlace(NULL);
	mbNavigating = true;
	ShowPlace(&sPlace);
	mbNavigating = false;
	sPlace.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The tabs as the project keeps them (CCircuitEditor::SetTabs): each
// one's place, the one edited where the editor is.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::StoreTabs(void)
{
	CArrayCircuitPlace	asTabs;
	SCircuitPlace*		psTab;
	size				i;

	asTabs.Init();
	for (i = 0; i < maTabCanvases.NumElements(); i++)
	{
		psTab = asTabs.Add();
		psTab->Init();
		if (i == muiActiveTab)
		{
			GetPlace(psTab);
		}
		else if (maTabCanvases.Get(i)->GetCircuitDraw()->GetOwnPlace() != NULL)
		{
			psTab->Copy(maTabCanvases.Get(i)->GetCircuitDraw()->GetOwnPlace());
		}
	}
	mpLogicimCircuit->GetEditor()->SetTabs(&asTabs, muiActiveTab);
	ClearCircuitPlaces(&asTabs);
	asTabs.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The tabs of the project just opened: those it kept (none in a new or
// imported project, or one from before tabs), the first tab the first,
// the one that was edited edited.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::RestoreTabs(void)
{
	CArrayCircuitPlace*	pasTabs;
	size				uiActive;
	size				i;

	CloseExtraTabs();
	pasTabs = mpLogicimCircuit->GetEditor()->GetTabs();
	if (pasTabs->NumElements() == 0)
	{
		return;
	}
	uiActive = mpLogicimCircuit->GetEditor()->GetActiveTab();
	for (i = 1; i < pasTabs->NumElements(); i++)
	{
		CreateTab(pasTabs->Get(i));
	}
	maTabCanvases.Get(0)->GetCircuitDraw()->SetOwnPlace(pasTabs->Get(0));
	MakeTabActive((uiActive < maTabPanes.NumElements()) ? uiActive : 0);
	ShowTabPlace(muiActiveTab);
	mpDock->Activate(mpCircuitPane);
}


//////////////////////////////////////////////////////////////////////////
//
// Only the first tab, edited.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::CloseExtraTabs(void)
{
	while (maTabPanes.NumElements() > 1)
	{
		mpDock->Close(maTabPanes.Get(maTabPanes.NumElements() - 1));
	}
	MakeTabActive(0);
	mpCircuit->GetCircuitDraw()->SetOwnPlace(NULL);
}


//////////////////////////////////////////////////////////////////////////
//
// Tab uiTab's canvas and pane are the ones the editor's views are in.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::MakeTabActive(size uiTab)
{
	muiActiveTab = uiTab;
	mpCircuit = maTabCanvases.Get(uiTab);
	mpCircuitPane = maTabPanes.Get(uiTab);
}


//////////////////////////////////////////////////////////////////////////
//
// After an edit, an undo or a load, each tab that isn't edited still
// shows a place that's there: its path as it is now (an instance found
// by name where it's moved to), or as far down it as is left, fitted
// when it's next edited.  A tab whose circuit has gone closes; the first,
// which can't, goes to the top of the circuit simulated.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::FollowTabPlaces(void)
{
	CCircuitEditor*	pcEditor;
	CCircuitView*	pcTop;
	SCircuitPlace*	psPlace;
	SCircuitPlace	sTop;
	CChars			szResolved;
	size			i;
	size			uiTab;

	pcEditor = mpLogicimCircuit->GetEditor();
	if (mbFollowingTabs || (pcEditor->GetCircuits() == NULL) || (pcEditor->GetTopCircuit() == NULL))
	{
		return;
	}
	mbFollowingTabs = true;
	for (i = maTabCanvases.NumElements(); i > 0; i--)
	{
		uiTab = i - 1;
		psPlace = maTabCanvases.Get(uiTab)->GetCircuitDraw()->GetOwnPlace();
		if ((uiTab == muiActiveTab) || (psPlace == NULL))
		{
			continue;
		}
		pcTop = pcEditor->GetCircuits()->Get(psPlace->szCircuit.Text());
		if ((pcTop == NULL) && (uiTab != 0))
		{
			mpDock->Close(maTabPanes.Get(uiTab));
		}
		else if (pcTop == NULL)
		{
			sTop.Init(pcEditor->GetTopCircuit()->GetTypeName(), NULL);
			maTabCanvases.Get(uiTab)->GetCircuitDraw()->SetOwnPlace(&sTop);
			sTop.Kill();
		}
		else
		{
			szResolved.Init();
			if (!ResolveInstancePath(pcTop, psPlace->szPath.Text(), &szResolved))
			{
				psPlace->fZoom = 0;
			}
			psPlace->szPath.Set(szResolved.Text());
			szResolved.Kill();
		}
	}
	mbFollowingTabs = false;
}


//////////////////////////////////////////////////////////////////////////
//
// Each tab's title: the circuit it shows, with the instance it's entered
// ("Gate in U3").
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::UpdateTabTitles(void)
{
	size			i;
	SCircuitPlace	sHere;
	SCircuitPlace*	psPlace;
	CChars			szTitle;

	sHere.Init();
	GetPlace(&sHere);
	for (i = 0; i < maTabPanes.NumElements(); i++)
	{
		psPlace = (i == muiActiveTab) ? &sHere : maTabCanvases.Get(i)->GetCircuitDraw()->GetOwnPlace();
		szTitle.Init();
		if (psPlace != NULL)
		{
			AppendTabTitle(psPlace, &szTitle);
		}
		if (!szTitle.Equals(maTabPanes.Get(i)->GetTitle()))
		{
			maTabPanes.Get(i)->SetTitle(szTitle.Text());
		}
		szTitle.Kill();
	}
	sHere.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// "Gate in U3" for the circuit at the end of the place's path, entered
// through instance U3; the circuit's name at its top; the place's
// circuit if it's gone.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::AppendTabTitle(SCircuitPlace* psPlace, CChars* pszDest)
{
	CCircuitEditor*							pcEditor;
	CCircuitView*							pcTop;
	CCircuitView*							pcCircuit;
	CArrayTemplatePtr<CCircuitInstanceView>	apcPath;
	CComponentView*							pcInstance;

	pcEditor = mpLogicimCircuit->GetEditor();
	pcTop = (pcEditor->GetCircuits() != NULL) ? pcEditor->GetCircuits()->Get(psPlace->szCircuit.Text()) : NULL;
	apcPath.Init();
	pcCircuit = ((pcTop != NULL) && !psPlace->szPath.Empty()) ? FindInstancePath(pcTop, psPlace->szPath.Text(), &apcPath) : NULL;
	if (pcCircuit != NULL)
	{
		pcInstance = GetInstanceComponent(apcPath.GetPtr(apcPath.NumElements() - 1));
		pszDest->Append(pcCircuit->GetTypeName());
		pszDest->Append(" in ");
		pszDest->Append(pcInstance->GetName());
	}
	else
	{
		pszDest->Append(psPlace->szCircuit.Text());
	}
	apcPath.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The canvas menu's Open in New Tab: the subcircuit instance's circuit,
// as entering it would show it, in a tab of its own.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::OpenInstanceInNewTab(CComponentView* pcInstance)
{
	CCircuitEditor*	pcEditor;
	SCircuitPlace	sPlace;
	size			uiIndex;

	pcEditor = mpLogicimCircuit->GetEditor();
	uiIndex = (pcInstance != NULL) ? pcEditor->GetCircuit()->FindComponentIndex(pcInstance) : ARRAY_ELEMENT_NOT_FOUND;
	if ((uiIndex == ARRAY_ELEMENT_NOT_FOUND) || (GetInstanceCircuit(pcInstance) == NULL))
	{
		return;
	}
	sPlace.Init();
	GetPlace(&sPlace);
	AppendInstancePathStep(&sPlace.szPath, uiIndex, pcInstance->GetName());
	sPlace.fZoom = 0;
	OpenInNewTab(&sPlace);
	sPlace.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Before the trees' menu opens on a node, the place its Open in New Tab
// opens: a Simulation node's instance, or a Subcircuits node's circuit
// at its top.  Nowhere if it isn't on a node.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::SimulationTreeMenuAction(SComponentInputEvent* psEvent)
{
	int32	iNode;

	iNode = GetTreeNodeAt(mpSimulationTree, psEvent);
	msTreeMenuPlace.szCircuit.Clear();
	msTreeMenuPlace.szPath.Clear();
	if ((iNode != TREE_NO_NODE) && (mpLogicimCircuit->GetEditor()->GetTopCircuit() != NULL))
	{
		msTreeMenuPlace.szCircuit.Append(mpLogicimCircuit->GetEditor()->GetTopCircuit()->GetTypeName());
		msTreeMenuPlace.szPath.Append(maszSimulationPaths.GetText((size)mpSimulationTree->GetNodeData(iNode)));
	}
	msTreeMenuPlace.fZoom = 0;
}


void CLogicimWindowContent::SubcircuitTreeMenuAction(SComponentInputEvent* psEvent)
{
	int32	iNode;

	iNode = GetTreeNodeAt(mpSubcircuitTree, psEvent);
	msTreeMenuPlace.szCircuit.Clear();
	msTreeMenuPlace.szPath.Clear();
	if (iNode != TREE_NO_NODE)
	{
		msTreeMenuPlace.szCircuit.Append(mpLogicimCircuit->GetEditor()->GetCircuits()->Get((size)mpSubcircuitTree->GetNodeData(iNode))->GetTypeName());
	}
	msTreeMenuPlace.fZoom = 0;
}


//////////////////////////////////////////////////////////////////////////
//
// The node under a press (whose position is the tree's), TREE_NO_NODE if
// it isn't on one.
//
//////////////////////////////////////////////////////////////////////////
int32 GetTreeNodeAt(Ptr<CTreeContainer> pTree, SComponentInputEvent* psEvent)
{
	SInt32Vec2	sPosition;

	sPosition = pTree->GetPosition();
	return pTree->GetRowNode(pTree->FindRowAt(psEvent->x + sPosition.x, psEvent->y + sPosition.y));
}


void CLogicimWindowContent::TreeMenuItemClicked(CMenuItem* pcItem)
{
	if (!msTreeMenuPlace.szCircuit.Empty())
	{
		OpenInNewTab(&msTreeMenuPlace);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A press in a tab's canvas edits that tab first, if the dock hasn't
// made it the active one already.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ActivateCanvas(CCircuitCanvas* pcCanvas)
{
	size	i;

	for (i = 0; i < maTabCanvases.NumElements(); i++)
	{
		if (&maTabCanvases.Get(i) == pcCanvas)
		{
			SwitchToTab(i);
			return;
		}
	}
}


size CLogicimWindowContent::NumTabs(void) { return maTabPanes.NumElements(); }
size CLogicimWindowContent::GetActiveTab(void) const { return muiActiveTab; }
Ptr<CDockPane> CLogicimWindowContent::GetTabPane(size uiTab) { return maTabPanes.Get(uiTab); }


//////////////////////////////////////////////////////////////////////////
//
// Where tab uiTab is: the editor's place for the tab edited, its own for
// the others.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::GetTabPlace(size uiTab, SCircuitPlace* psDest)
{
	if (uiTab == muiActiveTab)
	{
		GetPlace(psDest);
	}
	else if (maTabCanvases.Get(uiTab)->GetCircuitDraw()->GetOwnPlace() != NULL)
	{
		psDest->Copy(maTabCanvases.Get(uiTab)->GetCircuitDraw()->GetOwnPlace());
	}
}

