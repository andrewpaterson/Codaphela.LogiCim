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
#include <stdlib.h>
#include "BaseLib/FileUtil.h"
#include "BaseLib/Logger.h"
#include "BaseLib/StringHelper.h"
#include "BaseLib/XMLFile.h"
#include "BaseLib/XMLWriter.h"
#include "LogicimSettings.h"


#define LOGICIM_SETTINGS_DIRECTORY		"LogiCim"
#define LOGICIM_SETTINGS_FILE			"Settings.xml"
#define LOGICIM_RECENT_FILE				"RecentProjects.xml"
#define LOGICIM_LAYOUT_FILE				"Layout.xml"
#define LOGICIM_KEYMAP_FILE				"Keymap.xml"
#define LOGICIM_SETTINGS_TAG			"LogicimSettings"
#define LOGICIM_FILES_TAG				"Files"
#define LOGICIM_KICAD_IMPORT_ATTRIBUTE	"KicadImport"
#define LOGICIM_PROJECT_LOAD_ATTRIBUTE	"ProjectLoad"

#define LOGICIM_DEFAULT_KICAD_IMPORT	"Schematic.kicad_sch"
#define LOGICIM_DEFAULT_PROJECT_LOAD	"Project.xml"


//////////////////////////////////////////////////////////////////////////
//
// The dialogs start in the current folder until Read finds what the user
// chose last.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimSettings::Init(void)
{
	mszFile.Init();
	mszKicadImport.Init(LOGICIM_DEFAULT_KICAD_IMPORT);
	mszProjectLoad.Init(LOGICIM_DEFAULT_PROJECT_LOAD);
	mcRecent.Init();
	mszRecentFile.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CLogicimSettings::Kill(void)
{
	mszRecentFile.Kill();
	mcRecent.Kill();
	mszProjectLoad.Kill();
	mszKicadImport.Kill();
	mszFile.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// What the user chose last, from szFile, which is where they're written
// from now on.  It being missing only means nothing has been kept yet;
// "" keeps nothing.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimSettings::Read(const char* szFile)
{
	CXMLFile	cFile;
	CFileUtil	cFileUtil;
	CChars		szFileName;
	CChars		szDirectory;
	CMarkupTag*	pcRoot;
	CMarkupTag*	pcFiles;
	char*		szValue;

	mszFile.Set(szFile);
	if (mszFile.Empty() || !cFileUtil.Exists(szFile))
	{
		return;
	}
	szFileName.Init();
	szDirectory.Init();
	cFileUtil.SplitPath(szFile, &szFileName, &szDirectory);
	if (szDirectory.Empty())
	{
		szDirectory.Set(".");
	}
	cFile.Init();
	pcRoot = (cFile.Read(szFileName.Text(), szDirectory.Text()) && cFile.mcMarkup.mpcDoc) ? cFile.mcMarkup.mpcDoc->GetRootTag() : NULL;
	if (pcRoot && (StringCompare(pcRoot->GetName(), LOGICIM_SETTINGS_TAG) == 0))
	{
		pcFiles = pcRoot->GetTag(LOGICIM_FILES_TAG);
		if (pcFiles)
		{
			szValue = pcFiles->GetStringAttribute(LOGICIM_KICAD_IMPORT_ATTRIBUTE);
			if (!StrEmpty(szValue))
			{
				mszKicadImport.Set(szValue);
			}
			szValue = pcFiles->GetStringAttribute(LOGICIM_PROJECT_LOAD_ATTRIBUTE);
			if (!StrEmpty(szValue))
			{
				mszProjectLoad.Set(szValue);
			}
		}
	}
	else
	{
		gcLogger.Warning2(__METHOD__, " Settings [", szFile, "] couldn't be read.", NULL);
	}
	cFile.Kill();
	szDirectory.Kill();
	szFileName.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// What Read reads.  False if it couldn't be written; true if it was, or
// nothing is kept.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimSettings::Write(void)
{
	CXMLWriter	cWriter;
	CFileUtil	cFileUtil;
	bool		bResult;

	if (mszFile.Empty())
	{
		return true;
	}
	cFileUtil.TouchDir(mszFile.Text(), true);
	cWriter.Init();
	cWriter.Begin(LOGICIM_SETTINGS_TAG);
	cWriter.Begin(LOGICIM_FILES_TAG);
	cWriter.Attribute(LOGICIM_KICAD_IMPORT_ATTRIBUTE, mszKicadImport.Text());
	cWriter.Attribute(LOGICIM_PROJECT_LOAD_ATTRIBUTE, mszProjectLoad.Text());
	cWriter.End();
	cWriter.End();
	bResult = cWriter.Write(mszFile.Text());
	cWriter.Kill();
	if (!bResult)
	{
		gcLogger.Warning2(__METHOD__, " Settings [", mszFile.Text(), "] couldn't be written.", NULL);
	}
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
//
// Each is written as it changes, so a crash doesn't lose it.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimSettings::SetKicadImport(const char* szFilename)
{
	mszKicadImport.Set(szFilename);
	Write();
}


void CLogicimSettings::SetProjectLoad(const char* szFilename)
{
	mszProjectLoad.Set(szFilename);
	Write();
}


char* CLogicimSettings::GetKicadImport(void) { return mszKicadImport.Text(); }
char* CLogicimSettings::GetProjectLoad(void) { return mszProjectLoad.Text(); }


//////////////////////////////////////////////////////////////////////////
//
// The recent projects, from szFile, which is where they're written from
// now on.  It being missing only means there are none yet; "" keeps
// none.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimSettings::ReadRecent(const char* szFile)
{
	CFileUtil	cFileUtil;

	mszRecentFile.Set(szFile);
	mcRecent.Clear();
	if (!mszRecentFile.Empty() && cFileUtil.Exists(szFile) && !mcRecent.Read(szFile))
	{
		gcLogger.Warning2(__METHOD__, " Recent projects [", szFile, "] couldn't be read.", NULL);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// False if they couldn't be written; true if they were, or none are kept.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimSettings::WriteRecent(void)
{
	if (mszRecentFile.Empty())
	{
		return true;
	}
	if (!mcRecent.Write(mszRecentFile.Text()))
	{
		gcLogger.Warning2(__METHOD__, " Recent projects [", mszRecentFile.Text(), "] couldn't be written.", NULL);
		return false;
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// szFilename, loaded or saved, is the latest; written as it changes.
//
//////////////////////////////////////////////////////////////////////////
void CLogicimSettings::AddRecent(const char* szFilename)
{
	mcRecent.Add(szFilename);
	WriteRecent();
}


//////////////////////////////////////////////////////////////////////////
//
// szFilename taken off (it isn't there any more).  False if it wasn't
// recent.
//
//////////////////////////////////////////////////////////////////////////
bool CLogicimSettings::RemoveRecent(const char* szFilename)
{
	if (!mcRecent.Remove(szFilename))
	{
		return false;
	}
	WriteRecent();
	return true;
}


void CLogicimSettings::ClearRecent(void)
{
	mcRecent.Clear();
	WriteRecent();
}


CRecentFiles* CLogicimSettings::GetRecent(void) { return &mcRecent; }


//////////////////////////////////////////////////////////////////////////
//
// A file of the user's, szFile in %APPDATA%\LogiCim, or ~/.config/LogiCim
// on Linux; the variable szVariable if it is set.  A scripted run
// (LOGICIM_CHECK, LOGICIM_IMPORT or LOGICIM_SNAPSHOT) has none unless
// szVariable is set: the user's own isn't what's checked and mustn't be
// changed by it.  Empty if there's no such place.
//
//////////////////////////////////////////////////////////////////////////
static void GetLogicimUserFile(CChars* pszDest, const char* szFile, const char* szVariable)
{
	CFileUtil	cFileUtil;
	const char*	szBase;

	pszDest->Init();
	if (getenv(szVariable) != NULL)
	{
		pszDest->Set(getenv(szVariable));
		return;
	}
	if ((getenv("LOGICIM_CHECK") != NULL) || (getenv("LOGICIM_IMPORT") != NULL) || (getenv("LOGICIM_SNAPSHOT") != NULL))
	{
		return;
	}
#ifdef _WIN32
	szBase = getenv("APPDATA");
	if (szBase != NULL)
	{
		pszDest->Set(szBase);
	}
#else
	szBase = getenv("HOME");
	if (szBase != NULL)
	{
		pszDest->Set(szBase);
		cFileUtil.AppendToPath(pszDest, ".config");
	}
#endif
	if (pszDest->Empty())
	{
		return;
	}
	cFileUtil.AppendToPath(pszDest, LOGICIM_SETTINGS_DIRECTORY);
	cFileUtil.AppendToPath(pszDest, szFile);
}


//////////////////////////////////////////////////////////////////////////
//
// The user's settings (Settings.xml, LOGICIM_SETTINGS), recent projects
// (RecentProjects.xml, LOGICIM_RECENT), window layout (Layout.xml,
// LOGICIM_LAYOUT: the window and its tool windows, see CDockManager) and
// the keys they've changed (Keymap.xml, LOGICIM_KEYMAP, see
// CCommandSet::ReadKeys).
//
//////////////////////////////////////////////////////////////////////////
void GetLogicimSettingsFile(CChars* pszDest) { GetLogicimUserFile(pszDest, LOGICIM_SETTINGS_FILE, "LOGICIM_SETTINGS"); }
void GetLogicimRecentFile(CChars* pszDest) { GetLogicimUserFile(pszDest, LOGICIM_RECENT_FILE, "LOGICIM_RECENT"); }
void GetLogicimLayoutFile(CChars* pszDest) { GetLogicimUserFile(pszDest, LOGICIM_LAYOUT_FILE, "LOGICIM_LAYOUT"); }
void GetLogicimKeymapFile(CChars* pszDest) { GetLogicimUserFile(pszDest, LOGICIM_KEYMAP_FILE, "LOGICIM_KEYMAP"); }

