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
// The Subcircuits pane: at the top the project's circuits nothing uses
// (Main), under each circuit the circuits it uses, so a circuit used in
// several is under each, in the project's order; the circuit edited's
// first node selected with its parents opened.  The nodes are only made
// again when the circuits, or what they use, have changed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ListSubcircuits(void)
{
	CCircuitViews*	pcCircuits;
	CCircuitView*	pcEdited;
	CChars			szShape;
	size			i;
	int32			iNode;
	int32			iParent;

	if (mpSubcircuitTree.IsNull() || mpLogicimCircuit.IsNull() || (mpLogicimCircuit->GetEditor()->GetCircuits() == NULL))
	{
		return;
	}
	pcCircuits = mpLogicimCircuit->GetEditor()->GetCircuits();
	pcEdited = mpLogicimCircuit->GetEditor()->GetCircuit();

	mbListingSubcircuits = true;
	szShape.Init();
	for (i = 0; i < pcCircuits->NumCircuits(); i++)
	{
		if (!IsCircuitUsed(pcCircuits, pcCircuits->Get(i)))
		{
			AddCircuitNodes(TREE_NO_NODE, i, &szShape, false);
		}
	}
	if (!szShape.Equals(mszSubcircuitShape.Text()))
	{
		mszSubcircuitShape.Set(szShape.Text());
		mpSubcircuitTree->RemoveAllNodes();
		for (i = 0; i < pcCircuits->NumCircuits(); i++)
		{
			if (!IsCircuitUsed(pcCircuits, pcCircuits->Get(i)))
			{
				iNode = AddCircuitNodes(TREE_NO_NODE, i, NULL, true);
				mpSubcircuitTree->SetExpanded(iNode, mpSubcircuitTree->HasChildren(iNode));
			}
		}
	}
	szShape.Kill();

	for (iNode = 0; iNode < mpSubcircuitTree->NumNodes(); iNode++)
	{
		if (pcCircuits->Get((size)mpSubcircuitTree->GetNodeData(iNode)) == pcEdited)
		{
			for (iParent = mpSubcircuitTree->GetNodeParent(iNode); iParent != TREE_NO_NODE; iParent = mpSubcircuitTree->GetNodeParent(iParent))
			{
				mpSubcircuitTree->SetExpanded(iParent, true);
			}
			if (mpSubcircuitTree->GetSelectedNode() != iNode)
			{
				mpSubcircuitTree->SelectNode(iNode);
			}
			break;
		}
	}
	mbListingSubcircuits = false;
}


//////////////////////////////////////////////////////////////////////////
//
// The project's circuit uiCircuit and, under it, those it uses: as nodes
// under iParent (bNodes), and as text in pszShape (if it isn't NULL), to
// tell whether they changed.  The circuit's node, TREE_NO_NODE if none
// was made.
//
//////////////////////////////////////////////////////////////////////////
int32 CLogicimWindowContent::AddCircuitNodes(int32 iParent, size uiCircuit, CChars* pszShape, bool bNodes)
{
	CCircuitViews*	pcCircuits;
	CCircuitView*	pcCircuit;
	size			i;
	int32			iNode;

	pcCircuits = mpLogicimCircuit->GetEditor()->GetCircuits();
	pcCircuit = pcCircuits->Get(uiCircuit);
	iNode = bNodes ? mpSubcircuitTree->AddNode(iParent, pcCircuit->GetTypeName(), NULL, (int64)uiCircuit) : TREE_NO_NODE;
	if (pszShape != NULL)
	{
		pszShape->Append(pcCircuit->GetTypeName());
		pszShape->Append('(');
	}
	for (i = 0; i < pcCircuits->NumCircuits(); i++)
	{
		if (CircuitUsesDirectly(pcCircuit, pcCircuits->Get(i)))
		{
			AddCircuitNodes(iNode, i, pszShape, bNodes);
		}
	}
	if (pszShape != NULL)
	{
		pszShape->Append(')');
	}
	return iNode;
}


