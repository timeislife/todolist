// GanttStruct.cpp: implementation of the CGanttStruct class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GanttStruct.h"

#include "..\shared\DateHelper.h"
#include "..\shared\graphicsMisc.h"
#include "..\shared\misc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////

int GANTTDEPENDENCY::STUB = 0;

GANTTDEPENDENCY::GANTTDEPENDENCY() : ptFrom(0), ptTo(0), dwFromID(0), dwToID(0)
{
}

void GANTTDEPENDENCY::SetFrom(const CPoint& pt, DWORD dwTaskID)
{
	dwFromID = dwTaskID;
	ptFrom = pt;
}

void GANTTDEPENDENCY::SetTo(const CPoint& pt, DWORD dwTaskID)
{
	dwToID = dwTaskID;
	ptTo = pt;
}

DWORD GANTTDEPENDENCY::GetFromID() const
{
	ASSERT(dwFromID);
	return dwFromID;
}

DWORD GANTTDEPENDENCY::GetToID() const
{
	ASSERT(dwToID);
	return dwToID;
}

BOOL GANTTDEPENDENCY::Matches(DWORD dwFrom, DWORD dwTo) const
{
	ASSERT(dwFromID && dwToID);
	return ((dwFromID == dwFrom) && (dwToID == dwTo));
}

BOOL GANTTDEPENDENCY::Draw(CDC* pDC, const CRect& rClient, BOOL bDragging)
{
	if (!HitTest(rClient))
		return FALSE;
	
	// draw 3x3 box at ptFrom
	if (!bDragging)
	{
		CRect rBox(ptFrom.x - 2, ptFrom.y - 1, ptFrom.x + 1, ptFrom.y + 2);
		pDC->FillSolidRect(rBox, 0);
	}
		
	// max 5 points needed for line
	CPoint pts[5] = { 0 };
	int nPts = CalcDependencyPath(pts, bDragging);
	
	int nOldROP2 = pDC->SetROP2(bDragging ? R2_NOT : R2_BLACK);
	pDC->Polyline(pts, nPts);
	
	// draw arrow at ptTo
	CalcDependencyArrow(pts[nPts-1], pts);

	pDC->Polyline(pts, 3);
	pDC->SetROP2(nOldROP2);
	
	return TRUE;
}

BOOL GANTTDEPENDENCY::HitTest(const CRect& rect) const
{
	CRect rThis;
	return (CalcBoundingRect(rThis) && CRect().IntersectRect(rect, rThis));
}

BOOL GANTTDEPENDENCY::HitTest(const CPoint& point, int nTol) const
{
	CRect rThis;
	
	if (!CalcBoundingRect(rThis))
		return FALSE;

	// add tolerance
	rThis.InflateRect(nTol, nTol);

	if (!rThis.PtInRect(point))
		return FALSE;

	// check each line segment
	CPoint pts[5];
	int nPts = CalcDependencyPath(pts, FALSE);
	ASSERT (nPts > 1);

	nTol = max(nTol, 1);
	
	for (int i = 0; i < (nPts - 1); i++)
	{
		CRect rSeg;

		rSeg.left	= min(pts[i].x, pts[i+1].x) - nTol;
		rSeg.right	= max(pts[i].x, pts[i+1].x) + nTol;
		rSeg.top	= min(pts[i].y, pts[i+1].y) - nTol;
		rSeg.bottom = max(pts[i].y, pts[i+1].y) + nTol;

		if (rSeg.PtInRect(point))
			return TRUE;
	}

	// no hit
	return FALSE;
}

int GANTTDEPENDENCY::CalcDependencyPath(CPoint pts[5], BOOL bDragging) const
{
	pts[0] = ptFrom;
	int nPts = 1;
	
	CPoint ptTemp(ptFrom);
	BOOL bFromIsAbove = (ptFrom.y < ptTo.y);
	
	GCDEPEND_DRAWTYPE nDraw = GetDependencyDrawType();
	
	switch (nDraw)
	{
	case GCDDT_FROMISABOVELEFT:
	case GCDDT_FROMISBELOWLEFT:
		{
			ptTemp.x = ptTo.x;
			pts[nPts++] = ptTemp;
		}
		break;
		
	case GCDDT_FROMISABOVERIGHT:
	case GCDDT_FROMISBELOWRIGHT:
		{
			ptTemp.x += STUB;
			pts[nPts++] = ptTemp;
			
			ptTemp.y = (ptFrom.y + (bFromIsAbove ? STUB : -STUB));
			pts[nPts++] = ptTemp;
			
			ptTemp.x = ptTo.x;
			pts[nPts++] = ptTemp;
		}
		break;
		
	default:
		ASSERT(0);
		return 0;
	}
	
	// last point
	if (bDragging)
	{
		ptTemp = ptTo;
	}
	else
	{
		ptTemp.y = (ptTo.y + (bFromIsAbove ? (2 - STUB) : (-2 + STUB)) - 1);
	}
	
	pts[nPts++] = ptTemp;
	
	return nPts;
}

