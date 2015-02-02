// GanttTreeList.h: interface for the CGanttTreeList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GANTTTREELISTCTRL_H__016B94F3_1D28_4532_97EF_95F1D9D5CE55__INCLUDED_)
#define AFX_GANTTTREELISTCTRL_H__016B94F3_1D28_4532_97EF_95F1D9D5CE55__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Ganttstruct.h"

#include "..\shared\itasklist.h"
#include "..\shared\TreeListSyncer.h"
#include "..\shared\enheaderctrl.h"
#include "..\shared\treectrlhelper.h"
#include "..\shared\iuiextension.h"

/////////////////////////////////////////////////////////////////////////////

#define TVN_KEYUP (TVN_FIRST-16)
#define WM_GANTTCTRL_NOTIFY_ZOOM (WM_APP+1)

/////////////////////////////////////////////////////////////////////////////

// WPARAM = Hit test, LPARAM = Task ID
const UINT WM_GTLC_DATECHANGE = ::RegisterWindowMessage(_T("WM_GTLC_DATECHANGE"));

// WPARAM = Drag Mode, LPARAM = Task ID
const UINT WM_GTLC_DRAGCHANGE = ::RegisterWindowMessage(_T("WM_GTLC_DRAGCHANGE"));

//////////////////////////////////////////////////////////////////////

enum GTLC_COLUMN 
{
	GTLCC_NONE = -1,
	GTLCC_TITLE,
	GTLCC_STARTDATE,
	GTLCC_ENDDATE,
	GTLCC_ALLOCTO,
	GTLCC_PERCENT,
};

/////////////////////////////////////////////////////////////////////////////

class CGanttDependencyEditor
{
public:
	virtual BOOL SetFromTask(DWORD dwFromTaskID) = 0;
	virtual BOOL SetFromDependency(DWORD dwFromTaskID, DWORD dwCurToTaskID) = 0;
	virtual BOOL SetToTask(DWORD dwToTaskID) = 0;
	
	virtual DWORD GetFromTask() const = 0;
	virtual DWORD GetFromDependency(DWORD& dwCurToTaskID) const = 0;
	virtual DWORD GetToTask() const = 0;
	
	virtual BOOL IsPicking() const = 0;
	virtual BOOL IsPickingFromTask() const = 0;
	virtual BOOL IsPickingFromDependency() const = 0;
	virtual BOOL IsPickingToTask() const = 0;
	virtual BOOL IsPickingCancelled() const = 0;
	virtual BOOL IsPickingCompleted() const = 0;

	virtual BOOL IsAdding() const = 0;
	virtual BOOL IsEditing() const = 0;
	virtual BOOL IsDeleting() const = 0;

	virtual void Cancel() = 0;
};

/////////////////////////////////////////////////////////////////////////////

class CThemed;
class CGanttCreateDependsDlg;

/////////////////////////////////////////////////////////////////////////////

class CGanttTreeListCtrl : protected CTreeListSyncer  
{
	friend class CGanttLockUpdates;

public:
	CGanttTreeListCtrl();
	virtual ~CGanttTreeListCtrl();

	BOOL Initialize(HWND hwndTree, HWND hwndList, UINT nIDTreeHeader, int nMinItemHeight = 17);
	void Release();

	void UpdateTasks(const ITaskList* pTasks, IUI_UPDATETYPE nUpdate, int nEditAttribute = -1);
	bool PrepareNewTask(ITaskList* pTask) const;

	DWORD GetSelectedTaskID() const;
	BOOL SelectTask(DWORD dwTaskID);
	HTREEITEM GetSelectedItem() const;
	BOOL GetSelectedTaskDates(COleDateTime& dtStart, COleDateTime& dtDue) const;

	BOOL GetSelectedTaskDependencies(CStringArray& aDepends) const;
	BOOL SetSelectedTaskDependencies(const CStringArray& aDepends);

	DWORD HitTest(const CPoint& ptScreen) const;
	BOOL PtInHeader(const CPoint& ptScreen) const;
	void GetWindowRect(CRect& rWindow, BOOL bWithHeader = TRUE) const;

	void ExpandAll(BOOL bExpand = TRUE);
	void ExpandItem(HTREEITEM hti, BOOL bExpand = TRUE, BOOL bAndChildren = FALSE);
	BOOL CanExpandItem(HTREEITEM hti, BOOL bExpand = TRUE) const;

