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
#include "KicadSchemLogicimModel.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::Init(void)
{
	masItems.Init();
	masTerminals.Init();
	masConnections.Init();
	mcSubnetTerminals.Init();
}


//////////////////////////////////////////////////////////////////////////
//
// The views aren't the model's: they're the circuit's.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::Kill(void)
{
	mcSubnetTerminals.Kill();
	masConnections.Kill();
	masTerminals.Kill();
	masItems.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// An item for the view, where KiCad has it (psPosition, in Logicim grid
// units).  The anchor is the port that goes there, if not the view's
// position.  pcSymbol is the symbol it was made for, NULL if none.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimModel::AddItem(CComponentView* pcView, EKicadSchemLogicimItemKind eKind, SFloat64Vec2* psPosition, CPortView* pcAnchor, CKicadSchemSymbol* pcSymbol)
{
	SKicadSchemLogicimItem*		psItem;

	psItem = masItems.Add();
	psItem->pcView = pcView;
	psItem->eKind = eKind;
	psItem->bPlacedSplitter = false;
	psItem->sKicadPosition = *psPosition;
	psItem->pcPort = pcAnchor;
	psItem->pcAttachedTo = NULL;
	psItem->eAlign = KSLA_None;
	psItem->pcSymbol = pcSymbol;
	psItem->sOffset.Init(0, 0);
	psItem->sImportBox.Init();
	psItem->bImportBox = false;
	if (pcAnchor != NULL)
	{
		psItem->sAnchor = pcAnchor->GetGridPosition();
	}
	else
	{
		psItem->sAnchor.Init(0, 0);
	}
	return masItems.NumElements() - 1;
}


//////////////////////////////////////////////////////////////////////////
//
// Nothing is added for a port on no subnet, or no port.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::AddTerminal(CKicadSchemSubnet* pcSubnet, SFloat64Vec2* psKicadPosition, CPortView* pcPortView, size uiItem, bool bBus)
{
	SKicadSchemLogicimTerminal*		psTerminal;

	if ((pcSubnet == NULL) || (pcPortView == NULL))
	{
		return;
	}
	psTerminal = masTerminals.Add();
	psTerminal->pcSubnet = pcSubnet;
	psTerminal->sKicadPosition = *psKicadPosition;
	psTerminal->pcPortView = pcPortView;
	psTerminal->uiItem = uiItem;
	psTerminal->bBus = bBus;
	LinkTerminal(masTerminals.NumElements() - 1);
}


//////////////////////////////////////////////////////////////////////////
//
// The terminal goes on the end of its subnet's terminals.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::LinkTerminal(size uiTerminal)
{
	SKicadSchemLogicimTerminal*				psTerminal;
	SKicadSchemLogicimSubnetTerminals*		psSubnet;
	SKicadSchemLogicimSubnetTerminals		sSubnet;

	psTerminal = masTerminals.Get(uiTerminal);
	psTerminal->uiNextOnSubnet = ARRAY_ELEMENT_NOT_FOUND;
	psSubnet = (SKicadSchemLogicimSubnetTerminals*)mcSubnetTerminals.Get(&psTerminal->pcSubnet, sizeof(psTerminal->pcSubnet));
	if (psSubnet == NULL)
	{
		sSubnet.uiFirst = uiTerminal;
		sSubnet.uiLast = uiTerminal;
		mcSubnetTerminals.Put(&psTerminal->pcSubnet, sizeof(psTerminal->pcSubnet), &sSubnet, sizeof(sSubnet));
		return;
	}
	masTerminals.Get(psSubnet->uiLast)->uiNextOnSubnet = uiTerminal;
	psSubnet->uiLast = uiTerminal;
}


//////////////////////////////////////////////////////////////////////////
//
// Each subnet's terminals linked again, in order, after they've moved.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::IndexTerminals(void)
{
	size	i;

	mcSubnetTerminals.Kill();
	mcSubnetTerminals.Init();
	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		LinkTerminal(i);
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SKicadSchemLogicimConnection* CKicadSchemLogicimModel::AddConnection(void)
{
	return masConnections.Add();
}


//////////////////////////////////////////////////////////////////////////
//
// Takes out the items paiRemoved marks (not 0) and their terminals; the
// others keep their order and are numbered again.  Their views must
// already be gone.  Must be before there are connections.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::RemoveItems(CArrayInt* paiRemoved)
{
	size							i;
	size							uiKept;
	CArrayInt						aiNewIndices;
	SKicadSchemLogicimTerminal*		psTerminal;

	aiNewIndices.Init();
	uiKept = 0;
	for (i = 0; i < masItems.NumElements(); i++)
	{
		if (paiRemoved->GetValue(i) != 0)
		{
			aiNewIndices.Add(-1);
			continue;
		}
		aiNewIndices.Add((int)uiKept);
		if (uiKept != i)
		{
			*masItems.Get(uiKept) = *masItems.Get(i);
		}
		uiKept++;
	}
	masItems.SetUsedElements(uiKept);

	uiKept = 0;
	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		psTerminal = masTerminals.Get(i);
		if (aiNewIndices.GetValue(psTerminal->uiItem) == -1)
		{
			continue;
		}
		psTerminal->uiItem = (size)aiNewIndices.GetValue(psTerminal->uiItem);
		if (uiKept != i)
		{
			*masTerminals.Get(uiKept) = *psTerminal;
		}
		uiKept++;
	}
	masTerminals.SetUsedElements(uiKept);
	aiNewIndices.Kill();
	IndexTerminals();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimModel::NumItems(void) { return masItems.NumElements(); }
SKicadSchemLogicimItem* CKicadSchemLogicimModel::GetItem(size uiIndex) { return masItems.SafeGet(uiIndex); }
size CKicadSchemLogicimModel::NumTerminals(void) { return masTerminals.NumElements(); }
SKicadSchemLogicimTerminal* CKicadSchemLogicimModel::GetTerminal(size uiIndex) { return masTerminals.SafeGet(uiIndex); }
size CKicadSchemLogicimModel::NumConnections(void) { return masConnections.NumElements(); }
SKicadSchemLogicimConnection* CKicadSchemLogicimModel::GetConnection(size uiIndex) { return masConnections.SafeGet(uiIndex); }


//////////////////////////////////////////////////////////////////////////
//
// The first of the subnet's terminals, in the order they were added, and
// the next after uiTerminal; ARRAY_ELEMENT_NOT_FOUND when there are no
// more.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimModel::FirstSubnetTerminal(CKicadSchemSubnet* pcSubnet)
{
	SKicadSchemLogicimSubnetTerminals*	psSubnet;

	psSubnet = (SKicadSchemLogicimSubnetTerminals*)mcSubnetTerminals.Get(&pcSubnet, sizeof(pcSubnet));
	return (psSubnet != NULL) ? psSubnet->uiFirst : ARRAY_ELEMENT_NOT_FOUND;
}


size CKicadSchemLogicimModel::NextSubnetTerminal(size uiTerminal)
{
	return masTerminals.Get(uiTerminal)->uiNextOnSubnet;
}

