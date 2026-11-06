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
#include <stdlib.h>
#include "BaseLib/ConstructorCall.h"
#include "BaseLib/PointerFunctions.h"
#include "BaseLib/Logger.h"
#include "BaseLib/XMLFile.h"
#include "BaseLib/XMLWriter.h"
#include "BaseLib/Markup.h"
#include "BaseLib/LogString.h"
#include "BaseLib/ArrayElementNotFound.h"
#include "StandardLib/Objects.h"
#include "MicroprocessorLib/CircuitSimulation.h"
#include "CircuitPainter.h"
#include "CircuitWiring.h"
#include "CircuitFile.h"
#include "SimulationStateXML.h"
#include "ComponentPlacement.h"
#include "ComponentViewFactory.h"
#include "MoveComponentsEdit.h"
#include "SelectionEdit.h"
#include "WirePullEdit.h"
#include "TraceAutoConnector.h"
#include "SubcircuitInstanceView.h"
#include "CircuitEditor.h"
#include "CircuitEditorInternal.h"


//////////////////////////////////////////////////////////////////////////
//
// Every path that ends in the circuit being edited.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::GetEditedPaths(CArrayViewPathPtr* papcViewPaths)
{
	mpcCircuits->GetViewPaths()->GetPathsEndingWith(mpcCircuit, papcViewPaths);
}


//////////////////////////////////////////////////////////////////////////
//
// Makes the views' components, and those of the subcircuits of instances
// among them, in every simulation of every path through the circuit being
// edited, and starts them as a simulation starts its components.
//
//////////////////////////////////////////////////////////////////////////
void CCircuitEditor::CreateComponents(CArrayComponentViewPtr* papcComponents)
{
	CArrayViewPathPtr			apcViewPaths;
	CArrayCircuitComponentPtr	apcCreated;
	size						i;
	size						j;
	size						k;
	CViewPath*					pcViewPath;
	CCircuitSimulation*			pcSimulation;

	apcViewPaths.Init();
	GetEditedPaths(&apcViewPaths);
	for (i = 0; i < apcViewPaths.NumElements(); i++)
	{
		pcViewPath = apcViewPaths.GetPtr(i);
		for (j = 0; j < pcViewPath->NumCircuitSimulations(); j++)
		{
			pcSimulation = pcViewPath->GetCircuitSimulation(j);
			apcCreated.Init();
			mpcCircuit->CreateComponents(pcViewPath, pcSimulation, mpcFamilies, papcComponents, &apcCreated);
			for (k = 0; k < apcCreated.NumElements(); k++)
			{
				apcCreated.GetPtr(k)->Reset(pcSimulation->GetTimeline());
			}
			apcCreated.Kill();
		}
	}
	apcViewPaths.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// Rewires each simulation of each path through the circuit being edited,
// once for each simulation's top path.  The detached views (either can be
// NULL) join nothing, see CCircuitWiring::SetDetached.
//
//////////////////////////////////////////////////////////////////////////
bool CCircuitEditor::Rewire(CArrayComponentViewPtr* papcDetachedComponents, CArrayTraceViewPtr* papcDetachedTraces)
{
	CArrayViewPathPtr						apcViewPaths;
	CArrayViewPathPtr						apcTops;
	CArrayTemplatePtr<CCircuitSimulation>	apcSimulations;
	size									i;
	size									j;
	size									k;
	CViewPath*								pcViewPath;
	CViewPath*								pcTop;
	CCircuitSimulation*						pcSimulation;
	bool									bDone;
	CCircuitWiring							cWiring;
	bool									bResult;

	apcViewPaths.Init();
	apcTops.Init();
	apcSimulations.Init();
	GetEditedPaths(&apcViewPaths);
	for (i = 0; i < apcViewPaths.NumElements(); i++)
	{
		pcViewPath = apcViewPaths.GetPtr(i);
		pcTop = pcViewPath->GetTop();
		for (j = 0; j < pcViewPath->NumCircuitSimulations(); j++)
		{
			pcSimulation = pcViewPath->GetCircuitSimulation(j);
			bDone = false;
			for (k = 0; k < apcTops.NumElements(); k++)
			{
				if ((apcTops.GetPtr(k) == pcTop) && (apcSimulations.GetPtr(k) == pcSimulation))
				{
					bDone = true;
					break;
				}
			}
			if (!bDone)
			{
				apcTops.Add(pcTop);
				apcSimulations.Add(pcSimulation);
			}
		}
	}
	apcViewPaths.Kill();

	bResult = true;
	for (k = 0; k < apcTops.NumElements(); k++)
	{
		cWiring.Init(apcSimulations.GetPtr(k));
		cWiring.SetDetached(papcDetachedComponents, papcDetachedTraces);
		bResult &= cWiring.Rewire(apcTops.GetPtr(k));
		cWiring.Kill();
	}
	apcSimulations.Kill();
	apcTops.Kill();
	return bResult;
}