	void Resize(const CRect& rect);
	BOOL ZoomIn(BOOL bIn = TRUE);
	BOOL ZoomBy(int nAmount);

	void SetFocus();
	BOOL HasFocus() const { return CTreeListSyncer::HasFocus(); }

	void Sort(GTLC_COLUMN nBy, BOOL bAllowToggle);
	BOOL IsSorted() const;

	GTLC_MONTH_DISPLAY GetMonthDisplay() const { return m_nMonthDisplay; }
	BOOL SetMonthDisplay(GTLC_MONTH_DISPLAY nNewDisplay);

	void ScrollToToday();
	void ScrollToSelectedTask();

	void SetOption(DWORD dwOption, BOOL bSet = TRUE);
	BOOL HasOption(DWORD dwOption) const { return (m_dwOptions & dwOption); }

	BOOL HandleEraseBkgnd(CDC* pDC);
	void SetAlternateLineColor(COLORREF crAltLine);
	void SetGridLineColor(COLORREF crGridLine);
	void SetTodayColor(COLORREF crToday);
	void SetWeekendColor(COLORREF crWeekend);
	void SetParentColoring(GTLC_PARENTCOLORING nOption, COLORREF color);
	void SetDoneTaskAttributes(COLORREF color, BOOL bStrikeThru);
	void SetMilestoneTag(const CString& sTag);

	BOOL CancelOperation();
	void SetReadOnly(bool bReadOnly) { m_bReadOnly = bReadOnly; }

	void SetSnapMode(GTLC_SNAPMODE nSnap) { m_nSnapMode = nSnap; }
	GTLC_SNAPMODE GetSnapMode() const;

	BOOL BeginDependencyEdit(CGanttDependencyEditor* pDependEdit);
	void EndDependencyEdit();

	static BOOL WantAttributeUpdate(int nEditAttribute);

protected:
	CEnHeaderCtrl m_treeHeader, m_listHeader;
	CImageList m_ilSize;
	HFONT m_hFontDone;
	BOOL m_bSortAscending;
	GTLC_COLUMN m_nSortBy;
	int m_nMonthWidth;
	CArray<int, int> m_aMinMonthWidths;
	GTLC_MONTH_DISPLAY m_nMonthDisplay;
	DWORD m_dwOptions;
	COLORREF m_crAltLine, m_crGridLine, m_crToday, m_crWeekend, m_crParent, m_crDone;
	GTLC_PARENTCOLORING m_nParentColoring;
	CRect m_rect;
	int m_nSplitWidth;
	int m_nItemHeight;
	COleDateTime m_dtEarliest, m_dtLatest;
	BOOL m_bDraggingStart, m_bDraggingEnd, m_bDragging;
	GANTTITEM m_giPreDrag;
	CPoint m_ptDragStart, m_ptLastDependPick;
	CGanttDependencyEditor* m_pDependEdit;
	BOOL m_bReadOnly;

	// keep our own handles to these to speed lookups
	HWND m_hwndList, m_hwndTree;

	CGanttItemMap m_data;
	CGanttDisplayMap m_display;

private:
	mutable CTreeCtrlHelper* m_pTCH;
	mutable GTLC_SNAPMODE m_nSnapMode;

protected:
	LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	LRESULT ScWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

protected:
	// pseudo message handlers
	LRESULT OnCustomDrawTree(HWND hwndTree, NMTVCUSTOMDRAW* pTVCD);
	LRESULT OnCustomDrawList(HWND hwndList, NMLVCUSTOMDRAW* pLVCD);
	LRESULT OnCustomDrawListHeader(NMCUSTOMDRAW* pNMCD);

	void DrawListItem(CDC* pDC, int nItem, int nCol, DWORD dwTaskID);
	void PostDrawListItem(CDC* pDC, int nItem, DWORD dwTaskID);
	void DrawTreeItem(CDC* pDC, HTREEITEM hti, int nCol, const GANTTITEM& gi);
	void DrawListHeaderItem(CDC* pDC, int nCol);
	void DrawListHeaderRect(CDC* pDC, const CRect& rItem, const CString& sItem, CThemed* pTheme);

	int DrawGanttBar(CDC* pDC, const CRect& rMonth, int nMonth, int nYear, const GANTTITEM& gi, BOOL bSelected = FALSE);
	int DrawGanttDone(CDC* pDC, const CRect& rMonth, int nMonth, int nYear, const GANTTITEM& gi, BOOL bSelected = FALSE);
	int DrawGanttMilestone(CDC* pDC, const CRect& rMonth, int nMonth, int nYear, const GANTTITEM& gi, BOOL bSelected = FALSE);