void GANTTDEPENDENCY::CalcDependencyArrow(CPoint pt, CPoint pts[3]) const
{
	pts[0] = pts[1] = pts[2] = pt;

	const int ARROW = (STUB / 2);
	
	if (ptFrom.y < ptTo.y) // from is above
	{
		pts[0].Offset(-ARROW, -ARROW);
		pts[2].Offset(ARROW+1, -(ARROW+1));
	}
	else
	{
		pts[0].Offset(-ARROW, ARROW);
		pts[2].Offset(ARROW+1, ARROW+1);
	}
}

GCDEPEND_DRAWTYPE GANTTDEPENDENCY::GetDependencyDrawType() const
{
	BOOL bBelow = (ptFrom.y >= ptTo.y);
	BOOL bLeft = (ptTo.x >= (ptFrom.x + STUB));
	
	if (bBelow)
		return (bLeft ? GCDDT_FROMISBELOWLEFT : GCDDT_FROMISBELOWRIGHT);
	
	// else
	return (bLeft ? GCDDT_FROMISABOVELEFT : GCDDT_FROMISABOVERIGHT);
}

BOOL GANTTDEPENDENCY::CalcBoundingRect(CRect& rect) const
{
	if (ptFrom == ptTo)
		return FALSE;
	
	// allow for stub overhang
	rect.left	= min(ptFrom.x, ptTo.x) - STUB;
	rect.right	= max(ptFrom.x, ptTo.x) + STUB;
	rect.top	= min(ptFrom.y, ptTo.y);
	rect.bottom = max(ptFrom.y, ptTo.y);
	
	return TRUE;
}

//////////////////////////////////////////////////////////////////////

CString GANTTITEM::MILESTONE_TAG;

GANTTITEM::GANTTITEM() : crText(CLR_NONE), crBack(CLR_NONE), bParent(FALSE), dwRefID(0)
{

}

GANTTITEM::GANTTITEM(const GANTTITEM& gi)
{
	*this = gi;
}

GANTTITEM& GANTTITEM::operator=(const GANTTITEM& gi)
{
	sTitle = gi.sTitle;
	dtStart = gi.dtStart;
	dtStartCalc = gi.dtStartCalc;
	dtDue = gi.dtDue;
	dtDueCalc = gi.dtDueCalc;
	dtDone = gi.dtDone;
	crText = gi.crText;
	crBack = gi.crBack;
	sAllocTo = gi.sAllocTo;
	bParent = gi.bParent;
	dwRefID = gi.dwRefID;
	nPercent = gi.nPercent;
	
	aTags.Copy(gi.aTags);
	aDepends.Copy(gi.aDepends);
	
	return (*this);
}

BOOL GANTTITEM::operator==(const GANTTITEM& gi)
{
	return ((sTitle == gi.sTitle) &&
			(dtStart == gi.dtStart) &&
			(dtStartCalc == gi.dtStartCalc) &&
			(dtDue == gi.dtDue) &&
			(dtDueCalc == gi.dtDueCalc) &&
			(dtDone == gi.dtDone) &&
			(crText == gi.crText) &&
			(crBack == gi.crBack) &&
			(sAllocTo == gi.sAllocTo) &&
			(bParent == gi.bParent) &&
			(dwRefID == gi.dwRefID) &&
			(nPercent == gi.nPercent) &&	
			Misc::MatchAll(aTags, gi.aTags) &&
			Misc::MatchAll(aDepends, gi.aDepends));
}

GANTTITEM::~GANTTITEM()
{
	
}

void GANTTITEM::MinMaxDates(const GANTTITEM& giOther)
{
	if (giOther.bParent)
	{
		CDateHelper::Max(dtDueCalc, giOther.dtDueCalc);
		CDateHelper::Min(dtStartCalc, giOther.dtStartCalc);
	}
	else // leaf task
	{
		CDateHelper::Max(dtDueCalc, giOther.dtDue);
		CDateHelper::Min(dtStartCalc, giOther.dtStart);
	}
}

