/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela KicadImporterLib

Codaphela KicadImporterLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela KicadImporterLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela KicadImporterLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include <string.h>
#include "BaseLib/Chars.h"
#include "BaseLib/StringHelper.h"
#include "KicadSchemSupplyNames.h"


//Pin names on logic parts that are their power.
static const char* gaszKicadSchemPowerPinNames[] = { "GND", "VCC", "VDD", "VSS", "VEE", "3V", "5V", "3V3", "V+", "V-", "NC", "N.C." };


//Reference prefixes and library names of the symbols that are left out.
static const char* gaszKicadSchemSkippedReferences[] = { "R", "RN", "C", "CN", "L", "FB", "D", "LED", "H", "MH", "TP", "NT", "FID" };
static const char* gaszKicadSchemSkippedLibraryNames[] = { "MountingHole", "NetTie", "TestPoint", "PWR_FLAG", "Fiducial" };


//////////////////////////////////////////////////////////////////////////
//
// Whether the name is a ground's: "GND", "VSS", "AGND", "0V"...
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemGroundName(const char* szName)
{
	CChars	szUpper;
	bool	bGround;

	szUpper.Init(szName);
	szUpper.UpperCase();
	bGround = szUpper.StartsWith("GND") || szUpper.StartsWith("VSS") || szUpper.StartsWith("AGND") || szUpper.StartsWith("DGND") || szUpper.StartsWith("EARTH") || szUpper.Equals("0V");
	szUpper.Kill();
	return bGround;
}


//////////////////////////////////////////////////////////////////////////
//
// The voltage in a supply's name: "+5V" is 5, "3.3V" and "+3V3" are 3.3
// and "12VOut" is 12.  A name without one (VCC) is 5.
//
//////////////////////////////////////////////////////////////////////////
float32 GetKicadSchemSupplyVoltage(const char* szName)
{
	size		i;
	float64		fVoltage;
	float64		fScale;

	for (i = 0; (szName[i] != '\0') && ((szName[i] < '0') || (szName[i] > '9')); i++)
	{
	}
	if (szName[i] == '\0')
	{
		return 5.0f;
	}

	fVoltage = 0;
	for (; (szName[i] >= '0') && (szName[i] <= '9'); i++)
	{
		fVoltage = fVoltage * 10 + (szName[i] - '0');
	}
	if ((szName[i] == '.') || (((szName[i] == 'V') || (szName[i] == 'v')) && (szName[i + 1] >= '0') && (szName[i + 1] <= '9')))
	{
		i++;
		fScale = 0.1;
		for (; (szName[i] >= '0') && (szName[i] <= '9'); i++)
		{
			fVoltage += fScale * (szName[i] - '0');
			fScale /= 10;
		}
	}
	if (fVoltage <= 0)
	{
		return 5.0f;
	}
	return (float32)fVoltage;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the text has a digit, so it can say a voltage.
//
//////////////////////////////////////////////////////////////////////////
bool HasKicadSchemVoltage(const char* szText)
{
	size	i;

	for (i = 0; (szText != NULL) && (szText[i] != '\0'); i++)
	{
		if ((szText[i] >= '0') && (szText[i] <= '9'))
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the pin name, ignoring case, is one of a logic part's power
// pins (or unconnected pins): "GND", "VCC", "NC"...
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemPowerPinName(const char* szName)
{
	size	i;

	for (i = 0; i < sizeof(gaszKicadSchemPowerPinNames) / sizeof(const char*); i++)
	{
		if (StringInsensitiveCompare(szName, gaszKicadSchemPowerPinNames[i]) == 0)
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the net is a supply's: a ground ("GND"), a supply pin's name
// ("VCC") or a voltage ("+3V3", "3.3V", "5V").
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemPowerNetName(const char* szName)
{
	size	i;
	bool	bVolts;

	if (IsKicadSchemGroundName(szName) || IsKicadSchemPowerPinName(szName) || (szName[0] == '+') || (szName[0] == '-'))
	{
		return true;
	}
	if ((szName[0] < '0') || (szName[0] > '9'))
	{
		return false;
	}
	bVolts = false;
	for (i = 0; szName[i] != '\0'; i++)
	{
		if ((szName[i] == 'V') || (szName[i] == 'v'))
		{
			bVolts = true;
		}
		else if (((szName[i] < '0') || (szName[i] > '9')) && (szName[i] != '.'))
		{
			return false;
		}
	}
	return bVolts;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the pin's name, ignoring case, ends with szEnd and is either
// szEnd alone, starts with a V ("VOUT"), or has a voltage ("3.3VOut").
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemRegulatorPinName(const char* szName, const char* szEnd)
{
	size	uiLength;
	size	uiEnd;
	size	i;

	uiLength = StrLen(szName);
	uiEnd = StrLen(szEnd);
	if ((uiLength < uiEnd) || (StringInsensitiveCompare(&szName[uiLength - uiEnd], szEnd) != 0))
	{
		return false;
	}
	if ((uiLength == uiEnd) || (szName[0] == 'V') || (szName[0] == 'v'))
	{
		return true;
	}
	for (i = 0; i < uiLength - uiEnd; i++)
	{
		if ((szName[i] >= '0') && (szName[i] <= '9'))
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a power symbol is a power flag ("PWR_FLAG"), which only tells
// KiCad's checks a net is driven: by the power name its pin gives or its
// library symbol's name.
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemPowerFlag(const char* szPowerName, CKicadSchemLibSymbol* pcLibSymbol)
{
	return (StringCompare(szPowerName, "PWR_FLAG") == 0) || (strstr(pcLibSymbol->GetName(), "PWR_FLAG") != NULL);
}


//////////////////////////////////////////////////////////////////////////
//
// Passive parts and mechanical symbols, by their reference's letters or
// their library symbol.
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemSkippedSymbol(CKicadSchemSymbol* pcSymbol)
{
	CChars	szPrefix;
	char*	szReference;
	size	i;
	bool	bSkipped;

	for (i = 0; i < sizeof(gaszKicadSchemSkippedLibraryNames) / sizeof(const char*); i++)
	{
		if ((strstr(pcSymbol->GetLibId(), gaszKicadSchemSkippedLibraryNames[i]) != NULL) || (strstr(pcSymbol->GetLibSymbolName(), gaszKicadSchemSkippedLibraryNames[i]) != NULL))
		{
			return true;
		}
	}

	szReference = pcSymbol->GetReference();
	if (szReference == NULL)
	{
		return false;
	}
	for (i = 0; ((szReference[i] >= 'A') && (szReference[i] <= 'Z')) || ((szReference[i] >= 'a') && (szReference[i] <= 'z')); i++)
	{
	}
	szPrefix.Init(szReference, 0, i);
	szPrefix.UpperCase();
	bSkipped = false;
	for (i = 0; i < sizeof(gaszKicadSchemSkippedReferences) / sizeof(const char*); i++)
	{
		if (szPrefix.Equals(gaszKicadSchemSkippedReferences[i]))
		{
			bSkipped = true;
			break;
		}
	}
	szPrefix.Kill();
	return bSkipped;
}

