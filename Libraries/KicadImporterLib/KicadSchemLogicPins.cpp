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
#include "BaseLib/StringHelper.h"
#include "KicadSchemLogicPins.h"


//Which Logicim flip flop port a KiCad flip flop pin is, by its letters.
struct SKicadSchemFlipFlopPin
{
	const char*		szLetters;
	bool			bNegated;
	const char*		szPortName;
};


//The flip flop pins, by their letters and whether they're drawn negated.
static SKicadSchemFlipFlopPin gasKicadSchemFlipFlopPins[] =
{
	{ "CP", false, "CP" }, { "CLK", false, "CP" }, { "C", false, "CP" },
	{ "SD", true, "SD" }, { "S", true, "SD" }, { "PRE", true, "SD" }, { "SET", true, "SD" },
	{ "D", false, "D" },
	{ "RD", true, "RD" }, { "R", true, "RD" }, { "CLR", true, "RD" }, { "RESET", true, "RD" },
	{ "Q", false, "Q" },
	{ "Q", true, "QB" },
};


//////////////////////////////////////////////////////////////////////////
//
// The parts of a pin's name: the gate number it starts with and the
// letters after it, and whether it's negated (~{...}).
//
//////////////////////////////////////////////////////////////////////////
void SplitKicadSchemLogicPinName(const char* szName, CChars* pszIndex, CChars* pszLetters, bool* pbNegated)
{
	CChars	szName2;
	size	i;

	szName2.Init(szName);
	*pbNegated = false;
	if (szName2.StartsWith("~{"))
	{
		*pbNegated = true;
		szName2.RemoveFromStart(2);
		if (szName2.EndsWith("}"))
		{
			szName2.RemoveFromEnd(1);
		}
	}
	else if (szName2.StartsWith("~") && (szName2.Length() > 1))
	{
		*pbNegated = true;
		szName2.RemoveFromStart(1);
	}

	for (i = 0; i < szName2.Length(); i++)
	{
		if ((szName2.GetChar(i) < '0') || (szName2.GetChar(i) > '9'))
		{
			break;
		}
	}
	pszIndex->Init(szName2.Text(), 0, i);
	pszLetters->Init(szName2.Text(), i);
	pszLetters->UpperCase();
	szName2.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The pin types a gate's output can have.
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemOutputPinType(EKicadSchemPinType eType)
{
	return (eType == KSPT_Output) || (eType == KSPT_TriState) || (eType == KSPT_OpenCollector) || (eType == KSPT_OpenEmitter);
}


//////////////////////////////////////////////////////////////////////////
//
// The Logicim flip flop port for the pin, or NULL.
//
//////////////////////////////////////////////////////////////////////////
const char* GetKicadSchemFlipFlopPortName(CChars* pszLetters, bool bNegated)
{
	size	i;

	for (i = 0; i < sizeof(gasKicadSchemFlipFlopPins) / sizeof(SKicadSchemFlipFlopPin); i++)
	{
		if (pszLetters->Equals(gasKicadSchemFlipFlopPins[i].szLetters))
		{
			//Set and reset are active low however their pins are drawn.
			if ((gasKicadSchemFlipFlopPins[i].szPortName[0] == 'Q') && (gasKicadSchemFlipFlopPins[i].bNegated != bNegated))
			{
				continue;
			}
			return gasKicadSchemFlipFlopPins[i].szPortName;
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemLogicPins::Init(void)
{
	aszIndices.Init();
	asPins.Init();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemLogicPins::Kill(void)
{
	size	i;

	for (i = 0; i < asPins.NumElements(); i++)
	{
		asPins.Get(i)->szLetters.Kill();
	}
	asPins.Kill();
	aszIndices.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The pin, in the gate number szIndex ("" for none), added if the part
// hasn't had it yet.
//
//////////////////////////////////////////////////////////////////////////
void SKicadSchemLogicPins::AddPin(CKicadSchemLibPin* pcPin, const char* szIndex, const char* szLetters, bool bNegated, SFloat64Vec2* psPosition)
{
	SKicadSchemLogicPin*	psPin;
	size					uiGroup;

	uiGroup = aszIndices.GetIndex(szIndex);
	if (uiGroup == ARRAY_ELEMENT_NOT_FOUND)
	{
		aszIndices.Add(szIndex);
		uiGroup = aszIndices.NumElements() - 1;
	}
	psPin = asPins.Add();
	psPin->pcPin = pcPin;
	psPin->uiGroup = uiGroup;
	psPin->szLetters.Init(szLetters);
	psPin->bNegated = bNegated;
	psPin->sPosition = *psPosition;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size SKicadSchemLogicPins::NumPins(void)
{
	return asPins.NumElements();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SKicadSchemLogicPin* SKicadSchemLogicPins::GetPin(size uiIndex)
{
	return asPins.Get(uiIndex);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the pin is a gate's output (Y) or one of a flip flop's (Q or
// QB).
//
//////////////////////////////////////////////////////////////////////////
bool IsKicadSchemLogicOutput(SKicadSchemLogicPin* psPin, EKicadSchemLogicFunction eFunction)
{
	if (eFunction == KSLF_DFlipFlop)
	{
		return GetKicadSchemFlipFlopPortName(&psPin->szLetters, psPin->bNegated)[0] == 'Q';
	}
	return psPin->szLetters.Equals("Y");
}


//////////////////////////////////////////////////////////////////////////
//
// Gives a gate's unnamed pins (KiCad's own 74xx symbols name their gates'
// pins "~") letters: the inputs A up in pin number order, and the output
// Y.  Nothing changes unless every pin is unnamed.
//
//////////////////////////////////////////////////////////////////////////
void NameUnnamedKicadSchemLogicPins(SKicadSchemLogicPins* psPins)
{
	size					i;
	size					j;
	size					k;
	SKicadSchemLogicPin*	psPin;
	CKicadSchemLibPin*		pcPin;
	CKicadSchemLibPin*		pcOther;

	if (psPins->NumPins() == 0)
	{
		return;
	}
	for (i = 0; i < psPins->NumPins(); i++)
	{
		psPin = psPins->GetPin(i);
		if (!psPin->szLetters.Empty() && !psPin->szLetters.Equals("~"))
		{
			return;
		}
	}

	for (i = 0; i < psPins->NumPins(); i++)
	{
		psPin = psPins->GetPin(i);
		pcPin = psPin->pcPin;
		if (IsKicadSchemOutputPinType(pcPin->GetType()))
		{
			psPin->szLetters.Set("Y");
			continue;
		}
		k = 0;
		for (j = 0; j < psPins->NumPins(); j++)
		{
			pcOther = psPins->GetPin(j)->pcPin;
			if ((j != i) && !IsKicadSchemOutputPinType(pcOther->GetType()) &&
				((atoi(pcOther->GetNumber()) < atoi(pcPin->GetNumber())) || ((atoi(pcOther->GetNumber()) == atoi(pcPin->GetNumber())) && (j < i))))
			{
				k++;
			}
		}
		psPin->szLetters.Kill();
		psPin->szLetters.Init();
		psPin->szLetters.Append((char)('A' + k));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A KiCad pin's name as the label of the port it's on (see
// CLogicBlockView): its overbars ("~{OE}", "1~{OE}") taken out, and
// pbNegated whether it had any; a sigma ("Σ1", a sum) is an S.
//
//////////////////////////////////////////////////////////////////////////
void GetKicadSchemPinLabel(const char* szName, CChars* pszLabel, bool* pbNegated)
{
	size	i;

	*pbNegated = false;
	for (i = 0; (szName != NULL) && (szName[i] != '\0'); i++)
	{
		if ((szName[i] == '~') && (szName[i + 1] == '{'))
		{
			*pbNegated = true;
			i++;
		}
		else if (szName[i] == '}')
		{
		}
		else if (((unsigned char)szName[i] == 0xCE) && ((unsigned char)szName[i + 1] == 0xA3))
		{
			pszLabel->Append('S');
			i++;
		}
		else
		{
			pszLabel->Append(szName[i]);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The other names a pin's label goes by: a 74F283's carries ("C0", "C4")
// are a 74LVC283's "CIN" and "COUT", and a bus switch's channel letter
// after the name ("I0A", "YA", "EA") is the channel's number before it
// ("1I0", "1Y", "1E").  False if there's no other name.
//
//////////////////////////////////////////////////////////////////////////
bool GetKicadSchemPinLabelAlias(const char* szLabel, CChars* pszAlias)
{
	size	uiLength;
	char	cChannel;
	CChars	szName;

	if (StringCompare(szLabel, "C0") == 0)
	{
		pszAlias->Set("CIN");
		return true;
	}
	if (StringCompare(szLabel, "C4") == 0)
	{
		pszAlias->Set("COUT");
		return true;
	}
	uiLength = StrLen(szLabel);
	if ((uiLength >= 2) && ((szLabel[0] == 'I') || (szLabel[0] == 'Y') || (szLabel[0] == 'E')))
	{
		cChannel = szLabel[uiLength - 1];
		if ((cChannel >= 'A') && (cChannel <= 'D'))
		{
			szName.Init(szLabel, 0, uiLength - 1);
			pszAlias->Kill();
			pszAlias->Init();
			pszAlias->Append((int32)(cChannel - 'A' + 1));
			pszAlias->Append(szName.Text());
			szName.Kill();
			return true;
		}
	}
	return false;
}

