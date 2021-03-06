#if !defined(AFX_TODOCTRLLISTVIEW_H__4540F90B_05A9_43DD_B4D5_D35BE0345A6D__INCLUDED_)
#define AFX_TODOCTRLLISTVIEW_H__4540F90B_05A9_43DD_B4D5_D35BE0345A6D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ToDoCtrlListView.h : header file
//

#include "..\shared\enheaderctrl.h"

/////////////////////////////////////////////////////////////////////////////
// CTDCListView window

class CTDCListView : public CListCtrl
{
// Construction
public:
	CTDCListView();

	BOOL PtInHeader(const CPoint& ptScreen) const;
	int HitTest(const CPoint& ptClient, int* pSubItem = NULL) const;
	int GetHeaderHeight() const;
	void RedrawHeaderColumn(int nColumn);
	void RedrawHeader();
	HWND GetHeader() const { return m_header.GetSafeHwnd(); }
	int SetCurSel(int nIndex);
	int GetCurSel() const;
	void SetColumnItemData(int nColumn, DWORD dwItemData);
	DWORD GetColumnItemData(int nColumn) const;
	int GetColumnCount() const;
	int GetColumnDrawAlignment(int nColumn) const;
	void EnableHeaderSorting(BOOL bEnable = TRUE);
	int GetItemHeight() const;
	int GetTopIndex() const;
	BOOL SetTopIndex(int nIndex);

protected:
	mutable CEnHeaderCtrl m_header;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTDCListView)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTDCListView();

protected:
	virtual void PreSubclassWindow();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTDCListView)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg void OnSize(UINT nType, int cx, int cy);
	
	DECLARE_MESSAGE_MAP()

protected:
	BOOL InitHeader() const;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TODOCTRLLISTVIEW_H__4540F90B_05A9_43DD_B4D5_D35BE0345A6D__INCLUDED_)
