/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela LogicimLib

Codaphela LogicimLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela LogicimLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela LogicimLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include "BaseLib/ConstructorCall.h"
#include "BaseLib/PointerFunctions.h"
#include "BaseLib/Logger.h"
#include "BaseLib/LogString.h"
#include "BaseLib/StringHelper.h"
#include "MicroprocessorLib/CircuitSimulation.h"
#include "MicroprocessorLib/SubcircuitSimulation.h"
#include "MicroprocessorLib/Circuit.h"
#include "ComponentViewFactory.h"
#include "CircuitWiring.h"
#include "CircuitView.h"


//The attributes of a side's fixed pin positions, in the order of PA_Left on.
static const char*	gaszPinPositionsAttributes[4] = { "leftPositions", "rightPositions", "topPositions", "bottomPositions" };


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::Init(void)
{
	size	i;

	mszTypeName.Init();
	mapcComponents.Init();
	mapcTraces.Init();
	for (i = 0; i < 4; i++)
	{
		maiPinPositions[i] = 0;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The simulation keeps its components and traces.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::Kill(void)
{
	size				i;
	CComponentView*		pcComponent;
	CTraceView*			pcTrace;

	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		pcTrace = mapcTraces.GetPtr(i);
		SafeKill(pcTrace);
	}
	mapcTraces.Kill();
	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		pcComponent = mapcComponents.GetPtr(i);
		SafeKill(pcComponent);
	}
	mapcComponents.Kill();
	mszTypeName.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit view frees the component when it's killed.
//
//////////////////////////////////////////////////////////////////////////
CComponentView* CCircuitView::Add(CComponentView* pcComponent)
{
	mapcComponents.Add(pcComponent);
	return pcComponent;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
CTraceView* CCircuitView::AddTrace(int32 x0, int32 y0, int32 x1, int32 y1)
{
	CTraceView*		pcTrace;

	pcTrace = NewMalloc<CTraceView>();
	pcTrace->Init(x0, y0, x1, y1);
	mapcTraces.Add(pcTrace);
	return pcTrace;
}


//////////////////////////////////////////////////////////////////////////
//
// A wire from one point to another, horizontal first and then vertical if
// they aren't in a line.  Returns the last piece.
//
//////////////////////////////////////////////////////////////////////////
CTraceView* CCircuitView::AddTraces(int32 x0, int32 y0, int32 x1, int32 y1)
{
	if ((x0 == x1) || (y0 == y1))
	{
		return AddTrace(x0, y0, x1, y1);
	}
	AddTrace(x0, y0, x1, y0);
	return AddTrace(x1, y0, x1, y1);
}


//////////////////////////////////////////////////////////////////////////
//
// Frees every wire.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::KillTraces(void)
{
	size			i;
	CTraceView*		pcTrace;

	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		pcTrace = mapcTraces.GetPtr(i);
		SafeKill(pcTrace);
	}
	mapcTraces.ReInit();
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit view no longer frees the component.  Fails if it isn't
// here.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitView::Remove(CComponentView* pcComponent)
{
	size	i;

	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		if (mapcComponents.GetPtr(i) == pcComponent)
		{
			mapcComponents.RemoveAt(i, true);
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Puts the new component where the old one is in the order; the circuit
// view frees the new one and no longer frees the old.  Fails if the old
// one isn't here.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitView::Replace(CComponentView* pcOld, CComponentView* pcNew)
{
	size	uiIndex;

	uiIndex = mapcComponents.Find(pcOld);
	if (uiIndex == ARRAY_ELEMENT_NOT_FOUND)
	{
		return false;
	}
	mapcComponents.SetPtr(uiIndex, pcNew);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The circuit view no longer frees the wire.  Fails if it isn't here.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitView::RemoveTrace(CTraceView* pcTrace)
{
	size	i;

	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		if (mapcTraces.GetPtr(i) == pcTrace)
		{
			mapcTraces.RemoveAt(i, true);
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Puts a wire taken out (see RemoveTrace) back at uiIndex in the wires,
// or at the end if there are fewer.  The circuit owns it again.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::InsertTrace(CTraceView* pcTrace, size uiIndex)
{
	if (uiIndex >= mapcTraces.NumElements())
	{
		mapcTraces.Add(pcTrace);
	}
	else
	{
		mapcTraces.InsertAt(&pcTrace, uiIndex);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The wires ReplaceWires replaces: the seed wires still in the circuit
// (papcSeeds may be NULL) and every wire joined to them or to the lines.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::CollectReplacedTraces(CArrayTraceLine* pasLines, CArrayTraceViewPtr* papcSeeds, CArrayTraceViewPtr* papcReplaced)
{
	size			i;
	CTraceView*		pcTrace;
	STraceLine		sLine;

	if (papcSeeds != NULL)
	{
		for (i = 0; i < papcSeeds->NumElements(); i++)
		{
			pcTrace = papcSeeds->GetPtr(i);
			if ((mapcTraces.Find(pcTrace) != ARRAY_ELEMENT_NOT_FOUND) && (papcReplaced->Find(pcTrace) == ARRAY_ELEMENT_NOT_FOUND))
			{
				papcReplaced->Add(pcTrace);
			}
		}
	}
	for (i = 0; i < pasLines->NumElements(); i++)
	{
		AddTouchingTraces(pasLines->Get(i), papcReplaced);
	}

	//The array grows as it's walked, so this reaches the whole of each net.
	for (i = 0; i < papcReplaced->NumElements(); i++)
	{
		pcTrace = papcReplaced->GetPtr(i);
		sLine.Init(pcTrace->GetStart(), pcTrace->GetEnd());
		AddTouchingTraces(&sLine, papcReplaced);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Takes the wires out of the circuit and frees them, adding each one's
// line to pasLines.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::KillTracesToLines(CArrayTraceViewPtr* papcTraces, CArrayTraceLine* pasLines)
{
	size			i;
	CTraceView*		pcTrace;

	for (i = 0; i < papcTraces->NumElements(); i++)
	{
		pcTrace = papcTraces->GetPtr(i);
		pasLines->Add()->Init(pcTrace->GetStart(), pcTrace->GetEnd());
		RemoveTrace(pcTrace);
		SafeKill(pcTrace);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Where a port is on one of the lines, added to pasPorts.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::CollectPortsOnLines(CArrayTraceLine* pasLines, CArrayInt32Vec2* pasPorts)
{
	CArrayPortViewPtr	apcPortViews;
	SInt32Vec2			sPort;
	size				i;
	size				j;

	apcPortViews.Init();
	CollectPortViews(&apcPortViews);
	for (i = 0; i < apcPortViews.NumElements(); i++)
	{
		sPort = apcPortViews.GetPtr(i)->GetGridPosition();
		for (j = 0; j < pasLines->NumElements(); j++)
		{
			if (pasLines->Get(j)->Contains(sPort))
			{
				pasPorts->Add(&sPort);
				break;
			}
		}
	}
	apcPortViews.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Adds wires along the lines, as Logicim's createTraceViews.  The lines,
// and the wires of every net they or the seed wires (which may be NULL)
// touch, are merged and split again (see MergeTraceLines), and those wires
// are replaced by the result, added to papcAdded unless it's NULL.  The
// replaced wires are freed; a simulated circuit must be rewired after.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::ReplaceWires(CArrayTraceLine* pasLines, CArrayTraceViewPtr* papcSeeds, CArrayTraceViewPtr* papcAdded)
{
	CArrayTraceViewPtr	apcReplaced;
	CArrayTraceLine		asLines;
	CArrayInt32Vec2		asEnds;
	CArrayInt32Vec2		asPorts;
	STraceLine*			psLine;
	CTraceView*			pcTrace;
	size				i;

	apcReplaced.Init();
	CollectReplacedTraces(pasLines, papcSeeds, &apcReplaced);
	asLines.Init();
	for (i = 0; i < pasLines->NumElements(); i++)
	{
		asLines.Add(pasLines->Get(i));
	}
	KillTracesToLines(&apcReplaced, &asLines);
	apcReplaced.Kill();

	//The merged lines are split again where the lines ended and at ports (see SplitTraceLines).
	asEnds.Init();
	for (i = 0; i < asLines.NumElements(); i++)
	{
		asEnds.Add(&asLines.Get(i)->sStart);
		asEnds.Add(&asLines.Get(i)->sEnd);
	}
	MergeTraceLines(&asLines);
	asPorts.Init();
	CollectPortsOnLines(&asLines, &asPorts);
	SplitTraceLines(&asLines, &asEnds, &asPorts);
	asPorts.Kill();
	asEnds.Kill();

	for (i = 0; i < asLines.NumElements(); i++)
	{
		psLine = asLines.Get(i);
		pcTrace = AddTrace(psLine->sStart.x, psLine->sStart.y, psLine->sEnd.x, psLine->sEnd.y);
		if (papcAdded != NULL)
		{
			papcAdded->Add(pcTrace);
		}
	}
	asLines.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Adds the wires touching the line that aren't already in papcTraces.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::AddTouchingTraces(STraceLine* psLine, CArrayTraceViewPtr* papcTraces)
{
	size			i;
	CTraceView*		pcTrace;
	STraceLine		sLine;

	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		pcTrace = mapcTraces.GetPtr(i);
		sLine.Init(pcTrace->GetStart(), pcTrace->GetEnd());
		if (sLine.Touches(psLine) && (papcTraces->Find(pcTrace) == ARRAY_ELEMENT_NOT_FOUND))
		{
			papcTraces->Add(pcTrace);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CCircuitView::NumComponents(void) { return mapcComponents.NumElements(); }
CComponentView* CCircuitView::GetComponent(size uiIndex) { return mapcComponents.SafeGetPtr(uiIndex); }
size CCircuitView::NumTraces(void) { return mapcTraces.NumElements(); }
CTraceView* CCircuitView::GetTrace(size uiIndex) { return mapcTraces.SafeGetPtr(uiIndex); }


//////////////////////////////////////////////////////////////////////////
//
// ARRAY_ELEMENT_NOT_FOUND if it isn't in the circuit.
//
//////////////////////////////////////////////////////////////////////////
size CCircuitView::FindComponentIndex(CComponentView* pcComponent) { return mapcComponents.Find(pcComponent); }
size CCircuitView::FindTraceIndex(CTraceView* pcTrace) { return mapcTraces.Find(pcTrace); }


//////////////////////////////////////////////////////////////////////////
//
// The wires from index uiFirst on: those added since there were uiFirst
// (wires are only ever added after the others).
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::CollectTracesFrom(size uiFirst, CArrayTraceViewPtr* papcTraces)
{
	size	i;

	for (i = uiFirst; i < mapcTraces.NumElements(); i++)
	{
		papcTraces->Add(mapcTraces.GetPtr(i));
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::CollectPortViews(CArrayPortViewPtr* papcPortViews)
{
	size				i;
	size				j;
	CComponentView*		pcComponent;

	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		pcComponent = mapcComponents.GetPtr(i);
		for (j = 0; j < pcComponent->NumPortViews(); j++)
		{
			papcPortViews->Add(pcComponent->GetPortView(j));
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Makes every component of the circuit, and of the subcircuits in it all
// the way down, in the circuit simulation for the path, starts the
// simulation (if bStart) and wires the nets (see CCircuitWiring).  The
// path must end in this circuit.  A top path is given the simulation's
// top simulation; a path through an instance has its subcircuit
// simulation from the instance.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitView::CreateSimulation(CViewPath* pcViewPath, CCircuitSimulation* pcSimulation, CFamilyVoltageConfigurationStore* pcFamilies, bool bStart)
{
	CCircuitWiring	cWiring;
	bool			bResult;

	if (pcViewPath->GetCircuitView() != this)
	{
		return gcLogger.Error2(__METHOD__, " The path doesn't end in this circuit.", NULL);
	}
	if (pcViewPath->IsTop() && !pcViewPath->ContainsCircuitSimulation(pcSimulation))
	{
		pcViewPath->AddSubcircuitSimulation(pcSimulation->GetSubcircuitTopSimulation());
	}

	bResult = CreateComponents(pcViewPath, pcSimulation, pcFamilies, &mapcComponents, NULL);

	cWiring.Init(pcSimulation);
	bResult &= cWiring.Wire(pcViewPath, bStart);
	cWiring.Kill();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// The component of each of the views (which are in this circuit) for the
// path, then those of each instance's subcircuit for the path through the
// instance, which the instance's component gives its subcircuit
// simulation.  The components made are added to papcCreated unless it's
// NULL.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitView::CreateComponents(CViewPath* pcViewPath, CCircuitSimulation* pcSimulation, CFamilyVoltageConfigurationStore* pcFamilies, CArrayComponentViewPtr* papcComponents, CArrayCircuitComponentPtr* papcCreated)
{
	size					i;
	CComponentView*			pcComponent;
	CCircuitComponent*		pcCreated;
	CCircuitInstanceView*	pcInstanceView;
	CCircuitView*			pcSubcircuit;
	CViewPath*				pcChild;
	bool					bResult;

	bResult = true;
	for (i = 0; i < papcComponents->NumElements(); i++)
	{
		pcComponent = papcComponents->GetPtr(i);
		if (pcComponent->MakesComponent())
		{
			pcCreated = pcComponent->CreateComponent(pcViewPath, pcSimulation, pcFamilies);
			if (pcCreated == NULL)
			{
				bResult = false;
			}
			else if (papcCreated != NULL)
			{
				papcCreated->Add(pcCreated);
			}
		}
	}

	for (i = 0; i < papcComponents->NumElements(); i++)
	{
		pcInstanceView = papcComponents->GetPtr(i)->GetCircuitInstanceView();
		if (pcInstanceView != NULL)
		{
			pcChild = pcViewPath->GetChild(pcInstanceView);
			if ((pcChild == NULL) || !pcChild->ContainsCircuitSimulation(pcSimulation))
			{
				bResult = false;
			}
			else
			{
				pcSubcircuit = pcInstanceView->GetInstanceCircuitView();
				bResult &= pcSubcircuit->CreateComponents(pcChild, pcSimulation, pcFamilies, &pcSubcircuit->mapcComponents, papcCreated);
			}
		}
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// Takes the view's component for the path out of the running simulation,
// as CComponentView::DestroyComponent.  An instance's subcircuit is taken
// out first, all the way down, and the path through the instance loses
// the instance's simulation.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::DestroyComponent(CComponentView* pcComponent, CViewPath* pcViewPath, CCircuitSimulation* pcSimulation)
{
	CCircuitInstanceView*	pcInstanceView;
	CViewPath*				pcChild;
	CSubcircuitSimulation*	pcInstanceSimulation;

	pcInstanceSimulation = NULL;
	pcInstanceView = pcComponent->GetCircuitInstanceView();
	if (pcInstanceView != NULL)
	{
		pcChild = pcViewPath->FindChild(pcInstanceView);
		if (pcChild != NULL)
		{
			pcInstanceSimulation = pcChild->GetSubcircuitSimulation(pcSimulation);
			if (pcInstanceSimulation != NULL)
			{
				pcInstanceView->GetInstanceCircuitView()->DestroySimulation(pcChild, pcSimulation);
			}
		}
	}

	pcComponent->DestroyComponent(pcViewPath, pcSimulation);
	if ((pcInstanceSimulation != NULL) && pcInstanceSimulation->IsInstance())
	{
		pcSimulation->RemoveInstanceSimulation((CSubcircuitInstanceSimulation*)pcInstanceSimulation);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Takes every component of the path out of the running simulation, all
// the way down, as DestroyComponent; the wires forget their traces and
// the path forgets the simulation.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::DestroySimulation(CViewPath* pcViewPath, CCircuitSimulation* pcSimulation)
{
	size	i;

	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		DestroyComponent(mapcComponents.GetPtr(i), pcViewPath, pcSimulation);
	}
	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		mapcTraces.GetPtr(i)->ClearTraces(pcViewPath, pcSimulation);
	}
	pcViewPath->RemoveSubcircuitSimulation(pcSimulation);
}


//////////////////////////////////////////////////////////////////////////
//
// Forgets the components and traces of the path in the circuit
// simulation, and of the paths through the instances in it, which the
// simulation still owns.  The paths forget the simulation.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::DisconnectSimulation(CViewPath* pcViewPath, CCircuitSimulation* pcSimulation)
{
	size					i;
	CCircuitInstanceView*	pcInstanceView;
	CViewPath*				pcChild;

	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		pcInstanceView = mapcComponents.GetPtr(i)->GetCircuitInstanceView();
		if (pcInstanceView != NULL)
		{
			pcChild = pcViewPath->FindChild(pcInstanceView);
			if (pcChild != NULL)
			{
				pcInstanceView->GetInstanceCircuitView()->DisconnectSimulation(pcChild, pcSimulation);
			}
		}
	}
	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		mapcComponents.GetPtr(i)->DisconnectComponent(pcViewPath, pcSimulation);
	}
	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		mapcTraces.GetPtr(i)->ClearTraces(pcViewPath, pcSimulation);
	}
	pcViewPath->RemoveSubcircuitSimulation(pcSimulation);
}


//////////////////////////////////////////////////////////////////////////
//
// A circuit is the top of its own path.
//
//////////////////////////////////////////////////////////////////////////
CCircuitView* CCircuitView::GetInstanceCircuitView(void)
{
	return this;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::AppendDescription(CChars* psz)
{
	if (mszTypeName.Empty())
	{
		psz->Append("Circuit");
	}
	else
	{
		psz->Append(mszTypeName);
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::SetTypeName(const char* szTypeName)
{
	mszTypeName.Set(szTypeName);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
char* CCircuitView::GetTypeName(void)
{
	return mszTypeName.Text();
}


//////////////////////////////////////////////////////////////////////////
//
// A wire's end is a junction where a port is or where three or more wire
// arms meet (a wire passing through counts as two).
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::CalculateJunctions(void)
{
	CArrayPortViewPtr	apcPortViews;
	size				i;
	size				j;
	CTraceView*			pcTrace;
	SInt32Vec2			asEnds[2];
	bool				abJunction[2];
	int					iArms;
	int					iPorts;
	int					iEnd;
	CTraceView*			pcOther;

	apcPortViews.Init();
	CollectPortViews(&apcPortViews);

	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		pcTrace = mapcTraces.GetPtr(i);
		asEnds[0] = pcTrace->GetStart();
		asEnds[1] = pcTrace->GetEnd();
		for (iEnd = 0; iEnd < 2; iEnd++)
		{
			iArms = 0;
			iPorts = 0;
			for (j = 0; j < mapcTraces.NumElements(); j++)
			{
				pcOther = mapcTraces.GetPtr(j);
				if (pcOther->IsEnd(asEnds[iEnd]))
				{
					iArms++;
				}
				else if (pcOther->Contains(asEnds[iEnd]))
				{
					iArms += 2;
				}
			}
			for (j = 0; j < apcPortViews.NumElements(); j++)
			{
				if (apcPortViews.GetPtr(j)->GetGridPosition() == asEnds[iEnd])
				{
					iPorts++;
				}
			}
			abJunction[iEnd] = (iPorts > 0) || (iArms >= 3);
		}
		pcTrace->SetJunctions(abJunction[0], abJunction[1]);
	}
	apcPortViews.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::SaveXML(CXMLWriter* pcWriter)
{
	size	i;

	pcWriter->Begin("Circuit");
	if (!mszTypeName.Empty())
	{
		pcWriter->Attribute("type", mszTypeName.Text());
	}
	for (i = 0; i < 4; i++)
	{
		if (maiPinPositions[i] > 0)
		{
			pcWriter->Attribute(gaszPinPositionsAttributes[i], maiPinPositions[i]);
		}
	}
	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		mapcComponents.GetPtr(i)->SaveXML(pcWriter);
	}
	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		mapcTraces.GetPtr(i)->SaveXML(pcWriter);
	}
	pcWriter->End();
}


//////////////////////////////////////////////////////////////////////////
//
// The side's fixed pin positions (see the class), 0 to show its pins in
// their order.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::SetPinPositions(EPinAlignment eSide, int32 iPositions)
{
	if ((eSide >= PA_Left) && (eSide <= PA_Bottom))
	{
		maiPinPositions[eSide - PA_Left] = (iPositions > 0) ? iPositions : 0;
	}
}


int32 CCircuitView::GetPinPositions(EPinAlignment eSide)
{
	return ((eSide >= PA_Left) && (eSide <= PA_Bottom)) ? maiPinPositions[eSide - PA_Left] : 0;
}


//////////////////////////////////////////////////////////////////////////
//
// Stops at the first element it can't read; what was read before it stays
// in the circuit view.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitView::LoadXML(CMarkupTag* pcCircuitTag)
{
	STagIterator		sIter;
	CMarkupTag*			pcTag;
	CComponentView*		pcComponent;
	CTraceView*			pcTrace;
	char*				szTypeName;
	size				i;

	if (!pcCircuitTag->Is("Circuit"))
	{
		return gcLogger.Error2(__METHOD__, " Expected a [Circuit] element, not [", pcCircuitTag->GetName(), "].", NULL);
	}
	szTypeName = pcCircuitTag->GetStringAttribute("type");
	if (szTypeName != NULL)
	{
		mszTypeName.Set(szTypeName);
	}
	for (i = 0; i < 4; i++)
	{
		if ((pcCircuitTag->GetStringAttribute(gaszPinPositionsAttributes[i]) != NULL) && (!pcCircuitTag->GetInt32Attribute(gaszPinPositionsAttributes[i], &maiPinPositions[i]) || (maiPinPositions[i] < 0)))
		{
			return gcLogger.Error2(__METHOD__, " Circuit [", mszTypeName.Text(), "] needs a whole number of pin positions [", gaszPinPositionsAttributes[i], "], 0 or more.", NULL);
		}
	}

	pcTag = pcCircuitTag->GetTag(&sIter);
	while (pcTag != NULL)
	{
		if (pcTag->Is("Trace"))
		{
			pcTrace = NewMalloc<CTraceView>();
			if (!pcTrace->LoadXML(pcTag))
			{
				free(pcTrace);
				return false;
			}
			mapcTraces.Add(pcTrace);
		}
		else
		{
			pcComponent = NewComponentView(pcTag->GetName());
			if (pcComponent == NULL)
			{
				return gcLogger.Error2(__METHOD__, " Unknown component [", pcTag->GetName(), "].", NULL);
			}
			if (!pcComponent->LoadXML(pcTag))
			{
				free(pcComponent);
				return false;
			}
			if (!pcComponent->LoadXMLAfterInit(pcTag))
			{
				SafeKill(pcComponent);
				return false;
			}
			mapcComponents.Add(pcComponent);
		}
		pcTag = pcCircuitTag->GetNextTag(&sIter);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// Wires under components.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::Paint(CCircuitPainter* pcPainter)
{
	size	i;

	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		mapcTraces.GetPtr(i)->Paint(pcPainter);
	}
	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		mapcComponents.GetPtr(i)->Paint(pcPainter);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// In grid units.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitView::GetBoundingBox(SFloat64Bounds* psBox)
{
	return GetViewsBoundingBox(&mapcComponents, &mapcTraces, psBox);
}


//////////////////////////////////////////////////////////////////////////
//
// The box around the components' boxes and the wires, in grid units.
// False if there's nothing in it.
//
//////////////////////////////////////////////////////////////////////////
bool GetViewsBoundingBox(CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcTraces, SFloat64Bounds* psBox)
{
	size			i;
	SFloat64Bounds	sBox;
	CTraceView*		pcTrace;

	psBox->Init();
	for (i = 0; i < papcComponents->NumElements(); i++)
	{
		papcComponents->GetPtr(i)->GetBoundingBox(&sBox);
		psBox->Include(&sBox);
	}
	for (i = 0; i < papcTraces->NumElements(); i++)
	{
		pcTrace = papcTraces->GetPtr(i);
		psBox->Include(pcTrace->GetStart().x, pcTrace->GetStart().y);
		psBox->Include(pcTrace->GetEnd().x, pcTrace->GetEnd().y);
	}
	return psBox->bHasBounds;
}


//The nearest of the views hit, by the distance from the point to each one's centre.
template<class M>
struct SNearestHit
{
	M*		pcNearest;
	float64	fNearest;

	void Init(void)
	{
		pcNearest = NULL;
		fNearest = 0;
	}

	void Include(M* pcView, float64 x, float64 y, float64 fCentreX, float64 fCentreY)
	{
		float64	fDistance;

		fDistance = (x - fCentreX) * (x - fCentreX) + (y - fCentreY) * (y - fCentreY);
		if ((pcNearest == NULL) || (fDistance < fNearest))
		{
			pcNearest = pcView;
			fNearest = fDistance;
		}
	}
};


//////////////////////////////////////////////////////////////////////////
//
// The component whose box the grid position is in, as Logicim's
// getComponentViewInScreenSpace: of several, the one whose box's centre is
// nearest.  NULL if there's none.
//
//////////////////////////////////////////////////////////////////////////
CComponentView* CCircuitView::GetComponentAt(SFloat64Vec2 sPosition)
{
	size							i;
	CComponentView*					pcComponent;
	SFloat64Bounds					sBox;
	SFloat64Vec2					sCentre;
	SNearestHit<CComponentView>		sHit;

	sHit.Init();
	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		pcComponent = mapcComponents.GetPtr(i);
		if (pcComponent->GetBoundingBox(&sBox) && sBox.Contains(sPosition))
		{
			sCentre = sBox.GetCentre();
			sHit.Include(pcComponent, sPosition.x, sPosition.y, sCentre.x, sCentre.y);
		}
	}
	return sHit.pcNearest;
}


//////////////////////////////////////////////////////////////////////////
//
// The wire through the grid point, as Logicim's getTraceViewInScreenSpace:
// of several, the one whose middle is nearest.  NULL if there's none.
//
//////////////////////////////////////////////////////////////////////////
CTraceView* CCircuitView::GetTraceAt(SInt32Vec2 sPoint)
{
	size					i;
	CTraceView*				pcTrace;
	SNearestHit<CTraceView>	sHit;

	sHit.Init();
	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		pcTrace = mapcTraces.GetPtr(i);
		if (pcTrace->Contains(sPoint))
		{
			sHit.Include(pcTrace, sPoint.x, sPoint.y, (pcTrace->GetStart().x + pcTrace->GetEnd().x) / 2.0, (pcTrace->GetStart().y + pcTrace->GetEnd().y) / 2.0);
		}
	}
	return sHit.pcNearest;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a port or a wire's end is on the grid point, where Logicim has a
// connection.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitView::IsConnectionAt(SInt32Vec2 sPoint)
{
	size	i;

	if (GetPortViewAt(sPoint) != NULL)
	{
		return true;
	}
	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		if (mapcTraces.GetPtr(i)->IsEnd(sPoint))
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// A port view on the grid point; NULL if there's none.
//
//////////////////////////////////////////////////////////////////////////
CPortView* CCircuitView::GetPortViewAt(SInt32Vec2 sPoint)
{
	size				i;
	size				j;
	CComponentView*		pcComponent;
	CPortView*			pcPortView;

	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		pcComponent = mapcComponents.GetPtr(i);
		for (j = 0; j < pcComponent->NumPortViews(); j++)
		{
			pcPortView = pcComponent->GetPortView(j);
			if (pcPortView->GetGridPosition() == sPoint)
			{
				return pcPortView;
			}
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the box is wholly in the rectangle or, if bIntersections,
// touches it, as Logicim's BoundingBox.containsBox: a rectangle dragged
// right to left takes what it touches.
//
//////////////////////////////////////////////////////////////////////////
static bool IsBoxInRectangle(SFloat64Bounds* psRectangle, SFloat64Bounds* psBox, bool bIntersections)
{
	return bIntersections ? psRectangle->Overlaps(psBox) : psRectangle->Contains(psBox);
}


//////////////////////////////////////////////////////////////////////////
//
// The views a selection rectangle from sStart to sEnd (grid positions)
// picks, as Logicim's getSelectionFromRectangle.  If the corners round to
// the same grid point it's a click there: components whose boxes hold the
// point and wires through it.  Otherwise components and wires wholly in
// the rectangle, or (dragged right to left) touching it.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitView::GetViewsInRectangle(SFloat64Vec2 sStart, SFloat64Vec2 sEnd, CArrayComponentViewPtr* papcComponents, CArrayTraceViewPtr* papcTraces)
{
	size				i;
	CComponentView*		pcComponent;
	CTraceView*			pcTrace;
	SFloat64Bounds		sBox;
	SInt32Vec2			sPoint;
	SInt32Vec2			sEndPoint;
	SFloat64Vec2		sPointPosition;
	bool				bPoint;
	bool				bIntersections;
	SFloat64Bounds		sRectangle;

	sPoint.Init(RoundToGrid(sStart.x), RoundToGrid(sStart.y));
	sEndPoint.Init(RoundToGrid(sEnd.x), RoundToGrid(sEnd.y));
	sPointPosition.Init(sPoint.x, sPoint.y);
	bPoint = sPoint == sEndPoint;
	bIntersections = sStart.x > sEnd.x;
	sRectangle.Init();
	sRectangle.Include(sStart);
	sRectangle.Include(sEnd);

	for (i = 0; i < mapcComponents.NumElements(); i++)
	{
		pcComponent = mapcComponents.GetPtr(i);
		if (pcComponent->GetBoundingBox(&sBox))
		{
			if (bPoint ? sBox.Contains(sPointPosition) : IsBoxInRectangle(&sRectangle, &sBox, bIntersections))
			{
				papcComponents->Add(pcComponent);
			}
		}
	}

	for (i = 0; i < mapcTraces.NumElements(); i++)
	{
		pcTrace = mapcTraces.GetPtr(i);
		if (bPoint)
		{
			if (pcTrace->Contains(sPoint))
			{
				papcTraces->Add(pcTrace);
			}
		}
		else
		{
			sBox.Init();
			sBox.Include(pcTrace->GetStart().x, pcTrace->GetStart().y);
			sBox.Include(pcTrace->GetEnd().x, pcTrace->GetEnd().y);
			if (IsBoxInRectangle(&sRectangle, &sBox, bIntersections))
			{
				papcTraces->Add(pcTrace);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// As Logicim's Math.round to a grid point.
//
//////////////////////////////////////////////////////////////////////////
int32 RoundToGrid(float64 f)
{
	return (int32)floor(f + 0.5);
}


//////////////////////////////////////////////////////////////////////////
//
// Logicim's dot grid: none when zoomed far out, single pixel dots, darker
// dots, and from zoom 2 small squares.
//
//////////////////////////////////////////////////////////////////////////
void PaintCircuitGrid(CCircuitPainter* pcPainter)
{
	CCircuitViewport*	pcViewport;
	CCircuitColours*	pcColours;
	CGeometryMap*		pcMap;
	float64				fZoom;
	SFloat64Vec2		sScreen;
	SFloat64Vec2		sTopLeft;
	SFloat64Vec2		sBottomRight;
	SInt32Vec2			sSize;
	int32				iLeft;
	int32				iTop;
	int32				iRight;
	int32				iBottom;
	int32				x;
	int32				y;
	int32				iDot;
	ARGB32				sColour;

	pcViewport = pcPainter->GetViewport();
	pcColours = pcPainter->GetColours();
	pcMap = pcPainter->GetMap();
	fZoom = pcViewport->GetZoom();
	if (fZoom < 0.3)
	{
		return;
	}

	sColour = (fZoom < 1.5) ? pcColours->msSmallGridDot : pcColours->msLargeGridDot;
	iDot = (fZoom < 2.0) ? 1 : (int32)fZoom;

	sSize = pcViewport->GetSize();
	sScreen.Init(0, 0);
	pcViewport->ScreenToGrid(&sTopLeft, &sScreen);
	sScreen.Init(sSize.x, sSize.y);
	pcViewport->ScreenToGrid(&sBottomRight, &sScreen);
	iLeft = (int32)floor(sTopLeft.x) - 1;
	iTop = (int32)floor(sTopLeft.y) - 1;
	iRight = (int32)ceil(sBottomRight.x) + 1;
	iBottom = (int32)ceil(sBottomRight.y) + 1;

	for (y = iTop; y <= iBottom; y++)
	{
		for (x = iLeft; x <= iRight; x++)
		{
			sScreen.Init(pcViewport->GridToScreenX(x), pcViewport->GridToScreenY(y));
			pcMap->AddRectangle((int32)floor(sScreen.x + 0.5) - iDot / 2, (int32)floor(sScreen.y + 0.5) - iDot / 2, iDot, iDot, sColour);
		}
	}
}

