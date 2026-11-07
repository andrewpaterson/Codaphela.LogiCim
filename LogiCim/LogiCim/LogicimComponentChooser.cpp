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
#include "BaseLib/PointerFunctions.h"
#include "StandardLib/Objects.h"
#include "StandardLib/ObjectReader.h"
#include "StandardLib/ObjectWriter.h"
#include "WindowLib/ClearCanvasDraw.h"
#include "WindowLib/FillContainer.h"
#include "WindowLib/RoundedBoxCanvasDraw.h"
#include "WindowLib/IconComponent.h"
#include "WindowLib/JewelImageGenerator.h"
#include "WindowLib/JewelColours.h"
#include "LogicimLib/ComponentLibraries.h"
#include "LogicimLib/ComponentPlacement.h"
#include "LogicimLib/CircuitEditor.h"
#include "LogicimCircuit.h"
#include "LogicimComponentChooser.h"


#define CHOOSER_WIDTH			760
#define CHOOSER_HEIGHT			560
#define CHOOSER_ROW_HEIGHT		22
#define CHOOSER_GAP				8  //Between the filter, the list, the preview and the details.
#define CHOOSER_INDENT			6  //Of a library's chevron.
#define CHOOSER_CHEVRON_WIDTH	18  //The column of the libraries' chevrons, a component's name starts after it.
#define CHOOSER_NAME_WIDTH		170  //Of the column of components' names.