	BOOL DrawToday(CDC* pDC, const CRect& rMonth, int nMonth, int nYear, BOOL bSelected = FALSE);
	void DrawItemDivider(CDC* pDC, const CRect& rItem, BOOL bColumn, BOOL bVert, LPRECT prFocus = NULL);
	void DrawGanttParentEnds(CDC* pDC, const GANTTITEM& gi, const CRect& rBar, 
							 const COleDateTime& dtMonthStart, const COleDateTime& dtMonthEnd, HBRUSH hbrParent);

	void ClearDependencyPickLine(CDC* pDC = NULL);
	BOOL DrawDependencyPickLine(const CPoint& ptClient);

	void BuildListColumns();
	void UpdateListColumns(int nWidth = -1);
	void RecalcListColumnWidths(int nFromWidth, int nToWidth);
	void UpdateColumnsWidthAndText(int nWidth = -1);

	int GetListItem(HTREEITEM hti);
	void ExpandList(HTREEITEM hti, int& nNextIndex);
	void CollapseList(HTREEITEM hti);
	void ExpandList();
	void CollapseList();
	void GetTreeItemRect(HTREEITEM hti, int nCol, CRect& rItem, BOOL bText = FALSE) const;
	BOOL IsTreeItemLineOdd(HTREEITEM hti);
	void AddListColumn(int nMonth = 0, int nYear = 0);
	void SetListColumnDate(int nCol, int nMonth, int nYear);
	BOOL GetListColumnDate(int nCol, int& nMonth, int& nYear) const;
	void CalculateMinMonthWidths();
	void BuildTreeColumns();
	GTLC_MONTH_DISPLAY GetColumnDisplay(int nColWidth);
	int GetColumnWidth() const;
	int GetColumnWidth(GTLC_MONTH_DISPLAY nDisplay) const;
	int GetMonthWidth(int nColWidth) const;
	int GetRequiredColumnCount() const;
	BOOL ZoomTo(GTLC_MONTH_DISPLAY nNewDisplay, int nNewMonthWidth);
	void DeleteTreeItem(HTREEITEM hti);
	void RemoveDeletedTasks(HTREEITEM hti, const ITaskList12* pTasks);
	int FindColumn(int nScrollPos) const;
	int FindColumn(int nMonth, int nYear) const;
	int FindColumn(const COleDateTime& date) const;
	int GetDateInMonths(int nMonth, int nYear) const;
	BOOL GetDateFromScrollPos(int nScrollPos, COleDateTime& date) const;
	int GetScrollPosFromDate(const COleDateTime& date) const;
	BOOL GetListColumnRect(int nCol, CRect& rect, BOOL bScrolled = TRUE) const;
	void ScrollTo(const COleDateTime& date);
	void InitItemHeight();
	int CalcTreeWidth() const;
	int GetStartYear() const;
	int GetEndYear() const;
	int GetNumMonths() const;
	void Resize() { Resize(m_rect); }

	GANTTITEM* GetGanttItem(DWORD dwTaskID, BOOL bCopyRefID = TRUE) const;
	GANTTDISPLAY* GetGanttDisplay(DWORD dwTaskID);

	DWORD ListTaskHitTest(const CPoint& ptScreen, GTLC_HITTEST& nHit) const;
	DWORD ListDependsHitTest(const CPoint& ptClient, DWORD& dwToTaskID);

	double CalcDateDragTolerance(int nCol = 1) const;
	BOOL StartDragging(const CPoint& ptCursor);
	BOOL EndDragging(const CPoint& ptCursor);
	BOOL UpdateDragging(const CPoint& ptCursor);
	BOOL ValidateDragPoint(CPoint& ptDrag) const;
	void CancelDrag(BOOL bReleaseCapture);
	BOOL IsDragging() const;
	BOOL GetValidDragDate(const CPoint& ptCursor, COleDateTime& dtDrag) const;
	BOOL GetDateFromPoint(const CPoint& ptCursor, COleDateTime& date) const;
	COleDateTime GetNearestDate(const COleDateTime& date) const;

	void NotifyParentDateChange(GTLC_HITTEST nHit);
	void NotifyParentDragChange();

	int RecalcTreeWidth();
	int CalcMinTreeTitleWidth() const;
	int GetTreeTitleWidth() const;

