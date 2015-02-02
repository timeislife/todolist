// ownerdrawcomboboxbase.cpp : implementation file
//

#include "stdafx.h"
#include "dlgunits.h"
#include "ownerdrawcomboboxbase.h"
#include "graphicsmisc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COwnerdrawComboBoxBase

COwnerdrawComboBoxBase::COwnerdrawComboBoxBase(int nMinDLUHeight) 
	: 
	m_bItemHeightSet(FALSE), m_nMinDLUHeight(9)
{
	if (nMinDLUHeight > 0)
		m_nMinDLUHeight = nMinDLUHeight;
}

COwnerdrawComboBoxBase::~COwnerdrawComboBoxBase()
{
}


BEGIN_MESSAGE_MAP(COwnerdrawComboBoxBase, CComboBox)
	//{{AFX_MSG_MAP(COwnerdrawComboBoxBase)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_WM_CREATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COwnerdrawComboBoxBase message handlers

BOOL COwnerdrawComboBoxBase::SetMinDLUHeight(int nMinDLUHeight)
{
	ASSERT(GetSafeHwnd() == NULL);
	ASSERT(m_bItemHeightSet == FALSE);
	ASSERT(nMinDLUHeight > 0);

	if (!GetSafeHwnd() && !m_bItemHeightSet && (nMinDLUHeight > 0))
	{
		m_nMinDLUHeight = nMinDLUHeight;
		return TRUE;
	}

	return FALSE;
}

void COwnerdrawComboBoxBase::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC dc;
	
	if (!dc.Attach(lpDrawItemStruct->hDC))
		return;

	int nDC = dc.SaveDC();

	// initialise colours and fill bkgnd
	BOOL bSelected = (lpDrawItemStruct->itemState & ODS_SELECTED);

	COLORREF crBack = GetSysColor(IsWindowEnabled() ? (bSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW) : COLOR_3DFACE);
	COLORREF crText = GetSysColor(IsWindowEnabled() ? (bSelected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT) : COLOR_GRAYTEXT);

	CRect rItem(lpDrawItemStruct->rcItem);

	dc.FillSolidRect(rItem, crBack);
	dc.SetTextColor(crText);

	// draw the item
	rItem.DeflateRect(2, 1);

	if (0 <= (int)lpDrawItemStruct->itemID)	// Any item selected?
	{
		// draw text
		CString sText;

		if (GetStyle() & CBS_HASSTRINGS)
		{
			GetLBText(lpDrawItemStruct->itemID, sText);
		}

		// virtual call
		DrawItemText(dc, rItem, 
					lpDrawItemStruct->itemID, 
					lpDrawItemStruct->itemState,
					lpDrawItemStruct->itemData, 
					sText, 
					TRUE);
	}
	else
	{
		CString sText;
		GetWindowText(sText);

		DrawItemText(dc, rItem, -1, 0, 0, sText, FALSE);
	}

	// Restore the DC state before focus rect
	dc.RestoreDC(nDC);

	// draw focus rect last of all
	rItem = lpDrawItemStruct->rcItem;

	if (lpDrawItemStruct->itemAction & ODA_FOCUS)
	{
		dc.DrawFocusRect(rItem);
	}
	else if (lpDrawItemStruct->itemAction & ODA_DRAWENTIRE)
	{
		if (lpDrawItemStruct->itemState & ODS_FOCUS)
			dc.DrawFocusRect(rItem);
	}

	dc.Detach();
}

void COwnerdrawComboBoxBase::DrawItemText(CDC& dc, const CRect& rect, int /*nItem*/, UINT /*nItemState*/,
										  DWORD /*dwItemData*/, const CString& sItem, BOOL /*bList*/)
{
	if (!sItem.IsEmpty())
	{
		UINT nFlags = (DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | GraphicsMisc::GetRTLDrawTextFlags(*this));

		dc.DrawText(sItem, (LPRECT)(LPCRECT)rect, nFlags);
	}
}

BOOL COwnerdrawComboBoxBase::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= (CBS_OWNERDRAWFIXED | CBS_HASSTRINGS);

	return CComboBox::PreCreateWindow(cs);
}

void COwnerdrawComboBoxBase::InitItemHeight()
{
	ASSERT(GetSafeHwnd());

	if (!m_bItemHeightSet) 
	{
		m_bItemHeightSet = TRUE;
		
		int nMinHeight = CDlgUnits(GetParent()).ToPixelsY(m_nMinDLUHeight);
		
		SetItemHeight(-1, nMinHeight); 
		SetItemHeight(0, nMinHeight); 
	}
}

void COwnerdrawComboBoxBase::PreSubclassWindow() 
{
	InitItemHeight();
	CComboBox::PreSubclassWindow();
}

void COwnerdrawComboBoxBase::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	CDlgUnits dlu(CComboBox::GetParent());

	lpMeasureItemStruct->itemHeight = dlu.ToPixelsY(m_nMinDLUHeight); 
}

int COwnerdrawComboBoxBase::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	InitItemHeight();
	return 0;
}

void COwnerdrawComboBoxBase::OnDestroy()
{
	// reset flag in case we get reused
	m_bItemHeightSet = FALSE;

	CComboBox::OnDestroy();
}

