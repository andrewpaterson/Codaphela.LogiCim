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
#include <math.h>
#include "BaseLib/PointerFunctions.h"
#include "KicadSchemLogicimModel.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::Init(void)
{
	mpcCircuit = NULL;
	masItems.Init();
	masTerminals.Init();
	masConnections.Init();
	mcSubnetTerminals.Init();
	mcPortTerminals.Init();
	mcViewItems.Init();
	mcBoardPins.Init();
}


//////////////////////////////////////////////////////////////////////////
//
// The views aren't the model's: they're the circuit's.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::Kill(void)
{
	mcBoardPins.Kill();
	mcViewItems.Kill();
	mcPortTerminals.Kill();
	mcSubnetTerminals.Kill();
	masConnections.Kill();
	masTerminals.Kill();
	masItems.Kill();
	mpcCircuit = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit the items are made in.  Must be set before they're added.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::SetCircuit(CCircuitView* pcCircuit)
{
	mpcCircuit = pcCircuit;
}


//////////////////////////////////////////////////////////////////////////
//
// Adds the view, made at (0, 0), to the circuit, and an item for it as
// what eKind says, where KiCad has it (psKicadPosition, scaled to Logicim
// grid units).  The anchor is the port that goes there, if not the view's
// position.  pcSymbol is the symbol it was made for, NULL for a tunnel or
// splitter.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimModel::AddItem(CComponentView* pcView, EKicadSchemLogicimItemKind eKind, SFloat64Vec2* psKicadPosition, CPortView* pcAnchor, CKicadSchemSymbol* pcSymbol)
{
	SKicadSchemLogicimItem*		psItem;

	mpcCircuit->Add(pcView);
	psItem = &masItems.Add()->sItem;
	psItem->pcView = pcView;
	psItem->eKind = eKind;
	psItem->bPlacedSplitter = false;
	psItem->sKicadPosition.Init(psKicadPosition->x / KICAD_SCHEM_LOGICIM_GRID * KICAD_SCHEM_LOGICIM_SCALE, psKicadPosition->y / KICAD_SCHEM_LOGICIM_GRID * KICAD_SCHEM_LOGICIM_SCALE);
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
	LinkItem(masItems.NumElements() - 1);
	return masItems.NumElements() - 1;
}


//////////////////////////////////////////////////////////////////////////
//
// The item, with no terminals or attached items yet, is found by its view
// and, a board pin, by its name.  The first of a name is kept.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::LinkItem(size uiItem)
{
	SKicadSchemLogicimModelItem*	psModelItem;
	SKicadSchemLogicimItem*			psItem;

	psModelItem = masItems.Get(uiItem);
	psModelItem->uiFirstTerminal = ARRAY_ELEMENT_NOT_FOUND;
	psModelItem->uiLastTerminal = ARRAY_ELEMENT_NOT_FOUND;
	psModelItem->uiFirstAttached = ARRAY_ELEMENT_NOT_FOUND;
	psModelItem->uiLastAttached = ARRAY_ELEMENT_NOT_FOUND;
	psModelItem->uiNextAttached = ARRAY_ELEMENT_NOT_FOUND;
	psItem = &psModelItem->sItem;
	mcViewItems.Put(&psItem->pcView, sizeof(psItem->pcView), &uiItem, sizeof(uiItem));
	if ((psItem->eKind == KSLIK_BoardPin) && (mcBoardPins.Get(psItem->pcView->GetName()) == NULL))
	{
		mcBoardPins.Put(psItem->pcView->GetName(), &uiItem);
	}
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
	psTerminal = &masTerminals.Add()->sTerminal;
	psTerminal->pcSubnet = pcSubnet;
	psTerminal->sKicadPosition = *psKicadPosition;
	psTerminal->pcPortView = pcPortView;
	psTerminal->uiItem = uiItem;
	psTerminal->bBus = bBus;
	LinkTerminal(masTerminals.NumElements() - 1);
}


//////////////////////////////////////////////////////////////////////////
//
// The terminal goes on the end of its item's terminals and its subnet's,
// and is found by its port if it's the port's first.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::LinkTerminal(size uiTerminal)
{
	SKicadSchemLogicimModelTerminal*		psModelTerminal;
	SKicadSchemLogicimTerminal*				psTerminal;
	SKicadSchemLogicimSubnetTerminals*		psSubnet;
	SKicadSchemLogicimSubnetTerminals		sSubnet;
	SKicadSchemLogicimModelItem*			psItem;

	psModelTerminal = masTerminals.Get(uiTerminal);
	psModelTerminal->uiNextOnSubnet = ARRAY_ELEMENT_NOT_FOUND;
	psModelTerminal->uiNextOfItem = ARRAY_ELEMENT_NOT_FOUND;
	psTerminal = &psModelTerminal->sTerminal;

	psItem = masItems.Get(psTerminal->uiItem);
	if (psItem->uiFirstTerminal == ARRAY_ELEMENT_NOT_FOUND)
	{
		psItem->uiFirstTerminal = uiTerminal;
	}
	else
	{
		masTerminals.Get(psItem->uiLastTerminal)->uiNextOfItem = uiTerminal;
	}
	psItem->uiLastTerminal = uiTerminal;

	if (mcPortTerminals.Get(&psTerminal->pcPortView, sizeof(psTerminal->pcPortView)) == NULL)
	{
		mcPortTerminals.Put(&psTerminal->pcPortView, sizeof(psTerminal->pcPortView), &uiTerminal, sizeof(uiTerminal));
	}

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
// The item goes on the end of the items attached to its pcAttachedTo's
// owner.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::LinkAttached(size uiItem)
{
	SKicadSchemLogicimModelItem*	psOwner;
	size							uiOwner;

	uiOwner = FindItem(masItems.Get(uiItem)->sItem.pcAttachedTo->GetOwner());
	if (uiOwner == ARRAY_ELEMENT_NOT_FOUND)
	{
		return;
	}
	psOwner = masItems.Get(uiOwner);
	if (psOwner->uiFirstAttached == ARRAY_ELEMENT_NOT_FOUND)
	{
		psOwner->uiFirstAttached = uiItem;
	}
	else
	{
		masItems.Get(psOwner->uiLastAttached)->uiNextAttached = uiItem;
	}
	psOwner->uiLastAttached = uiItem;
}


//////////////////////////////////////////////////////////////////////////
//
// Every index made again, in order, after items and terminals have moved.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::Index(void)
{
	size	i;

	mcBoardPins.Kill();
	mcViewItems.Kill();
	mcPortTerminals.Kill();
	mcSubnetTerminals.Kill();
	mcSubnetTerminals.Init();
	mcPortTerminals.Init();
	mcViewItems.Init();
	mcBoardPins.Init();
	for (i = 0; i < masItems.NumElements(); i++)
	{
		LinkItem(i);
	}
	for (i = 0; i < masTerminals.NumElements(); i++)
	{
		LinkTerminal(i);
	}
	for (i = 0; i < masItems.NumElements(); i++)
	{
		if (masItems.Get(i)->sItem.pcAttachedTo != NULL)
		{
			LinkAttached(i);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The item stands on the port: it's attached to the port's component.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::AttachItem(size uiItem, CPortView* pcTo)
{
	masItems.Get(uiItem)->sItem.pcAttachedTo = pcTo;
	LinkAttached(uiItem);
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
// Takes out the items paiRemoved marks (not 0), their views (from the
// circuit, killed) and their terminals; the others keep their order and
// are numbered again.  Must be before there are connections.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::RemoveItems(CArrayInt* paiRemoved)
{
	size								i;
	size								uiKept;
	CArraySize							auiNewIndices;
	SKicadSchemLogicimModelTerminal*	psTerminal;
	CComponentView*						pcView;

	auiNewIndices.Init();
	uiKept = 0;
	for (i = 0; i < masItems.NumElements(); i++)
	{
		if (paiRemoved->GetValue(i) != 0)
		{
			pcView = masItems.Get(i)->sItem.pcView;
			mpcCircuit->Remove(pcView);
			SafeKill(pcView);
			auiNewIndices.Add(ARRAY_ELEMENT_NOT_FOUND);
			continue;
		}
		auiNewIndices.Add(uiKept);
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
		if (auiNewIndices.GetValue(psTerminal->sTerminal.uiItem) == ARRAY_ELEMENT_NOT_FOUND)
		{
			continue;
		}
		psTerminal->sTerminal.uiItem = auiNewIndices.GetValue(psTerminal->sTerminal.uiItem);
		if (uiKept != i)
		{
			*masTerminals.Get(uiKept) = *psTerminal;
		}
		uiKept++;
	}
	masTerminals.SetUsedElements(uiKept);
	auiNewIndices.Kill();
	Index();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CKicadSchemLogicimModel::GetCircuit(void) { return mpcCircuit; }
size CKicadSchemLogicimModel::NumItems(void) { return masItems.NumElements(); }
size CKicadSchemLogicimModel::NumTerminals(void) { return masTerminals.NumElements(); }
size CKicadSchemLogicimModel::NumConnections(void) { return masConnections.NumElements(); }
SKicadSchemLogicimConnection* CKicadSchemLogicimModel::GetConnection(size uiIndex) { return masConnections.SafeGet(uiIndex); }


//////////////////////////////////////////////////////////////////////////
//
// The item and the terminal at the index, NULL if there's none.
//
//////////////////////////////////////////////////////////////////////////
SKicadSchemLogicimItem* CKicadSchemLogicimModel::GetItem(size uiIndex)
{
	SKicadSchemLogicimModelItem*	psItem;

	psItem = masItems.SafeGet(uiIndex);
	return (psItem != NULL) ? &psItem->sItem : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SKicadSchemLogicimTerminal* CKicadSchemLogicimModel::GetTerminal(size uiIndex)
{
	SKicadSchemLogicimModelTerminal*	psTerminal;

	psTerminal = masTerminals.SafeGet(uiIndex);
	return (psTerminal != NULL) ? &psTerminal->sTerminal : NULL;
}


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


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimModel::NextSubnetTerminal(size uiTerminal)
{
	return masTerminals.Get(uiTerminal)->uiNextOnSubnet;
}


//////////////////////////////////////////////////////////////////////////
//
// The item's terminals and the items attached to it, as the subnet's
// terminals (see FirstSubnetTerminal).
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimModel::FirstItemTerminal(size uiItem) { return masItems.Get(uiItem)->uiFirstTerminal; }
size CKicadSchemLogicimModel::NextItemTerminal(size uiTerminal) { return masTerminals.Get(uiTerminal)->uiNextOfItem; }
size CKicadSchemLogicimModel::FirstAttached(size uiItem) { return masItems.Get(uiItem)->uiFirstAttached; }
size CKicadSchemLogicimModel::NextAttached(size uiItem) { return masItems.Get(uiItem)->uiNextAttached; }


//////////////////////////////////////////////////////////////////////////
//
// How many terminals the item has.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimModel::NumItemTerminals(size uiItem)
{
	size	i;
	size	uiCount;

	uiCount = 0;
	for (i = FirstItemTerminal(uiItem); i != ARRAY_ELEMENT_NOT_FOUND; i = NextItemTerminal(i))
	{
		uiCount++;
	}
	return uiCount;
}


//////////////////////////////////////////////////////////////////////////
//
// The port's first terminal, the view's item and the item of the board
// pin named szName; ARRAY_ELEMENT_NOT_FOUND if there's none.
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimModel::FindPortTerminal(CPortView* pcPortView)
{
	size*	puiTerminal;

	puiTerminal = (size*)mcPortTerminals.Get(&pcPortView, sizeof(pcPortView));
	return (puiTerminal != NULL) ? *puiTerminal : ARRAY_ELEMENT_NOT_FOUND;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimModel::FindItem(CComponentView* pcView)
{
	size*	puiItem;

	puiItem = (size*)mcViewItems.Get(&pcView, sizeof(pcView));
	return (puiItem != NULL) ? *puiItem : ARRAY_ELEMENT_NOT_FOUND;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CKicadSchemLogicimModel::FindBoardPin(const char* szName)
{
	size*	puiItem;

	puiItem = mcBoardPins.Get(szName);
	return (puiItem != NULL) ? *puiItem : ARRAY_ELEMENT_NOT_FOUND;
}


//////////////////////////////////////////////////////////////////////////
//
// The terminals of the subnet, added to pauiTerminals.
//
//////////////////////////////////////////////////////////////////////////
void CKicadSchemLogicimModel::GetSubnetTerminals(CKicadSchemSubnet* pcSubnet, CArraySize* pauiTerminals)
{
	size	i;

	for (i = FirstSubnetTerminal(pcSubnet); i != ARRAY_ELEMENT_NOT_FOUND; i = NextSubnetTerminal(i))
	{
		pauiTerminals->Add(i);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The item's import box where it is now.  Its bounding box if it hasn't
// been measured.
//
//////////////////////////////////////////////////////////////////////////
bool CKicadSchemLogicimModel::GetItemImportBox(size uiItem, SFloat64Bounds* psBox)
{
	SKicadSchemLogicimItem*		psItem;
	SInt32Vec2					sPosition;

	psItem = &masItems.Get(uiItem)->sItem;
	if (!psItem->bImportBox)
	{
		return psItem->pcView->GetBoundingBox(psBox);
	}
	sPosition = psItem->pcView->GetPosition();
	*psBox = psItem->sImportBox;
	psBox->Move(SFloat64Vec2(sPosition.x, sPosition.y));
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// KiCad's angle in anticlockwise quarter turns (to the nearest), 0 to 3.
//
//////////////////////////////////////////////////////////////////////////
int GetKicadSchemQuarterTurns(float64 fAngle)
{
	return (((int)floor(fAngle / 90.0 + 0.5)) % 4 + 4) % 4;
}


//////////////////////////////////////////////////////////////////////////
//
// The rotation of a thing that is eUpright when KiCad hasn't turned it,
// turned anticlockwise by KiCad's angle (to the nearest right angle).
//
//////////////////////////////////////////////////////////////////////////
ERotation GetKicadSchemRotation(ERotation eUpright, float64 fAngle)
{
	return RotateRight(eUpright, (4 - GetKicadSchemQuarterTurns(fAngle)) % 4);
}


//////////////////////////////////////////////////////////////////////////
//
// No points yet.
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemPointMean::Init(void)
{
	sSum.Zero();
	uiCount = 0;
}


//////////////////////////////////////////////////////////////////////////
//
// One more point.
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemPointMean::Add(SFloat64Vec2* psPoint)
{
	sSum += *psPoint;
	uiCount++;
}


//////////////////////////////////////////////////////////////////////////
//
// The middle of the points, the origin if there are none.
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemPointMean::Get(SFloat64Vec2* psMean)
{
	if (uiCount == 0)
	{
		psMean->Zero();
		return;
	}
	*psMean = sSum / (float64)uiCount;
}

