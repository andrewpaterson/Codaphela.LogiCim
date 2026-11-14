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
#ifndef __LOGICIM_COMPONENT_CHOOSER_H__
#define __LOGICIM_COMPONENT_CHOOSER_H__
#include "BaseLib/ArrayInt.h"
#include "BaseLib/ArrayChars.h"
#include "BaseLib/ArrayTemplate.h"
#include "WindowLib/ListContainer.h"
#include "WindowLib/ScrollPane.h"
#include "WindowLib/Label.h"
#include "WindowLib/Dialog.h"
#include "WindowLib/Button.h"
#include "WindowLib/KeymapPage.h"
#include "WindowLib/KeyGiver.h"
#include "CircuitCanvasDraw.h"


class CLogicimCircuit;
class CComponentView;


//A row of the chooser's tree: its library (NumComponentLibraries() for the subcircuits) and its component in the
//library, -1 for the library's own row.
struct SLogicimChooserRow
{
	int		iLibrary;
	int		iEntry;
};


typedef CArrayTemplate<SLogicimChooserRow>	CArrayLogicimChooserRow;


//Logicim's Select Component dialog, laid out as KiCad's symbol chooser: a filter along the top; a tree of the libraries
//of components (see ComponentLibraries.h), and of the project's subcircuits that can go in the circuit being edited,
//down the left, each component with its description; a preview of the selected component on the right; and its name,
//description, library and keywords along the bottom.
//
//The libraries start closed, the library of the component last chosen selected.  A double click on a library (or
//enter) opens or closes it.  Typing in the filter shows only the components with every word of it in
//them, their libraries opened; clearing it closes them again.  A double click on a component (or enter, or OK)
//chooses it.
//
//Above the preview is the selected component's shortcut, its Place command's keys (a subcircuit's are the project's),
//which Add Shortcut and Remove change there and then, as Settings' Keymap would (see CKeyGiver): whoever shows the
//chooser binds and keeps them (KeysChanged).
class CLogicimComponentChooser : public CObject, public CKeyCaptureDescriber
{
CONSTRUCTABLE(CLogicimComponentChooser);
DESTRUCTABLE(CLogicimComponentChooser);
protected:
	Ptr<CDialog>			mpDialog;
	CLogicimCircuit*		mpcCircuit;
	Ptr<CTextField>			mpFilter;
	Ptr<CListContainer>		mpList;
	Ptr<CCircuitCanvasDraw>	mpPreviewDraw;
	Ptr<CCanvas>			mpPreview;  //Drawn again when its preview changes, which its draw can't tell it.
	Ptr<CLabel>				mpName;
	Ptr<CLabel>				mpDescription;
	Ptr<CLabel>				mpLibrary;
	Ptr<CLabel>				mpShortcut;
	Ptr<CButton>			mpAddShortcut;
	Ptr<CButton>			mpRemoveShortcut;
	Ptr<CLabel>				mpShortcutStatus;  //What the last shortcut change did, or why not.
	Ptr<CKeyCaptureDialog>	mpCapture;
	CCommandSet*			mpcCommands;  //Not held: the window's.
	CCommandSetKeys			mcKeys;
	CKeyGiver				mcGiver;
	bool					mbKeysChanged;
	CArrayInt				maiExpanded;  //1 for each library that's open, the subcircuits last.
	CArrayLogicimChooserRow	masRows;
	CArrayChars				maszSubcircuits;
	CComponentView*			mpcPreview;
	CChars					mszChosen;
	bool					mbChosenSubcircuit;

public:
	void				Init(Ptr<CWindow> pOwner, SDialogLook* psLook, CLogicimCircuit* pcCircuit, const char* szSelected, CCommandSet* pcCommands, CComponentInput* pcInput);
	void				Class(void) override;
	void				Free(void) override;

	bool				Save(CObjectWriter* pcFile) override;
	bool				Load(CObjectReader* pcFile) override;

	bool				Show(void);
	char*				GetChosen(bool* pbSubcircuit);
	bool				KeysChanged(void);
	void				ApplyEdits(const char* szEdits);

	void				FilterChanged(void);
	void				SelectionChanged(void);
	void				ItemActivated(void);
	void				AddShortcutClicked(CButton* pcButton);
	void				RemoveShortcutClicked(CButton* pcButton);
	bool				DescribeCapturedKeys(const char* szContext, const char* szKeys, CChars* pszMessage) override;

protected:
	Ptr<CComponent>		CreateBody(void);
	void				FindSubcircuits(void);
	int					NumLibraries(void);
	int					NumEntries(int iLibrary);
	const char*			GetLibraryName(int iLibrary);
	const char*			GetEntryName(int iLibrary, int iEntry);
	const char*			GetEntryDescription(int iLibrary, int iEntry);
	bool				EntryMatches(int iLibrary, int iEntry);
	bool				FindEntry(const char* szName, int* piLibrary, int* piEntry);
	void				ExpandMatching(void);
	void				FillList(int iSelectLibrary, int iSelectEntry);
	void				AddLibraryRow(int iLibrary);
	void				AddComponentRow(int iLibrary, int iEntry);
	SLogicimChooserRow*	GetRow(int32 iRow);
	const char*			GetRowName(int32 iRow);
	void				ShowComponent(int32 iRow);
	void				KillPreview(void);
	Ptr<CComponent>		CreateShortcut(void);
	CCommand*			GetRowCommand(int32 iRow);
	void				ShowShortcut(void);
	void				ShowCapture(void);
	bool				AddShortcut(const char* szKeys, const char* szContext);
	void				RemoveShortcut(void);
};


#endif // __LOGICIM_COMPONENT_CHOOSER_H__

