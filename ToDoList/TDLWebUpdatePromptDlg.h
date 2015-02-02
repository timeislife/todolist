#if !defined(AFX_TDLWEBUPDATEDLG_H__C551EDC9_69C8_40C3_B5D9_CA44A02C339D__INCLUDED_)
#define AFX_TDLWEBUPDATEDLG_H__C551EDC9_69C8_40C3_B5D9_CA44A02C339D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TDLWebUpdateDlg.h : header file
//

#include "tdcenum.h"

/////////////////////////////////////////////////////////////////////////////
// CTDLWebUpdatePromptDlg dialog

class CTDLWebUpdatePromptPage : public CPropertyPageEx
{
	DECLARE_DYNCREATE(CTDLWebUpdatePromptPage)

// Construction
public:
	CTDLWebUpdatePromptPage();   // standard constructor

	void AttachFont(HFONT hFont) { m_hFont = hFont; }
	void SetInfo(LPCTSTR szExeVer, const CStringArray& aChanges);

protected:
// Dialog Data
	//{{AFX_DATA(CTDLWebUpdatePromptDlg)
	CString	m_sPrompt;
	CString	m_sChanges;
	//}}AFX_DATA
	HFONT m_hFont;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTDLWebUpdatePromptDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTDLWebUpdatePromptDlg)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CWelcomeWizard

class CTDLWebUpdatePromptDlg : public CPropertySheetEx
{
	DECLARE_DYNAMIC(CTDLWebUpdatePromptDlg)
		
	// Construction
public:
	static TDL_WEBUPDATE_CHECK CheckForUpdates(BOOL bStaging, LPCTSTR szCurVer = NULL);

protected:
	CTDLWebUpdatePromptDlg(LPCTSTR szExeVer, const CStringArray& aChanges);

	// Attributes
protected:
	CTDLWebUpdatePromptPage m_page;
	HFONT m_hFont;
	
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWelcomeWizard)
protected:
	//}}AFX_VIRTUAL
	
	// Implementation
public:
	virtual ~CTDLWebUpdatePromptDlg();
	
	// Generated message map functions
protected:
	//{{AFX_MSG(CWelcomeWizard)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDLWEBUPDATEDLG_H__C551EDC9_69C8_40C3_B5D9_CA44A02C339D__INCLUDED_)
