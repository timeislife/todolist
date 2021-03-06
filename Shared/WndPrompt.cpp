// WndPrompt.cpp: implementation of the CWndPrompt class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wndPrompt.h"
#include "winclasses.h"
#include "wclassdefines.h"
#include "enstring.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWndPrompt::CWndPrompt() : m_nCheckMsg(0), m_lCheckResult(0), m_nVertOffset(0)
{

}

CWndPrompt::~CWndPrompt()
{

}

BOOL CWndPrompt::Initialize(HWND hWnd, LPCTSTR szPrompt, UINT nCheckMsg, LRESULT lRes, int nVertOffset)
{
	ASSERT (hWnd);
	ASSERT (!IsHooked());
//	ASSERT (szPrompt && *szPrompt);
	ASSERT (nCheckMsg);

	if (!IsHooked() && hWnd && szPrompt && *szPrompt && nCheckMsg)
	{
		if (HookWindow(hWnd))
		{
			m_sPrompt = szPrompt;
			m_nCheckMsg = nCheckMsg;
			m_lCheckResult = lRes;
			m_sClass = CWinClasses::GetClass(hWnd);
			m_nVertOffset = max(nVertOffset, 0);
			
			if (WantPrompt())
				Invalidate();

			return TRUE;
		}
	}

	return FALSE;
}

void CWndPrompt::SetPrompt(LPCTSTR szPrompt, int nVertOffset)
{
	ASSERT (IsHooked());
	ASSERT (szPrompt && *szPrompt);

	if (IsHooked() && szPrompt && *szPrompt)
	{
		m_sPrompt = szPrompt;

		if (nVertOffset >= 0)
			m_nVertOffset = nVertOffset;

		if (WantPrompt())
			Invalidate();
	}
}

LRESULT CWndPrompt::WindowProc(HWND hRealWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_KILLFOCUS:
	case WM_SETFOCUS:
		if (CWinClasses::IsEditControl(hRealWnd) && WantPrompt(FALSE))
			Invalidate();
		break;

	case WM_PRINT:
		{
			LRESULT lr = CSubclassWnd::WindowProc(hRealWnd, msg, wp, lp);
			DrawPrompt((HDC)wp);
			return lr;
		}
		break;

	case WM_PAINT:
		{
			LRESULT lr = CSubclassWnd::WindowProc(hRealWnd, msg, wp, lp);
			DrawPrompt(NULL);
			return lr;
		}
		break;
	}

	return CSubclassWnd::WindowProc(hRealWnd, msg, wp, lp);
}

BOOL CWndPrompt::WantPrompt(BOOL bCheckFocus) 
{ 
	BOOL bWantPrompt = (SendMessage(m_nCheckMsg) == m_lCheckResult); 
	HWND hWnd = GetHwnd();

	if (bWantPrompt && CWinClasses::IsEditControl(hWnd))
	{
		if (bCheckFocus)
			bWantPrompt = (GetFocus() != hWnd);

		if (bWantPrompt)
			bWantPrompt = (IsWindowEnabled() && !(GetStyle() & ES_READONLY));
	}

	return bWantPrompt;
}

void CWndPrompt::DrawPrompt(HDC hdc)
{
	if (WantPrompt())
	{
		ASSERT (m_sPrompt.GetLength());

		HDC hdcOrg = hdc;
		int nSaveDC = 0;

		if (!hdc)
		{
			hdc = GetDC(GetHwnd());
			nSaveDC = ::SaveDC(hdc);
		}
		
		CRect rClient;
		GetClientRect(rClient);
		rClient.DeflateRect(2, 1, 2, 0);
		rClient.top += m_nVertOffset;
		
		HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
		
		if (!hFont)
			hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		
		::SelectObject(hdc, hFont);
		::SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
		::SetBkMode(hdc, OPAQUE);
		::SetTextColor(hdc, GetSysColor(COLOR_3DSHADOW));
		::DrawText(hdc, m_sPrompt, m_sPrompt.GetLength(), rClient, DT_TOP | DT_LEFT | DT_NOPREFIX);
		
		if (!hdcOrg)
		{
			::RestoreDC(hdc, nSaveDC);
			::ReleaseDC(GetHwnd(), hdc);
		}
	}
}

//////////////////////////////////////////////////////////////////////

CWndPromptManager::CWndPromptManager()
{
}

CWndPromptManager::~CWndPromptManager()
{
	// cleanup
	HWND hwnd;
	CWndPrompt* pWnd;

	POSITION pos = m_mapWnds.GetStartPosition();

	while (pos)
	{
		m_mapWnds.GetNextAssoc(pos, hwnd, pWnd);

		if (pWnd->IsValid())
			pWnd->HookWindow(NULL);

		delete pWnd;
	}
}

BOOL CWndPromptManager::SetPrompt(HWND hWnd, LPCTSTR szPrompt, UINT nCheckMsg, LRESULT lRes, int nVertOffset)
{
	// have we already got it?
	CWndPrompt* pWnd = NULL;

	if (m_mapWnds.Lookup(hWnd, pWnd))
	{
		if (pWnd->IsValid())
		{
			// empty prompt => remove
			if (!szPrompt || !*szPrompt)
			{
				pWnd->HookWindow(NULL);
				delete pWnd;
				m_mapWnds.RemoveKey(hWnd);
			}
			else // update prompt
				pWnd->SetPrompt(szPrompt, nVertOffset);

			return TRUE;
		}
		else // control has been destroyed and hwnd is being reused
		{
			delete pWnd;
			m_mapWnds.RemoveKey(hWnd);
		}

		pWnd = NULL;
	}

	// create new wnd prompt
	pWnd = new CWndPrompt;

	if (pWnd && pWnd->Initialize(hWnd, szPrompt, nCheckMsg, lRes, nVertOffset))
	{
		m_mapWnds[hWnd] = pWnd;
		return TRUE;
	}
	
	// else
	delete pWnd;
	return FALSE;
}

