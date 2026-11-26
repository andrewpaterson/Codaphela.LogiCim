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


//The regulators on the HCP65's boards, from their datasheets (HCP65/Datasheets); the last is any other.
static SKicadSchemRegulatorPart gasKicadSchemRegulatorParts[] =
{
	{ "LD1117", 1.1f, 1000000 },  //1.1V at 500mA; 1ms for the 100uF the boards put on it.
	{ "AP7365", 0.3f, 200000 },  //Start-up 200us.
	{ "L78", 2.0f, 1000000 },
	{ "", 1.0f, 1000000 },
};


//The DS1813's trip voltages by its part number's suffix, typical.
struct SKicadSchemSupervisorTrip
{
	const char*		szSuffix;
	float32			fTripVoltage;
};
static SKicadSchemSupervisorTrip gasKicadSchemSupervisorTrips[] =
{
	{ "-5", 4.62f },
	{ "-10", 4.35f },
	{ "-15", 4.13f },
};


//////////////////////////////////////////////////////////////////////////
//
// Whether szText starts with szPrefix.
//
//////////////////////////////////////////////////////////////////////////
static bool StartsWithKicadSchemPart(const char* szText, const char* szPrefix)
{
	return (szText != NULL) && (strncmp(szText, szPrefix, StrLen(szPrefix)) == 0);
}


//////////////////////////////////////////////////////////////////////////
//
// The regulator part the value starts with ("LD1117S33C"), or the any
// other regulator entry.
//
//////////////////////////////////////////////////////////////////////////
SKicadSchemRegulatorPart* GetKicadSchemRegulatorPart(const char* szValue)
{
	size	i;
	size	uiParts;

	uiParts = sizeof(gasKicadSchemRegulatorParts) / sizeof(SKicadSchemRegulatorPart);
	for (i = 0; i < uiParts - 1; i++)
	{
		if (StartsWithKicadSchemPart(szValue, gasKicadSchemRegulatorParts[i].szPart))
		{
			return &gasKicadSchemRegulatorParts[i];
		}
	}
	return &gasKicadSchemRegulatorParts[uiParts - 1];
}


//////////////////////////////////////////////////////////////////////////
//
// A DS1813's trip voltage from its value ("DS1813R-10+T&R"), 0 if the
// value isn't a DS1813's.
//
//////////////////////////////////////////////////////////////////////////
float32 GetKicadSchemSupervisorTripVoltage(const char* szValue)
{
	size		i;
	const char*	szSuffix;

	if (!StartsWithKicadSchemPart(szValue, "DS1813"))
	{
		return 0.0f;
	}
	szSuffix = strchr(szValue, '-');
	for (i = 0; (szSuffix != NULL) && (i < sizeof(gasKicadSchemSupervisorTrips) / sizeof(SKicadSchemSupervisorTrip)); i++)
	{
		if (StartsWithKicadSchemPart(szSuffix, gasKicadSchemSupervisorTrips[i].szSuffix))
		{
			return gasKicadSchemSupervisorTrips[i].fTripVoltage;
		}
	}
	return 4.35f;
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
	CChars		szPrefix;
	const char*	szReference;
	size		i;
	bool		bSkipped;

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



//////////////////////////////////////////////////////////////////////////
//
// Whether a label's whole text names a supply: a ground ("GND",
// "GND_{OUT1}"), a supply pin's name (VCC, VCCIO, VCCINT) or a voltage
// (see IsKicadSchemVoltageLabel).  The HCP65's schematics name their
// supplies with labels, not power symbols, which KiCad made global and
// broke a schematic with sub-sheets (the user, 2026-11-26: "GND, 3.3V,
// 5V, 12V, 18V ... should be treated as supplies"; "GND_{OUT*} yes, its
// power label.  VCC, VCCIO and VCCINT yes").  The grounds like them are
// ordinary nets: GND_{A} EN to GND_{D} EN enable the Pico boards'
// switched grounds GNDa to GNDd, and GND_{TOP} and GND_{1} to GND_{4}
// are what they switch.
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemSupplyLabel(const char* szText)
{
	return IsKicadSchemGroundLabel(szText) || IsKicadSchemVccLabel(szText) || IsKicadSchemVoltageLabel(szText);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a label's whole text is a ground's: "GND", or a switched
// ground's output, "GND_{OUT" and a number and "}".
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemGroundLabel(const char* szText)
{
	size	i;

	if (szText == NULL)
	{
		return false;
	}
	if (strcmp(szText, "GND") == 0)
	{
		return true;
	}
	if (strncmp(szText, "GND_{OUT", 8) != 0)
	{
		return false;
	}
	for (i = 8; (szText[i] >= '0') && (szText[i] <= '9'); i++)
	{
	}
	return (i > 8) && (szText[i] == '}') && (szText[i + 1] == '\0');
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a label's whole text is a supply pin's name that says no
// voltage: VCC, VCCIO or VCCINT (a CPLD's I/O and core supplies), all
// 5V (see GetKicadSchemSupplyLabelVoltage).
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemVccLabel(const char* szText)
{
	return (szText != NULL) && ((strcmp(szText, "VCC") == 0) || (strcmp(szText, "VCCIO") == 0) || (strcmp(szText, "VCCINT") == 0));
}


//////////////////////////////////////////////////////////////////////////
//
// Whether a label's whole text is a voltage: an optional sign, digits, an
// optional decimal part and V ("3.3V", "-5V", "1.25V").
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemVoltageLabel(const char* szText)
{
	size	i;
	size	uiDigits;

	if (szText == NULL)
	{
		return false;
	}
	i = ((szText[0] == '+') || (szText[0] == '-')) ? 1 : 0;
	for (uiDigits = 0; (szText[i] >= '0') && (szText[i] <= '9'); i++)
	{
		uiDigits++;
	}
	if (uiDigits == 0)
	{
		return false;
	}
	if (szText[i] == '.')
	{
		i++;
		for (uiDigits = 0; (szText[i] >= '0') && (szText[i] <= '9'); i++)
		{
			uiDigits++;
		}
		if (uiDigits == 0)
		{
			return false;
		}
	}
	return (szText[i] == 'V') && (szText[i + 1] == '\0');
}


//////////////////////////////////////////////////////////////////////////
//
// The voltage a supply label names: 0 for a ground, negative for "-5V",
// and 5V for VCC, VCCIO and VCCINT (GetKicadSchemSupplyVoltage's for a
// name with no voltage).  VCC is always 5V: the CPLD is a MAX 7000, whose
// VCCINT is 5V, and the MAX II programmer's 3.3V read as 5V is fine (the
// user, 2026-11-26).
//
//////////////////////////////////////////////////////////////////////////
float32 GetKicadSchemSupplyLabelVoltage(const char* szText)
{
	if (IsKicadSchemGroundLabel(szText))
	{
		return 0.0f;
	}
	return (szText[0] == '-') ? -GetKicadSchemSupplyVoltage(szText) : GetKicadSchemSupplyVoltage(szText);
}
