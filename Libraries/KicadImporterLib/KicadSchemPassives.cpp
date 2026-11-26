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
#include <math.h>
#include "BaseLib/StringHelper.h"
#include "MicroprocessorLib/Voltage.h"
#include "KicadSchemPassives.h"


//The multiplier letters values have after their numbers.  The micro signs are UTF-8: Greek mu (U+03BC) and the
//micro sign (U+00B5).
struct SKicadSchemValueMultiplier
{
	const char*	szLetter;
	float64		fMultiplier;
};
static SKicadSchemValueMultiplier gasKicadSchemResistanceMultipliers[] = { { "K", 1e3 }, { "k", 1e3 }, { "M", 1e6 }, { "G", 1e9 } };
static SKicadSchemValueMultiplier gasKicadSchemCapacitanceMultipliers[] = { { "p", 1e-12 }, { "n", 1e-9 }, { "u", 1e-6 }, { "\xCE\xBC", 1e-6 }, { "\xC2\xB5", 1e-6 }, { "m", 1e-3 } };

//The resistor networks' part numbers, each followed by "-" and its value as three digits (after another dash for the
//4609X and 4816P).
static const char* gaszKicadSchemResistorNetworks[] = { "CAY16-", "4609X-", "4816P-" };


//////////////////////////////////////////////////////////////////////////
//
// Whether the symbol's reference is szLetters then a digit ("R1"), or
// szLetters and "N" then a digit, a network ("RN5").
//
//////////////////////////////////////////////////////////////////////////
static bool IsKicadSchemPassive(CKicadSchemSymbol* pcSymbol, char cLetter)
{
	const char*	szReference;
	size		i;

	szReference = pcSymbol->GetReference();
	if ((szReference == NULL) || (ToUpper(szReference[0]) != cLetter))
	{
		return false;
	}
	i = (ToUpper(szReference[1]) == 'N') ? 2 : 1;
	return (szReference[i] >= '0') && (szReference[i] <= '9');
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemResistor(CKicadSchemSymbol* pcSymbol)
{
	return IsKicadSchemPassive(pcSymbol, 'R');
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemCapacitor(CKicadSchemSymbol* pcSymbol)
{
	return IsKicadSchemPassive(pcSymbol, 'C');
}


//////////////////////////////////////////////////////////////////////////
//
// Three digits from szCode on: two digits and a power of ten ("221" is
// 220).  NaN if they aren't digits.
//
//////////////////////////////////////////////////////////////////////////
static float64 GetKicadSchemValueCode(const char* szCode)
{
	size	i;

	for (i = 0; i < 3; i++)
	{
		if ((szCode[i] < '0') || (szCode[i] > '9'))
		{
			return NoVoltage();
		}
	}
	return ((szCode[0] - '0') * 10 + (szCode[1] - '0')) * pow(10.0, szCode[2] - '0');
}


//////////////////////////////////////////////////////////////////////////
//
// A number then a multiplier letter ("4.7K", "0.1μ"), optional if bBare
// ("220"); what's after is the unit, which isn't checked.  NaN if it
// doesn't start with a number, or needs a letter and has none (a part
// number, "885012207033").
//
//////////////////////////////////////////////////////////////////////////
static float64 GetKicadSchemValueNumber(const char* szValue, SKicadSchemValueMultiplier* asMultipliers, size uiMultipliers, bool bBare)
{
	char*	szEnd;
	float64	fValue;
	size	i;

	if ((szValue[0] < '0') || (szValue[0] > '9'))
	{
		return NoVoltage();
	}
	fValue = strtod(szValue, &szEnd);
	for (i = 0; i < uiMultipliers; i++)
	{
		if (strncmp(szEnd, asMultipliers[i].szLetter, StrLen(asMultipliers[i].szLetter)) == 0)
		{
			return fValue * asMultipliers[i].fMultiplier;
		}
	}
	return bBare ? fValue : NoVoltage();
}


//////////////////////////////////////////////////////////////////////////
//
// Ohms: "4.7KΩ", "220", or a resistor network's part code, its value the
// three digits after its last dash (CAY16-221J4LF is 220, 4609X-101-103LF
// 10k, 4816P-1-222LF 2.2k).  NaN if it can't be read ("?Ω", "R_0805").
//
//////////////////////////////////////////////////////////////////////////
float64 GetKicadSchemResistance(const char* szValue)
{
	size	i;

	if (szValue == NULL)
	{
		return NoVoltage();
	}
	for (i = 0; i < sizeof(gaszKicadSchemResistorNetworks) / sizeof(const char*); i++)
	{
		if (strncmp(szValue, gaszKicadSchemResistorNetworks[i], StrLen(gaszKicadSchemResistorNetworks[i])) == 0)
		{
			return GetKicadSchemValueCode(&strrchr(szValue, '-')[1]);
		}
	}
	return GetKicadSchemValueNumber(szValue, gasKicadSchemResistanceMultipliers, sizeof(gasKicadSchemResistanceMultipliers) / sizeof(SKicadSchemValueMultiplier), true);
}


//////////////////////////////////////////////////////////////////////////
//
// Farads: "16pF", "0.1μF" (always with a multiplier), or a capacitor
// network's part code (its last three digits, in picofarads:
// CA0612KRX7R9BB100 is 10pF).  NaN if it can't be read ("?μF", "C_0805",
// "885012207033").
//
//////////////////////////////////////////////////////////////////////////
float64 GetKicadSchemCapacitance(const char* szValue)
{
	size	uiLength;

	if (szValue == NULL)
	{
		return NoVoltage();
	}
	if (strncmp(szValue, "CA0612", 6) == 0)
	{
		uiLength = StrLen(szValue);
		return (uiLength >= 9) ? GetKicadSchemValueCode(&szValue[uiLength - 3]) * 1e-12 : NoVoltage();
	}
	return GetKicadSchemValueNumber(szValue, gasKicadSchemCapacitanceMultipliers, sizeof(gasKicadSchemCapacitanceMultipliers) / sizeof(SKicadSchemValueMultiplier), false);
}


//////////////////////////////////////////////////////////////////////////
//
// The net a symbol's pin is on, NULL for none.
//
//////////////////////////////////////////////////////////////////////////
CKicadSchemNet* GetKicadSchemPinNet(CKicadSchemNetlist* pcNetlist, CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin)
{
	CKicadSchemSubnet*	pcSubnet;

	pcSubnet = pcNetlist->GetPinSubnet(pcSymbol, pcPin);
	return (pcSubnet != NULL) ? pcSubnet->GetNet() : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The elements of the sheet's resistors (bResistors) or capacitors.  A
// part with two pins is one element whatever they're named.
//
//////////////////////////////////////////////////////////////////////////
void GetKicadSchemPassiveElements(CKicadSchemFile* pcFile, CKicadSchemNetlist* pcNetlist, bool bResistors, CArrayKicadSchemPassiveElement* pasElements)
{
	size						i;
	size						j;
	size						k;
	CKicadSchemSymbol*			pcSymbol;
	CKicadSchemLibSymbol*		pcLibSymbol;
	CKicadSchemLibPin*			pcPin;
	CKicadSchemLibPin*			pcOther;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	SKicadSchemPassiveElement*	psElement;
	float64						fValue;

	apcPins.Init();
	for (i = 0; i < pcFile->NumSymbols(); i++)
	{
		pcSymbol = pcFile->GetSymbol(i);
		pcLibSymbol = pcFile->GetLibSymbol(pcSymbol);
		if ((pcLibSymbol == NULL) || !(bResistors ? IsKicadSchemResistor(pcSymbol) : IsKicadSchemCapacitor(pcSymbol)))
		{
			continue;
		}
		fValue = bResistors ? GetKicadSchemResistance(pcSymbol->GetValue()) : GetKicadSchemCapacitance(pcSymbol->GetValue());
		apcPins.ReInit();
		for (j = 0; j < pcLibSymbol->NumPins(); j++)
		{
			if (pcSymbol->IsUnitPin(pcLibSymbol->GetPin(j)))
			{
				apcPins.Add(pcLibSymbol->GetPin(j));
			}
		}
		for (j = 0; j < apcPins.NumElements(); j++)
		{
			pcPin = apcPins.GetPtr(j);
			for (k = j + 1; k < apcPins.NumElements(); k++)
			{
				pcOther = apcPins.GetPtr(k);
				if ((apcPins.NumElements() == 2) || (StringCompare(pcPin->GetName(), pcOther->GetName()) == 0))
				{
					psElement = pasElements->Add();
					psElement->pcSymbol = pcSymbol;
					psElement->apcPins[0] = pcPin;
					psElement->apcPins[1] = pcOther;
					psElement->apcNets[0] = GetKicadSchemPinNet(pcNetlist, pcSymbol, pcPin);
					psElement->apcNets[1] = GetKicadSchemPinNet(pcNetlist, pcSymbol, pcOther);
					psElement->fValue = fValue;
				}
			}
		}
	}
	apcPins.Kill();
}