BOOL CWndPromptManager::SetPrompt(HWND hWnd, UINT nIDPrompt, UINT nCheckMsg, LRESULT lRes, int nVertOffset)
{
	return SetPrompt(hWnd, CEnString(nIDPrompt), nCheckMsg, lRes, nVertOffset);
}

BOOL CWndPromptManager::SetPrompt(UINT nIDCtrl, HWND hwndParent, LPCTSTR szPrompt, UINT nCheckMsg, 
								  LRESULT lRes, int nVertOffset)
{
	return SetPrompt(GetDlgItem(hwndParent, nIDCtrl), szPrompt, nCheckMsg, lRes, nVertOffset);
}

BOOL CWndPromptManager::SetPrompt(UINT nIDCtrl, HWND hwndParent, UINT nIDPrompt, UINT nCheckMsg, 
								  LRESULT lRes, int nVertOffset)
{
	return SetPrompt(nIDCtrl, hwndParent, CEnString(nIDPrompt), nCheckMsg, lRes, nVertOffset);
}

// -------------------------------------------------------------------------------

BOOL CWndPromptManager::SetEditPrompt(UINT nIDEdit, HWND hwndParent, LPCTSTR szPrompt)
{
	return SetEditPrompt(GetDlgItem(hwndParent, nIDEdit), szPrompt);
}

BOOL CWndPromptManager::SetEditPrompt(UINT nIDEdit, HWND hwndParent, UINT nIDPrompt)
{
	return SetEditPrompt(nIDEdit, hwndParent, CEnString(nIDPrompt));
}

BOOL CWndPromptManager::SetEditPrompt(HWND hwndEdit, LPCTSTR szPrompt)
{
	if (!CWinClasses::IsEditControl(hwndEdit))
		return FALSE;

	return SetPrompt(hwndEdit, szPrompt, WM_GETTEXTLENGTH);
}

BOOL CWndPromptManager::SetEditPrompt(HWND hwndEdit, UINT nIDPrompt)
{
	return SetEditPrompt(hwndEdit, CEnString(nIDPrompt));
}

// -------------------------------------------------------------------------------

BOOL CWndPromptManager::SetComboEditPrompt(UINT nIDCombo, HWND hwndParent, LPCTSTR szPrompt)
{
	return SetComboEditPrompt(GetDlgItem(hwndParent, nIDCombo), szPrompt);
}

BOOL CWndPromptManager::SetComboEditPrompt(UINT nIDCombo, HWND hwndParent, UINT nIDPrompt)
{
	return SetComboEditPrompt(nIDCombo, hwndParent, CEnString(nIDPrompt));
}

BOOL CWndPromptManager::SetComboEditPrompt(HWND hwndCombo, LPCTSTR szPrompt)
{
	CString sClass = CWinClasses::GetClass(hwndCombo);

	if (!CWinClasses::IsClass(sClass, WC_COMBOBOX) && 
		!CWinClasses::IsClass(sClass, WC_COMBOBOXEX))
		return FALSE;

	return SetPrompt(1001, hwndCombo, szPrompt, WM_GETTEXTLENGTH);
}

BOOL CWndPromptManager::SetComboEditPrompt(HWND hwndCombo, UINT nIDPrompt)
{
	return SetComboEditPrompt(hwndCombo, CEnString(nIDPrompt));
}

// -------------------------------------------------------------------------------

BOOL CWndPromptManager::SetComboPrompt(UINT nIDCombo, HWND hwndParent, LPCTSTR szPrompt)
{
	return SetComboPrompt(GetDlgItem(hwndParent, nIDCombo), szPrompt);
}

BOOL CWndPromptManager::SetComboPrompt(HWND hwndCombo, LPCTSTR szPrompt)
{
	CString sClass = CWinClasses::GetClass(hwndCombo);

	if (!CWinClasses::IsClass(sClass, WC_COMBOBOX) && 
		!CWinClasses::IsClass(sClass, WC_COMBOBOXEX))
		return FALSE;

	// if the combo has an edit field then this is where the 
	// prompt must be set
	UINT nStyle = GetWindowLong(hwndCombo, GWL_STYLE);

	if ((nStyle & 0xf) != CBS_DROPDOWNLIST)
		return SetComboEditPrompt(hwndCombo, szPrompt);
	
	// else
	return SetPrompt(hwndCombo, szPrompt, WM_GETTEXTLENGTH);
}

BOOL CWndPromptManager::SetComboPrompt(UINT nIDCombo, HWND hwndParent, UINT nIDPrompt)
{
	return SetComboPrompt(GetDlgItem(hwndParent, nIDCombo), CEnString(nIDPrompt));
}

BOOL CWndPromptManager::SetComboPrompt(HWND hwndCombo, UINT nIDPrompt)
{
	return SetComboPrompt(hwndCombo, CEnString(nIDPrompt));
}

// -------------------------------------------------------------------------------