//////////////////////////////////////////////////////////////////////////
//
// The name of the circuit being edited, empty before there is one.
//
//////////////////////////////////////////////////////////////////////////
const char* CLogicimWindowContent::GetEditedCircuitName(void)
{
	CCircuitView*	pcCircuit;

	pcCircuit = mpLogicimCircuit.IsNotNull() ? mpLogicimCircuit->GetEditor()->GetCircuit() : NULL;
	return (pcCircuit != NULL) ? pcCircuit->GetTypeName() : "";
}


Ptr<CTreeContainer> CLogicimWindowContent::GetSubcircuitTree(void) { return mpSubcircuitTree; }


//////////////////////////////////////////////////////////////////////////
//
// The Simulation pane: the circuit simulated at the top and under each
// circuit its subcircuit instances, in the circuit's order ("U3
// (Board)"), the place being edited selected with its parents opened.
// The nodes are only made again when the instances have changed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::ListSimulation(void)
{
	CCircuitEditor*	pcEditor;
	CCircuitView*	pcTop;
	CChars			szShape;
	CChars			szPath;
	int32			iNode;
	int32			iParent;

	if (mpSimulationTree.IsNull() || mpLogicimCircuit.IsNull())
	{
		return;
	}
	pcEditor = mpLogicimCircuit->GetEditor();
	pcTop = pcEditor->GetTopCircuit();

	szShape.Init();
	if (pcTop != NULL)
	{
		szShape.Append(pcTop->GetTypeName());
		AppendInstanceShape(&szShape, pcTop);
	}
	if (!szShape.Equals(mszSimulationShape.Text()))
	{
		mszSimulationShape.Set(szShape.Text());
		mpSimulationTree->RemoveAllNodes();
		maszSimulationPaths.Kill();
		maszSimulationPaths.Init();
		if (pcTop != NULL)
		{
			maszSimulationPaths.Add("");
			iNode = mpSimulationTree->AddNode(TREE_NO_NODE, pcTop->GetTypeName(), NULL, 0);
			AddInstanceNodes(iNode, pcTop, "");
			mpSimulationTree->SetExpanded(iNode, true);
		}
	}
	szShape.Kill();

	//The place edited.
	szPath.Init();
	pcEditor->GetInstancePathText(&szPath);
	for (iNode = 0; iNode < mpSimulationTree->NumNodes(); iNode++)
	{
		if (StringCompare(maszSimulationPaths.GetText((size)mpSimulationTree->GetNodeData(iNode)), szPath.Text()) == 0)
		{
			for (iParent = mpSimulationTree->GetNodeParent(iNode); iParent != TREE_NO_NODE; iParent = mpSimulationTree->GetNodeParent(iParent))
			{
				mpSimulationTree->SetExpanded(iParent, true);
			}
			if (mpSimulationTree->GetSelectedNode() != iNode)
			{
				mpSimulationTree->SelectNode(iNode);
			}
			break;
		}
	}
	szPath.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The subcircuit instances in pcCircuit and in theirs, as the Simulation
// tree shows them, for telling whether they changed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::AppendInstanceShape(CChars* pszDest, CCircuitView* pcCircuit)
{
	size				i;
	CCircuitView*		pcSubcircuit;

	pszDest->Append('(');
	for (i = 0; i < pcCircuit->NumComponents(); i++)
	{
		pcSubcircuit = GetInstanceCircuit(pcCircuit->GetComponent(i));
		if (pcSubcircuit != NULL)
		{
			pszDest->Append((int64)i);
			pszDest->Append(' ');
			pszDest->Append(pcCircuit->GetComponent(i)->GetName());
			pszDest->Append(' ');
			pszDest->Append(pcSubcircuit->GetTypeName());
			AppendInstanceShape(pszDest, pcSubcircuit);
		}
	}
	pszDest->Append(')');
}


//////////////////////////////////////////////////////////////////////////
//
// A node under iParent for each subcircuit instance in pcCircuit (at
// szPath), and under each the instances in its circuit.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::AddInstanceNodes(int32 iParent, CCircuitView* pcCircuit, const char* szPath)
{
	size				i;
	CCircuitView*		pcSubcircuit;
	CChars				szText;
	CChars				szChildPath;
	int32				iNode;

	for (i = 0; i < pcCircuit->NumComponents(); i++)
	{
		pcSubcircuit = GetInstanceCircuit(pcCircuit->GetComponent(i));
		if (pcSubcircuit != NULL)
		{
			szChildPath.Init(szPath);
			if (!StrEmpty(szPath))
			{
				szChildPath.Append('/');
			}
			szChildPath.Append((int64)i);
			szText.Init(pcCircuit->GetComponent(i)->GetName());
			szText.Append(" (");
			szText.Append(pcSubcircuit->GetTypeName());
			szText.Append(")");
			iNode = mpSimulationTree->AddNode(iParent, szText.Text(), NULL, (int64)maszSimulationPaths.NumElements());
			maszSimulationPaths.Add(szChildPath.Text());
			AddInstanceNodes(iNode, pcSubcircuit, szChildPath.Text());
			szText.Kill();
			szChildPath.Kill();
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A double click (or enter) on a node edits its instance, fitted to the
// canvas.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimWindowContent::SimulationNodeActivated(void)
{
	int32			iNode;
	SCircuitPlace	sPlace;

	iNode = mpSimulationTree->GetSelectedNode();
	if ((iNode == TREE_NO_NODE) || (mpLogicimCircuit->GetEditor()->GetTopCircuit() == NULL))
	{
		return;
	}
	sPlace.Init(mpLogicimCircuit->GetEditor()->GetTopCircuit()->GetTypeName(), maszSimulationPaths.GetText((size)mpSimulationTree->GetNodeData(iNode)));
	ShowPlace(&sPlace);
	sPlace.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// For checking: selects the node of the instance at szPath and activates
// it, as a double click does.  False if there's no such node.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimWindowContent::ActivateSimulationNode(const char* szPath)
{
	int32	iNode;

	for (iNode = 0; iNode < mpSimulationTree->NumNodes(); iNode++)
	{
		if (StringCompare(maszSimulationPaths.GetText((size)mpSimulationTree->GetNodeData(iNode)), szPath) == 0)
		{
			mpSimulationTree->SelectNode(iNode);
			mpSimulationTree->Activate();
			return true;
		}
	}
	return false;
}


Ptr<CTreeContainer> CLogicimWindowContent::GetSimulationTree(void) { return mpSimulationTree; }


//////////////////////////////////////////////////////////////////////////
//
// Whether pcCircuit has an instance of pcUsed in it (not in its
// subcircuits).
//
//////////////////////////////////////////////////////////////////////////
bool CircuitUsesDirectly(CCircuitView* pcCircuit, CCircuitView* pcUsed)
{
	size	i;

	for (i = 0; i < pcCircuit->NumComponents(); i++)
	{
		if (GetInstanceCircuit(pcCircuit->GetComponent(i)) == pcUsed)
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether any of the project's circuits has an instance of pcCircuit.
//
//////////////////////////////////////////////////////////////////////////
bool IsCircuitUsed(CCircuitViews* pcCircuits, CCircuitView* pcCircuit)
{
	size	i;

	for (i = 0; i < pcCircuits->NumCircuits(); i++)
	{
		if (CircuitUsesDirectly(pcCircuits->Get(i), pcCircuit))
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit of a subcircuit instance, NULL for any other component.
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* GetInstanceCircuit(CComponentView* pcComponent)
{
	CCircuitInstanceView*	pcInstance;

	pcInstance = pcComponent->GetCircuitInstanceView();
	return (pcInstance != NULL) ? pcInstance->GetInstanceCircuitView() : NULL;
}

