#include "BaseLib/PointerFunctions.h"
#include "StandardLib/Objects.h"
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
//
//////////////////////////////////////////////////////////////////////////
void CChooserListListener::Init(CLogicimComponentChooser* pcChooser)
{
	mpcChooser = pcChooser;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CChooserListListener::SelectionChanged(CUnknown* pcSource, void* pvContext)
{
	mpcChooser->SelectionChanged();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CChooserListListener::ItemActivated(CUnknown* pcSource, void* pvContext)
{
	mpcChooser->ItemActivated();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CChooserFilterListener::Init(CLogicimComponentChooser* pcChooser)
{
	mpcChooser = pcChooser;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CChooserFilterListener::TextChanged(CUnknown* pcSource, void* pvContext)
{
	mpcChooser->FilterChanged();
}


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

	mpcCircuit = pcCircuit;
	maiExpanded.Init();
	mpcPreview = NULL;
	mszChosen.Init();
	mbChosenSubcircuit = false;
	maiRowLibraries.Init();
	maiRowEntries.Init();
	maszSubcircuits.Init();
	mcListListener.Init(this);
	mcFilterListener.Init(this);

	mcDialog.Init(pOwner, psLook, "Choose Component");
	mcDialog.GetWindow()->GetColourFormat(&cFormat);
	mpCollapsedChevron = CJewelImageGenerator::CreateChevron(JEWEL_ICON, &cFormat);
	mpExpandedChevron = CJewelImageGenerator::CreateDownChevron(JEWEL_ICON, &cFormat);
	pBody = CreateBody();
	mcDialog.SetBody(pBody, mpFilter);

	FindSubcircuits();
	for (i = 0; i < NumLibraries(); i++)
	{
		maiExpanded.Add(0);
	}
	mpFilter->AddListener(&mcFilterListener);

	iLibrary = -1;
	iEntry = -1;
	FindEntry(szSelected, &iLibrary, &iEntry);
	FillList(iLibrary, -1);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimComponentChooser::Kill(void)
{
	mpFilter->RemoveListener(&mcFilterListener);
	mpList->RemoveListener(&mcListListener);
	mpPreviewDraw->SetPreview(NULL);
	KillPreview();

	mpLibrary = NULL;
	mpDescription = NULL;
	mpName = NULL;
	mpPreviewDraw = NULL;
	mpExpandedChevron = NULL;
	mpCollapsedChevron = NULL;
	mpList = NULL;
	mpFilter = NULL;
	mcDialog.Kill();

	maszSubcircuits.Kill();
	maiRowEntries.Kill();
	maiRowLibraries.Kill();
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

	pWindow = mcDialog.GetWindow();
	psLook = mcDialog.GetLook();
	pBody = OMalloc<CGridContainer>(pWindow);

	mpFilter = OMalloc<CTextField>(pWindow, psLook->pTheme, "");
	mpFilter->SetColumns(10);
	mpFilter->SetAlignment(CSH_Stretch, CSV_Center);
	pBody->AddComponent(mpFilter, 0, 0, 3, 1, 1, 0);

	pGap = OMalloc<CFillContainer>(pWindow);
	pGap->SetDesiredSize(CHOOSER_GAP, CHOOSER_GAP);
	pBody->AddComponent(pGap, 0, 1);

	sInsets.Init(aiInsets);
	mpList = OMalloc<CListContainer>(pWindow, psLook->pTheme);
	mpList->SetHighlightDraws(OMalloc<CRoundedBoxCanvasDraw>(psLook->pTheme->GetListSelectedColour(), 4, &sInsets), OMalloc<CRoundedBoxCanvasDraw>(psLook->pTheme->GetListHoverColour(), 4, &sInsets));
	mpList->SetAlignment(CSH_Stretch, CSV_Top);
	mpList->AddListener(&mcListListener);
	pScrollPane = OMalloc<CScrollPane>(pWindow, psLook->pTheme);
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
	mpName = OMalloc<CLabel>(pWindow, psLook->pTheme->GetFont(), " ");
	mpName->SetAlignment(CSH_Left, CSV_Center);
	mpDescription = OMalloc<CLabel>(pWindow, psLook->pDimFont, " ");
	mpDescription->SetAlignment(CSH_Left, CSV_Center);
	mpLibrary = OMalloc<CLabel>(pWindow, psLook->pDimFont, " ");
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

	mpList->RemoveListener(&mcListListener);
	mpList->RemoveAllComponents();
	maiRowLibraries.ReInit();
	maiRowEntries.ReInit();

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

	mpList->AddListener(&mcListListener);
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

	pWindow = mcDialog.GetWindow();
	iRow = mpList->AddRow(true);
	pGap = OMalloc<CFillContainer>(pWindow);
	pGap->SetDesiredSize(CHOOSER_INDENT, CHOOSER_ROW_HEIGHT);
	pChevron = OMalloc<CIconComponent>(pWindow, maiExpanded.GetValue(iLibrary) ? mpExpandedChevron : mpCollapsedChevron, CHOOSER_CHEVRON_WIDTH, CHOOSER_ROW_HEIGHT);
	pChevron->SetCelAlignment(CSH_Left, CSV_Center);
	pLabel = OMalloc<CLabel>(pWindow, mcDialog.GetLook()->pTheme->GetFont(), GetLibraryName(iLibrary));
	pLabel->SetAlignment(CSH_Left, CSV_Center);
	mpList->AddToRow(iRow, pGap, 0);
	mpList->AddToRow(iRow, pChevron, 1);
	mpList->AddToRow(iRow, pLabel, 2, 2, 1);
	maiRowLibraries.Add(iLibrary);
	maiRowEntries.Add(-1);
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

	pWindow = mcDialog.GetWindow();
	iRow = mpList->AddRow(true);
	pGap = OMalloc<CFillContainer>(pWindow);
	pGap->SetDesiredSize(CHOOSER_INDENT, CHOOSER_ROW_HEIGHT);
	pName = OMalloc<CLabel>(pWindow, mcDialog.GetLook()->pTheme->GetFont(), GetEntryName(iLibrary, iEntry));
	pName->SetDesiredSize(CHOOSER_NAME_WIDTH, -1);
	pName->SetAlignment(CSH_Left, CSV_Center);
	pDescription = OMalloc<CLabel>(pWindow, mcDialog.GetLook()->pDimFont, GetEntryDescription(iLibrary, iEntry));
	pDescription->SetAlignment(CSH_Left, CSV_Center);
	mpList->AddToRow(iRow, pGap, 0, 2);
	mpList->AddToRow(iRow, pName, 2);
	mpList->AddToRow(iRow, pDescription, 3, 1, 1);
	maiRowLibraries.Add(iLibrary);
	maiRowEntries.Add(iEntry);
}


//////////////////////////////////////////////////////////////////////////
//
// The element name of the row's component or its subcircuit's name; NULL
// for a library's row.
//
//////////////////////////////////////////////////////////////////////////
const char* CLogicimComponentChooser::GetRowName(int32 iRow)
{
	int		iLibrary;
	int		iEntry;

	if ((iRow < 0) || (iRow >= (int32)maiRowEntries.NumElements()))
	{
		return NULL;
	}
	iLibrary = maiRowLibraries.GetValue(iRow);
	iEntry = maiRowEntries.GetValue(iRow);
	if (iEntry == -1)
	{
		return NULL;
	}
	if (iLibrary == (int)NumComponentLibraries())
	{
		return maszSubcircuits.GetText(iEntry);
	}
	return GetComponentLibrary(iLibrary)->asEntries[iEntry].szXMLName;
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

	mpPreviewDraw->SetPreview(NULL);
	KillPreview();

	if ((iRow < 0) || (iRow >= (int32)maiRowEntries.NumElements()))
	{
		mpName->SetText(" ");
		mpDescription->SetText(" ");
		mpLibrary->SetText(" ");
		return;
	}
	iLibrary = maiRowLibraries.GetValue(iRow);
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
		psEntry = &psLibrary->asEntries[maiRowEntries.GetValue(iRow)];
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

	if (!mcDialog.Show(CHOOSER_WIDTH, CHOOSER_HEIGHT))
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
	mbChosenSubcircuit = maiRowLibraries.GetValue(iRow) == (int)NumComponentLibraries();
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
	int32	iRow;
	int		iLibrary;
	int		iEntry;

	iLibrary = -1;
	iEntry = -1;
	iRow = mpList->GetSelected();
	if ((iRow >= 0) && (iRow < (int32)maiRowEntries.NumElements()))
	{
		iLibrary = maiRowLibraries.GetValue(iRow);
		iEntry = maiRowEntries.GetValue(iRow);
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
	int32	iRow;
	int		iLibrary;

	iRow = mpList->GetSelected();
	if ((iRow < 0) || (iRow >= (int32)maiRowEntries.NumElements()))
	{
		return;
	}
	if (maiRowEntries.GetValue(iRow) != -1)
	{
		mcDialog.Close("OK");
		return;
	}
	iLibrary = maiRowLibraries.GetValue(iRow);
	maiExpanded.SetValue(iLibrary, maiExpanded.GetValue(iLibrary) ? 0 : 1);
	FillList(iLibrary, -1);
}

