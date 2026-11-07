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
#ifndef __BOUNDS_H__
#define __BOUNDS_H__
#include "BaseLib/Float64Vec2.h"
#include "BaseLib/Int32Vec2.h"


//A box grown to hold points, as the box around everything a view draws.  It holds nothing (and has no corners) until
//it's given its first point.
template<class V, class T>
struct SBoundsTemplate
{
	V		sMinimum;
	V		sMaximum;
	bool	bHasBounds;

	void	Init(void);
	void	Init(V sMinimum, V sMaximum);
	void	Include(T x, T y);
	void	Include(V sPoint);
	void	Include(SBoundsTemplate<V, T>* psBox);
	void	Move(V sBy);
	V		GetCentre(void);
	bool	Contains(V sPoint);
	bool	Contains(SBoundsTemplate<V, T>* psBox);
	bool	Overlaps(SBoundsTemplate<V, T>* psBox);
};


typedef SBoundsTemplate<SFloat64Vec2, float64>	SFloat64Bounds;
typedef SBoundsTemplate<SInt32Vec2, int32>		SInt32Bounds;


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
template<class V, class T>
void SBoundsTemplate<V, T>::Init(void)
{
	sMinimum.Init(0, 0);
	sMaximum.Init(0, 0);
	bHasBounds = false;
}


//////////////////////////////////////////////////////////////////////////
//
// The first point is the whole box.
//
//////////////////////////////////////////////////////////////////////////
template<class V, class T>
void SBoundsTemplate<V, T>::Include(T x, T y)
{
	if (!bHasBounds)
	{
		sMinimum.Init(x, y);
		sMaximum.Init(x, y);
		bHasBounds = true;
		return;
	}
	if (x < sMinimum.x)
	{
		sMinimum.x = x;
	}
	if (y < sMinimum.y)
	{
		sMinimum.y = y;
	}
	if (x > sMaximum.x)
	{
		sMaximum.x = x;
	}
	if (y > sMaximum.y)
	{
		sMaximum.y = y;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The box from sMinimum to sMaximum.
//
//////////////////////////////////////////////////////////////////////////
template<class V, class T>
void SBoundsTemplate<V, T>::Init(V sMinimum, V sMaximum)
{
	this->sMinimum = sMinimum;
	this->sMaximum = sMaximum;
	bHasBounds = true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
template<class V, class T>
void SBoundsTemplate<V, T>::Include(V sPoint)
{
	Include(sPoint.x, sPoint.y);
}


//////////////////////////////////////////////////////////////////////////
//
// Grows to hold the box too, if it holds anything.
//
//////////////////////////////////////////////////////////////////////////
template<class V, class T>
void SBoundsTemplate<V, T>::Include(SBoundsTemplate<V, T>* psBox)
{
	if (psBox->bHasBounds)
	{
		Include(psBox->sMinimum.x, psBox->sMinimum.y);
		Include(psBox->sMaximum.x, psBox->sMaximum.y);
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
template<class V, class T>
void SBoundsTemplate<V, T>::Move(V sBy)
{
	sMinimum += sBy;
	sMaximum += sBy;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
template<class V, class T>
V SBoundsTemplate<V, T>::GetCentre(void)
{
	V	sCentre;

	sCentre = (sMinimum + sMaximum) / (T)2;
	return sCentre;
}


//////////////////////////////////////////////////////////////////////////
//
// Edges count as in, as Logicim's BoundingBox.containsPoint.
//
//////////////////////////////////////////////////////////////////////////
template<class V, class T>
bool SBoundsTemplate<V, T>::Contains(V sPoint)
{
	return (sPoint.x >= sMinimum.x) && (sPoint.x <= sMaximum.x) && (sPoint.y >= sMinimum.y) && (sPoint.y <= sMaximum.y);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the box is wholly inside, edges included, as Logicim's
// BoundingBox.containsBox.
//
//////////////////////////////////////////////////////////////////////////
template<class V, class T>
bool SBoundsTemplate<V, T>::Contains(SBoundsTemplate<V, T>* psBox)
{
	return (sMinimum.x <= psBox->sMinimum.x) && (sMinimum.y <= psBox->sMinimum.y) && (sMaximum.x >= psBox->sMaximum.x) && (sMaximum.y >= psBox->sMaximum.y);
}


//////////////////////////////////////////////////////////////////////////
//
// Whether the boxes touch or overlap.
//
//////////////////////////////////////////////////////////////////////////
template<class V, class T>
bool SBoundsTemplate<V, T>::Overlaps(SBoundsTemplate<V, T>* psBox)
{
	return (sMinimum.x <= psBox->sMaximum.x) && (sMinimum.y <= psBox->sMaximum.y) && (sMaximum.x >= psBox->sMinimum.x) && (sMaximum.y >= psBox->sMinimum.y);
}


#endif // __BOUNDS_H__

