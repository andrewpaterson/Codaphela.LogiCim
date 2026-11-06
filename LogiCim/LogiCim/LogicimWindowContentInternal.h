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
#ifndef __LOGICIM_WINDOW_CONTENT_INTERNAL_H__
#define __LOGICIM_WINDOW_CONTENT_INTERNAL_H__
#include <stdio.h>
#include <stdlib.h>
#include "BaseLib/Logger.h"
#include "BaseLib/FileUtil.h"
#include "StandardLib/Objects.h"
#include "StandardLib/ObjectReader.h"
#include "StandardLib/ObjectWriter.h"
#include "SupportLib/ColourARGB32.h"
#include "SupportLib/ImageColour.h"
#include "SupportLib/ImageColourConverter.h"
#include "SupportLib/ImageReader.h"
#include "SupportLib/TrueTypeFont.h"
#include "SupportLib/TrueTypeFontFactory.h"
#include "WindowLib/JewelColours.h"
#include "WindowLib/JewelTheme.h"
#include "WindowLib/ClearCanvasDraw.h"
#include "WindowLib/FillContainer.h"
#include "WindowLib/RoundedBoxCanvasDraw.h"
#include "LogicimComponentChooser.h"
#include "WindowLib/FileChooser.h"
#include "LogicimLib/ComponentProperties.h"
#include "MicroprocessorLib/LongTime.h"
#include "LogicimWindowContent.h"


#define LOGICIM_ROW_HEIGHT		24
#define LOGICIM_DIALOG_WIDTH		440


//The glyphs generated: printable ASCII.
#define LOGICIM_FONT_CHARACTERS	" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"


struct SLogicimActionDesc
{
	const char*		szName;
	const char*		szIcon;  //The Logicim icon's name, without _24.png.  NULL for none.
};


//A separator in a menu or a toolbar.
#define LA_Separator	NUM_LOGICIM_ACTIONS


//The helpers the files of LogicimWindowContent share.
extern SLogicimActionDesc gasLogicimActions[NUM_LOGICIM_ACTIONS];
void AppendTenths(CChars* psz, float32 f);


#endif // __LOGICIM_WINDOW_CONTENT_INTERNAL_H__
