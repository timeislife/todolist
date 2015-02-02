// tdlfindresultslistctrl.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "tdlfindresultslistctrl.h"

#include "..\shared\preferences.h"
#include "..\shared\misc.h"
#include "..\shared\graphicsmisc.h"

#include "..\3rdparty\shellicons.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP     0x00004000
#endif

/////////////////////////////////////////////////////////////////////////////
// CTDLFindResultsListCtrl

CTDLFindResultsListCtrl::CTDLFindResultsListCtrl() : m_nCurGroupID(-1)
{
}

CTDLFindResultsListCtrl::~CTDLFindResultsListCtrl()
{
	GraphicsMisc::VerifyDeleteObject(m_fontBold);
	GraphicsMisc::VerifyDeleteObject(m_fontStrike);
}


BEGIN_MESSAGE_MAP(CTDLFindResultsListCtrl, CEnListCtrl)
	//{{AFX_MSG_MAP(CTDLFindResultsListCtrl)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDLFindResultsListCtrl message handlers

void CTDLFindResultsListCtrl::PreSubclassWindow()
{
	CEnListCtrl::PreSubclassWindow();

	// setup up result list
	InsertColumn(0, CEnString(IDS_FT_TASK), LVCFMT_LEFT, 150);
	InsertColumn(1, CEnString(IDS_FT_WHATMATCHED), LVCFMT_LEFT, 100);
	InsertColumn(2, CEnString(IDS_FT_PATH), LVCFMT_LEFT, 250);

	ListView_SetExtendedListViewStyleEx(*this, LVS_EX_ONECLICKACTIVATE, LVS_EX_ONECLICKACTIVATE);
	ListView_SetExtendedListViewStyleEx(*this, LVS_EX_UNDERLINEHOT, LVS_EX_UNDERLINEHOT);
	ListView_SetExtendedListViewStyleEx(*this, LVS_EX_LABELTIP, LVS_EX_LABELTIP);
	ListView_SetExtendedListViewStyleEx(*this, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	SetHotCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	RefreshUserPreferences();
}

int CTDLFindResultsListCtrl::GetResultCount() const
{
	return GetResultCount(NULL);
}

int CTDLFindResultsListCtrl::GetResultCount(const CFilteredToDoCtrl* pTDC) const
{
	int nCount = 0;
	int nItem = GetItemCount();
	
	while (nItem--)
	{
		FTDRESULT* pRes = GetResult(nItem);

		if (pRes && (pTDC == NULL || pRes->pTDC == pTDC))
			nCount++;
	}
	
	return nCount;
}

int CTDLFindResultsListCtrl::GetAllResults(CFTDResultsArray& aResults) const
{
	return GetResults(NULL, aResults);
}

int CTDLFindResultsListCtrl::GetResults(const CFilteredToDoCtrl* pTDC, CFTDResultsArray& aResults) const
{
	int nNumItem = GetItemCount();
	int nCount = 0;

	aResults.RemoveAll();
	aResults.SetSize(GetResultCount(pTDC));

	for (int nItem = 0; nItem < nNumItem; nItem++)
	{
		FTDRESULT* pRes = GetResult(nItem);

		if (pRes && (pTDC == NULL || pRes->pTDC == pTDC))
		{
			aResults.SetAt(nCount, *pRes);
			nCount++;
		}
	}

	return aResults.GetSize();
}

int CTDLFindResultsListCtrl::GetResultIDs(const CFilteredToDoCtrl* pTDC, CDWordArray& aTaskIDs) const
{
	CFTDResultsArray aResults;
	int nNumRes = GetResults(pTDC, aResults);

	for (int nRes = 0; nRes < nNumRes; nRes++)
		aTaskIDs.Add(aResults[nRes].dwTaskID);

	return aResults.GetSize();
}

void CTDLFindResultsListCtrl::DeleteResults(const CFilteredToDoCtrl* pTDC)
{
	// work backwards from the last list res
	int nItem = GetItemCount();

	while (nItem--)
	{
		FTDRESULT* pRes = GetResult(nItem);

		if (pRes && pRes->pTDC == pTDC)
		{
			DeleteItem(nItem);
			delete pRes;
		}
	}
}

void CTDLFindResultsListCtrl::DeleteAllResults()
{
	// work backwards from the last list res
	int nItem = GetItemCount();

	while (nItem--)
	{
		FTDRESULT* pRes = GetResult(nItem);
		delete pRes;

		DeleteItem(nItem);
	}
}

void CTDLFindResultsListCtrl::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = CDRF_DODEFAULT;
	LPNMLVCUSTOMDRAW lpNMCustomDraw = (LPNMLVCUSTOMDRAW)pNMHDR;

	int nItem = (int)lpNMCustomDraw->nmcd.dwItemSpec;
	int nSubItem = (int)lpNMCustomDraw->iSubItem;

	switch (lpNMCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = (CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT);
		break;

	case CDDS_ITEMPREPAINT:
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
		break;
		
	case (CDDS_ITEMPREPAINT | CDDS_SUBITEM):
		{
			// only handle when not hot tracking but
			// CDIS_HOT does not work so we must do it ourselves
			CPoint ptCursor;
			GetCursorPos(&ptCursor);
			ScreenToClient(&ptCursor);
			ptCursor.x = lpNMCustomDraw->nmcd.rc.left; // we want the whole row

			if (!::PtInRect(&lpNMCustomDraw->nmcd.rc, ptCursor))
			{
				BOOL bChange = FALSE;

				if (lpNMCustomDraw->iSubItem == 0)
				{
					CFont* pFont = GetItemFont(nItem, nSubItem);

					if (pFont)
					{
						::SelectObject(lpNMCustomDraw->nmcd.hdc, pFont->GetSafeHandle());
						bChange = TRUE;
					}
				}

				// colour affects all columns but not selected items
				BOOL bSel = IsItemSelected(nItem);
				COLORREF crText = GetItemTextColor(nItem, bSel);

				if (crText != CLR_NONE)
				{
					lpNMCustomDraw->clrText = crText;
					bChange = TRUE;
				}

				if (bChange)
					*pResult = CDRF_NEWFONT;
			}

			// for references
			*pResult |= CDRF_NOTIFYPOSTPAINT;
		}
		break;

		// draw reference indicator
	case (CDDS_ITEMPOSTPAINT | CDDS_SUBITEM):
		if ((nSubItem == 0) && (lpNMCustomDraw->nmcd.rc.top > 0))
		{
			FTDRESULT* pRes = (FTDRESULT*)lpNMCustomDraw->nmcd.lItemlParam;

			if (pRes && pRes->bRef)
			{
				CDC* pDC = CDC::FromHandle(lpNMCustomDraw->nmcd.hdc);
				ShellIcons::DrawIcon(pDC, ShellIcons::SI_SHORTCUT, CRect(lpNMCustomDraw->nmcd.rc).TopLeft());
			}
		}
		break;

	default:
		break;
	}
}

