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
#ifndef __LOGICIM_SETTINGS_H__
#define __LOGICIM_SETTINGS_H__
#include "BaseLib/Chars.h"


//What LogiCim keeps of how the user left it, between runs: the KiCad schematic last imported and the project last
//loaded, where Import KiCad Schematic and Load Project open.  They're kept in the user's own file (see
//GetLogicimSettingsFile), written as soon as one changes.  With no file they're kept only while LogiCim runs.
class CLogicimSettings
{
protected:
	CChars	mszFile;			//Where they're kept, empty if they aren't.
	CChars	mszKicadImport;		//The KiCad schematic last imported.
	CChars	mszProjectLoad;		//The project last loaded.

public:
	void	Init(void);
	void	Kill(void);

	void	Read(const char* szFile);
	bool	Write(void);

	char*	GetKicadImport(void);
	char*	GetProjectLoad(void);
	void	SetKicadImport(const char* szFilename);
	void	SetProjectLoad(const char* szFilename);
};


void GetLogicimSettingsFile(CChars* pszDest);


#endif // __LOGICIM_SETTINGS_H__

