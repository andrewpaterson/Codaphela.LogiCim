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
#ifdef LINUX_GNU
#include <stdio.h>
#include <string.h>
#include "BaseLib/GlobalMemory.h"
#include "StandardLib/Objects.h"
#include "X11Lib/X11WindowFactory.h"
#include "X11Lib/X11Input.h"
#include "KicadImporterLib/KicadSchemLogicimDecode.h"
#include "LogicimApp.h"


//////////////////////////////////////////////////////////////////////////
//
// LogiCim --decode <board.kicad_sch>: the board's address decode, a line
// for each range of addresses a chip select is active over in each mode
// (see CKicadSchemLogicimDecode), with no window.
//
//////////////////////////////////////////////////////////////////////////
static bool PrintLogicimDecode(const char* szSchematic)
{
	CChars	szRanges;
	bool	bResult;

	szRanges.Init();
	bResult = DecodeKicadSchemFile(szSchematic, "Resources/PropagationTimes.csv", "Resources/Fonts", &szRanges);
	if (bResult)
	{
		fputs(szRanges.Text(), stdout);
	}
	szRanges.Kill();
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// LogiCim's window on Linux, or its decode (--decode).  Run from the
// LogiCim directory, the resources and WinInputLib's XML are found
// relative to it.
//
//////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	bool	bResult;

	InitLogicimGlobals();

	if ((argc == 3) && (strcmp(argv[1], "--decode") == 0))
	{
		bResult = PrintLogicimDecode(argv[2]);
	}
	else
	{
		Ptr<CX11WindowFactory>		pNativeFactory;
		CX11Input					cX11Input;

		//Rooted while LogiCim runs.  Its windows hold it too, so it's freed after the last of them.
		pNativeFactory = OMalloc<CX11WindowFactory>(&gcMemoryAllocator, (const char*)NULL);
		ORoot()->Add(pNativeFactory);
		bResult = pNativeFactory->IsOpen();
		if (bResult)
		{
			cX11Input.Init();
			pNativeFactory->SetInput(&cX11Input);

			bResult = RunLogicimApp(&pNativeFactory, &cX11Input, "../../../CommonLibraries/Libraries/WinInputLib/xml");

			pNativeFactory->SetInput(NULL);
			cX11Input.Kill();
		}
		ORoot()->Remove(pNativeFactory);
		pNativeFactory = NULL;
	}

	KillLogicimGlobals();
	return bResult ? 0 : 1;
}


#endif // LINUX_GNU

