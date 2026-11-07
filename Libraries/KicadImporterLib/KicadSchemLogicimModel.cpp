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
}


//////////////////////////////////////////////////////////////////////////
//
// The views aren't the model's: they're the circuit's.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::Kill(void)
{
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

