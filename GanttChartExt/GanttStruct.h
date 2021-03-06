// GanttStruct.h: interface for the CGanttStruct class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GANTTSTRUCT_H__C83C53D4_887E_4D5C_A8A7_85C8FDB19307__INCLUDED_)
#define AFX_GANTTSTRUCT_H__C83C53D4_887E_4D5C_A8A7_85C8FDB19307__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ganttenum.h"

#include <afxtempl.h>

/////////////////////////////////////////////////////////////////////////////

struct GANTTITEM 
{ 
	GANTTITEM();
	GANTTITEM(const GANTTITEM& gi);
	virtual ~GANTTITEM();
	
	GANTTITEM& operator=(const GANTTITEM& gi);
	BOOL operator==(const GANTTITEM& gi);
	
	CString sTitle;
	COleDateTime dtStart, dtStartCalc;
	COleDateTime dtDue, dtDueCalc; 
	COleDateTime dtDone; 
	COLORREF crText, crBack;
	CString sAllocTo;
	BOOL bParent;
	DWORD dwRefID;
	CStringArray aDepends;
	CStringArray aTags;
	int nPercent;
	
	static CString MILESTONE_TAG;
	
	void MinMaxDates(const GANTTITEM& giOther);
	BOOL IsDone() const;
	BOOL HasStart() const;
	BOOL HasDue() const;
	BOOL IsMilestone() const;
	
	COLORREF GetDefaultFillColor() const;
	COLORREF GetDefaultBorderColor() const;
};

/////////////////////////////////////////////////////////////////////////////

class CGanttItemMap : public CMap<DWORD, DWORD, GANTTITEM*, GANTTITEM*&>
{
public:
	virtual ~CGanttItemMap();

	void RemoveAll();
	BOOL RemoveKey(DWORD dwKey);
	BOOL HasTask(DWORD dwTaskID) const;
};

/////////////////////////////////////////////////////////////////////////////

struct GANTTDISPLAY
{ 
	GANTTDISPLAY();
	
	int nEndPos, nDonePos;
	
	int GetBestTextPos() const;
	BOOL IsPosSet() const;
};

/////////////////////////////////////////////////////////////////////////////

class CGanttDisplayMap : public CMap<DWORD, DWORD, GANTTDISPLAY*, GANTTDISPLAY*&>
{
public:
	virtual ~CGanttDisplayMap();

	void RemoveAll();
	BOOL RemoveKey(DWORD dwKey);
	BOOL HasTask(DWORD dwTaskID) const;
};

/////////////////////////////////////////////////////////////////////////////

struct GANTTDEPENDENCY
{
	GANTTDEPENDENCY();

	void SetFrom(const CPoint& pt, DWORD dwTaskID = 0);
	void SetTo(const CPoint& pt, DWORD dwTaskID = 0);

	DWORD GetFromID() const;
	DWORD GetToID() const;

	BOOL Matches(DWORD dwFrom, DWORD dwTo) const;
	
	BOOL HitTest(const CRect& rect) const;
	BOOL HitTest(const CPoint& point, int nTol = 2) const;

	BOOL Draw(CDC* pDC, const CRect& rClient, BOOL bDragging);
	
	static int STUB;
	
protected:
	CPoint ptFrom, ptTo;
	DWORD dwFromID, dwToID;
	
protected:
	GCDEPEND_DRAWTYPE GetDependencyDrawType() const;
	int CalcDependencyPath(CPoint pts[5], BOOL bDragging) const;
	BOOL CalcBoundingRect(CRect& rect) const;
	void CalcDependencyArrow(CPoint pt, CPoint pts[3]) const;
};

typedef CArray<GANTTDEPENDENCY, GANTTDEPENDENCY&> CGanttDependArray;

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_GANTTSTRUCT_H__C83C53D4_887E_4D5C_A8A7_85C8FDB19307__INCLUDED_)
