#if !defined(AFX_MASKEDIT_H__03CD6C84_2D70_4E44_9E7E_5673B6F6F97E__INCLUDED_)
#define AFX_MASKEDIT_H__03CD6C84_2D70_4E44_9E7E_5673B6F6F97E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// maskedit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMaskEdit window

enum
{
	ME_EXCLUDE			= 0x01,
	ME_LOCALIZEDECIMAL	= 0x02,
};

class CMaskEdit : public CEdit
{
	DECLARE_DYNAMIC(CMaskEdit)

// Construction
public:
	CMaskEdit(LPCTSTR szMask = NULL, DWORD dwFlags = 0);

	void SetMask(LPCTSTR szMask, DWORD dwFlags = 0);
	BOOL IsMasked() const { return !m_sMask.IsEmpty(); }

protected:
	CString m_sMask;
	DWORD m_dwFlags;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMaskEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMaskEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMaskEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

protected:
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MASKEDIT_H__03CD6C84_2D70_4E44_9E7E_5673B6F6F97E__INCLUDED_)
