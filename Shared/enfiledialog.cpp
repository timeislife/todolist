// filedialogex.cpp : implementation file
//

#include "stdafx.h"
#include "enfiledialog.h"
#include "osversion.h"
#include "enstring.h"
#include "IPreferences.h"
#include "filemisc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEnFileDialog

IMPLEMENT_DYNAMIC(CEnFileDialog, CFileDialog)

// Windows 2000 version of OPENFILENAME
#if _MSC_VER < 1300
struct OPENFILENAMEEX : public OPENFILENAME 
{ 
  void *        pvReserved;
  DWORD         dwReserved;
  DWORD         FlagsEx;
};
#endif		

CEnFileDialog::CEnFileDialog(BOOL bOpenFileDialog, 
							 LPCTSTR lpszTitle,
							 LPCTSTR lpszDefExt, 
							 LPCTSTR lpszFileName, 
							 DWORD dwFlags, 
							 LPCTSTR lpszFilter, 
							 CWnd* pParentWnd) 
	:
	CFileDialog(bOpenFileDialog, 
				lpszDefExt, 
				lpszFileName, 
				dwFlags, 
				lpszFilter, 
				pParentWnd), m_sTitle(lpszTitle)
{
#if _MSC_VER < 1300
	DWORD dwVersion = ::GetVersion();
	DWORD dwWinMajor = (DWORD)(LOBYTE(LOWORD(dwVersion)));

	if ((BYTE)dwWinMajor >= 5)
		m_ofn.lStructSize = sizeof(OPENFILENAMEEX);
#endif		

	ASSERT(dwFlags); // make sure caller sets this
}


BEGIN_MESSAGE_MAP(CEnFileDialog, CFileDialog)
	//{{AFX_MSG_MAP(CEnFileDialog)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CEnFileDialog::DoModal(IPreferences* pPrefs)
{
	/*
	if (COSVersion() >= OSV_XPP)
	{
		m_ofn.lpfnHook = NULL;
		m_ofn.Flags &= ~OFN_ENABLEHOOK;
	}
	*/

	if (!m_sTitle.IsEmpty())
		m_ofn.lpstrTitle = m_sTitle;

	// if a filename has been specified, split off the folder 
	CString sInitialFolder, sFilename;
	
	if (HasInitialFileName())
	{
		sFilename = FileMisc::GetFileNameFromPath(m_ofn.lpstrFile);
		sInitialFolder = FileMisc::GetFolderFromFilePath(m_ofn.lpstrFile);
	}

	// restore last saved folder in preference to initial dir
	if (pPrefs && !m_sTitle.IsEmpty())
	{
		CString sLastFolder = pPrefs->GetProfileString(GetPreferenceKey(), m_sTitle);

		if (!sLastFolder.IsEmpty())
			sInitialFolder = sLastFolder;
	}

	// save off original values and apply new ones
	LPCTSTR szOrgFolder = NULL;

	if (!sInitialFolder.IsEmpty())
	{
		szOrgFolder = m_ofn.lpstrInitialDir;
		m_ofn.lpstrInitialDir = sInitialFolder;
	}

	if (!sFilename.IsEmpty())
	{
		// overwrite original filename
		lstrcpyn(m_szFileName, sFilename, (sizeof(m_szFileName) / sizeof(TCHAR)));
	}
	
	int nRet = CFileDialog::DoModal();

	// restore original folder
	if (szOrgFolder)
		m_ofn.lpstrInitialDir = szOrgFolder;
		
	// get and save off current working directory
	::GetCurrentDirectory(MAX_PATH, m_sLastFolder.GetBuffer(MAX_PATH + 1));
	m_sLastFolder.ReleaseBuffer();

	if ((nRet == IDOK) && pPrefs && !m_sTitle.IsEmpty())
	{
		pPrefs->WriteProfileString(GetPreferenceKey(), m_sTitle, m_sLastFolder);
	}

	return nRet;
}

CString CEnFileDialog::GetPreferenceKey() const
{
	return (m_bOpenFileDialog ? _T("OpenFileDialog") : _T("SaveFileDialog"));
}

void CEnFileDialog::SetTitle(LPCTSTR szTitle)
{
	m_sTitle = szTitle;
}

BOOL CEnFileDialog::HasInitialFileName() const
{
	return !CString(m_ofn.lpstrFile).IsEmpty();
}

BOOL CEnFileDialog::HasInitialFolder() const
{
	return !CString(m_ofn.lpstrInitialDir).IsEmpty();
}

/////////////////////////////////////////////////////////////////////////////
// CFileOpenDialog

IMPLEMENT_DYNAMIC(CFileOpenDialog, CEnFileDialog)

CFileOpenDialog::CFileOpenDialog(LPCTSTR lpszTitle,
								LPCTSTR lpszDefExt, 
								LPCTSTR lpszFileName,
								DWORD dwFlags, 
								LPCTSTR lpszFilter, 
								CWnd* pParentWnd)
: 
	CEnFileDialog(TRUE, lpszTitle, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
	// nothing else to do
}

CFileOpenDialog::CFileOpenDialog(UINT nIDTitle,
								LPCTSTR lpszDefExt, 
								LPCTSTR lpszFileName,
								DWORD dwFlags, 
								LPCTSTR lpszFilter, 
								CWnd* pParentWnd)
: 
	CEnFileDialog(TRUE, CEnString(nIDTitle), lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
	// nothing else to do
}


/////////////////////////////////////////////////////////////////////////////
// CEnFileDialog

IMPLEMENT_DYNAMIC(CFileSaveDialog, CEnFileDialog)

CFileSaveDialog::CFileSaveDialog(LPCTSTR lpszTitle,
								 LPCTSTR lpszDefExt, 
								 LPCTSTR lpszFileName,
								 DWORD dwFlags, 
								 LPCTSTR lpszFilter, 
								 CWnd* pParentWnd)
: 
	CEnFileDialog(FALSE, lpszTitle, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
	// nothing else to do
}

CFileSaveDialog::CFileSaveDialog(UINT nIDTitle,
								 LPCTSTR lpszDefExt, 
								 LPCTSTR lpszFileName,
								 DWORD dwFlags, 
								 LPCTSTR lpszFilter, 
								 CWnd* pParentWnd)
: 
	CEnFileDialog(FALSE, CEnString(nIDTitle), lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
	// nothing else to do
}

