// ToDoCtrlListView.cpp : implementation file
//

#include "stdafx.h"
#include "TDCListView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTDCListView

CTDCListView::CTDCListView()
{
}

CTDCListView::~CTDCListView()
{
}


BEGIN_MESSAGE_MAP(CTDCListView, CListCtrl)
	//{{AFX_MSG_MAP(CTDCListView)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_WM_SIZE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDCListView message handlers

void CTDCListView::PreSubclassWindow()
{
	CListCtrl::PreSubclassWindow();

	ASSERT((GetStyle() & LVS_TYPEMASK) == LVS_REPORT);
	InitHeader();
}

void CTDCListView::OnSize(UINT nType, int cx, int cy)
{
	CListCtrl::OnSize(nType, cx, cy);
	InitHeader();
}

int CTDCListView::GetItemHeight() const
{
	if (GetItemCount() == 0)
		return -1;

	CRect rItem;
	GetItemRect(0, rItem, LVIR_BOUNDS);

	return rItem.Height();
}

int CTDCListView::GetTopIndex() const
{
	if (GetItemCount() == 0)
		return -1;

	return CListCtrl::GetTopIndex();
}

BOOL CTDCListView::SetTopIndex(int nIndex)
{
	if ((nIndex >= 0) && (nIndex < GetItemCount()))
	{
		int nCurTop = GetTopIndex();
		ASSERT(nCurTop != -1);

		if (nIndex == nCurTop)
			return TRUE;

		// else calculate offset and scroll
		CRect rItem;
		
		if (GetItemRect(nCurTop, rItem, LVIR_BOUNDS))
		{
			int nItemHeight = rItem.Height();
			Scroll(CSize(0, (nIndex - nCurTop) * nItemHeight));

			return TRUE;
		}
	}

	// else
	return FALSE;
}

BOOL CTDCListView::InitHeader() const
{
	if (!m_header.GetSafeHwnd())
	{
		m_header.SubclassDlgItem(0, (CWnd*)this);
		m_header.EnableTracking(FALSE);
	}

	// else 
	return (m_header.GetSafeHwnd() != NULL);
}

void CTDCListView::EnableHeaderSorting(BOOL bEnable)
{
	if (!InitHeader())
		return;

	if (bEnable)
		m_header.ModifyStyle(0, HDS_BUTTONS);
	else
		m_header.ModifyStyle(HDS_BUTTONS, 0);
}

int CTDCListView::GetHeaderHeight() const 
{
	if (!InitHeader())
		return -1;

	CRect rHeader;
	m_header.GetWindowRect(rHeader);
	ScreenToClient(rHeader);

	return rHeader.bottom;
}

BOOL CTDCListView::PtInHeader(const CPoint& ptScreen) const
{
	if (!InitHeader())
		return FALSE;

	CRect rHeader;
	m_header.GetWindowRect(rHeader);
		
	return rHeader.PtInRect(ptScreen);
}

int CTDCListView::HitTest(const CPoint& ptClient, int* pSubItem) const
{
	LVHITTESTINFO lvhti = { { ptClient.x, ptClient.y }, 0 };

	CTDCListView* pThis = const_cast<CTDCListView*>(this);

	if (pThis->SubItemHitTest(&lvhti) != -1)
	{
		if (pSubItem)
			*pSubItem = lvhti.iSubItem;

		return lvhti.iItem;
	}

	// else
	return -1;
}

void CTDCListView::RedrawHeaderColumn(int nColumn)
{
	if (InitHeader())
	{
		CRect rColumn;

		if (m_header.GetItemRect(nColumn, rColumn))
			m_header.InvalidateRect(rColumn, FALSE);
	}
}

void CTDCListView::SetColumnItemData(int nColumn, DWORD dwItemData)
{

	ASSERT(nColumn >= 0 && nColumn < GetColumnCount());

	if (InitHeader())
	{
		// set column item data
		HD_ITEM hdi = { 0 };
		hdi.mask = HDI_LPARAM;
		hdi.lParam = dwItemData;

		m_header.SetItem(nColumn, &hdi);
	}
}

int CTDCListView::GetColumnDrawAlignment(int nColumn) const
{
	ASSERT(nColumn >= 0 && nColumn < GetColumnCount());

	LVCOLUMN lvc = { LVCF_FMT, 0 };

	if (GetColumn(nColumn, &lvc))
	{
		switch (lvc.fmt & LVCFMT_JUSTIFYMASK)
		{
		case LVCFMT_CENTER:
			return DT_CENTER;

		case LVCFMT_RIGHT:
			return DT_RIGHT;
		}
	}

	// all else
	return DT_LEFT;
}

DWORD CTDCListView::GetColumnItemData(int nColumn) const
{
	ASSERT(m_header.GetSafeHwnd());

	if (InitHeader())
	{
		// set column item data
		HD_ITEM hdi = { HDI_LPARAM, 0 };

		if (m_header.GetItem(nColumn, &hdi))
			return hdi.lParam;
	}

	return 0;
}

int CTDCListView::GetColumnCount() const
{
	if (InitHeader())
		return m_header.GetItemCount();

	return 0;
}

void CTDCListView::RedrawHeader()
{
	if (InitHeader())
		m_header.Invalidate(FALSE);
}

int CTDCListView::SetCurSel(int nIndex)
{
	ASSERT (nIndex >= -1 && nIndex < GetItemCount());
	
	CRect rItem;
	
	UINT nMask = LVIS_SELECTED | LVIS_FOCUSED;

	// clear state of current selection
	int nCurSel = -1;
	POSITION pos = GetFirstSelectedItemPosition();

	if (pos)
		nCurSel = GetNextSelectedItem(pos);

	if (nCurSel != -1)
		SetItemState(nCurSel, 0, nMask);

	// set state of new item
	SetItemState(nIndex, nMask, nMask);
	UpdateWindow();

	return nIndex;
}

int CTDCListView::GetCurSel() const
{
	POSITION pos = GetFirstSelectedItemPosition();
	return GetNextSelectedItem(pos);
}

void CTDCListView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// don't let the selection to be set to -1
	if (HitTest(point, NULL) == -1)
		return;
	
	CListCtrl::OnLButtonDown(nFlags, point);
}

void CTDCListView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// don't let the selection to be set to -1
	if (HitTest(point, NULL) == -1)
		return;
	
	CListCtrl::OnLButtonDblClk(nFlags, point);
}

void CTDCListView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// don't let the selection to be set to -1
	int nCurSel = GetCurSel();
	
	CListCtrl::OnRButtonDown(nFlags, point);

	if (HitTest(point, NULL) == -1)
		SetCurSel(nCurSel);
}