	BOOL HasAltLineColor() const { return (m_crAltLine != (COLORREF)-1); }
	COLORREF GetWeekendColor(BOOL bSelected) const;
	HBRUSH GetGanttBarColors(const GANTTITEM& gi, COLORREF& crBorder, COLORREF& crFill, BOOL bSelected) const;
 	COLORREF GetTreeTextColor(const GANTTITEM& gi, BOOL bSelected, BOOL bFullRow, BOOL bTitle, BOOL bLighter = FALSE) const;
 	COLORREF GetTreeTextBkColor(const GANTTITEM& gi, BOOL bSelected, BOOL bAlternate, BOOL bFullRow, BOOL bTitle) const;
	void SetColor(COLORREF& color, COLORREF crNew);

	void RebuildTree(const ITaskList12* pTasks);
	void BuildTreeItem(const ITaskList12* pTasks, HTASKITEM hTask, CTreeCtrl& tree, HTREEITEM htiParent, BOOL bAndSiblings);
	BOOL UpdateTask(const ITaskList12* pTasks, HTASKITEM hTask, IUI_UPDATETYPE nUpdate, int nEditAttribute, BOOL bAndSiblings);
	void RecalcParentDates();
	void RecalcParentDates(const CTreeCtrl& tree, HTREEITEM htiParent, GANTTITEM*& pGI);
	BOOL GetStartDueDates(const GANTTITEM& gi, COleDateTime& dtStart, COleDateTime& dtDue) const;
	BOOL HasDisplayDates(const GANTTITEM& gi) const;
	BOOL HasDoneDate(const GANTTITEM& gi) const;
	void MinMaxDates(const GANTTITEM& gi);
	void MinMaxDates(const COleDateTime& date);

	void RedrawList(BOOL bErase = FALSE);
	void RedrawTree(BOOL bErase = FALSE);

	int GetExpandedState(CDWordArray& aExpanded, HTREEITEM hti = NULL) const;
	void SetExpandedState(const CDWordArray& aExpanded);

	CTreeCtrlHelper* TCH();
	const CTreeCtrlHelper* TCH() const;

	inline HWND GetTree() const 
	{ 
		ASSERT(m_hwndTree);
		return m_hwndTree; 
	}

	inline HWND GetList() const
	{
		ASSERT(m_hwndList);
		return m_hwndList; 
	}
	
	BOOL CalcDependencyEndPos(int nItem, GANTTDEPENDENCY& depend, BOOL bFrom, LPPOINT lpp = NULL) const;
	BOOL BuildDependency(int nFrom, int nTo, GANTTDEPENDENCY& depend) const;
	int BuildVisibleDependencyList(CGanttDependArray& aDepends) const;
	BOOL IsDependencyPickLinePosValid() const;
	void ResetDependencyPickLinePos();

	BOOL IsDependencyEditing() const;
	BOOL IsPickingDependencyFromTask() const;
	BOOL IsPickingFromDependency() const;
	BOOL IsPickingDependencyToTask() const;
	BOOL IsDependencyEditingCancelled() const;
	BOOL IsDependencyEditingComplete() const;

	static CString FormatColumnHeaderText(GTLC_MONTH_DISPLAY nDisplay, int nMonth = 0, int nYear = 0);
	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	static int GetColumnWidth(GTLC_MONTH_DISPLAY nDisplay, int nMonthWidth);
	static COleDateTime GetDate(time64_t tDate, BOOL bEndOfDay);
	static COLORREF GetColor(COLORREF crBase, double dLighter, BOOL bSelected);
	static BOOL CalcDateRect(const CRect& rMonth, int nMonth, int nYear, 
							const COleDateTime& dtFrom, const COleDateTime& dtTo, CRect& rDate);
	static BOOL CalcDateRect(const CRect& rMonth, int nDaysInMonth, 
							const COleDateTime& dtMonthStart, const COleDateTime& dtMonthEnd, 
							const COleDateTime& dtFrom, const COleDateTime& dtTo, CRect& rDate);
	static BOOL GetMonthDates(int nMonth, int nYear, COleDateTime& dtStart, COleDateTime& dtEnd);
	static CString GetTaskAllocTo(const ITaskList12* pTasks, HTASKITEM hTask);
	static BOOL WantAttributeUpdate(int nEditAttrib, int nAttribMask);
};

#endif // !defined(AFX_GANTTTREELIST_H__016B94F3_1D28_4532_97EF_95F1D9D5CE55__INCLUDED_)