//////////////////////////////////////////////////////////////////////////
//
// Starts with no filter and every library closed, the library of the
// component of that element (or subcircuit) name selected, otherwise the
// first.  The circuit (its editor, colours and fonts) must outlive the
// chooser.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::Init(Ptr<CWindow> pOwner, SDialogLook* psLook, CLogicimCircuit* pcCircuit, const char* szSelected)
{
	Ptr<CComponent>		pBody;
	CColourFormatHelper	cFormat;
	int					i;
	int					iLibrary;
	int					iEntry;

	PreInit();

	mpcCircuit = pcCircuit;
	maiExpanded.Init();
	mpcPreview = NULL;
	mszChosen.Init();
	mbChosenSubcircuit = false;
	masRows.Init();
	maszSubcircuits.Init();

	mpDialog = OMalloc<CDialog>(pOwner, psLook, "Choose Component");
	mpDialog->GetWindow()->GetColourFormat(&cFormat);
	mpCollapsedChevron = CJewelImageGenerator::CreateChevron(JEWEL_ICON, &cFormat);
	mpExpandedChevron = CJewelImageGenerator::CreateDownChevron(JEWEL_ICON, &cFormat);
	pBody = CreateBody();
	mpDialog->SetBody(pBody, mpFilter);

	FindSubcircuits();
	for (i = 0; i < NumLibraries(); i++)
	{
		maiExpanded.Add(0);
	}
	mpFilter->GetChangeEvent()->Add(this, &CLogicimComponentChooser::FilterChanged);

	iLibrary = -1;
	iEntry = -1;
	FindEntry(szSelected, &iLibrary, &iEntry);
	FillList(iLibrary, -1);

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::Class(void)
{
	M_Pointer(mpDialog);
	U_Pointer(mpcCircuit);
	M_Pointer(mpFilter);
	M_Pointer(mpList);
	M_Pointer(mpPreviewDraw);
	M_Pointer(mpName);
	M_Pointer(mpDescription);
	M_Pointer(mpLibrary);
	M_Pointer(mpCollapsedChevron);
	M_Pointer(mpExpandedChevron);
	U_Unknown(CArrayInt, maiExpanded);
	U_Unknown(CArrayLogicimChooserRow, masRows);
	U_Unknown(CArrayChars, maszSubcircuits);
	U_Pointer(mpcPreview);
	U_String(mszChosen);
	U_Bool(mbChosenSubcircuit);
}


//////////////////////////////////////////////////////////////////////////
//
// A chooser isn't saved.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimComponentChooser::Save(CObjectWriter* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimComponentChooser::Load(CObjectReader* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::Free(void)
{
	//An object's memory is freed without its destructor running so the chooser takes itself off the widgets still
	//here.  The dialog, the widgets and the preview's draw are freed with the chooser's pointers; the draw only points
	//to the preview.
	if (mpList.IsNotNull())
	{
		mpList->GetSelectionEvent()->Remove(this);
		mpList->GetActivationEvent()->Remove(this);
	}
	if (mpFilter.IsNotNull())
	{
		mpFilter->GetChangeEvent()->Remove(this);
	}
	KillPreview();

	maszSubcircuits.Kill();
	masRows.Kill();
	maiExpanded.Kill();
	mszChosen.Kill();
	mpcCircuit = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The filter on top, the list and preview side by side under it, and the
// details along the bottom.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CLogicimComponentChooser::CreateBody(void)
{
	Ptr<CWindow>			pWindow;
	SDialogLook*			psLook;
	Ptr<CGridContainer>		pBody;
	Ptr<CGridContainer>		pDetails;
	Ptr<CScrollPane>		pScrollPane;
	Ptr<CCanvas>			pPreview;
	Ptr<CFillContainer>		pGap;
	SInsets					sInsets;
	int						aiInsets[4] = { 0, 6, 0, 6 };

	pWindow = mpDialog->GetWindow();
	psLook = mpDialog->GetLook();
	pBody = OMalloc<CGridContainer>(pWindow);

	mpFilter = OMalloc<CTextField>(pWindow, psLook->pcTheme, "");
	mpFilter->SetColumns(10);
	mpFilter->SetAlignment(CSH_Stretch, CSV_Center);
	pBody->AddComponent(mpFilter, 0, 0, 3, 1, 1, 0);

	pGap = OMalloc<CFillContainer>(pWindow);
	pGap->SetDesiredSize(CHOOSER_GAP, CHOOSER_GAP);
	pBody->AddComponent(pGap, 0, 1);

	sInsets.Init(aiInsets);
	mpList = OMalloc<CListContainer>(pWindow, psLook->pcTheme);
	mpList->SetHighlightDraws(OMalloc<CRoundedBoxCanvasDraw>(psLook->pcTheme->GetColour(TC_ListSelected), 4, &sInsets), OMalloc<CRoundedBoxCanvasDraw>(psLook->pcTheme->GetColour(TC_ListHover), 4, &sInsets));
	mpList->SetAlignment(CSH_Stretch, CSV_Top);
	mpList->GetSelectionEvent()->Add(this, &CLogicimComponentChooser::SelectionChanged);
	mpList->GetActivationEvent()->Add(this, &CLogicimComponentChooser::ItemActivated);
	pScrollPane = OMalloc<CScrollPane>(pWindow, psLook->pcTheme);
	pScrollPane->SetContent(mpList);
	pScrollPane->SetAlignment(CSH_Stretch, CSV_Stretch);
	pBody->AddComponent(pScrollPane, 0, 2, 1, 1, 1, 1);

	pGap = OMalloc<CFillContainer>(pWindow);
	pGap->SetDesiredSize(CHOOSER_GAP, CHOOSER_GAP);
	pBody->AddComponent(pGap, 1, 2);

	mpPreviewDraw = OMalloc<CCircuitCanvasDraw>(mpcCircuit);
	mpPreviewDraw->SetGrid(false);
	pPreview = OMalloc<CCanvas>(pWindow, mpPreviewDraw);
	pPreview->SetAlignment(CSH_Stretch, CSV_Stretch);
	pBody->AddComponent(pPreview, 2, 2, 1, 1, 1, 1);

	pGap = OMalloc<CFillContainer>(pWindow);
	pGap->SetDesiredSize(CHOOSER_GAP, CHOOSER_GAP);
	pBody->AddComponent(pGap, 0, 3);

	pDetails = OMalloc<CGridContainer>(pWindow);
	pDetails->SetAlignment(CSH_Stretch, CSV_Top);
	mpName = OMalloc<CLabel>(pWindow, psLook->pcTheme->GetFont(), " ");
	mpName->SetAlignment(CSH_Left, CSV_Center);
	mpDescription = OMalloc<CLabel>(pWindow, psLook->pcDimFont, " ");
	mpDescription->SetAlignment(CSH_Left, CSV_Center);
	mpLibrary = OMalloc<CLabel>(pWindow, psLook->pcDimFont, " ");
	mpLibrary->SetAlignment(CSH_Left, CSV_Center);
	pDetails->AddComponent(mpName, 0, 0, 1, 1, 1, 0);
	pDetails->AddComponent(mpDescription, 0, 1, 1, 1, 1, 0);
	pDetails->AddComponent(mpLibrary, 0, 2, 1, 1, 1, 0);
	pBody->AddComponent(pDetails, 0, 4, 3, 1, 1, 0);
	return pBody;
}


//////////////////////////////////////////////////////////////////////////
//
// The project's circuits that can go in the circuit being edited: not
// itself, and not one that holds it.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::FindSubcircuits(void)
{
	CCircuitEditor*		pcEditor;
	CCircuitViews*		pcCircuits;
	CCircuitView*		pcSubcircuit;
	size				i;

	pcEditor = mpcCircuit->GetEditor();
	pcCircuits = pcEditor->GetCircuits();
	for (i = 0; i < pcCircuits->NumCircuits(); i++)
	{
		pcSubcircuit = pcCircuits->Get(i);
		if ((pcSubcircuit != pcEditor->GetCircuit()) && !CircuitUses(pcSubcircuit, pcEditor->GetCircuit()))
		{
			maszSubcircuits.Add(pcSubcircuit->GetTypeName());
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The libraries, and the subcircuits as the last.
//
//////////////////////////////////////////////////////////////////////////
int CLogicimComponentChooser::NumLibraries(void)
{
	return (int)NumComponentLibraries() + 1;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int CLogicimComponentChooser::NumEntries(int iLibrary)
{
	if (iLibrary == (int)NumComponentLibraries())
	{
		return (int)maszSubcircuits.NumElements();
	}
	return (int)GetComponentLibrary(iLibrary)->uiNumEntries;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CLogicimComponentChooser::GetLibraryName(int iLibrary)
{
	if (iLibrary == (int)NumComponentLibraries())
	{
		return "Subcircuits";
	}
	return GetComponentLibrary(iLibrary)->szName;
}


//////////////////////////////////////////////////////////////////////////
//
// The name the component is shown by.
//
//////////////////////////////////////////////////////////////////////////
const char* CLogicimComponentChooser::GetEntryName(int iLibrary, int iEntry)
{
	if (iLibrary == (int)NumComponentLibraries())
	{
		return maszSubcircuits.GetText(iEntry);
	}
	return GetComponentLibrary(iLibrary)->asEntries[iEntry].szName;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
const char* CLogicimComponentChooser::GetEntryDescription(int iLibrary, int iEntry)
{
	if (iLibrary == (int)NumComponentLibraries())
	{
		return "An instance of the project's subcircuit.";
	}
	return GetComponentLibrary(iLibrary)->asEntries[iEntry].szDescription;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the filter matches the component (see ComponentMatchesFilter).
// A subcircuit is matched on its name and its library's.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimComponentChooser::EntryMatches(int iLibrary, int iEntry)
{
	SComponentLibrary*	psLibrary;
	char*				szFilter;

	szFilter = mpFilter->GetText();
	if (iLibrary == (int)NumComponentLibraries())
	{
		return TextMatchesFilter(maszSubcircuits.GetText(iEntry), szFilter) || TextMatchesFilter("Subcircuits", szFilter);
	}
	psLibrary = GetComponentLibrary(iLibrary);
	return ComponentMatchesFilter(psLibrary, &psLibrary->asEntries[iEntry], szFilter);
}


//////////////////////////////////////////////////////////////////////////
//
// The library and entry of the component of the element (or subcircuit)
// name.  False if there's none.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimComponentChooser::FindEntry(const char* szName, int* piLibrary, int* piEntry)
{
	int		i;
	int		j;

	for (i = 0; i < NumLibraries(); i++)
	{
		for (j = 0; j < NumEntries(i); j++)
		{
			if (((i == (int)NumComponentLibraries()) ? StringCompare(maszSubcircuits.GetText(j), szName) : StringCompare(GetComponentLibrary(i)->asEntries[j].szXMLName, szName)) == 0)
			{
				*piLibrary = i;
				*piEntry = j;
				return true;
			}
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// With a filter, every library with a component it matches is opened and
// the others closed, as KiCad's are.  Without one they're all closed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::ExpandMatching(void)
{
	int		i;
	int		j;
	bool	bFiltered;
	int		iExpanded;

	bFiltered = !StrEmpty(mpFilter->GetText());
	for (i = 0; i < NumLibraries(); i++)
	{
		iExpanded = 0;
		if (bFiltered)
		{
			for (j = 0; j < NumEntries(i); j++)
			{
				if (EntryMatches(i, j))
				{
					iExpanded = 1;
					break;
				}
			}
		}
		maiExpanded.SetValue(i, iExpanded);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A row for each library with a component the filter matches and, under
// each open library, a row for each of those components.  The row of the
// library and entry given is selected (iSelectEntry is -1 for a library's
// row) if it's shown, otherwise the first.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::FillList(int iSelectLibrary, int iSelectEntry)
{
	int		i;
	int		j;
	int		iMatches;
	int32	iSelect;

	//The chooser doesn't hear the rows it takes away and puts back, only the row it selects after.
	mpList->GetSelectionEvent()->Remove(this);
	mpList->RemoveAllComponents();
	masRows.ReInit();

	iSelect = -1;
	for (i = 0; i < NumLibraries(); i++)
	{
		iMatches = 0;
		for (j = 0; j < NumEntries(i); j++)
		{
			if (EntryMatches(i, j))
			{
				iMatches++;
			}
		}
		if (iMatches == 0)
		{
			continue;
		}

		if ((i == iSelectLibrary) && (iSelectEntry == -1))
		{
			iSelect = mpList->NumRows();
		}
		AddLibraryRow(i);
		if (maiExpanded.GetValue(i))
		{
			for (j = 0; j < NumEntries(i); j++)
			{
				if (EntryMatches(i, j))
				{
					if ((i == iSelectLibrary) && (j == iSelectEntry))
					{
						iSelect = mpList->NumRows();
					}
					AddComponentRow(i, j);
				}
			}
		}
	}
	if ((iSelect == -1) && (mpList->NumRows() > 0))
	{
		iSelect = 0;
	}

	mpList->GetSelectionEvent()->Add(this, &CLogicimComponentChooser::SelectionChanged);
	mpList->Select(iSelect);
	ShowComponent(iSelect);
}


//////////////////////////////////////////////////////////////////////////
//
// A chevron, pointing down if the library's open, and its name.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::AddLibraryRow(int iLibrary)
{
	Ptr<CWindow>			pWindow;
	Ptr<CFillContainer>		pGap;
	Ptr<CIconComponent>		pChevron;
	Ptr<CLabel>				pLabel;
	int32					iRow;
	SLogicimChooserRow*		psRow;

	pWindow = mpDialog->GetWindow();
	iRow = mpList->AddRow(true);
	pGap = OMalloc<CFillContainer>(pWindow);
	pGap->SetDesiredSize(CHOOSER_INDENT, CHOOSER_ROW_HEIGHT);
	pChevron = OMalloc<CIconComponent>(pWindow, maiExpanded.GetValue(iLibrary) ? mpExpandedChevron : mpCollapsedChevron, CHOOSER_CHEVRON_WIDTH, CHOOSER_ROW_HEIGHT);
	pChevron->SetCelAlignment(CSH_Left, CSV_Center);
	pLabel = OMalloc<CLabel>(pWindow, mpDialog->GetLook()->pcTheme->GetFont(), GetLibraryName(iLibrary));
	pLabel->SetAlignment(CSH_Left, CSV_Center);
	mpList->AddToRow(iRow, pGap, 0);
	mpList->AddToRow(iRow, pChevron, 1);
	mpList->AddToRow(iRow, pLabel, 2, 2, 1);
	psRow = masRows.Add();
	psRow->iLibrary = iLibrary;
	psRow->iEntry = -1;
}


//////////////////////////////////////////////////////////////////////////
//
// The component's name under its library's name, then its description,
// dimmed.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::AddComponentRow(int iLibrary, int iEntry)
{
	Ptr<CWindow>			pWindow;
	Ptr<CFillContainer>		pGap;
	Ptr<CLabel>				pName;
	Ptr<CLabel>				pDescription;
	int32					iRow;
	SLogicimChooserRow*		psRow;

	pWindow = mpDialog->GetWindow();
	iRow = mpList->AddRow(true);
	pGap = OMalloc<CFillContainer>(pWindow);
	pGap->SetDesiredSize(CHOOSER_INDENT, CHOOSER_ROW_HEIGHT);
	pName = OMalloc<CLabel>(pWindow, mpDialog->GetLook()->pcTheme->GetFont(), GetEntryName(iLibrary, iEntry));
	pName->SetDesiredSize(CHOOSER_NAME_WIDTH, -1);
	pName->SetAlignment(CSH_Left, CSV_Center);
	pDescription = OMalloc<CLabel>(pWindow, mpDialog->GetLook()->pcDimFont, GetEntryDescription(iLibrary, iEntry));
	pDescription->SetAlignment(CSH_Left, CSV_Center);
	mpList->AddToRow(iRow, pGap, 0, 2);
	mpList->AddToRow(iRow, pName, 2);
	mpList->AddToRow(iRow, pDescription, 3, 1, 1);
	psRow = masRows.Add();
	psRow->iLibrary = iLibrary;
	psRow->iEntry = iEntry;
}


//////////////////////////////////////////////////////////////////////////
//
// The element name of the row's component or its subcircuit's name; NULL
// for a library's row.
//
//////////////////////////////////////////////////////////////////////////
const char* CLogicimComponentChooser::GetRowName(int32 iRow)
{
	SLogicimChooserRow*		psRow;

	psRow = GetRow(iRow);
	if ((psRow == NULL) || (psRow->iEntry == -1))
	{
		return NULL;
	}
	if (psRow->iLibrary == (int)NumComponentLibraries())
	{
		return maszSubcircuits.GetText(psRow->iEntry);
	}
	return GetComponentLibrary(psRow->iLibrary)->asEntries[psRow->iEntry].szXMLName;
}


//////////////////////////////////////////////////////////////////////////
//
// The list's row iRow, NULL if there's no such row (-1 for none).
//
//////////////////////////////////////////////////////////////////////////
SLogicimChooserRow* CLogicimComponentChooser::GetRow(int32 iRow)
{
	if ((iRow < 0) || ((size)iRow >= masRows.NumElements()))
	{
		return NULL;
	}
	return masRows.Get(iRow);
}


//////////////////////////////////////////////////////////////////////////
//
// Previews the row's component, as it would be placed, and shows its
// details.  Nothing for a library's row or no row.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::ShowComponent(int32 iRow)
{
	CCircuitEditor*			pcEditor;
	const char*				szName;
	int						iLibrary;
	SComponentLibrary*		psLibrary;
	SComponentLibraryEntry*	psEntry;
	CChars					sz;
	SLogicimChooserRow*		psRow;

	mpPreviewDraw->SetPreview(NULL);
	KillPreview();

	psRow = GetRow(iRow);
	szName = GetRowName(iRow);
	if (szName == NULL)
	{
		mpName->SetText(" ");
		mpDescription->SetText(" ");
		mpLibrary->SetText(" ");
		return;
	}

	pcEditor = mpcCircuit->GetEditor();
	sz.Init();
	iLibrary = psRow->iLibrary;
	if (iLibrary == (int)NumComponentLibraries())
	{
		mpcPreview = NewPlacedSubcircuitInstanceView(pcEditor->GetCircuits()->Get(szName), 0, 0, ROT_East, mpcCircuit->GetFonts());
		mpName->SetText(szName);
		mpDescription->SetText("An instance of the project's subcircuit.");
		sz.Append("Library: Subcircuits");
	}
	else
	{
		psLibrary = GetComponentLibrary(iLibrary);
		psEntry = &psLibrary->asEntries[psRow->iEntry];
		mpcPreview = NewPlacedComponentView(szName, 0, 0, ROT_East, pcEditor->GetCircuit(), mpcCircuit->GetFonts());
		mpName->SetText(psEntry->szName);
		mpDescription->SetText(psEntry->szDescription);
		sz.Append("Library: ");
		sz.Append(psLibrary->szName);
		sz.Append("    Keywords: ");
		sz.Append(psEntry->szKeywords);
	}
	mpLibrary->SetText(sz.Text());
	sz.Kill();
	mpPreviewDraw->SetPreview(mpcPreview);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::KillPreview(void)
{
	SafeKill(mpcPreview);
}


//////////////////////////////////////////////////////////////////////////
//
// Returns true if a component was chosen.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimComponentChooser::Show(void)
{
	const char*	szName;
	int32		iRow;

	if (!mpDialog->Show(CHOOSER_WIDTH, CHOOSER_HEIGHT))
	{
		return false;
	}
	iRow = mpList->GetSelected();
	szName = GetRowName(iRow);
	if (szName == NULL)
	{
		return false;
	}
	mszChosen.Set(szName);
	mbChosenSubcircuit = GetRow(iRow)->iLibrary == (int)NumComponentLibraries();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The element name of the component chosen, or the subcircuit's name if
// pbSubcircuit is set.
//
//////////////////////////////////////////////////////////////////////////
char* CLogicimComponentChooser::GetChosen(bool* pbSubcircuit)
{
	*pbSubcircuit = mbChosenSubcircuit;
	return mszChosen.Text();
}


//////////////////////////////////////////////////////////////////////////
//
// The libraries the filter matches are opened, or all closed without a
// filter but for the selected component's.  The selected row stays
// selected if it's still shown.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::FilterChanged(void)
{
	SLogicimChooserRow*		psRow;
	int						iLibrary;
	int						iEntry;

	iLibrary = -1;
	iEntry = -1;
	psRow = GetRow(mpList->GetSelected());
	if (psRow != NULL)
	{
		iLibrary = psRow->iLibrary;
		iEntry = psRow->iEntry;
	}

	ExpandMatching();
	if (StrEmpty(mpFilter->GetText()) && (iEntry != -1))
	{
		maiExpanded.SetValue(iLibrary, 1);
	}
	FillList(iLibrary, iEntry);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::SelectionChanged(void)
{
	ShowComponent(mpList->GetSelected());
}


//////////////////////////////////////////////////////////////////////////
//
// A double click, or enter, on a component chooses it; on a library it
// opens or closes it.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::ItemActivated(void)
{
	SLogicimChooserRow*		psRow;
	int						iLibrary;

	psRow = GetRow(mpList->GetSelected());
	if (psRow == NULL)
	{
		return;
	}
	if (psRow->iEntry != -1)
	{
		mpDialog->Close("OK");
		return;
	}
	iLibrary = psRow->iLibrary;
	maiExpanded.SetValue(iLibrary, maiExpanded.GetValue(iLibrary) ? 0 : 1);
	FillList(iLibrary, -1);
}

