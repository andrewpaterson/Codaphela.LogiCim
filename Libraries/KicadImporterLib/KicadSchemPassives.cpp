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
#include <stdlib.h>
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
//4609X and 4816P), and a bussed one's common pin's number (NULL for an isolated network).
struct SKicadSchemResistorNetwork
{
	const char*	szPart;
	const char*	szCommonPin;
};
static SKicadSchemResistorNetwork gasKicadSchemResistorNetworks[] = { { "CAY16-", NULL }, { "4609X-", "1" }, { "4816P-1-", "16" }, { "4816P-", NULL } };


//////////////////////////////////////////////////////////////////////////
//
// The network the value is, NULL if it isn't one.
//
//////////////////////////////////////////////////////////////////////////
static SKicadSchemResistorNetwork* GetKicadSchemResistorNetwork(const char* szValue)
{
	size	i;

	for (i = 0; (szValue != NULL) && (i < sizeof(gasKicadSchemResistorNetworks) / sizeof(SKicadSchemResistorNetwork)); i++)
	{
		if (strncmp(szValue, gasKicadSchemResistorNetworks[i].szPart, StrLen(gasKicadSchemResistorNetworks[i].szPart)) == 0)
		{
			return &gasKicadSchemResistorNetworks[i];
		}
	}
	return NULL;
}


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
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemDiode(CKicadSchemSymbol* pcSymbol)
{
	const char*	szReference;

	szReference = pcSymbol->GetReference();
	return (szReference != NULL) && (ToUpper(szReference[0]) == 'D') && (szReference[1] >= '0') && (szReference[1] <= '9');
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
	if (szValue == NULL)
	{
		return NoVoltage();
	}
	if (GetKicadSchemResistorNetwork(szValue) != NULL)
	{
		return GetKicadSchemValueCode(&strrchr(szValue, '-')[1]);
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
// An element of the symbol from pcPin to pcOther.
//
//////////////////////////////////////////////////////////////////////////
static void AddKicadSchemPassiveElement(CKicadSchemNetlist* pcNetlist, CKicadSchemSymbol* pcSymbol, CKicadSchemLibPin* pcPin, CKicadSchemLibPin* pcOther, float64 fValue, bool bBussed, CArrayKicadSchemPassiveElement* pasElements)
{
	SKicadSchemPassiveElement*	psElement;

	psElement = pasElements->Add();
	psElement->pcSymbol = pcSymbol;
	psElement->apcPins[0] = pcPin;
	psElement->apcPins[1] = pcOther;
	psElement->apcNets[0] = GetKicadSchemPinNet(pcNetlist, pcSymbol, pcPin);
	psElement->apcNets[1] = GetKicadSchemPinNet(pcNetlist, pcSymbol, pcOther);
	psElement->fValue = fValue;
	psElement->bBussed = bBussed;
}


//////////////////////////////////////////////////////////////////////////
//
// An isolated network whose pins are only numbered (a CA0612 capacitor
// array): pin n is across from pin N + 1 - n, round the package.
//
//////////////////////////////////////////////////////////////////////////
static void AddKicadSchemNumberedElements(CKicadSchemNetlist* pcNetlist, CKicadSchemSymbol* pcSymbol, CArrayTemplatePtr<CKicadSchemLibPin>* papcPins, float64 fValue, CArrayKicadSchemPassiveElement* pasElements)
{
	size				i;
	size				j;
	int					iNumber;
	int					iOther;
	int					iCount;
	CKicadSchemLibPin*	pcPin;
	CKicadSchemLibPin*	pcOther;

	iCount = (int)papcPins->NumElements();
	if ((iCount < 4) || ((iCount % 2) != 0))
	{
		return;
	}
	for (i = 0; i < papcPins->NumElements(); i++)
	{
		pcPin = papcPins->GetPtr(i);
		iNumber = atoi(pcPin->GetNumber());
		if ((iNumber < 1) || (iNumber > iCount / 2))
		{
			continue;
		}
		iOther = iCount + 1 - iNumber;
		for (j = 0; j < papcPins->NumElements(); j++)
		{
			pcOther = papcPins->GetPtr(j);
			if (atoi(pcOther->GetNumber()) == iOther)
			{
				AddKicadSchemPassiveElement(pcNetlist, pcSymbol, pcPin, pcOther, fValue, false, pasElements);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The elements of the sheet's resistors (bResistors) or capacitors.  A
// part with two pins is one element whatever they're named; a bussed
// network's common pin is found by its number; a network whose pins
// don't pair by name is numbered round its package (see
// AddKicadSchemNumberedElements).
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
	CKicadSchemLibPin*			pcCommon;
	CArrayTemplatePtr<CKicadSchemLibPin>	apcPins;
	SKicadSchemResistorNetwork*	psNetwork;
	float64						fValue;
	size						uiFirst;

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
		uiFirst = pasElements->NumElements();
		psNetwork = bResistors ? GetKicadSchemResistorNetwork(pcSymbol->GetValue()) : NULL;
		pcCommon = NULL;
		for (j = 0; (psNetwork != NULL) && (psNetwork->szCommonPin != NULL) && (j < apcPins.NumElements()); j++)
		{
			if (StringCompare(apcPins.GetPtr(j)->GetNumber(), psNetwork->szCommonPin) == 0)
			{
				pcCommon = apcPins.GetPtr(j);
			}
		}
		for (j = 0; j < apcPins.NumElements(); j++)
		{
			pcPin = apcPins.GetPtr(j);
			if (pcCommon != NULL)
			{
				if (pcPin != pcCommon)
				{
					AddKicadSchemPassiveElement(pcNetlist, pcSymbol, pcPin, pcCommon, fValue, true, pasElements);
				}
				continue;
			}
			for (k = j + 1; k < apcPins.NumElements(); k++)
			{
				pcOther = apcPins.GetPtr(k);
				if ((apcPins.NumElements() == 2) || (StringCompare(pcPin->GetName(), pcOther->GetName()) == 0))
				{
					AddKicadSchemPassiveElement(pcNetlist, pcSymbol, pcPin, pcOther, fValue, false, pasElements);
				}
			}
		}
		if ((pcCommon == NULL) && (pasElements->NumElements() == uiFirst))
		{
			AddKicadSchemNumberedElements(pcNetlist, pcSymbol, &apcPins, fValue, pasElements);
		}
	}
	apcPins.Kill();
}



//////////////////////////////////////////////////////////////////////////
//
// The sheet's diodes: each anode (a pin named A, or A and a number) with
// the cathode of the same name but K.  Its value is NaN.
//
//////////////////////////////////////////////////////////////////////////
void GetKicadSchemDiodeElements(CKicadSchemFile* pcFile, CKicadSchemNetlist* pcNetlist, CArrayKicadSchemPassiveElement* pasElements)
{
	size						i;
	size						j;
	size						k;
	CKicadSchemSymbol*			pcSymbol;
	CKicadSchemLibSymbol*		pcLibSymbol;
	CKicadSchemLibPin*			pcAnode;
	CKicadSchemLibPin*			pcCathode;

	for (i = 0; i < pcFile->NumSymbols(); i++)
	{
		pcSymbol = pcFile->GetSymbol(i);
		pcLibSymbol = pcFile->GetLibSymbol(pcSymbol);
		if ((pcLibSymbol == NULL) || !IsKicadSchemDiode(pcSymbol))
		{
			continue;
		}
		for (j = 0; j < pcLibSymbol->NumPins(); j++)
		{
			pcAnode = pcLibSymbol->GetPin(j);
			if (!pcSymbol->IsUnitPin(pcAnode) || (ToUpper(pcAnode->GetName()[0]) != 'A'))
			{
				continue;
			}
			for (k = 0; k < pcLibSymbol->NumPins(); k++)
			{
				pcCathode = pcLibSymbol->GetPin(k);
				if (pcSymbol->IsUnitPin(pcCathode) && (ToUpper(pcCathode->GetName()[0]) == 'K') && (StringCompare(&pcAnode->GetName()[1], &pcCathode->GetName()[1]) == 0))
				{
					AddKicadSchemPassiveElement(pcNetlist, pcSymbol, pcAnode, pcCathode, NoVoltage(), false, pasElements);
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The element's name: its symbol's reference, and for one of a network's
// elements the number of its first pin too ("RN2.3"), so each is named
// apart.
//
//////////////////////////////////////////////////////////////////////////
void GetKicadSchemPassiveElementName(CChars* psz, SKicadSchemPassiveElement* psElement)
{
	psz->Append(psElement->pcSymbol->GetReference());
	if (GetKicadSchemResistorNetwork(psElement->pcSymbol->GetValue()) != NULL)
	{
		psz->Append('.');
		psz->Append(psElement->apcPins[0]->GetNumber());
	}
}