BOOL GANTTITEM::IsDone() const
{
	return CDateHelper::IsDateSet(dtDone);
}

BOOL GANTTITEM::HasStart() const
{
	return CDateHelper::IsDateSet(dtStart);
}

BOOL GANTTITEM::HasDue() const
{
	return CDateHelper::IsDateSet(dtDue);
}

BOOL GANTTITEM::IsMilestone() const
{
	if (MILESTONE_TAG.IsEmpty() || (aTags.GetSize() == 0))
		return FALSE;

	if (!bParent && !CDateHelper::IsDateSet(dtDue))
		return FALSE;
	
	if (bParent && !CDateHelper::IsDateSet(dtDueCalc))
		return FALSE;

	// else
	return (Misc::Find(aTags, MILESTONE_TAG, FALSE, FALSE) != -1);
}

COLORREF GANTTITEM::GetDefaultFillColor() const
{
	if (crBack != CLR_NONE)
		return crBack;

	if ((crText != CLR_NONE) && (crText != 0))
		return crText;

	// else
	return GetSysColor(COLOR_WINDOW);
}

COLORREF GANTTITEM::GetDefaultBorderColor() const
{
	COLORREF crDefFill = GetDefaultFillColor();

	if (crDefFill == GetSysColor(COLOR_WINDOW))
		return 0;

	// else
	return GraphicsMisc::Darker(crDefFill, 0.5);
}

//////////////////////////////////////////////////////////////////////

CGanttItemMap::~CGanttItemMap()
{
	RemoveAll();
}

void CGanttItemMap::RemoveAll()
{
	DWORD dwTaskID = 0;
	GANTTITEM* pGI = NULL;
	
	POSITION pos = GetStartPosition();
	
	while (pos)
	{
		GetNextAssoc(pos, dwTaskID, pGI);
		ASSERT(pGI);
		
		delete pGI;
	}
	
	CMap<DWORD, DWORD, GANTTITEM*, GANTTITEM*&>::RemoveAll();
}

BOOL CGanttItemMap::RemoveKey(DWORD dwKey)
{
	GANTTITEM* pGI = NULL;
	
	if (Lookup(dwKey, pGI))
	{
		delete pGI;
		return CMap<DWORD, DWORD, GANTTITEM*, GANTTITEM*&>::RemoveKey(dwKey);
	}
	
	// else
	return FALSE;
}

BOOL CGanttItemMap::HasTask(DWORD dwKey) const
{
	GANTTITEM* pGI = NULL;
	
	if (Lookup(dwKey, pGI))
	{
		ASSERT(pGI);
		return TRUE;
	}
	
	// else
	return FALSE;
}

//////////////////////////////////////////////////////////////////////

GANTTDISPLAY::GANTTDISPLAY() : nEndPos(GCDR_NOTDRAWN), nDonePos(GCDR_NOTDRAWN) 
{
}

int GANTTDISPLAY::GetBestTextPos() const
{
	return max(nEndPos, nDonePos);
}

BOOL GANTTDISPLAY::IsPosSet() const
{
	return (GetBestTextPos() > GCDR_NOTDRAWN);
}

//////////////////////////////////////////////////////////////////////

CGanttDisplayMap::~CGanttDisplayMap()
{
	RemoveAll();
}

void CGanttDisplayMap::RemoveAll()
{
	DWORD dwTaskID = 0;
	GANTTDISPLAY* pGD = NULL;
	
	POSITION pos = GetStartPosition();
	
	while (pos)
	{
		GetNextAssoc(pos, dwTaskID, pGD);
		ASSERT(pGD);
		
		delete pGD;
	}
	
	CMap<DWORD, DWORD, GANTTDISPLAY*, GANTTDISPLAY*&>::RemoveAll();
}

BOOL CGanttDisplayMap::RemoveKey(DWORD dwKey)
{
	GANTTDISPLAY* pGD = NULL;

	if (Lookup(dwKey, pGD))
	{
		delete pGD;
		return CMap<DWORD, DWORD, GANTTDISPLAY*, GANTTDISPLAY*&>::RemoveKey(dwKey);
	}

	// else
	return FALSE;
}

BOOL CGanttDisplayMap::HasTask(DWORD dwKey) const
{
	GANTTDISPLAY* pGD = NULL;
	
	if (Lookup(dwKey, pGD))
	{
		ASSERT(pGD);
		return TRUE;
	}

	// else
	return FALSE;
}

//////////////////////////////////////////////////////////////////////

