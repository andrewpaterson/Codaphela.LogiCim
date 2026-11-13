/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela LogicimLib

Codaphela LogicimLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela LogicimLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela LogicimLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include <math.h>
#include "BaseLib/Numbers.h"
#include "ComponentShapes.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentShapes::Init(void)
{
	masShapes.Init();
	masPoints.Init();
	maszTexts.Init();
	muiRevision = 0;
	InitComponentSpriteMemo(&msSpriteMemo);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentShapes::Kill(void)
{
	maszTexts.Kill();
	masPoints.Kill();
	masShapes.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentShapes::Clear(void)
{
	maszTexts.ReInit();
	masPoints.ReInit();
	masShapes.ReInit();
	muiRevision++;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::AddShape(EComponentShape eType, bool bFill, bool bBorder)
{
	SComponentShape*	psShape;

	muiRevision++;
	psShape = masShapes.Add();
	psShape->eType = eType;
	psShape->bFill = bFill;
	psShape->bBorder = bBorder;
	psShape->fLineWidth = SHAPE_LINE_WIDTH;
	psShape->eFill = SC_Shape;
	psShape->eBorder = SC_Shape;
	psShape->sFill = 0;
	psShape->sBorder = 0;
	psShape->uiFirstPoint = 0;
	psShape->uiNumPoints = 0;
	psShape->sCentre.Init(0, 0);
	psShape->fRadiusX = 0;
	psShape->fRadiusY = 0;
	psShape->fStartAngle = 0;
	psShape->fArcAngle = 0;
	psShape->uiText = 0;
	psShape->eFont = CF_SansSerif;
	psShape->fFontSize = 0;
	psShape->eAlignment = TA_Left;
	psShape->iRelativeRightRotations = 0;
	psShape->bCentred = false;
	psShape->bVisible = true;
	return psShape;
}


//////////////////////////////////////////////////////////////////////////
//
// Drawn in the border colour.
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::AddLine(float64 x0, float64 y0, float64 x1, float64 y1, float64 fWidth)
{
	SComponentShape*	psShape;

	psShape = AddShape(CS_Line, false, true);
	psShape->fLineWidth = fWidth;
	psShape->uiFirstPoint = masPoints.NumElements();
	psShape->uiNumPoints = 2;
	masPoints.Add()->Init(x0, y0);
	masPoints.Add()->Init(x1, y1);
	return psShape;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::AddRectangle(float64 fLeft, float64 fTop, float64 fRight, float64 fBottom, bool bFill, bool bBorder)
{
	SFloat64Vec2	asCorners[4];

	asCorners[0].Init(fLeft, fTop);
	asCorners[1].Init(fRight, fTop);
	asCorners[2].Init(fRight, fBottom);
	asCorners[3].Init(fLeft, fBottom);
	return AddPolygon(asCorners, 4, bFill, bBorder);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::AddPolygon(SFloat64Vec2* asPoints, size uiNumPoints, bool bFill, bool bBorder)
{
	SComponentShape*	psShape;

	psShape = AddShape(CS_Polygon, bFill, bBorder);
	psShape->uiFirstPoint = masPoints.NumElements();
	psShape->uiNumPoints = uiNumPoints;
	masPoints.InsertBlockAfterEnd(asPoints, uiNumPoints);
	return psShape;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::AddTriangle(float64 x0, float64 y0, float64 x1, float64 y1, float64 x2, float64 y2, bool bFill, bool bBorder)
{
	SFloat64Vec2	asCorners[3];

	asCorners[0].Init(x0, y0);
	asCorners[1].Init(x1, y1);
	asCorners[2].Init(x2, y2);
	return AddPolygon(asCorners, 3, bFill, bBorder);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::AddArc(float64 fCentreX, float64 fCentreY, float64 fRadiusX, float64 fRadiusY, float64 fStartAngle, float64 fArcAngle, bool bFill, bool bBorder)
{
	SComponentShape*	psShape;

	psShape = AddShape(CS_Arc, bFill, bBorder);
	psShape->sCentre.Init(fCentreX, fCentreY);
	psShape->fRadiusX = fRadiusX;
	psShape->fRadiusY = fRadiusY;
	psShape->fStartAngle = fStartAngle;
	psShape->fArcAngle = fArcAngle;
	return psShape;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::AddCircle(float64 fCentreX, float64 fCentreY, float64 fRadius, bool bFill, bool bBorder)
{
	SComponentShape*	psShape;

	psShape = AddShape(CS_Circle, bFill, bBorder);
	psShape->sCentre.Init(fCentreX, fCentreY);
	psShape->fRadiusX = fRadius;
	psShape->fRadiusY = fRadius;
	return psShape;
}


//////////////////////////////////////////////////////////////////////////
//
// A rectangle with its corners rounded to quarter circles, as a polygon.
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::AddRoundedRectangle(float64 fLeft, float64 fTop, float64 fRight, float64 fBottom, float64 fRadius, bool bFill, bool bBorder)
{
	SFloat64Vec2		asPoints[4 * (ROUNDED_CORNER_STEPS + 1)];
	SFloat64Vec2		asCentres[4];
	size				uiCorner;
	size				i;
	float64				fAngle;
	size				uiPoint;

	//Corners clockwise on the screen from the top right; angles counterclockwise with y down.
	asCentres[0].Init(fRight - fRadius, fTop + fRadius);
	asCentres[1].Init(fRight - fRadius, fBottom - fRadius);
	asCentres[2].Init(fLeft + fRadius, fBottom - fRadius);
	asCentres[3].Init(fLeft + fRadius, fTop + fRadius);
	uiPoint = 0;
	for (uiCorner = 0; uiCorner < 4; uiCorner++)
	{
		for (i = 0; i <= ROUNDED_CORNER_STEPS; i++)
		{
			fAngle = (90.0 - (float64)uiCorner * 90.0 - 90.0 * (float64)i / ROUNDED_CORNER_STEPS) * N_PI_64 / 180.0;
			asPoints[uiPoint].Init(asCentres[uiCorner].x + fRadius * cos(fAngle), asCentres[uiCorner].y - fRadius * sin(fAngle));
			uiPoint++;
		}
	}
	return AddPolygon(asPoints, uiPoint, bFill, bBorder);
}


//////////////////////////////////////////////////////////////////////////
//
// Text keeps its own copy of the string.  Remember the shape's index (not
// its pointer, which moves as shapes are added) to change it later.
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::AddText(float64 x, float64 y, const char* szText, ECircuitFont eFont, float64 fFontSize, ETextAlignment eAlignment, int iRelativeRightRotations)
{
	SComponentShape*	psShape;

	psShape = AddShape(CS_Text, false, true);
	psShape->eBorder = SC_Text;
	psShape->sCentre.Init(x, y);
	psShape->uiText = maszTexts.NumElements();
	maszTexts.Add(szText);
	psShape->eFont = eFont;
	psShape->fFontSize = fFontSize;
	psShape->eAlignment = eAlignment;
	psShape->iRelativeRightRotations = iRelativeRightRotations;
	return psShape;
}


//////////////////////////////////////////////////////////////////////////
//
// Text centred on x, y and upright whichever way the component is turned.
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::AddCentredText(float64 x, float64 y, const char* szText, ECircuitFont eFont, float64 fFontSize)
{
	SComponentShape*	psShape;

	psShape = AddText(x, y, szText, eFont, fFontSize, TA_Centre);
	psShape->bCentred = true;
	return psShape;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentShapes::SetVisible(size uiShape, bool bVisible)
{
	masShapes.Get(uiShape)->bVisible = bVisible;
	muiRevision++;
}


//////////////////////////////////////////////////////////////////////////
//
// sBorder is only used for SC_Explicit.
//
//////////////////////////////////////////////////////////////////////////
void CComponentShapes::SetBorder(size uiShape, EShapeColour eBorder, ARGB32 sBorder)
{
	SComponentShape*	psShape;

	psShape = masShapes.Get(uiShape);
	psShape->eBorder = eBorder;
	psShape->sBorder = sBorder;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentShapes::SetText(size uiShape, const char* szText)
{
	SComponentShape*	psShape;

	psShape = masShapes.Get(uiShape);
	maszTexts.Get(psShape->uiText)->Set(szText);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
char* CComponentShapes::GetText(size uiShape)
{
	SComponentShape*	psShape;

	psShape = masShapes.Get(uiShape);
	return maszTexts.GetText(psShape->uiText);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size CComponentShapes::NumShapes(void)
{
	return masShapes.NumElements();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SComponentShape* CComponentShapes::GetShape(size uiIndex)
{
	return masShapes.Get(uiIndex);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
SFloat64Vec2* CComponentShapes::GetPoint(size uiIndex)
{
	return masPoints.Get(uiIndex);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
ARGB32 CComponentShapes::GetFill(CCircuitPainter* pcPainter, SComponentShape* psShape)
{
	if (psShape->eFill == SC_Explicit)
	{
		return psShape->sFill;
	}
	else if (psShape->eFill == SC_DarkShape)
	{
		return pcPainter->GetColours()->msDarkShapeFill;
	}
	else if (psShape->eFill == SC_Background)
	{
		return pcPainter->GetColours()->msBackground;
	}
	return pcPainter->GetColours()->msShapeFill;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
ARGB32 CComponentShapes::GetBorder(CCircuitPainter* pcPainter, SComponentShape* psShape)
{
	if (psShape->eBorder == SC_Explicit)
	{
		return psShape->sBorder;
	}
	else if (psShape->eBorder == SC_Text)
	{
		return pcPainter->GetColours()->msText;
	}
	else if (psShape->eBorder == SC_CommentText)
	{
		return pcPainter->GetColours()->msCommentText;
	}
	else if (psShape->eBorder == SC_HiddenText)
	{
		return pcPainter->GetColours()->msHiddenText;
	}
	return pcPainter->GetColours()->msShapeBorder;
}


//////////////////////////////////////////////////////////////////////////
//
// The painter's transform places the shapes.
//
//////////////////////////////////////////////////////////////////////////
void CComponentShapes::Paint(CCircuitPainter* pcPainter)
{
	size				i;
	SComponentShape*	psShape;
	uint64				uiState;

	if (pcPainter->GetSpriteCache() == NULL)
	{
		for (i = 0; i < masShapes.NumElements(); i++)
		{
			Paint(pcPainter, masShapes.Get(i));
		}
		return;
	}

	//Every shape but text is blitted from the component's sprite.  Text can change every tick (a value, say) so it is
	//drawn each paint, over the shapes as it always is.  The memo finds the sprite without painting the shapes.
	uiState = GetSpriteState(pcPainter);
	if (!pcPainter->DrawMemo(&msSpriteMemo, uiState, 0, 0))
	{
		pcPainter->BeginSprite();
		for (i = 0; i < masShapes.NumElements(); i++)
		{
			psShape = masShapes.Get(i);
			if (psShape->eType != CS_Text)
			{
				Paint(pcPainter, psShape);
			}
		}
		pcPainter->EndSprite(&msSpriteMemo, uiState);
	}

	for (i = 0; i < masShapes.NumElements(); i++)
	{
		psShape = masShapes.Get(i);
		if (psShape->eType == CS_Text)
		{
			Paint(pcPainter, psShape);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Everything the sprite of the shapes but text depends on: where and how
// the painter draws them, their geometry (the revision) and each one's
// colours, width and visibility.  Text isn't in it, so changing text
// draws the same sprite.
//
//////////////////////////////////////////////////////////////////////////
uint64 CComponentShapes::GetSpriteState(CCircuitPainter* pcPainter)
{
	uint64				uiState;
	size				i;
	SComponentShape*	psShape;
	ARGB32				sFill;
	ARGB32				sBorder;

	uiState = pcPainter->GetSpriteState(0, 0);
	uiState = FNVHash64(&muiRevision, sizeof(uint32), uiState);
	for (i = 0; i < masShapes.NumElements(); i++)
	{
		psShape = masShapes.Get(i);
		if (psShape->eType == CS_Text)
		{
			continue;
		}
		uiState = FNVHash64(&psShape->bVisible, sizeof(bool), uiState);
		if (!psShape->bVisible)
		{
			continue;
		}
		sFill = GetFill(pcPainter, psShape);
		sBorder = GetBorder(pcPainter, psShape);
		uiState = FNVHash64(&psShape->bFill, sizeof(bool), uiState);
		uiState = FNVHash64(&psShape->bBorder, sizeof(bool), uiState);
		uiState = FNVHash64(&psShape->fLineWidth, sizeof(float64), uiState);
		uiState = FNVHash64(&sFill, sizeof(ARGB32), uiState);
		uiState = FNVHash64(&sBorder, sizeof(ARGB32), uiState);
	}
	return uiState;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CComponentShapes::Paint(CCircuitPainter* pcPainter, SComponentShape* psShape)
{
	SFloat64Vec2*	asPoints;

	if (!psShape->bVisible)
	{
		return;
	}

	switch (psShape->eType)
	{
	case CS_Line:
		asPoints = masPoints.Get(psShape->uiFirstPoint);
		pcPainter->Line(asPoints[0].x, asPoints[0].y, asPoints[1].x, asPoints[1].y, psShape->fLineWidth, GetBorder(pcPainter, psShape));
		break;
	case CS_Polygon:
		pcPainter->Polygon(masPoints.Get(psShape->uiFirstPoint), psShape->uiNumPoints, psShape->bFill, psShape->bBorder, psShape->fLineWidth, GetFill(pcPainter, psShape), GetBorder(pcPainter, psShape));
		break;
	case CS_Arc:
		pcPainter->Arc(psShape->sCentre.x, psShape->sCentre.y, psShape->fRadiusX, psShape->fRadiusY, psShape->fStartAngle, psShape->fArcAngle,
					   psShape->bFill, psShape->bBorder, psShape->fLineWidth, GetFill(pcPainter, psShape), GetBorder(pcPainter, psShape));
		break;
	case CS_Circle:
		pcPainter->Circle(psShape->sCentre.x, psShape->sCentre.y, psShape->fRadiusX, psShape->bFill, psShape->bBorder, psShape->fLineWidth, GetFill(pcPainter, psShape), GetBorder(pcPainter, psShape));
		break;
	case CS_Text:
		if (psShape->bCentred)
		{
			pcPainter->CentredText(psShape->sCentre.x, psShape->sCentre.y, maszTexts.GetText(psShape->uiText), psShape->eFont, psShape->fFontSize, GetBorder(pcPainter, psShape));
			break;
		}
		pcPainter->Text(psShape->sCentre.x, psShape->sCentre.y, maszTexts.GetText(psShape->uiText), psShape->eFont, psShape->fFontSize, psShape->eAlignment, psShape->iRelativeRightRotations, GetBorder(pcPainter, psShape));
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static void IncludeArcPoint(SComponentShape* psShape, float64 fDegrees, SFloat64Bounds* psBounds)
{
	float64		fAngle;
	float64		x;
	float64		y;

	fAngle = fDegrees * N_PI_64 / 180.0;
	x = psShape->sCentre.x + psShape->fRadiusX * cos(fAngle);
	y = psShape->sCentre.y - psShape->fRadiusY * sin(fAngle);
	psBounds->Include(x, y);
}


//////////////////////////////////////////////////////////////////////////
//
// Grows the box to hold every shape, in the shapes' unturned grid space.
// Line widths aren't included, nor is text (so a component's size
// doesn't change with the font).  Arcs include only the part of the
// ellipse they sweep (and the centre when filled).
//
//////////////////////////////////////////////////////////////////////////
void CComponentShapes::IncludeBounds(SFloat64Bounds* psBounds)
{
	size				i;
	size				j;
	SComponentShape*	psShape;
	SFloat64Vec2*		psPoint;
	float64				fAngle;
	float64				fStart;
	float64				fEnd;

	for (i = 0; i < masShapes.NumElements(); i++)
	{
		psShape = masShapes.Get(i);
		if (psShape->eType == CS_Text)
		{
			continue;
		}
		if ((psShape->eType == CS_Line) || (psShape->eType == CS_Polygon))
		{
			for (j = 0; j < psShape->uiNumPoints; j++)
			{
				psPoint = masPoints.Get(psShape->uiFirstPoint + j);
				psBounds->Include(psPoint->x, psPoint->y);
			}
		}
		else
		{
			//The swept ends and every quarter turn between them are the arc's extremes.
			if (psShape->eType == CS_Circle)
			{
				fStart = 0;
				fEnd = 360;
			}
			else if (psShape->fArcAngle >= 0)
			{
				fStart = psShape->fStartAngle;
				fEnd = psShape->fStartAngle + psShape->fArcAngle;
			}
			else
			{
				fStart = psShape->fStartAngle + psShape->fArcAngle;
				fEnd = psShape->fStartAngle;
			}
			IncludeArcPoint(psShape, fStart, psBounds);
			IncludeArcPoint(psShape, fEnd, psBounds);
			for (fAngle = ceil(fStart / 90.0) * 90.0; fAngle < fEnd; fAngle += 90.0)
			{
				IncludeArcPoint(psShape, fAngle, psBounds);
			}
			if ((psShape->eType == CS_Arc) && psShape->bFill)
			{
				psBounds->Include(psShape->sCentre.x, psShape->sCentre.y);
			}
		}
	}
}

