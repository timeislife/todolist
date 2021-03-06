#if !defined(AFX_COLORCOMBOBOX_H__47DE226A_7C73_48BA_AE5B_E43B90D752A9__INCLUDED_)
#define AFX_COLORCOMBOBOX_H__47DE226A_7C73_48BA_AE5B_E43B90D752A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// colorcombobox.h : header file
//

#include "ownerdrawcomboboxbase.h"

/////////////////////////////////////////////////////////////////////////////
// CColorComboBox window

class CColorComboBox : public COwnerdrawComboBoxBase
{
// Construction
public:
	CColorComboBox(BOOL bRoundRect = FALSE);

// Operations
public:
	int AddColor(COLORREF color, LPCTSTR szDescription = NULL);
	int InsertColor(int nIndex, COLORREF color, LPCTSTR szDescription = NULL);
	COLORREF SetColor(int nIndex, COLORREF color);

// Attributes
protected:
	BOOL m_bRoundRect;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorComboBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorComboBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorComboBox)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

protected:
	virtual void DrawItemText(CDC& dc, const CRect& rect, int nItem, UINT nItemState, 
								DWORD dwItemData, const CString& sItem, BOOL bList);	

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORCOMBOBOX_H__47DE226A_7C73_48BA_AE5B_E43B90D752A9__INCLUDED_)
