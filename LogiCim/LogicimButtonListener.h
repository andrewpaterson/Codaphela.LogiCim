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
#ifndef __LOGICIM_BUTTON_LISTENER_H__
#define __LOGICIM_BUTTON_LISTENER_H__
#include "WindowLib/ButtonListener.h"
#include "WindowLib/MenuListener.h"
#include "WindowLib/ListListener.h"


class CLogicimWindowContent;


//The window content can't be every kind of listener itself, each is a CListener, so these pass what they hear on.
//This one passes on the toolbar's and the stripes' button clicks (see CLogicimWindowContent::ButtonClicked).
class CLogicimButtonListener : public CButtonListener
{
CONSTRUCTABLE(CLogicimButtonListener);
public:
	CLogicimWindowContent*	mpcContent;

	void	Init(CLogicimWindowContent* pcContent);
	void	ButtonClicked(CUnknown* pcSource, void* pvContext) override;
};


//Passes on the menu bar's item clicks (see CLogicimWindowContent::MenuItemClicked).
class CLogicimMenuListener : public CMenuListener
{
CONSTRUCTABLE(CLogicimMenuListener);
public:
	CLogicimWindowContent*	mpcContent;

	void	Init(CLogicimWindowContent* pcContent);
	void	MenuItemClicked(CUnknown* pcSource, void* pvContext) override;
};


//Passes on the subcircuit and simulation lists' selection changes (see CLogicimWindowContent::ListSelectionChanged).
class CLogicimListListener : public CListListener
{
CONSTRUCTABLE(CLogicimListListener);
public:
	CLogicimWindowContent*	mpcContent;

	void	Init(CLogicimWindowContent* pcContent);
	void	SelectionChanged(CUnknown* pcSource, void* pvContext) override;
};


#endif // __LOGICIM_BUTTON_LISTENER_H__