COLORREF CTDLFindResultsListCtrl::GetItemTextColor(int nItem, BOOL bSelected) const
{
   if (!bSelected && ((m_crDone != CLR_NONE) || (m_crRef != CLR_NONE)))
   {
      FTDRESULT* pRes = (FTDRESULT*)GetItemData(nItem);

      if (pRes)
	  {
		  if (pRes->bDone)
			  return m_crDone;

		  if (pRes->bRef)
			  return m_crRef;
	  }
   }

   // else
   return CLR_NONE;
}

CFont* CTDLFindResultsListCtrl::GetItemFont(int nItem, int nSubItem)
{
	if ((nSubItem == 0) && m_fontStrike.GetSafeHandle())
	{
		FTDRESULT* pRes = GetResult(nItem);

		if (pRes && pRes->bDone)
			return &m_fontStrike;
	}

	// else
	return GetFont();
}

void CTDLFindResultsListCtrl::RefreshUserPreferences()
{
	CPreferences prefs;
	
	// update user completed tasks colour
	if (prefs.GetProfileInt(_T("Preferences"), _T("SpecifyDoneColor"), FALSE))
		m_crDone = (COLORREF)prefs.GetProfileInt(_T("Preferences\\Colors"), _T("TaskDone"), -1);
	else
		m_crDone = CLR_NONE;
	
	// update user reference tasks colour
	if (prefs.GetProfileInt(_T("Preferences"), _T("ReferenceColor"), FALSE))
		m_crRef = (COLORREF)prefs.GetProfileInt(_T("Preferences\\Colors"), _T("Reference"), -1);
	else
		m_crRef = CLR_NONE;

	// update strike thru font
	if (prefs.GetProfileInt(_T("Preferences"), _T("StrikethroughDone"), FALSE))
	{
		if (!m_fontStrike.GetSafeHandle())
			GraphicsMisc::CreateFont(m_fontStrike, (HFONT)SendMessage(WM_GETFONT), GMFS_STRIKETHRU);
	}
	else
		GraphicsMisc::VerifyDeleteObject(m_fontStrike);

	if (IsWindowVisible())
		Invalidate();
}

int CTDLFindResultsListCtrl::AddResult(const SEARCHRESULT& result, LPCTSTR szTask, LPCTSTR szPath, const CFilteredToDoCtrl* pTDC)
{
	int nPos = GetItemCount();
		
	// add result
	int nIndex = InsertItem(nPos, szTask);
	SetItemText(nIndex, 1, Misc::FormatArray(result.aMatched));
	SetItemText(nIndex, 2, szPath);

	if (m_nCurGroupID != -1)
		SetItemGroupId(nIndex, m_nCurGroupID);

	UpdateWindow();
		
	// map identifying data
	FTDRESULT* pRes = new FTDRESULT(result, pTDC);
	SetItemData(nIndex, (DWORD)pRes);

	return nIndex;
}

BOOL CTDLFindResultsListCtrl::AddHeaderRow(LPCTSTR szText)
{
	if (m_nCurGroupID == -1)
		EnableGroupView();

	return InsertGroupHeader(-1, ++m_nCurGroupID, szText);
}

