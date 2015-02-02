// Fi M_BlISlteredToDoCtrl.cpp: implementation of the CTabbedToDoCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TabbedToDoCtrl.h"
#include "todoitem.h"
#include "resource.h"
#include "tdcstatic.h"
#include "tdcmsg.h"
#include "tdccustomattributehelper.h"
#include "tdltaskicondlg.h"
#include "tdcuiextensionhelper.h"

#include "..\shared\holdredraw.h"
#include "..\shared\datehelper.h"
#include "..\shared\enstring.h"
#include "..\shared\preferences.h"
#include "..\shared\deferwndmove.h"
#include "..\shared\autoflag.h"
#include "..\shared\holdredraw.h"
#include "..\shared\osversion.h"
#include "..\shared\graphicsmisc.h"
#include "..\shared\iuiextension.h"
#include "..\shared\uiextensionmgr.h"

#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////

#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER 0x00010000
#endif

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP     0x00004000
#endif

const UINT SORTWIDTH = 10;
const UINT DEFTEXTFLAGS = (DT_END_ELLIPSIS | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTabbedToDoCtrl::CTabbedToDoCtrl(CUIExtensionMgr& mgrUIExt, CContentMgr& mgrContent, 
								 const CONTENTFORMAT& cfDefault, const TDCCOLEDITFILTERVISIBILITY& visDefault) 
	:
	CToDoCtrl(mgrContent, cfDefault, visDefault), 
	m_bTreeNeedResort(FALSE),
	m_bTaskColorChange(FALSE),
	m_bUpdatingExtensions(FALSE),
	m_bExtModifyingApp(FALSE),
	m_mgrUIExt(mgrUIExt)
{
	// add extra controls to implement list-view
	for (int nCtrl = 0; nCtrl < NUM_FTDCCTRLS; nCtrl++)
	{
		const TDCCONTROL& ctrl = FTDCCONTROLS[nCtrl];

		AddRCControl(_T("CONTROL"), ctrl.szClass, CString((LPCTSTR)ctrl.nIDCaption), 
					ctrl.dwStyle, ctrl.dwExStyle,
					ctrl.nX, ctrl.nY, ctrl.nCx, ctrl.nCy, ctrl.nID);
	}

	// tab is on by default
	m_aStyles.SetAt(TDCS_SHOWTREELISTBAR, 1);
}

CTabbedToDoCtrl::~CTabbedToDoCtrl()
{
	// cleanup extension views
	int nView = m_aExtViews.GetSize();

	while (nView--)
	{
		IUIExtensionWindow* pExtWnd = m_aExtViews[nView];

		if (pExtWnd)
			pExtWnd->Release();
	}

	m_aExtViews.RemoveAll();
}

BEGIN_MESSAGE_MAP(CTabbedToDoCtrl, CToDoCtrl)
//{{AFX_MSG_MAP(CTabbedToDoCtrl)
	ON_WM_SETCURSOR()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_FTC_TASKLIST, OnClickListHeader)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_FTC_TASKLIST, OnListGetDispInfo)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_FTC_TASKLIST, OnListGetInfoTip)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_FTC_TASKLIST, OnListSelChanged)
	ON_NOTIFY(NM_CLICK, IDC_FTC_TASKLIST, OnListClick)
	ON_NOTIFY(NM_CUSTOMDRAW, 0, OnListHeaderCustomDraw)
	ON_NOTIFY(NM_DBLCLK, IDC_FTC_TASKLIST, OnListDblClick)
	ON_NOTIFY(NM_KEYDOWN, IDC_FTC_TASKLIST, OnListKeyDown)
	ON_NOTIFY(NM_RCLICK, 0, OnRClickListHeader)
	ON_NOTIFY(NM_RCLICK, IDC_FTC_TABCTRL, OnTabCtrlRClick)
	ON_REGISTERED_MESSAGE(WM_IUI_EDITSELECTEDTASKTITLE, OnUIExtEditSelectedTaskTitle)
	ON_REGISTERED_MESSAGE(WM_IUI_MODIFYSELECTEDTASK, OnUIExtModifySelectedTask)
	ON_REGISTERED_MESSAGE(WM_IUI_SELECTTASK, OnUIExtSelectTask)
	ON_REGISTERED_MESSAGE(WM_NCG_RECALCCOLWIDTH, OnGutterRecalcColWidth)
	ON_REGISTERED_MESSAGE(WM_NCG_WIDTHCHANGE, OnGutterWidthChange)
	ON_REGISTERED_MESSAGE(WM_PCANCELEDIT, OnEditCancel)
	ON_REGISTERED_MESSAGE(WM_TDCN_VIEWPOSTCHANGE, OnPostTabViewChange)
	ON_REGISTERED_MESSAGE(WM_TDCN_VIEWPRECHANGE, OnPreTabViewChange)
	ON_REGISTERED_MESSAGE(WM_TLDT_DROP, OnDropObject)
	ON_WM_DRAWITEM()
	ON_WM_ERASEBKGND()
	ON_WM_MEASUREITEM()
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////

void CTabbedToDoCtrl::DoDataExchange(CDataExchange* pDX)
{
	CToDoCtrl::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_FTC_TASKLIST, m_list);
	DDX_Control(pDX, IDC_FTC_TABCTRL, m_tabViews);
}

BOOL CTabbedToDoCtrl::OnInitDialog()
{
	CToDoCtrl::OnInitDialog();

	ListView_SetExtendedListViewStyleEx(m_list, 
										LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, 
										LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_dtList.Register(&m_list, this);

	// prevent the list overwriting the label edit
	m_list.ModifyStyle(0, WS_CLIPSIBLINGS);

	// move the list to be after the tree in the z-order
	m_list.SetWindowPos(&m_tree, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// and hook it
	ScHookWindow(m_list);

	// add all columns
	BuildListColumns();
		
	m_tabViews.AttachView(m_tree, FTCV_TASKTREE, CEnString(IDS_TASKTREE), GraphicsMisc::LoadIcon(IDI_TASKTREE_STD), NULL);
	m_tabViews.AttachView(m_list, FTCV_TASKLIST, CEnString(IDS_LISTVIEW), GraphicsMisc::LoadIcon(IDI_LISTVIEW_STD), NewViewData());

	for (int nExt = 0; nExt < m_mgrUIExt.GetNumUIExtensions(); nExt++)
		AddView(m_mgrUIExt.GetUIExtension(nExt));

	Resize();

	return FALSE;
}

BOOL CTabbedToDoCtrl::PreTranslateMessage(MSG* pMsg) 
{
	// see if an UI extension wants this
	FTC_VIEW nView = GetView();
	
	switch (nView)
	{
	case FTCV_TASKLIST:
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		break;
		
	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		{
			IUIExtensionWindow* pExtWnd = GetExtensionWnd(nView, FALSE);
			ASSERT(pExtWnd);

			if (pExtWnd->ProcessMessage(pMsg))
				return true;
		}
		break;
		
	default:
		ASSERT(0);
	}

	return CToDoCtrl::PreTranslateMessage(pMsg);
}

void CTabbedToDoCtrl::SetUITheme(const CUIThemeFile& theme)
{
	CToDoCtrl::SetUITheme(theme);

	m_tabViews.SetBackgroundColor(theme.crAppBackLight);

	// update extensions
	int nExt = m_aExtViews.GetSize();
			
	while (nExt--)
	{
		IUIExtensionWindow* pExtWnd = m_aExtViews[nExt];
		
		if (pExtWnd)
		{
			// prepare theme file
			CUIThemeFile themeExt(theme);

			themeExt.SetToolbarImageFile(pExtWnd->GetTypeID());
			pExtWnd->SetUITheme(&themeExt);
		}
	}
}

BOOL CTabbedToDoCtrl::LoadTasks(const CTaskFile& file)
{
	CPreferences prefs;

	BOOL bSuccess = CToDoCtrl::LoadTasks(file);

	// reload last view
	if (GetView() == FTCV_UNSET)
	{
		CString sKey = GetPreferencesKey(); // no subkey
		
		if (!sKey.IsEmpty()) // first time
		{
			CPreferences prefs;
			// restore view visibility
			ShowListViewTab(prefs.GetProfileInt(sKey, _T("ListViewVisible"), TRUE));

			CStringArray aTypeIDs;
			int nExt = prefs.GetProfileInt(sKey, _T("VisibleExtensionCount"), -1);

			if (nExt >= 0)
			{
				while (nExt--)
				{
					CString sSubKey = Misc::MakeKey(_T("VisibleExt%d"), nExt);
					aTypeIDs.Add(prefs.GetProfileString(sKey, sSubKey));
				}

				SetVisibleExtensionViews(aTypeIDs);
			}
			
			FTC_VIEW nView = (FTC_VIEW)prefs.GetProfileInt(sKey, _T("View"), FTCV_UNSET);

			if ((nView != FTCV_UNSET) && (nView != GetView()))
				SetView(nView);

			// clear the view so we don't keep restoring it
			prefs.WriteProfileInt(sKey, _T("View"), FTCV_UNSET);
		}
	}

	return bSuccess;
}

void CTabbedToDoCtrl::OnDestroy() 
{
	if (GetView() != FTCV_UNSET)
	{
		CPreferences prefs;
		CString sKey = GetPreferencesKey(); // no subkey
		
		// save view
		if (!sKey.IsEmpty())
		{
			prefs.WriteProfileInt(sKey, _T("View"), GetView());
			prefs.WriteProfileInt(sKey, _T("View"), GetView());

			// save view visibility
			prefs.WriteProfileInt(sKey, _T("ListViewVisible"), IsListViewTabShowing());

			CStringArray aTypeIDs;
			int nExt = GetVisibleExtensionViews(aTypeIDs);

			prefs.WriteProfileInt(sKey, _T("VisibleExtensionCount"), nExt);

			while (nExt--)
			{
				CString sSubKey = Misc::MakeKey(_T("VisibleExt%d"), nExt);
				prefs.WriteProfileString(sKey, sSubKey, aTypeIDs[nExt]);
			}

			// extensions
			int nView = m_aExtViews.GetSize();

			if (nView)
			{
				CString sKey = GetPreferencesKey(_T("UIExtensions"));

				while (nView--)
				{
					IUIExtensionWindow* pExtWnd = m_aExtViews[nView];

					if (pExtWnd)
						pExtWnd->SavePreferences(&prefs, sKey);
				}
			}
		}
	}
		
	CToDoCtrl::OnDestroy();
}

void CTabbedToDoCtrl::BuildListColumns(BOOL bResizeCols)
{
	while (m_list.DeleteColumn(0));
	
	// we handle title column separately
	int nPos = 0;

	for (int nCol = 0; nCol < NUM_COLUMNS - 1; nCol++)
	{
		const TDCCOLUMN& col = COLUMNS[nCol];

		// insert custom columns before dependency
		if (col.nColID == TDCC_DEPENDENCY)
		{
			for (int nAttrib = 0; nAttrib < m_aCustomAttribDefs.GetSize(); nAttrib++)
			{
				const TDCCUSTOMATTRIBUTEDEFINITION& attribDef = m_aCustomAttribDefs[nAttrib];
				int nColAlign = GetListColumnAlignment(attribDef.nColumnAlignment);

				int nIndex = m_list.InsertColumn(nPos++, _T(""), nColAlign, 1);
				ASSERT(nIndex >= 0);

				// set column item data
				TDC_COLUMN nColID = attribDef.GetColumnID();
				m_list.SetColumnItemData(nIndex, nColID);
			}
		}
		
		int nColAlign = GetListColumnAlignment(col.nAlignment);
		int nIndex = m_list.InsertColumn(nPos++, _T(""), nColAlign, 10);
		ASSERT(nIndex >= 0);

		// set column item data
		m_list.SetColumnItemData(nIndex, col.nColID);
	}

	// title column
	if (HasStyle(TDCS_RIGHTSIDECOLUMNS))
	{
		int nIndex = m_list.InsertColumn(0, _T(""), LVCFMT_LEFT, 10);
		m_list.SetColumnItemData(nIndex, TDCC_CLIENT);
	}
	else
	{
		int nIndex = m_list.InsertColumn(nPos, _T(""), LVCFMT_LEFT, 10);
		m_list.SetColumnItemData(nIndex, TDCC_CLIENT);
	}

	if (bResizeCols)
		UpdateListColumnWidths();
}

int CTabbedToDoCtrl::GetListColumnAlignment(int nDTAlign)
{
	switch (nDTAlign)
	{
	case DT_RIGHT:	return LVCFMT_RIGHT;
	case DT_CENTER:	return LVCFMT_CENTER;
	default:		return LVCFMT_LEFT; // all else
	}
}


void CTabbedToDoCtrl::OnRClickListHeader(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	// forward on to parent
	const MSG* pMsg = GetCurrentMessage();
	LPARAM lPos = MAKELPARAM(pMsg->pt.x, pMsg->pt.y);

	GetParent()->SendMessage(WM_CONTEXTMENU, (WPARAM)GetSafeHwnd(), lPos);

	*pResult = 0;
}

void CTabbedToDoCtrl::OnClickListHeader(NMHDR* pNMHDR, LRESULT* pResult)
{
	ASSERT(InListView());

	NMLISTVIEW* pNMLV = (NMLISTVIEW*)pNMHDR;
	
	int nCol = pNMLV->iSubItem;
	TDC_COLUMN nColID = GetListColumnID(nCol);
	BOOL bSortable = FALSE;

	TDCCOLUMN* pCol = GetListColumn(nCol);

	if (pCol)
	{
		bSortable = TRUE;
	}
	else if (CTDCCustomAttributeHelper::IsCustomColumn(nColID))
	{
		bSortable = CTDCCustomAttributeHelper::IsColumnSortable(nColID, m_aCustomAttribDefs);
	}

	if (bSortable)
	{
		VIEWDATA* pLVData = GetActiveViewData();
		TDC_COLUMN nPrev = pLVData->sort.single.nBy;
		
		Sort(nColID);
		
		// notify parent
		if (!pLVData->sort.single.IsSortingBy(nPrev))
			GetParent()->SendMessage(WM_TDCN_SORT, GetDlgCtrlID(), MAKELPARAM((WORD)nPrev, (WORD)pLVData->sort.single.nBy));
	}

	*pResult = 0;
}

void CTabbedToDoCtrl::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* lplvdi = (NMLVDISPINFO*)pNMHDR;
	*pResult = 0;

	UINT nMask = lplvdi->item.mask;
	DWORD dwTaskID = (DWORD)lplvdi->item.lParam;

	if ((nMask & LVIF_TEXT) &&  m_dwEditingID != dwTaskID)
	{
		const TODOITEM* pTDI = GetTask(dwTaskID);

		// it's possible that the task does not exist if it's just been 
		// deleted from the tree view
		if (!pTDI)
			return;

		// all else
		lplvdi->item.pszText = (LPTSTR)(LPCTSTR)pTDI->sTitle;
	}

	if (nMask & LVIF_IMAGE)
	{
		if (IsColumnShowing(TDCC_ICON))
		{
			lplvdi->item.iImage = -1;
		}
		else
		{
			const TODOITEM* pTDI = NULL;
			const TODOSTRUCTURE* pTDS = NULL;
			
			if (!m_data.GetTask(dwTaskID, pTDI, pTDS))
				return;

			BOOL bHasChildren = pTDS->HasSubTasks();
			int nImage = -1;
			
			if (!pTDI->sIcon.IsEmpty())
				nImage = m_ilTaskIcons.GetImageIndex(pTDI->sIcon);
			
			else if (HasStyle(TDCS_SHOWPARENTSASFOLDERS) && bHasChildren)
				nImage = 0;
			
			lplvdi->item.iImage = nImage;
		}
	}
}

TDC_COLUMN CTabbedToDoCtrl::GetListColumnID(int nCol) const
{
	return (TDC_COLUMN)m_list.GetColumnItemData(nCol);
}

TDCCOLUMN* CTabbedToDoCtrl::GetListColumn(int nCol) const
{
	TDC_COLUMN nColID = GetListColumnID(nCol);

	nCol = NUM_COLUMNS;
	
	while (nCol--)
	{
		if (COLUMNS[nCol].nColID == nColID)
			return &COLUMNS[nCol];
	}

	// else
	return NULL;
}

int CTabbedToDoCtrl::GetListColumnIndex(TDC_COLUMN nColID) const
{
	int nCol = m_list.GetColumnCount();

	while (nCol--)
	{
		if (GetListColumnID(nCol) == nColID)
			return nCol;
	}

	ASSERT(0);
	return -1;
}

void CTabbedToDoCtrl::UpdateVisibleColumns()
{
	CToDoCtrl::UpdateVisibleColumns();

	UpdateListColumnWidths();
}

IUIExtensionWindow* CTabbedToDoCtrl::GetExtensionWnd(FTC_VIEW nView) const
{
	ASSERT(nView >= FTCV_FIRSTUIEXTENSION && nView <= FTCV_LASTUIEXTENSION);

	if (nView < FTCV_FIRSTUIEXTENSION || nView > FTCV_LASTUIEXTENSION)
		return NULL;

	int nExtension = (nView - FTCV_FIRSTUIEXTENSION);
	ASSERT(nExtension < m_aExtViews.GetSize());

	IUIExtensionWindow* pExtWnd = m_aExtViews[nExtension];
	ASSERT(pExtWnd || (m_tabViews.GetViewHwnd(nView) == NULL));

	return pExtWnd;
}

IUIExtensionWindow* CTabbedToDoCtrl::GetExtensionWnd(FTC_VIEW nView, BOOL bAutoCreate)
{
	ASSERT(nView >= FTCV_FIRSTUIEXTENSION && nView <= FTCV_LASTUIEXTENSION);

	if (nView < FTCV_FIRSTUIEXTENSION || nView > FTCV_LASTUIEXTENSION)
		return NULL;

	// try for existing first
	IUIExtensionWindow* pExtWnd = GetExtensionWnd(nView);

	if (pExtWnd || !bAutoCreate)
		return pExtWnd;

	// sanity checks
	ASSERT(m_tabViews.GetViewHwnd(nView) == NULL);

	VIEWDATA* pData = GetViewData(nView);

	if (!pData)
		return NULL;
	
	// this may take a while
	CWaitCursor cursor;

	// Create the extension window
	int nExtension = (nView - FTCV_FIRSTUIEXTENSION);
	UINT nCtrlID = (IDC_FTC_EXTENSIONWINDOW1 + nExtension);

	pExtWnd = pData->pExtension->CreateExtWindow(nCtrlID, WS_CHILD, 0, 0, 0, 0, GetSafeHwnd());
	
	if (pExtWnd == NULL)
		return NULL;
	
	HWND hWnd = pExtWnd->GetHwnd();
	ASSERT (hWnd);
	
	if (!hWnd)
		return NULL;
	
	pExtWnd->SetUITheme(&m_theme);
	pExtWnd->SetReadOnly(HasStyle(TDCS_READONLY) != FALSE);
	
	// update focus first because initializing views can take time
	::SetFocus(hWnd);
	
	m_aExtViews[nExtension] = pExtWnd;
	
	// restore state
	CPreferences prefs;
	CString sKey = GetPreferencesKey(_T("UIExtensions"));
	
	pExtWnd->LoadPreferences(&prefs, sKey);
	
	// and update tab control with our new HWND
	m_tabViews.SetViewHwnd((FTC_VIEW)nView, hWnd);
	
	// initialize update state
	pData->bNeedTaskUpdate = TRUE;

	// and capabilities
	if (pData->bCanPrepareNewTask == -1)
	{
		CTaskFile task;
		task.NewTask(_T("Test Task"));

		pData->bCanPrepareNewTask = pExtWnd->PrepareNewTask(&task);
	}

	// insert the view after the list in z-order
	::SetWindowPos(pExtWnd->GetHwnd(), m_list, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE));
	
	Invalidate();

	return pExtWnd;
}

LRESULT CTabbedToDoCtrl::OnPreTabViewChange(WPARAM nOldView, LPARAM nNewView) 
{
	// show the incoming selection and hide the outgoing in that order
	EndLabelEdit(FALSE);

	// notify parent
	GetParent()->SendMessage(WM_TDCN_VIEWPRECHANGE, nOldView, nNewView);

	// take a note of what task is currently singly selected
	// so that we can prevent unnecessary calls to UpdateControls
	DWORD dwSelTaskID = GetSingleSelectedTaskID();
	
	switch (nNewView)
	{
	case FTCV_TASKTREE:
		// make sure something is selected
		if (GetSelectedCount() == 0)
		{
			HTREEITEM hti = m_tree.GetSelectedItem();

			if (!hti)
				hti = m_tree.GetChildItem(NULL);

			CToDoCtrl::SelectTask(GetTaskID(hti));
		}

		// update sort
		if (m_bTreeNeedResort)
		{
			m_bTreeNeedResort = FALSE;
			Resort();
		}

		m_tree.EnsureVisible(Selection().GetFirstItem());
		break;

	case FTCV_TASKLIST:
		// processed any unhandled comments
		HandleUnsavedComments(); 		
		
		// set the prompt now we know how tall the header is
		m_mgrPrompts.SetPrompt(m_list, IDS_TDC_FILTEREDTASKLISTPROMPT, LVM_GETITEMCOUNT, 0, m_list.GetHeaderHeight());

		// make sure row height is correct by forcing a WM_MEASUREITEM
		RemeasureList();

		// update column widths
		UpdateListColumnWidths(FALSE);

		// restore selection
		ResyncListSelection();

		m_list.EnsureVisible(GetFirstSelectedItem(), FALSE);
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		{
			VIEWDATA* pData = GetViewData((FTC_VIEW)nNewView);

			if (!pData)
				return 1L; // prevent tab change

			// start progress if initializing from another view, 
			// will be cleaned up in OnPostTabViewChange
			UINT nProgressMsg = 0;

			if (nOldView != -1)
			{
				if (GetExtensionWnd((FTC_VIEW)nNewView, FALSE) == NULL)
					nProgressMsg = IDS_INITIALISINGTABBEDVIEW;

				else if (pData->bNeedTaskUpdate)
					nProgressMsg = IDS_UPDATINGTABBEDVIEW;

				if (nProgressMsg)
					BeginExtensionProgress(pData, nProgressMsg);
			}

			IUIExtensionWindow* pExtWnd = GetExtensionWnd((FTC_VIEW)nNewView, TRUE);
			ASSERT(pExtWnd && pExtWnd->GetHwnd());
			
			if (pData->bNeedTaskUpdate)
			{
				// start progress if not already
				// will be cleaned up in OnPostTabViewChange
				if (nProgressMsg == 0)
					BeginExtensionProgress(pData);

				CTaskFile tasks;
				GetAllTasks(tasks);
				
				UpdateExtensionView(pExtWnd, tasks, IUI_ALL);
				pData->bNeedTaskUpdate = FALSE;
			}
				
			// set the selection
			pExtWnd->SelectTask(dwSelTaskID);
		}
		break;
	}

	// update controls only if the selection has changed
	if (HasSingleSelectionChanged(dwSelTaskID))
		UpdateControls();

	return 0L; // allow tab change
}

void CTabbedToDoCtrl::UpdateExtensionView(IUIExtensionWindow* pExtWnd, const CTaskFile& tasks, 
										  IUI_UPDATETYPE nType, TDC_ATTRIBUTE nAttrib)
{
	CWaitCursor cursor;
	CAutoFlag af(m_bUpdatingExtensions, TRUE);

	pExtWnd->UpdateTasks(&tasks, nType, nAttrib);
}

void CTabbedToDoCtrl::UpdateExtensionViewSelection()
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
	case FTCV_TASKLIST:
		ASSERT(0);
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		{
			IUIExtensionWindow* pExtWnd = GetExtensionWnd(nView);
			ASSERT(pExtWnd && pExtWnd->GetHwnd());

			CDWordArray aTaskIDs;
			CToDoCtrl::GetSelectedTaskIDs(aTaskIDs);

			if (!pExtWnd->SelectTasks(aTaskIDs.GetData(), aTaskIDs.GetSize()))
			{
				if (!pExtWnd->SelectTask(GetSingleSelectedTaskID()))
				{
					// clear tasklist selection
					Selection().RemoveAll();
					UpdateControls();
				}
			}
		}
		break;

	default:
		ASSERT(0);
	}
}

DWORD CTabbedToDoCtrl::GetSingleSelectedTaskID() const
{
	if (GetSelectedCount() == 1) 
		return GetTaskID(GetSelectedItem());

	// else
	return 0;
}

BOOL CTabbedToDoCtrl::HasSingleSelectionChanged(DWORD dwSelID) const
{
	// multi-selection
	if (GetSelectedCount() != 1)
		return TRUE;

	// different selection
	if (GetTaskID(GetSelectedItem()) != dwSelID)
		return TRUE;

	// dwSelID is still the only selection
	return FALSE;
}

LRESULT CTabbedToDoCtrl::OnPostTabViewChange(WPARAM nOldView, LPARAM nNewView)
{
	switch (nNewView)
	{
	case FTCV_TASKTREE:
		break;

	case FTCV_TASKLIST:
		{
			// update sort
			VIEWDATA* pLVData = GetViewData(FTCV_TASKLIST);

			if (pLVData->bNeedResort)
			{
				pLVData->bNeedResort = FALSE;
				Resort();
			}

			// update column widths
			UpdateListColumnWidths(FALSE);
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		// stop any progress
		GetParent()->SendMessage(WM_TDCM_LENGTHYOPERATION, FALSE);

		// resync selection
		UpdateExtensionViewSelection();
		break;
	}

	// notify parent
	GetParent()->SendMessage(WM_TDCN_VIEWPOSTCHANGE, nOldView, nNewView);

	return 0L;
}

VIEWDATA* CTabbedToDoCtrl::GetViewData(FTC_VIEW nView) const
{
	VIEWDATA* pData = (VIEWDATA*)m_tabViews.GetViewData(nView);

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		ASSERT(pData == NULL);
		break;
		
	case FTCV_TASKLIST:
		ASSERT(pData && !pData->pExtension);
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		ASSERT(pData && pData->pExtension);
		break;

	// all else
	default:
		ASSERT(0);
	}

	return pData;
}

VIEWDATA* CTabbedToDoCtrl::GetActiveViewData() const
{
	return GetViewData(GetView());
}

void CTabbedToDoCtrl::SetView(FTC_VIEW nView) 
{
	// take a note of what task is currently singly selected
	// so that we can prevent unnecessary calls to UpdateControls
	DWORD dwSelTaskID = GetSingleSelectedTaskID();
	
	if (!m_tabViews.SetActiveView(nView, TRUE))
		return;

	// update controls only if the selection has changed and 
	if (HasSingleSelectionChanged(dwSelTaskID))
		UpdateControls();
}

void CTabbedToDoCtrl::SetNextView() 
{
	// take a note of what task is currently singly selected
	// so that we can prevent unnecessary calls to UpdateControls
	DWORD dwSelTaskID = GetSingleSelectedTaskID();
	
	m_tabViews.ActivateNextView();

	// update controls only if the selection has changed and 
	if (HasSingleSelectionChanged(dwSelTaskID))
		UpdateControls();
}

void CTabbedToDoCtrl::RemeasureList()
{
	CRect rList;
	m_list.GetWindowRect(rList);
	ScreenToClient(rList);

	WINDOWPOS wpos = { m_list, NULL, rList.left, rList.top, rList.Width(), rList.Height(), SWP_NOZORDER };
	m_list.SendMessage(WM_WINDOWPOSCHANGED, 0, (LPARAM)&wpos);

	// set tree header height to match listview
	int nHeight = m_list.GetHeaderHeight();

	if (nHeight != -1)
		m_tree.SetHeaderHeight(nHeight);
}

LRESULT CTabbedToDoCtrl::OnGutterRecalcColWidth(WPARAM wParam, LPARAM lParam)
{
	NCGRECALCCOLUMN* pNCRC = (NCGRECALCCOLUMN*)lParam;
	
	// special case: PATH column
	if (pNCRC->nColID != TDCC_PATH)
		return CToDoCtrl::OnGutterRecalcColWidth(wParam, lParam);

	// else tree does not show the path column
	pNCRC->nWidth = 0;
	return TRUE;
}

LRESULT CTabbedToDoCtrl::OnUIExtSelectTask(WPARAM /*wParam*/, LPARAM lParam)
{
	if (!m_bUpdatingExtensions)
	{
		// check there's an actual change
		DWORD dwTaskID = (DWORD)lParam;

		if (HasSingleSelectionChanged(dwTaskID))
			return SelectTask(dwTaskID);
	}

	// else
	return 0L;
}

LRESULT CTabbedToDoCtrl::OnUIExtEditSelectedTaskTitle(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	BOOL bEdit = EditSelectedTask();
	ASSERT(bEdit);

	return bEdit;
}

LRESULT CTabbedToDoCtrl::OnUIExtModifySelectedTask(WPARAM wParam, LPARAM lParam)
{
	ASSERT(!IsReadOnly());

	if (IsReadOnly())
		return FALSE;

	HandleUnsavedComments();

	CStringArray aValues;
	CBinaryData bdEmpty;

	// prevent change being propagated back to active view
	CAutoFlag af(m_bExtModifyingApp, TRUE);

	BOOL bResult = TRUE;
	BOOL bDependChange = FALSE;
	
	CUndoAction ua(m_data, TDCUAT_EDIT);

	try
	{
		const IUITASKMOD* pMod = (const IUITASKMOD*)lParam;
		int nNumMod = (int)wParam;

		ASSERT(nNumMod > 0);

		for (int nMod = 0; ((nMod < nNumMod) && bResult); nMod++, pMod++)
		{
			switch (pMod->nAttrib)
			{
			case TDCA_TASKNAME:
				bResult &= SetSelectedTaskTitle(pMod->szValue);
				break;

			case TDCA_DONEDATE:
				bResult &= SetSelectedTaskDate(TDCD_DONE, CDateHelper::GetDate(pMod->tValue));
				break;

			case TDCA_DUEDATE:
				if (SetSelectedTaskDate(TDCD_DUE, CDateHelper::GetDate(pMod->tValue)))
				{
					if (HasStyle(TDCS_AUTOADJUSTDEPENDENCYDATES))
						bDependChange = TRUE;
				}
				else
					bResult = FALSE;
				break;

			case TDCA_STARTDATE:
				bResult &= SetSelectedTaskDate(TDCD_START, CDateHelper::GetDate(pMod->tValue));
				break;

			case TDCA_PRIORITY:
				bResult &= SetSelectedTaskPriority(pMod->nValue);
				break;

			case TDCA_COLOR:
				bResult &= SetSelectedTaskColor(pMod->crValue);
				break;

			case TDCA_ALLOCTO:
				Misc::Split(pMod->szValue, aValues);
				bResult &= SetSelectedTaskAllocTo(aValues);
				break;

			case TDCA_ALLOCBY:
				bResult &= SetSelectedTaskAllocBy(pMod->szValue);
				break;

			case TDCA_STATUS:
				bResult &= SetSelectedTaskStatus(pMod->szValue);
				break;

			case TDCA_CATEGORY:
				Misc::Split(pMod->szValue, aValues);
				bResult &= SetSelectedTaskCategories(aValues);
				break;

			case TDCA_TAGS:
				Misc::Split(pMod->szValue, aValues);
				bResult &= SetSelectedTaskTags(aValues);
				break;

			case TDCA_PERCENT:
				bResult &= SetSelectedTaskPercentDone(pMod->nValue);
				break;

			case TDCA_TIMEEST:
				bResult &= SetSelectedTaskTimeEstimate(pMod->dValue);
				break;

			case TDCA_TIMESPENT:
				bResult &= SetSelectedTaskTimeSpent(pMod->dValue);
				break;

			case TDCA_FILEREF:
				bResult &= SetSelectedTaskFileRef(pMod->szValue);
				break;

			case TDCA_COMMENTS:
				bResult &= SetSelectedTaskComments(pMod->szValue, bdEmpty);
				break;

			case TDCA_FLAG:
				bResult &= SetSelectedTaskFlag(pMod->bValue);
				break;

			case TDCA_RISK: 
				bResult &= SetSelectedTaskRisk(pMod->nValue);
				break;

			case TDCA_EXTERNALID: 
				bResult &= SetSelectedTaskExtID(pMod->szValue);
				break;

			case TDCA_COST: 
				bResult &= SetSelectedTaskCost(pMod->dValue);
				break;

			case TDCA_DEPENDENCY: 
				Misc::Split(pMod->szValue, aValues);

				if (SetSelectedTaskDependencies(aValues))
					bDependChange = TRUE;
				else
					bResult = FALSE;
				break;

			case TDCA_VERSION:
				bResult &= SetSelectedTaskVersion(pMod->szValue);
				break;

			case TDCA_RECURRENCE: 
			case TDCA_CREATIONDATE:
			case TDCA_CREATEDBY:
			default:
				ASSERT(0);
				return FALSE;
			}
		}
	}
	catch (...)
	{
		ASSERT(0);
		return FALSE;
	}

	// since we prevented changes being propagated back to the active view
	// we may need to update more than the selected task as a consequence
	// of dependency changes
	if (bDependChange)
	{
		// update all tasks
		CTaskFile tasks;
		GetTasks(tasks);
		
		IUIExtensionWindow* pExtWnd = GetExtensionWnd(GetView());
		UpdateExtensionView(pExtWnd, tasks, IUI_EDIT, TDCA_ALL);
	}
	
	return bResult;
}

void CTabbedToDoCtrl::UpdateListColumnWidths(BOOL bCheckListVisibility)
{
	if (bCheckListVisibility && InTreeView())
		return;

	int nNumCol = m_list.GetColumnCount();
	int nTotalWidth = 0;
	BOOL bFirstWidth = TRUE;

	CClientDC dc(&m_list);
	CFont* pOldFont = GraphicsMisc::PrepareDCFont(&dc, &m_list);
	float fAve = GraphicsMisc::GetAverageCharWidth(&dc);

	CHoldRedraw hr(m_list, NCR_PAINT | NCR_UPDATE);
	
	for (int nCol = 0; nCol < nNumCol; nCol++)
	{
		TDC_COLUMN nColID = GetListColumnID(nCol);

		// get column width from tree except for some special cases
		// the reason we can't just take the tree's widths is that the
		// list always shows _all_ items whereas the tree hides some
		int nWidth = 0, nTreeColWidth = m_tree.GetColumnWidth(nColID);
		CString sLongest;

		if (IsColumnShowing(nColID))
		{
			switch (nColID)
			{
			case TDCC_CLIENT:
				continue; // we'll deal with this at the end

			case TDCC_CATEGORY:
			case TDCC_TAGS:
			case TDCC_ALLOCTO:
				{
					TDC_ATTRIBUTE nAttrib = TDCA_NONE;
					CWnd* pCtrl = NULL;
					
					if (GetColumnAttribAndCtrl(nColID, nAttrib, pCtrl))
					{
						// determine the longest visible string
						CString sEdit = CDialogHelper::GetCtrlText(pCtrl);
						sLongest = m_find.GetLongestValue(nAttrib, sEdit);
					}
				}
				break;

			case TDCC_EXTERNALID:
			case TDCC_CREATEDBY:
			case TDCC_POSITION:
			case TDCC_RECURRENCE:
			case TDCC_PATH:
				{
					TDC_ATTRIBUTE nAttrib = TDC::MapColumnToAttribute(nColID);
					ASSERT(nAttrib != TDCA_NONE);
					
					// determine the longest visible string
					sLongest = m_find.GetLongestValue(nAttrib, FALSE);
				}
				break;

			case TDCC_TIMEEST:
			case TDCC_TIMESPENT:
				if (HasStyle(TDCS_DISPLAYHMSTIMEFORMAT))
				{
					nWidth = nTreeColWidth;
				}
				else
				{
					BOOL bTimeEst = (nColID == TDCC_TIMEEST);
					sLongest = m_find.GetLongestTime(s_tdDefault.nTimeEstUnits, bTimeEst);
				}
				break;

				// all else
			default:
				break;
			}

			if (!sLongest.IsEmpty())
			{
				int nAveExtent = (int)(sLongest.GetLength() * fAve);
				int nTextExtent = dc.GetTextExtent(sLongest).cx;

				// mimic width of tree columns
				nWidth = max(nAveExtent, nTextExtent) + 2 * NCG_COLPADDING;

				// can't be narrower than tree column 
				nWidth = max(nWidth, nTreeColWidth);
			}
			else if (nWidth == 0)
			{
				nWidth = nTreeColWidth;
			}

			// maximum width
			if (m_nMaxColWidth != -1 && nWidth > m_nMaxColWidth)
				nWidth = m_nMaxColWidth;

			// DON'T KNOW WHAT THIS IS FOR???
			if (nWidth && bFirstWidth)
			{
				bFirstWidth = FALSE;
				nWidth += 1;
			}

			// sort indicator 
			BOOL bTreeColWidened = m_sort.IsSortingBy(nColID, FALSE);
			
			const VIEWDATA* pLVData = GetViewData(FTCV_TASKLIST);
			BOOL bListColWidened = pLVData->sort.IsSortingBy(nColID, FALSE);
			
			if (!bTreeColWidened && bListColWidened)
				nWidth += SORTWIDTH;
			
			else if (bTreeColWidened && !bListColWidened)
				nWidth -= SORTWIDTH;
		}

		m_list.SetColumnWidth(nCol, nWidth);

		nTotalWidth += nWidth;
	}

	// client column is what's left
	CRect rList;
	m_list.GetClientRect(rList);

	int nColWidth = max(300, rList.Width() - nTotalWidth);
	m_list.SetColumnWidth(GetListColumnIndex(TDCC_CLIENT), nColWidth);

	// cleanup
	dc.SelectObject(pOldFont);
}

void CTabbedToDoCtrl::RebuildCustomAttributeUI()
{
	CToDoCtrl::RebuildCustomAttributeUI();

	// rebuild custom list columns
	BuildListColumns(TRUE);
}

void CTabbedToDoCtrl::ReposTaskTree(CDeferWndMove* pDWM, const CRect& rPos)
{
	CRect rView;
	m_tabViews.Resize(rPos, pDWM, rView);

	CToDoCtrl::ReposTaskTree(pDWM, rView);
}

void CTabbedToDoCtrl::UpdateTasklistVisibility()
{
	BOOL bTasksVis = (m_nMaxState != TDCMS_MAXCOMMENTS);
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::UpdateTasklistVisibility();
		break;

	case FTCV_TASKLIST:
		m_list.ShowWindow(bTasksVis ? SW_SHOW : SW_HIDE);
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}

	// handle tab control
	m_tabViews.ShowWindow(bTasksVis && HasStyle(TDCS_SHOWTREELISTBAR) ? SW_SHOW : SW_HIDE);
}

BOOL CTabbedToDoCtrl::OnEraseBkgnd(CDC* pDC)
{
	// clip out tab ctrl
	if (m_tabViews.GetSafeHwnd())
		ExcludeChild(&m_tabViews, pDC);

	return CToDoCtrl::OnEraseBkgnd(pDC);
}

void CTabbedToDoCtrl::Resize(int cx, int cy, BOOL bSplitting)
{
	CToDoCtrl::Resize(cx, cy, bSplitting);

	if (m_list.GetSafeHwnd())
		UpdateListColumnWidths();
}

BOOL CTabbedToDoCtrl::WantTaskContextMenu() const
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
	case FTCV_TASKLIST:
		return TRUE;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		return TRUE;
//		return FALSE;

	default:
		ASSERT(0);
	}

	return FALSE;
}

BOOL CTabbedToDoCtrl::GetSelectionBoundingRect(CRect& rSelection) const
{
	rSelection.SetRectEmpty();
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::GetSelectionBoundingRect(rSelection);
		break;

	case FTCV_TASKLIST:
		{
			rSelection.SetRectEmpty();

			POSITION pos = m_list.GetFirstSelectedItemPosition();

			// initialize to first item
			if (pos)
				VERIFY(GetListItemTitleRect(m_list.GetNextSelectedItem(pos), TDCTR_LABEL, rSelection));

			// rest of selection
			while (pos)
			{
				CRect rItem;
				VERIFY(GetListItemTitleRect(m_list.GetNextSelectedItem(pos), TDCTR_LABEL, rItem));
					
				rSelection |= rItem;
			}

			m_list.ClientToScreen(rSelection);
			ScreenToClient(rSelection);
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		{
			IUIExtensionWindow* pExtWnd = GetExtensionWnd(nView);
			ASSERT(pExtWnd);
			
			if (pExtWnd)
			{
				pExtWnd->GetLabelEditRect(rSelection);
				ScreenToClient(rSelection);
			}
		}
		break;

	default:
		ASSERT(0);
	}

	return (!rSelection.IsRectEmpty());
}

void CTabbedToDoCtrl::SelectAll()
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::SelectAll();
		break;

	case FTCV_TASKLIST:
		{
			int nNumItems = m_list.GetItemCount();
			BOOL bAllTasks = (CToDoCtrl::GetTaskCount() == (UINT)nNumItems);
			CDWordArray aTaskIDs;

			for (int nItem = 0; nItem < nNumItems; nItem++)
			{
				// select item
				m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

				// save ID only not showing all tasks
				if (!bAllTasks)
					aTaskIDs.Add(m_list.GetItemData(nItem));
			}

			// select items in tree
			if (bAllTasks)
				CToDoCtrl::SelectAll();
			else
				MultiSelectItems(aTaskIDs, TSHS_SELECT, FALSE);
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}
}

void CTabbedToDoCtrl::DeselectAll()
{
	CToDoCtrl::DeselectAll();
	ClearListSelection();
}

int CTabbedToDoCtrl::GetTasks(CTaskFile& tasks, const TDCGETTASKS& filter) const
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::GetTasks(tasks, filter);

	case FTCV_TASKLIST:
		{
			tasks.Reset();

			// ISO date strings
			// must be done first before any tasks are added
			tasks.EnableISODates(HasStyle(TDCS_SHOWDATESINISO));

			// we return exactly what's selected in the list and in the same order
			// so we make sure the filter includes TDCGT_NOTSUBTASKS
			for (int nItem = 0; nItem < m_list.GetItemCount(); nItem++)
			{
				HTREEITEM hti = GetTreeItem(nItem);
				DWORD dwParentID = m_data.GetTaskParentID(GetTaskID(hti));
				
				AddTreeItemToTaskFile(hti, tasks, NULL, filter, FALSE, dwParentID);
			}

			if (filter.dwFlags & TDCGTF_FILENAME)
				tasks.SetFileName(m_sLastSavePath);

			return tasks.GetTaskCount();
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		return CToDoCtrl::GetTasks(tasks, filter); // for now

	default:
		ASSERT(0);
	}

	return 0;
}

int CTabbedToDoCtrl::GetSelectedTasks(CTaskFile& tasks, const TDCGETTASKS& filter, DWORD dwFlags) const
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::GetSelectedTasks(tasks, filter, dwFlags);

	case FTCV_TASKLIST:
		{
			// we return exactly what's selected in the list and in the same order
			// so we make sure the filter includes TDCGT_NOTSUBTASKS
			POSITION pos = m_list.GetFirstSelectedItemPosition();

			while (pos)
			{
				int nItem = m_list.GetNextSelectedItem(pos);

				HTREEITEM hti = GetTreeItem(nItem);
				DWORD dwParentID = m_data.GetTaskParentID(GetTaskID(nItem));

				AddTreeItemToTaskFile(hti, tasks, NULL, filter, FALSE, dwParentID);
			}

			return tasks.GetTaskCount();
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		return CToDoCtrl::GetSelectedTasks(tasks, filter, dwFlags); // for now

	default:
		ASSERT(0);
	}

	return 0;
}

void CTabbedToDoCtrl::OnListHeaderCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMCUSTOMDRAW* pNMCD = (NMCUSTOMDRAW*)pNMHDR;
	
	if (pNMCD->dwDrawStage == CDDS_PREPAINT)
		*pResult |= CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;	
	
	else if (pNMCD->dwDrawStage == CDDS_ITEMPREPAINT)
		*pResult |= CDRF_NOTIFYPOSTPAINT;	

	else if (pNMCD->dwDrawStage == CDDS_ITEMPOSTPAINT)
	{
		CDC* pDC = CDC::FromHandle(pNMCD->hdc);
		CFont* pOldFont = (CFont*)pDC->SelectObject(CWnd::GetFont());

		DrawListColumnHeaderText(pDC, pNMCD->dwItemSpec, pNMCD->rc, pNMCD->uItemState);

		pDC->SelectObject(pOldFont);
	}
}

void CTabbedToDoCtrl::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (nIDCtl == IDC_FTC_TASKLIST)
		lpMeasureItemStruct->itemHeight = m_tree.TCH().GetItemHeight();
	else
		CToDoCtrl::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

void CTabbedToDoCtrl::RemoveDeletedListItems()
{
	int nItem = m_list.GetItemCount();

	while (nItem--)
	{
		DWORD dwTaskID = m_list.GetItemData(nItem);
		const TODOITEM* pTDI = GetTask(dwTaskID);

		if (pTDI == NULL)
			m_list.DeleteItem(nItem);
	}
}

CTabbedToDoCtrl::TDI_STATE CTabbedToDoCtrl::GetListItemState(int nItem)
{
	if (m_list.GetItemState(nItem, LVIS_DROPHILITED) & LVIS_DROPHILITED)
		return TDIS_DROPHILITED;
	
	else if (m_list.GetItemState(nItem, LVIS_SELECTED) & LVIS_SELECTED)
		return (TasksHaveFocus() ? TDIS_SELECTED : TDIS_SELECTEDNOTFOCUSED);
	
	return TDIS_NONE;
}

void CTabbedToDoCtrl::GetItemColors(int nItem, NCGITEMCOLORS& colors)
{
	TDI_STATE nState = GetListItemState(nItem);
	DWORD dwID = GetTaskID(nItem);

	colors.crText = GetSysColor(COLOR_WINDOWTEXT);
	colors.crBack = GetSysColor(COLOR_WINDOW);

	if (nItem % 2)
	{
		COLORREF crAlt = m_tree.GetAlternateLineColor();

		if (crAlt != CLR_NONE)
			colors.crBack = crAlt;
	}

	CToDoCtrl::GetItemColors(dwID, &colors, nState);
}

void CTabbedToDoCtrl::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nIDCtl == IDC_FTC_TASKLIST)
	{
		// don't bother drawing if we're just switching to the tree view
		if (InTreeView())
			return;
			
		int nItem = lpDrawItemStruct->itemID;
		DWORD dwTaskID = lpDrawItemStruct->itemData, dwRefID = dwTaskID;

		const TODOITEM* pTDI = NULL;
		const TODOSTRUCTURE* pTDS = NULL;

		if (!m_data.GetTask(dwTaskID, pTDI, pTDS))
			return;

		// resolve dwRefID
 		if (dwRefID == dwTaskID)
 			dwRefID = 0;

		CRect rItem, rTitleBounds;

		m_list.GetSubItemRect(nItem, 0, LVIR_BOUNDS, rItem);
		GetListItemTitleRect(nItem, TDCTR_BOUNDS, rTitleBounds);

		CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
		int nSaveDC = pDC->SaveDC(); // so that DrawFocusRect works

		pDC->SetBkMode(TRANSPARENT);

		COLORREF crGrid = m_tree.GetGridlineColor();
		NCGITEMCOLORS colors = { 0, 0, 0, FALSE, FALSE };
		
		GetItemColors(nItem, colors);
		TDI_STATE nState = GetListItemState(nItem);

		// fill back color
		if (HasStyle(TDCS_FULLROWSELECTION) || !colors.bBackSet)
		{
			DrawItemBackColor(pDC, rItem, colors.crBack);
		}
		else
		{
			// calculate the rect containing the rest of the columns
			CRect rCols(rItem);

			if (HasStyle(TDCS_RIGHTSIDECOLUMNS))
			{
				rCols.left = rTitleBounds.right;
			}
			else
				rCols.right = rTitleBounds.left;

			// fill the background colour of the rest of the columns
			rCols.right--;

			DrawItemBackColor(pDC, rCols, colors.crBack);

			// check to see whether we should fill the label cell background
			// with the alternate line colour
			if (nItem % 2) // odd row
			{
				COLORREF crAltLine = m_tree.GetAlternateLineColor();
					
				if (crAltLine != CLR_NONE)
				{
					rTitleBounds.OffsetRect(-1, 0);
					pDC->FillSolidRect(rTitleBounds, crAltLine);
					rTitleBounds.OffsetRect(1, 0);
				}
			}
		}

		// and set the text color
		if (colors.bTextSet)
			pDC->SetTextColor(colors.crText);
		else
			pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
		
		// render column text
		// use CToDoCtrl::OnGutterDrawItem to do the hard work
		CRect rFocus; // will be set up during TDCC_CLIENT handling
		int nNumCol = m_list.GetColumnCount();
		
		for (int nCol = 0; nCol < nNumCol; nCol++)
		{
			// client column/task title is special case
			TDC_COLUMN nColID = GetListColumnID(nCol);

			if (nColID == TDCC_CLIENT)
			{
				BOOL bTopLevel = pTDS->ParentIsRoot();
				BOOL bFolder = pTDS->HasSubTasks() && HasStyle(TDCS_SHOWPARENTSASFOLDERS);
				BOOL bDoneAndStrikeThru = pTDI->IsDone() && HasStyle(TDCS_STRIKETHOUGHDONETASKS);

				// icon
				CRect rSubItem;
				
				if (GetListItemTitleRect(nItem, TDCTR_ICON, rSubItem))
				{
					if (!pTDI->sIcon.IsEmpty())
					{
						int nImage = m_ilTaskIcons.GetImageIndex(pTDI->sIcon);
						m_ilTaskIcons.Draw(pDC, nImage, rSubItem.TopLeft(), ILD_TRANSPARENT);
					}
					else if (bFolder)
						m_ilTaskIcons.Draw(pDC, 0, rSubItem.TopLeft(), ILD_TRANSPARENT);
				}

				// checkbox
				if (GetListItemTitleRect(nItem, TDCTR_CHECKBOX, rSubItem))
				{
					int nImage = pTDI->IsDone() ? 2 : 1;
					CImageList::FromHandle(m_hilDone)->Draw(pDC, nImage, rSubItem.TopLeft(), ILD_TRANSPARENT);
				}

				// set fonts before calculating title rect
				HFONT hFontOld = NULL;

				if (bDoneAndStrikeThru)
					hFontOld = (HFONT)::SelectObject(lpDrawItemStruct->hDC, m_hFontDone);

				else if (bTopLevel) 
					hFontOld = (HFONT)::SelectObject(lpDrawItemStruct->hDC, m_hFontBold);

				// Task title 
				GetListItemTitleRect(nItem, TDCTR_LABEL, rSubItem, pDC, pTDI->sTitle);
				rSubItem.OffsetRect(-1, 0);

				// back colour
				if (!HasStyle(TDCS_FULLROWSELECTION) && colors.bBackSet)
				{
					DrawItemBackColor(pDC, rSubItem, colors.crBack);
				}

				// text
				DrawGutterItemText(pDC, pTDI->sTitle, rSubItem, LVCFMT_LEFT);
				rFocus = rSubItem; // save for focus rect drawing

				// vertical divider
				if (crGrid != CLR_NONE)
					pDC->FillSolidRect(rTitleBounds.right - 1, rTitleBounds.top, 1, rTitleBounds.Height(), crGrid);

				// draw shortcut for reference tasks
				// NOTE: there should no longer be any references
				ASSERT(dwRefID == 0);
// 				if (dwRefID)
// 					DrawShortcutIcon(pDC, rSubItem.TopLeft());

				// render comment text if not editing this task label
				if (m_dwEditingID != dwTaskID)
				{
					// deselect bold font if set
					if (bTopLevel && !bDoneAndStrikeThru)
					{
						::SelectObject(lpDrawItemStruct->hDC, hFontOld);
						hFontOld = NULL;
					}	

					rTitleBounds.top++;
					rTitleBounds.left = rSubItem.right + 4;
					DrawCommentsText(pDC, pTDI, pTDS, rTitleBounds, nState);
				}

				if (hFontOld)
					::SelectObject(lpDrawItemStruct->hDC, hFontOld);
			}
			else
			{
				// get col rect
				CRect rSubItem;
				m_list.GetSubItemRect(nItem, nCol, LVIR_LABEL, rSubItem);
				rSubItem.OffsetRect(-1, 0);

				NCGDRAWITEM ncgDI;

				ncgDI.pDC = pDC;
				ncgDI.dwItem = NULL;
				ncgDI.dwParentItem = NULL;
				ncgDI.nLevel = 0;
				ncgDI.nItemPos = 0;
				ncgDI.rWindow = &rSubItem;
				ncgDI.rItem = &rSubItem;
				ncgDI.nColID = nColID;
				ncgDI.nTextAlign = m_list.GetColumnDrawAlignment(nCol);
				
				DrawItemColumn(pTDI, pTDS, &ncgDI, nState, dwRefID);
			}
		}

		// base gridline
		if (crGrid != CLR_NONE)
			pDC->FillSolidRect(rItem.left, rItem.bottom - 1, rItem.Width() - 1, 1, crGrid);

		pDC->RestoreDC(nSaveDC); // so that DrawFocusRect works
		
		// focus rect
		if ((lpDrawItemStruct->itemState & ODS_FOCUS) && (nState == TDIS_SELECTED))
		{
			pDC->DrawFocusRect(rFocus);
		}

	}
	else
		CToDoCtrl::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

COLORREF CTabbedToDoCtrl::GetItemLineColor(HTREEITEM hti)
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::GetItemLineColor(hti);

	case FTCV_TASKLIST:
		{
			COLORREF crAltLines = m_tree.GetAlternateLineColor();

			if (crAltLines != CLR_NONE)
			{
				int nItem = FindListTask(GetTaskID(hti));

				if (nItem != -1 && (nItem % 2))
					return crAltLines;
			}
				
			// else
			return GetSysColor(COLOR_WINDOW);
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}

	// else
	return GetSysColor(COLOR_WINDOW);
}

void CTabbedToDoCtrl::DrawListColumnHeaderText(CDC* pDC, int nCol, const CRect& rCol, UINT nState)
{
	// filter out zero width columns
	if (rCol.Width() == 0)
		return;

	TDC_COLUMN nColID = GetListColumnID(nCol);
	int nAlignment = m_list.GetColumnDrawAlignment(nCol);
	CEnString sColumn;
	CString sFont;

	if (CTDCCustomAttributeHelper::IsCustomColumn(nColID))
	{
		TDCCUSTOMATTRIBUTEDEFINITION attribDef;

		if (CTDCCustomAttributeHelper::GetAttributeDef(nColID, m_aCustomAttribDefs, attribDef))
			sColumn = attribDef.GetColumnTitle();
	}
	else
	{
		const TDCCOLUMN* pCol = GetListColumn(nCol);
		ASSERT(pCol);

		sColumn.LoadString(pCol->nIDName);

		// special cases
		if (sColumn.IsEmpty())
			sColumn = CString(static_cast<TCHAR>(pCol->nIDName));

		sFont = pCol->szFont;
	}

	if (nColID == TDCC_CLIENT && HasStyle(TDCS_SHOWPATHINHEADER) && m_list.GetSelectedCount() == 1)
	{
		int nColWidthInChars = (int)(rCol.Width() / m_fAveHeaderCharWidth);
		CString sPath = m_data.GetTaskPath(GetSelectedTaskID(), nColWidthInChars);
			
		if (!sPath.IsEmpty())
			sColumn.Format(_T("%s [%s]"), CEnString(IDS_TDC_COLUMN_TASK), sPath);
	}
	else if (sColumn == _T("%%"))
		sColumn = _T("%");

	pDC->SetBkMode(TRANSPARENT);
	
	CRect rText(rCol);
	rText.bottom -= 2;
	
	switch (nAlignment)
	{
	case DT_LEFT:
		rText.left += NCG_COLPADDING - 1;
		break;
		
	case DT_RIGHT:
		rText.right -= NCG_COLPADDING;
		break;

	case DT_CENTER:
		rText.right--;
	}
	
	BOOL bDown = (nState & CDIS_SELECTED);

	const VIEWDATA* pLVData = GetViewData(FTCV_TASKLIST);
	BOOL bSorted = pLVData->sort.IsSortingBy(nColID, FALSE);
	
	if (bDown)
		rText.OffsetRect(1, 1);
	
	if (bSorted)
		rText.right -= (SORTWIDTH + 2);

	UINT nTextFlags = (DT_END_ELLIPSIS | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | 
						nAlignment | GraphicsMisc::GetRTLDrawTextFlags(m_tree));

	if (!sFont.IsEmpty())
	{
		CFont* pOldFont = NULL;

		if (sFont.CompareNoCase(_T("WingDings")) == 0)
			pOldFont = pDC->SelectObject(&GraphicsMisc::WingDings());
		
		else if (sFont.CompareNoCase(_T("Marlett")) == 0)
			pOldFont = pDC->SelectObject(&GraphicsMisc::Marlett());
		
		pDC->DrawText(sColumn, rText, nTextFlags);
		
		if (pOldFont)
			pDC->SelectObject(pOldFont);
	}
	else
		pDC->DrawText(sColumn, rText, nTextFlags);

	// draw sort direction if required
	if (bSorted)
	{
		// adjust for sort arrow
		if (nAlignment == DT_CENTER)
			rText.left = ((rText.left + rText.right + pDC->GetTextExtent(sColumn).cx) / 2) + 2;
			
		else if (nAlignment == DT_RIGHT)
			rText.left = rText.right + 2;
		else
			rText.left = rText.left + pDC->GetTextExtent(sColumn).cx + 2;

		BOOL bAscending = (pLVData->sort.single.bAscending ? 1 : -1);
		int nMidY = (bDown ? 1 : 0) + (rText.top + rText.bottom) / 2;
		POINT ptArrow[3] = { 
							{ 0, 0 }, 
							{ 3, -bAscending * 3 }, 
							{ 7, bAscending } };
		
		// translate the arrow to the appropriate location
		int nPoint = 3;
		
		while (nPoint--)
		{
			ptArrow[nPoint].x += rText.left + 3 + (bDown ? 1 : 0);
			ptArrow[nPoint].y += nMidY;
		}
		pDC->Polyline(ptArrow, 3);
	}
}

void CTabbedToDoCtrl::UpdateColumnHeaderSorting()
{
	CToDoCtrl::UpdateColumnHeaderSorting();

	m_list.EnableHeaderSorting(HasStyle(TDCS_COLUMNHEADERSORTING));
}

BOOL CTabbedToDoCtrl::SetStyle(TDC_STYLE nStyle, BOOL bOn, BOOL bWantUpdate)
{
	// base class processing
	if (CToDoCtrl::SetStyle(nStyle, bOn, bWantUpdate))
	{
		// post-precessing
		switch (nStyle)
		{
		case TDCS_RIGHTSIDECOLUMNS:
			BuildListColumns();
			break;

		case TDCS_SHOWINFOTIPS:
			ListView_SetExtendedListViewStyleEx(m_list, LVS_EX_INFOTIP, bOn ? LVS_EX_INFOTIP : 0);
			break;

		case TDCS_SHOWTREELISTBAR:
			m_tabViews.ShowTabControl(bOn);

			if (bWantUpdate)
				Resize();
			break;

		// detect preferences that can affect task text colors
		// and then handle this in NotifyEndPreferencesUpdate
		case TDCS_COLORTEXTBYPRIORITY:
		case TDCS_COLORTEXTBYATTRIBUTE:
		case TDCS_COLORTEXTBYNONE:
		case TDCS_TREATSUBCOMPLETEDASDONE:
		case TDCS_USEEARLIESTDUEDATE:
		case TDCS_USELATESTDUEDATE:
		case TDCS_USEEARLIESTSTARTDATE:
		case TDCS_USELATESTSTARTDATE:
		case TDCS_USEHIGHESTPRIORITY:
		case TDCS_INCLUDEDONEINPRIORITYCALC:
		case TDCS_DUEHAVEHIGHESTPRIORITY:
		case TDCS_DONEHAVELOWESTPRIORITY:
		case TDCS_TASKCOLORISBACKGROUND:
			m_bTaskColorChange = TRUE;
			break;
		}

		return TRUE;
	}

	return FALSE;
}

BOOL CTabbedToDoCtrl::SetPriorityColors(const CDWordArray& aColors)
{
	CDWordArray aPriorityExist;
	aPriorityExist.Copy(m_aPriorityColors);

	if (CToDoCtrl::SetPriorityColors(aColors))
	{
		if (!m_bTaskColorChange)
			m_bTaskColorChange = !Misc::MatchAllT(aColors, m_aPriorityColors);

		return TRUE;
	}

	// else
	return FALSE;
}

void CTabbedToDoCtrl::SetCompletedTaskColor(COLORREF color)
{
	if (!m_bTaskColorChange)
		m_bTaskColorChange = (color != m_crDone);

	CToDoCtrl::SetCompletedTaskColor(color);
}

void CTabbedToDoCtrl::SetFlaggedTaskColor(COLORREF color)
{
	if (!m_bTaskColorChange)
		m_bTaskColorChange = (color != m_crFlagged);

	CToDoCtrl::SetFlaggedTaskColor(color);
}

void CTabbedToDoCtrl::SetReferenceTaskColor(COLORREF color)
{
	if (!m_bTaskColorChange)
		m_bTaskColorChange = (color != m_crReference);

	CToDoCtrl::SetReferenceTaskColor(color);
}

void CTabbedToDoCtrl::SetAttributeColors(TDC_ATTRIBUTE nAttrib, const CTDCColorMap& colors)
{
	if (!m_bTaskColorChange)
	{
		m_bTaskColorChange = (colors.GetCount() != m_mapAttribColors.GetCount());

		if (!m_bTaskColorChange)
		{
			CString sItem;
			COLORREF crItem, crExist;
			POSITION pos = colors.GetStartPosition();

			while (pos && !m_bTaskColorChange)
			{
				colors.GetNextAssoc(pos, sItem, crItem);
				m_bTaskColorChange = (!m_mapAttribColors.Lookup(sItem, crExist) || (crItem != crExist));
			}
		}
	}

	CToDoCtrl::SetAttributeColors(nAttrib, colors);
}

void CTabbedToDoCtrl::SetStartedTaskColors(COLORREF crStarted, COLORREF crStartedToday)
{
	if (!m_bTaskColorChange)
		m_bTaskColorChange = (crStarted != m_crStarted || crStartedToday != m_crStartedToday);

	CToDoCtrl::SetStartedTaskColors(crStarted, crStartedToday);
}

void CTabbedToDoCtrl::SetDueTaskColors(COLORREF crDue, COLORREF crDueToday)
{
	if (!m_bTaskColorChange)
		m_bTaskColorChange = (crDue != m_crDue || crDueToday != m_crDueToday);

	CToDoCtrl::SetDueTaskColors(crDue, crDueToday);
}

void CTabbedToDoCtrl::NotifyBeginPreferencesUpdate()
{
	// base class
	CToDoCtrl::NotifyBeginPreferencesUpdate();

	// nothing else for us to do
}

void CTabbedToDoCtrl::NotifyEndPreferencesUpdate()
{
	// base class
	CToDoCtrl::NotifyEndPreferencesUpdate();

	// notify extension windows
	if (HasAnyExtensionViews())
	{
		// we need to update in 2 ways:
		// 1. Tell the extensions that application settings have changed
		// 2. Refresh tasks if their text colour may have changed
		CPreferences prefs;
		CString sKey = GetPreferencesKey(_T("UIExtensions"));
		
		int nExt = m_aExtViews.GetSize();
		FTC_VIEW nCurView = GetView();

		while (nExt--)
		{
			FTC_VIEW nExtView = (FTC_VIEW)(FTCV_FIRSTUIEXTENSION + nExt);
			IUIExtensionWindow* pExtWnd = m_aExtViews[nExt];
			
			if (pExtWnd)
			{
				VIEWDATA* pData = GetViewData(nExtView);

				if (!pData)
					continue;

				// if this extension is active and wants a 
				// color update we want to start progress
				BOOL bWantColorUpdate = (m_bTaskColorChange && pExtWnd->WantUpdate(TDCA_COLOR));

				if (bWantColorUpdate && nExtView == nCurView)
					BeginExtensionProgress(pData);

				// notify all extensions of prefs change
				pExtWnd->LoadPreferences(&prefs, sKey, TRUE);

				// Update task colours if necessary
				if (bWantColorUpdate)
				{
					if (nExtView == nCurView)
					{
						TDCGETTASKS filter;
						filter.aAttribs.Add(TDCA_COLOR);
						
						CTaskFile tasks;
						GetTasks(tasks, filter);
						
						UpdateExtensionView(pExtWnd, tasks, IUI_EDIT, TDCA_COLOR);
						pData->bNeedTaskUpdate = FALSE;
					}
					else // mark for update
					{
						pData->bNeedTaskUpdate = TRUE;
					}
				}

				// cleanup progress
				if (bWantColorUpdate && nExtView == nCurView)
					EndExtensionProgress();
			}
		}
	}

	m_bTaskColorChange = FALSE;
}

void CTabbedToDoCtrl::BeginExtensionProgress(const VIEWDATA* pData, UINT nMsg)
{
	ASSERT(pData);

	if (nMsg == 0)
		nMsg = IDS_UPDATINGTABBEDVIEW;

	CEnString sMsg(nMsg, pData->pExtension->GetMenuText());
	GetParent()->SendMessage(WM_TDCM_LENGTHYOPERATION, TRUE, (LPARAM)(LPCTSTR)sMsg);
}

void CTabbedToDoCtrl::EndExtensionProgress()
{
	GetParent()->SendMessage(WM_TDCM_LENGTHYOPERATION, FALSE);
}

CString CTabbedToDoCtrl::GetControlDescription(const CWnd* pCtrl) const
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		break; // handled below

	case FTCV_TASKLIST:
		if (pCtrl == &m_list)
			return CEnString(IDS_LISTVIEW);
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		if (pCtrl)
		{
			HWND hwnd = m_tabViews.GetViewHwnd(nView);

			if (CDialogHelper::IsChildOrSame(hwnd, pCtrl->GetSafeHwnd()))
			{
				return m_tabViews.GetViewName(nView);
			}
		}
		break;

	default:
		ASSERT(0);
	}

	return CToDoCtrl::GetControlDescription(pCtrl);
}

BOOL CTabbedToDoCtrl::DeleteSelectedTask(BOOL bWarnUser, BOOL bResetSel)
{
	BOOL bDel = CToDoCtrl::DeleteSelectedTask(bWarnUser, bResetSel);

	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		// handled above
		break;

	case FTCV_TASKLIST:
		if (bDel)
		{
			// work out what to select
			DWORD dwSelID = GetSelectedTaskID();
			int nSel = FindListTask(dwSelID);

			if (nSel == -1 && m_list.GetItemCount())
				nSel = 0;

			if (nSel != -1)
				SelectTask(m_list.GetItemData(nSel));
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}

	return bDel;
}

BOOL CTabbedToDoCtrl::SelectedTasksHaveChildren() const
{
	return CToDoCtrl::SelectedTasksHaveChildren();
}

HTREEITEM CTabbedToDoCtrl::CreateNewTask(const CString& sText, TDC_INSERTWHERE nWhere, BOOL bEditText)
{
	HTREEITEM htiNew = NULL;
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		htiNew = CToDoCtrl::CreateNewTask(sText, nWhere, bEditText);
		break;

	case FTCV_TASKLIST:
		htiNew = CToDoCtrl::CreateNewTask(sText, nWhere, FALSE); // note FALSE

		if (htiNew)
		{
			DWORD dwTaskID = GetTaskID(htiNew);
			ASSERT(dwTaskID == m_dwNextUniqueID - 1);
			
			// make the new task appear
			RebuildList(NULL); 
			
			// select that task
			SelectTask(dwTaskID);
			
			if (bEditText)
			{
				m_dwLastAddedID = dwTaskID;
				EditSelectedTask(TRUE);
			}
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		htiNew = CToDoCtrl::CreateNewTask(sText, nWhere, FALSE); // note FALSE

		if (htiNew)
		{
			DWORD dwTaskID = GetTaskID(htiNew);
			ASSERT(dwTaskID == m_dwNextUniqueID - 1);
			
			// select that task
			SelectTask(dwTaskID);
			
			if (bEditText)
			{
				m_dwLastAddedID = dwTaskID;
				EditSelectedTask(TRUE);
			}
		}
		break;

	default:
		ASSERT(0);
	}

	return htiNew;
}

TODOITEM* CTabbedToDoCtrl::CreateNewTask(HTREEITEM htiParent)
{
	TODOITEM* pTDI = CToDoCtrl::CreateNewTask(htiParent);
	ASSERT(pTDI);

	// give active extension view a chance to initialise
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		{
			IUIExtensionWindow* pExtWnd = GetExtensionWnd(nView);
			ASSERT(pExtWnd);

			if (pExtWnd)
			{
				CTaskFile task;
				HTASKITEM hTask = task.NewTask(pTDI->sTitle);

				task.SetTaskAttributes(hTask, pTDI);

				if (pExtWnd->PrepareNewTask(&task))
					task.GetTaskAttributes(hTask, pTDI);

				// fall thru
			}
		}
		break;
	}

	return pTDI;
}

BOOL CTabbedToDoCtrl::CanCreateNewTask(TDC_INSERTWHERE nInsertWhere) const
{
	FTC_VIEW nView = GetView();

	BOOL bCanCreate = CToDoCtrl::CanCreateNewTask(nInsertWhere);

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
	case FTCV_TASKLIST:
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		{
			const VIEWDATA* pData = GetViewData(nView);

			if (pData)
				bCanCreate &= pData->bCanPrepareNewTask;
		}
		break;

	default:
		bCanCreate = FALSE;
		ASSERT(0);
		break;
	}

	return bCanCreate;
}

void CTabbedToDoCtrl::RebuildList(const void* pContext)
{
	if (!m_data.GetTaskCount())
	{
		m_list.DeleteAllItems(); 
		return;
	}

	// cache current selection
	TDCSELECTIONCACHE cache;
	CacheListSelection(cache);

	// note: the call to RestoreListSelection at the bottom fails if the 
	// list has redraw disabled so it must happen outside the scope of hr2
	{
		CHoldRedraw hr(GetSafeHwnd());
		CHoldRedraw hr2(m_list);
		CWaitCursor cursor;

		// remove all existing items
		m_list.DeleteAllItems();
		
		// rebuild the list from the tree
		AddTreeItemToList(NULL, pContext);

		// redo last sort
		FTC_VIEW nView = GetView();

		switch (nView)
		{
		case FTCV_TASKTREE:
		case FTCV_UNSET:
			break;

		case FTCV_TASKLIST:
			if (IsSorting())
			{
				GetViewData(FTCV_TASKLIST)->bNeedResort = FALSE;
				Resort();
			}
			break;

		case FTCV_UIEXTENSION1:
		case FTCV_UIEXTENSION2:
		case FTCV_UIEXTENSION3:
		case FTCV_UIEXTENSION4:
		case FTCV_UIEXTENSION5:
		case FTCV_UIEXTENSION6:
		case FTCV_UIEXTENSION7:
		case FTCV_UIEXTENSION8:
		case FTCV_UIEXTENSION9:
		case FTCV_UIEXTENSION10:
		case FTCV_UIEXTENSION11:
		case FTCV_UIEXTENSION12:
		case FTCV_UIEXTENSION13:
		case FTCV_UIEXTENSION14:
		case FTCV_UIEXTENSION15:
		case FTCV_UIEXTENSION16:
			break;

		default:
			ASSERT(0);
		}
	}
	
	// restore selection
	RestoreListSelection(cache);
	
	// don't update controls is only one item is selected and it did not
	// change as a result of the filter
	BOOL bSelChange = !(GetSelectedCount() == 1 && 
		cache.aSelTaskIDs.GetSize() == 1 &&
		GetTaskID(GetSelectedItem()) == cache.aSelTaskIDs[0]);
	
	if (bSelChange)
		UpdateControls();

	UpdateListColumnWidths();
}

int CTabbedToDoCtrl::AddItemToList(DWORD dwTaskID)
{
	// omit task references from list
	if (CToDoCtrl::IsTaskReference(dwTaskID))
		return -1;

	// else
	return m_list.InsertItem(LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, 
							m_list.GetItemCount(), 
							LPSTR_TEXTCALLBACK, 
							0, // nState
							0, // nStateMask
							I_IMAGECALLBACK, 
							dwTaskID);
}

void CTabbedToDoCtrl::AddTreeItemToList(HTREEITEM hti, const void* pContext)
{
	// add task
	if (hti)
	{
		// if the add fails then it's a task reference
		if (CTabbedToDoCtrl::AddItemToList(GetTaskID(hti)) == -1)
		{
			return; 
		}
	}

	// children
	HTREEITEM htiChild = m_tree.GetChildItem(hti);

	while (htiChild)
	{
		AddTreeItemToList(htiChild, pContext);
		htiChild = m_tree.GetNextItem(htiChild, TVGN_NEXT);
	}
}

void CTabbedToDoCtrl::SetExtensionsNeedUpdate(BOOL bUpdate, FTC_VIEW nIgnore)
{
	for (int nExt = 0; nExt < m_aExtViews.GetSize(); nExt++)
	{
		FTC_VIEW nView = (FTC_VIEW)(FTCV_UIEXTENSION1 + nExt);
		
		if (nView == nIgnore)
			continue;
		
		// else
		VIEWDATA* pData = GetViewData(nView);
		
		if (pData)
			pData->bNeedTaskUpdate = bUpdate;
	}
}

void CTabbedToDoCtrl::SetModified(BOOL bMod, TDC_ATTRIBUTE nAttrib, DWORD dwModTaskID)
{
	CToDoCtrl::SetModified(bMod, nAttrib, dwModTaskID);

	if (bMod)
	{
		FTC_VIEW nView = GetView();
		
		switch (nView)
		{
		case FTCV_TASKLIST:
			{
				switch (nAttrib)
				{
				case TDCA_DELETE:
				case TDCA_ARCHIVE:
					RemoveDeletedListItems();
					break;
					
				case TDCA_NEWTASK:
				case TDCA_UNDO:
				case TDCA_PASTE:
					RebuildList(NULL);
					break;
					
				default: // all other attributes
					m_list.Invalidate(FALSE);
				}
			}
			break;
			
		case FTCV_TASKTREE:
		case FTCV_UNSET:
			GetViewData(FTCV_TASKLIST)->bNeedTaskUpdate = TRUE;
			break;
			
		case FTCV_UIEXTENSION1:
		case FTCV_UIEXTENSION2:
		case FTCV_UIEXTENSION3:
		case FTCV_UIEXTENSION4:
		case FTCV_UIEXTENSION5:
		case FTCV_UIEXTENSION6:
		case FTCV_UIEXTENSION7:
		case FTCV_UIEXTENSION8:
		case FTCV_UIEXTENSION9:
		case FTCV_UIEXTENSION10:
		case FTCV_UIEXTENSION11:
		case FTCV_UIEXTENSION12:
		case FTCV_UIEXTENSION13:
		case FTCV_UIEXTENSION14:
		case FTCV_UIEXTENSION15:
		case FTCV_UIEXTENSION16:
			// handled below
			break;
			
		default:
			ASSERT(0);
		}
		
		UpdateExtensionViews(nAttrib, dwModTaskID);
	}
}

BOOL CTabbedToDoCtrl::ModCausesColorChange(TDC_ATTRIBUTE nModType) const
{
	switch (nModType)
	{
	case TDCA_COLOR:
		return !HasStyle(TDCS_COLORTEXTBYPRIORITY) &&
				!HasStyle(TDCS_COLORTEXTBYATTRIBUTE) &&
				!HasStyle(TDCS_COLORTEXTBYNONE);

	case TDCA_CATEGORY:
	case TDCA_ALLOCBY:
	case TDCA_ALLOCTO:
	case TDCA_STATUS:
	case TDCA_VERSION:
	case TDCA_EXTERNALID:
	case TDCA_TAGS:
		return (HasStyle(TDCS_COLORTEXTBYATTRIBUTE) && m_nColorByAttrib == nModType);

	case TDCA_DONEDATE:
		return (m_crDone != CLR_NONE);

	case TDCA_DUEDATE:
		return (m_crDue != CLR_NONE || m_crDueToday != CLR_NONE);

	case TDCA_PRIORITY:
		return HasStyle(TDCS_COLORTEXTBYPRIORITY);
	}

	// all else
	return FALSE;
}

void CTabbedToDoCtrl::UpdateExtensionViews(TDC_ATTRIBUTE nAttrib, DWORD dwTaskID)
{
	if (!HasAnyExtensionViews())
		return;

	FTC_VIEW nCurView = GetView();

	switch (nAttrib)
	{
	// for a simple attribute change (or addition) update all extensions
	// at the same time so that they won't need updating when the user switches view
	case TDCA_TASKNAME:
		// Initial edit of new task is special case
		if (m_dwLastAddedID == GetSelectedTaskID())
		{
			UpdateExtensionViews(TDCA_NEWTASK, m_dwLastAddedID); // RECURSIVE CALL
			return;
		}
		// else fall thru

	case TDCA_DONEDATE:
	case TDCA_DUEDATE:
	case TDCA_STARTDATE:
	case TDCA_PRIORITY:
	case TDCA_COLOR:
	case TDCA_ALLOCTO:
	case TDCA_ALLOCBY:
	case TDCA_STATUS:
	case TDCA_CATEGORY:
	case TDCA_TAGS:
	case TDCA_PERCENT:
	case TDCA_TIMEEST:
	case TDCA_TIMESPENT:
	case TDCA_FILEREF:
	case TDCA_COMMENTS:
	case TDCA_FLAG:
	case TDCA_CREATIONDATE:
	case TDCA_CREATEDBY:
	case TDCA_RISK: 
	case TDCA_EXTERNALID: 
	case TDCA_COST: 
	case TDCA_DEPENDENCY: 
	case TDCA_RECURRENCE: 
	case TDCA_VERSION:
		{	
			// Check extensions for this OR a color change
			BOOL bColorChange = ModCausesColorChange(nAttrib);

			if (!AnyExtensionViewWantsChange(nAttrib))
			{
				// if noone wants either we can stop
				if (!bColorChange || !AnyExtensionViewWantsChange(TDCA_COLOR))
					return;
			}

			// note: we need to get 'True' tasks and all their parents
			// because of the possibility of colour changes
			CTaskFile tasks;
			DWORD dwFlags = TDCGSTF_RESOLVEREFERENCES;

			// don't include subtasks unless the completion date changed
			if (nAttrib != TDCA_DONEDATE)
				dwFlags |= TDCGSTF_NOTSUBTASKS;

			if (bColorChange)
				dwFlags |= TDCGSTF_ALLPARENTS;

			// update all tasks if the selected tasks have 
			// dependents and the due dates was modified
			if ((nAttrib == TDCA_DUEDATE) && SelectionHasDependents())
			{
				GetTasks(tasks);
				nAttrib = TDCA_ALL; // so that start date changes get picked up
			}
			else
			{
				CToDoCtrl::GetSelectedTasks(tasks, TDCGT_ALL, dwFlags);
			}
			
			// refresh all extensions 
			int nExt = m_aExtViews.GetSize();
			
			while (nExt--)
			{
				IUIExtensionWindow* pExtWnd = m_aExtViews[nExt];
				
				if (pExtWnd)
				{
					if (ExtensionViewWantsChange(nExt, nAttrib))
					{
						UpdateExtensionView(pExtWnd, tasks, IUI_EDIT, nAttrib);
					}
					else if (bColorChange && ExtensionViewWantsChange(nExt, TDCA_COLOR))
					{
						UpdateExtensionView(pExtWnd, tasks, IUI_EDIT, TDCA_COLOR);
					}
				}

				// clear the update flag for all created extensions
				FTC_VIEW nExtView = (FTC_VIEW)(FTCV_FIRSTUIEXTENSION + nExt);
				VIEWDATA* pData = GetViewData(nExtView);

				if (pData)
					pData->bNeedTaskUpdate = (pExtWnd == NULL);
			}
		}
		break;
		
	// refresh the current view (if it's an extension) 
	// and mark the others as needing updates.
	case TDCA_NEWTASK: 
	case TDCA_DELETE:
	case TDCA_UNDO:
	case TDCA_POSITION: // == move
	case TDCA_PASTE:
	case TDCA_ARCHIVE:
		{
			int nExt = m_aExtViews.GetSize();
			
			while (nExt--)
			{
				FTC_VIEW nView = (FTC_VIEW)(FTCV_FIRSTUIEXTENSION + nExt);
				VIEWDATA* pData = GetViewData(nView);

				if (pData)
				{
					IUIExtensionWindow* pExtWnd = GetExtensionWnd(nView);

					if (pExtWnd && (nView == nCurView))
					{
						BeginExtensionProgress(pData);

						// update all tasks
						CTaskFile tasks;
						GetTasks(tasks);

						IUI_UPDATETYPE nUpdate = IUI_ALL;

						if ((nAttrib == TDCA_DELETE) || (nAttrib == TDCA_ARCHIVE))
							nUpdate = IUI_DELETE;
						
						UpdateExtensionView(pExtWnd, tasks, nUpdate);
						pData->bNeedTaskUpdate = FALSE;

						if ((nAttrib == TDCA_NEWTASK) && dwTaskID)
							pExtWnd->SelectTask(dwTaskID);
						else
							UpdateExtensionViewSelection();

						EndExtensionProgress();
					}
					else
					{
						pData->bNeedTaskUpdate = TRUE;
					}
				}
			}
		}
		break;	
		
	case TDCA_PROJNAME:
	case TDCA_ENCRYPT:
	default:
		// do nothing
		break;
	}
}

BOOL CTabbedToDoCtrl::ExtensionViewWantsChange(int nExt, TDC_ATTRIBUTE nAttrib) const
{
	FTC_VIEW nCurView = GetView();
	FTC_VIEW nExtView = (FTC_VIEW)(FTCV_FIRSTUIEXTENSION + nExt);

	// if the window is not active and is already marked
	// for a full update then we don't need to do
	// anything more because it will get this update when
	// it is next activated
	if (nExtView != nCurView)
	{
		const VIEWDATA* pData = GetViewData(nExtView);

		if (!pData || pData->bNeedTaskUpdate)
			return FALSE;
	}
	else // active view
	{
		// if this update has come about as a consequence
		// of this extension window modifying the selected 
		// task, then we assume that it won't want the update
		if (m_bExtModifyingApp)
			return FALSE;
	}
	
	IUIExtensionWindow* pExtWnd = m_aExtViews[nExt];
	ASSERT(pExtWnd);
	
	return (pExtWnd && pExtWnd->WantUpdate(nAttrib));
}

BOOL CTabbedToDoCtrl::AnyExtensionViewWantsChange(TDC_ATTRIBUTE nAttrib) const
{
	// find the first extension wanting this change
	FTC_VIEW nCurView = GetView();
	int nExt = m_aExtViews.GetSize();
	
	while (nExt--)
	{
		if (ExtensionViewWantsChange(nExt, nAttrib))
			return TRUE;
	}

	// not found
	return FALSE;
}

BOOL CTabbedToDoCtrl::ModNeedsResort(TDC_ATTRIBUTE nModType) const
{
	if (!HasStyle(TDCS_RESORTONMODIFY))
		return FALSE;

	VIEWDATA* pLVData = GetViewData(FTCV_TASKLIST);

	BOOL bListNeedsResort = CToDoCtrl::ModNeedsResort(nModType, pLVData->sort);
	BOOL bTreeNeedsResort = CToDoCtrl::ModNeedsResort(nModType);
	
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		pLVData->bNeedResort |= bListNeedsResort;
		return bTreeNeedsResort;

	case FTCV_TASKLIST:
		m_bTreeNeedResort |= bTreeNeedsResort;
		return bListNeedsResort;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}

	return FALSE;
}

void CTabbedToDoCtrl::ResortSelectedTaskParents() 
{ 
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::ResortSelectedTaskParents();
		break;

	case FTCV_TASKLIST:
		Resort(); // do a full sort
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}
}

TDC_COLUMN CTabbedToDoCtrl::GetSortBy() const 
{ 
	VIEWDATA* pVData = GetActiveViewData();

	return (pVData ? pVData->sort.single.nBy : m_sort.single.nBy); 
}

void CTabbedToDoCtrl::GetSortBy(TDSORTCOLUMNS& sort) const
{
	VIEWDATA* pVData = GetActiveViewData();

	sort = (pVData ? pVData->sort.multi : m_sort.multi);
}

BOOL CTabbedToDoCtrl::SelectTask(DWORD dwTaskID, BOOL bTrue)
{
	BOOL bRes = CToDoCtrl::SelectTask(dwTaskID, bTrue);

	// check task has not been filtered out
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		// handled above
		break;

	case FTCV_TASKLIST:
		{
			int nItem = FindListTask(dwTaskID);

			if (nItem == -1)
			{
				ASSERT(0);
				return FALSE;
			}
			
			// remove focused state from existing task
			int nFocus = m_list.GetNextItem(-1, LVNI_FOCUSED | LVNI_SELECTED);

			if (nFocus != -1)
				m_list.SetItemState(nFocus, 0, LVIS_SELECTED | LVIS_FOCUSED);

			m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			m_list.EnsureVisible(nItem, FALSE);

			ScrollListClientColumnIntoView();
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		{
			IUIExtensionWindow* pExtWnd = GetExtensionWnd(nView);
			ASSERT(pExtWnd);

			if (pExtWnd)
				pExtWnd->SelectTask(dwTaskID);
		}
		break;

	default:
		ASSERT(0);
	}

	return bRes;
}

void CTabbedToDoCtrl::ScrollListClientColumnIntoView()
{
	// check task has not been filtered out
	if (m_list.GetItemCount())
	{
		// scroll client column into view if necessary
		CRect rItem, rClient;
		GetListItemTitleRect(0, TDCTR_EDIT, rItem);
		
		m_list.GetWindowRect(rClient);
		ScreenToClient(rClient);
		
		CSize pos(0, 0);
		
		if (rItem.left < rClient.left)
		{
			// scroll left edge to left client edge
			pos.cx = rItem.left - rClient.left;
		}
		else if (rItem.right > rClient.right)
		{
			// scroll right edge to right client edge so long as
			// left edge is still in view
			int nToScrollRight = rItem.right - rClient.right;
			int nMaxScrollLeft = rItem.left - rClient.left;
			
			pos.cx = min(nMaxScrollLeft, nToScrollRight);
		}
		
		if (pos.cx)
			m_list.Scroll(pos);
	}
}

LRESULT CTabbedToDoCtrl::OnEditCancel(WPARAM wParam, LPARAM lParam)
{
	// check if we need to delete the just added item
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		// handled below
		break;

	case FTCV_TASKLIST:
		// delete the just added task
		if (GetSelectedTaskID() == m_dwLastAddedID)
		{
			int nDelItem = GetFirstSelectedItem();
			m_list.DeleteItem(nDelItem);
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		// delete the just added task from the active view
		if (GetSelectedTaskID() == m_dwLastAddedID)
		{
			LRESULT lr = CToDoCtrl::OnEditCancel(wParam, lParam);
			UpdateExtensionViews(TDCA_DELETE);
		}
		break;

	default:
		ASSERT(0);
	}

	return CToDoCtrl::OnEditCancel(wParam, lParam);
}

int CTabbedToDoCtrl::FindListTask(DWORD dwTaskID) const
{
	if (!dwTaskID)
		return -1;

	LVFINDINFO lvfi;
	ZeroMemory(&lvfi, sizeof(lvfi));

    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = dwTaskID;
    lvfi.vkDirection = VK_DOWN;

	return m_list.FindItem(&lvfi);
}

DWORD CTabbedToDoCtrl::GetFocusedListTaskID() const
{
	int nItem = GetFocusedListItem();

	if (nItem != -1)
		return m_list.GetItemData(nItem);

	// else
	return 0;
}

int CTabbedToDoCtrl::GetFocusedListItem() const
{
	return m_list.GetNextItem(-1, LVNI_FOCUSED | LVNI_SELECTED);
}

void CTabbedToDoCtrl::SetSelectedListTasks(const CDWordArray& aTaskIDs, DWORD dwFocusedTaskID)
{
	ClearListSelection();

	BOOL bFocusHandled = FALSE;

	for (int nTask = 0; nTask < aTaskIDs.GetSize(); nTask++)
	{
		DWORD dwTaskID = aTaskIDs[nTask];
		int nItem = FindListTask(dwTaskID);

		if (nItem != -1)
		{
			if (dwTaskID == dwFocusedTaskID)
			{
				m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				bFocusHandled = TRUE;
			}
			else
				m_list.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
		}
		else // deselect in tree
		{
			HTREEITEM hti = m_find.GetItem(dwTaskID);
			Selection().SetItem(hti, TSHS_DESELECT, FALSE);
		}
	}

	// handle focus if it wasn't in aTasksID
	if (!bFocusHandled)
	{
		int nItem = FindListTask(dwFocusedTaskID);

		if (nItem != -1)
			m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
}

LRESULT CTabbedToDoCtrl::OnGutterWidthChange(WPARAM wParam, LPARAM lParam)
{
	CToDoCtrl::OnGutterWidthChange(wParam, lParam);

	// update column widths if in list view
	UpdateListColumnWidths();
	
	return 0;
}

void CTabbedToDoCtrl::OnListSelChanged(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	*pResult = 0;
}

void CTabbedToDoCtrl::ClearListSelection()
{
	//CHoldRedraw hr2(m_list);

	POSITION pos = m_list.GetFirstSelectedItemPosition();

	while (pos)
	{
		int nItem = m_list.GetNextSelectedItem(pos);
		m_list.SetItemState(nItem, 0, LVIS_SELECTED);
	}
}

int CTabbedToDoCtrl::GetFirstSelectedItem() const
{
	return m_list.GetNextItem(-1, LVNI_SELECTED);
}

int CTabbedToDoCtrl::GetSelectedListTaskIDs(CDWordArray& aTaskIDs, DWORD& dwFocusedTaskID) const
{
	aTaskIDs.RemoveAll();
	aTaskIDs.SetSize(m_list.GetSelectedCount());

	int nCount = 0;
	POSITION pos = m_list.GetFirstSelectedItemPosition();

	while (pos)
	{
		int nItem = m_list.GetNextSelectedItem(pos);

		aTaskIDs[nCount] = m_list.GetItemData(nItem);
		nCount++;
	}

	dwFocusedTaskID = GetFocusedListTaskID();

	return aTaskIDs.GetSize();
}

void CTabbedToDoCtrl::CacheListSelection(TDCSELECTIONCACHE& cache) const
{
	if (GetSelectedListTaskIDs(cache.aSelTaskIDs, cache.dwFocusedTaskID) == 0)
		return;

	cache.dwFirstVisibleTaskID = GetTaskID(m_list.GetTopIndex());
	
	// breadcrumbs
	cache.aBreadcrumbs.RemoveAll();

	// cache the preceding and following 10 tasks
	int nFocus = GetFocusedListItem(), nItem;
	int nMin = 0, nMax = m_list.GetItemCount() - 1;

	nMin = max(nMin, nFocus - 11);
	nMax = min(nMax, nFocus + 11);

	// following tasks first
	for (nItem = (nFocus + 1); nItem <= nMax; nItem++)
		cache.aBreadcrumbs.InsertAt(0, m_list.GetItemData(nItem));

	for (nItem = (nFocus - 1); nItem >= nMin; nItem--)
		cache.aBreadcrumbs.InsertAt(0, m_list.GetItemData(nItem));
}

void CTabbedToDoCtrl::RestoreListSelection(const TDCSELECTIONCACHE& cache)
{
	ClearListSelection();

	if (cache.aSelTaskIDs.GetSize() == 0)
		return;

	DWORD dwFocusedTaskID = cache.dwFocusedTaskID;
	ASSERT(dwFocusedTaskID);

	if (FindListTask(dwFocusedTaskID) == -1)
	{
		dwFocusedTaskID = 0;
		int nID = cache.aBreadcrumbs.GetSize();
		
		while (nID--)
		{
			dwFocusedTaskID = cache.aBreadcrumbs[nID];
			
			if (FindListTask(dwFocusedTaskID) != -1)
				break;
			else
				dwFocusedTaskID = 0;
		}
	}
	
	SetSelectedListTasks(cache.aSelTaskIDs, dwFocusedTaskID);

	// restore pos
	if (cache.dwFirstVisibleTaskID)
		m_list.SetTopIndex(FindListTask(cache.dwFirstVisibleTaskID));
}

BOOL CTabbedToDoCtrl::SetTreeFont(HFONT hFont)
{
	CToDoCtrl::SetTreeFont(hFont);

	if (m_list.GetSafeHwnd())
	{
		if (!hFont) // set to our font
		{
			// for some reason i can not yet explain, our font
			// is not correctly set so we use our parent's font instead
			// hFont = (HFONT)SendMessage(WM_GETFONT);
			hFont = (HFONT)GetParent()->SendMessage(WM_GETFONT);
		}

		HFONT hListFont = (HFONT)m_list.SendMessage(WM_GETFONT);
		BOOL bChange = (hFont != hListFont || !GraphicsMisc::SameFontNameSize(hFont, hListFont));

		if (bChange)
		{
			m_list.SendMessage(WM_SETFONT, (WPARAM)hFont, TRUE);
			RemeasureList();

			if (InListView())
				m_list.Invalidate(TRUE);
		}
	}

	// other views
	// TODO

	return TRUE;
}

BOOL CTabbedToDoCtrl::AddView(IUIExtension* pExtension)
{
	if (!pExtension)
		return FALSE;

	// remove any existing views of this type
	RemoveView(pExtension);

	// add to tab control
	HICON hIcon = pExtension->GetIcon();
	CEnString sName(pExtension->GetMenuText());

	int nIndex = m_aExtViews.GetSize();
	FTC_VIEW nView = (FTC_VIEW)(FTCV_UIEXTENSION1 + nIndex);

	VIEWDATA* pData = NewViewData();
	ASSERT(pData);

	pData->pExtension = pExtension;

	// we pass NULL for the hWnd because we are going to load
	// only on demand
	if (m_tabViews.AttachView(NULL, nView, sName, hIcon, pData))
	{
		m_aExtViews.Add(NULL); // placeholder
		return TRUE;
	}

	return FALSE;
}

BOOL CTabbedToDoCtrl::RemoveView(IUIExtension* pExtension)
{
	// search for any views having this type
	int nView = m_aExtViews.GetSize();

	while (nView--)
	{
		IUIExtensionWindow* pExtWnd = m_aExtViews[nView];

		if (pExtWnd) // can be NULL
		{
			CString sExtType = pExtension->GetTypeID();
			CString sExtWndType = pExtWnd->GetTypeID();

			if (sExtType == sExtWndType)
			{
				VERIFY (m_tabViews.DetachView(pExtWnd->GetHwnd()));
				pExtWnd->Release();

				m_aExtViews.RemoveAt(nView);

				return TRUE;
			}
		}
	}

	return FALSE;
}

void CTabbedToDoCtrl::OnTabCtrlRClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	CMenu menu;

	if (menu.LoadMenu(IDR_TASKVIEWVISIBILITY))
	{
		CMenu* pPopup = menu.GetSubMenu(0);
		CPoint ptCursor(GetMessagePos());

		// prepare list view
		// NOTE: task tree is already prepared
		pPopup->CheckMenuItem(ID_SHOWVIEW_LISTVIEW, IsListViewTabShowing() ? MF_CHECKED : 0);

		// extension views
		CStringArray aTypeIDs;
		GetVisibleExtensionViews(aTypeIDs);

		CTDCUIExtensionHelper::PrepareViewVisibilityMenu(pPopup, m_mgrUIExt, aTypeIDs);

		UINT nCmdID = ::TrackPopupMenu(*pPopup, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON, 
										ptCursor.x, ptCursor.y, 0, GetSafeHwnd(), NULL);

		m_tabViews.Invalidate(FALSE);
		m_tabViews.UpdateWindow();

		switch (nCmdID)
		{
		case ID_SHOWVIEW_TASKTREE:
			ASSERT(0); // this is not accessible
			break;
			
		case ID_SHOWVIEW_LISTVIEW:
			ShowListViewTab(!IsListViewTabShowing());
			break;
			
		default:
			if (CTDCUIExtensionHelper::ProcessViewVisibilityMenuCmd(nCmdID, m_mgrUIExt, aTypeIDs))
				SetVisibleExtensionViews(aTypeIDs);
			break;
		}
	}
	
	*pResult = 0;
}

void CTabbedToDoCtrl::OnListClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMIA = (LPNMITEMACTIVATE)pNMHDR;

	UpdateTreeSelection();

	if (pNMIA->iItem != -1) // validate item
	{
		int nHit = pNMIA->iItem;
		TDC_COLUMN nCol = GetListColumnID(pNMIA->iSubItem);

		DWORD dwClickID = m_list.GetItemData(nHit);
		HTREEITEM htiHit = m_find.GetItem(dwClickID);
		ASSERT(htiHit);

		// column specific handling
		BOOL bCtrl = Misc::KeyIsPressed(VK_CONTROL);

		switch (nCol)
		{
		case TDCC_CLIENT:
			if (dwClickID == m_dw2ndClickItem)
				m_list.PostMessage(LVM_EDITLABEL);
			else
			{
				CRect rCheck;
								
				if (GetListItemTitleRect(nHit, TDCTR_CHECKBOX, rCheck))
				{
					CPoint ptHit(::GetMessagePos());
					m_list.ScreenToClient(&ptHit);

					if (rCheck.PtInRect(ptHit))
					{
						BOOL bDone = m_data.IsTaskDone(dwClickID);
						SetSelectedTaskDone(!bDone);
					}
				}
			}
			break;
			
		case TDCC_FILEREF:
			if (bCtrl)
			{
				CString sFile = m_data.GetTaskFileRef(dwClickID);
				
				if (!sFile.IsEmpty())
					GotoFile(sFile, TRUE);
			}
			break;
			
		case TDCC_DEPENDENCY:
			if (bCtrl)
			{
				CStringArray aDepends;
				m_data.GetTaskDependencies(dwClickID, aDepends);
				
				if (aDepends.GetSize())
					ShowTaskLink(aDepends[0]);
			}
			break;
			
		case TDCC_TRACKTIME:
			if (!IsReadOnly())
			{
				if (GetSelectedCount() == 1 && IsItemSelected(nHit) && m_data.IsTaskTimeTrackable(dwClickID))
				{
					int nPrev = FindListTask(m_dwTimeTrackTaskID);

					if (nPrev == -1)
					{
						m_list.RedrawItems(nPrev, nPrev);
						m_list.UpdateWindow();
					}

					TimeTrackTask(htiHit);
					m_list.RedrawItems(nHit, nHit);
					m_list.UpdateWindow();

					// resort if required
					const VIEWDATA* pLVData = GetViewData(FTCV_TASKLIST);

					if (pLVData->sort.IsSortingBy(TDCC_TRACKTIME, FALSE))
						Sort(TDCC_TRACKTIME, FALSE);
				}
			}
			break;
			
		case TDCC_DONE:
			if (!IsReadOnly())
				SetSelectedTaskDone(!m_data.IsTaskDone(dwClickID));
			break;
			
		case TDCC_FLAG:
			if (!IsReadOnly())
			{
				BOOL bFlagged = m_data.IsTaskFlagged(dwClickID);
				SetSelectedTaskFlag(!bFlagged);
			}
			break;

		default:
			// handle clicks on 'flag' custom attributes
			if (HandleCustomColumnClick(nCol))
				return;
		}
	}

	m_dw2ndClickItem = 0;
	
	*pResult = 0;
}

void CTabbedToDoCtrl::OnListDblClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMIA = (LPNMITEMACTIVATE)pNMHDR;

	if (pNMIA->iItem != -1) // validate item
	{
		DWORD dwTaskID = GetTaskID(pNMIA->iItem);
		TDC_COLUMN nCol = GetListColumnID(pNMIA->iSubItem);
		
		switch (nCol)
		{
		case TDCC_CLIENT:
			{
				// where did they double-click?
				CRect rItem;
				
				if (GetListItemTitleRect(pNMIA->iItem, TDCTR_ICON, rItem) && rItem.PtInRect(pNMIA->ptAction))
					EditSelectedTaskIcon();
				else
				{
					CClientDC dc(&m_list);
					GetListItemTitleRect(pNMIA->iItem, TDCTR_LABEL, rItem, &dc, GetSelectedTaskTitle());
					
					if (rItem.PtInRect(pNMIA->ptAction))
						m_list.PostMessage(LVM_EDITLABEL);
				}
			}
			break;

		case TDCC_FILEREF:
			{
				CString sFile = m_data.GetTaskFileRef(dwTaskID);
				
				if (!sFile.IsEmpty())
					GotoFile(sFile, TRUE);
			}
			break;
			
		case TDCC_DEPENDENCY:
			{
				CStringArray aDepends;
				m_data.GetTaskDependencies(dwTaskID, aDepends);
				
				if (aDepends.GetSize())
					ShowTaskLink(aDepends[0]);
			}
			break;
						
		case TDCC_RECURRENCE:
			m_eRecurrence.DoEdit();
			break;
					
		case TDCC_ICON:
			EditSelectedTaskIcon();
			break;
			
		case TDCC_REMINDER:
			AfxGetMainWnd()->SendMessage(WM_TDCN_DOUBLECLKREMINDERCOL);
			break;
		}
	}
	
	*pResult = 0;
}

void CTabbedToDoCtrl::OnListKeyDown(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	// for reasons I have not yet divined, we are not receiving this message
	// as expected. So I've added an ASSERT(0) should it ever come to life
	// and have handled the key down message in ScWindowProc
	//LPNMKEY pNMK = (LPNMKEY)pNMHDR;
	ASSERT(0);
//	TRACE ("CTabbedToDoCtrl::OnListKeyDown\n");
//	UpdateTreeSelection();

	*pResult = 0;
}

TDC_HITTEST CTabbedToDoCtrl::HitTest(const CPoint& ptScreen) const
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::HitTest(ptScreen);

	case FTCV_TASKLIST:
		{
			if (m_list.PtInHeader(ptScreen))
				return TDCHT_COLUMNHEADER;
			
			// then list client rect
			CPoint ptList(ptScreen);
			m_list.ScreenToClient(&ptList);

			CRect rList;
			m_list.GetClientRect(rList);

			if (!rList.PtInRect(ptList))
				return TDCHT_NOWHERE;

			// then hit task
			int nHit = m_list.HitTest(ptList);

			return (nHit >= 0 ? TDCHT_TASK : TDCHT_TASKLIST);
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		{
			IUIExtensionWindow* pExt = GetExtensionWnd(nView);
			ASSERT(pExt);

			if (pExt)
			{
				IUI_HITTEST nHit = pExt->HitTest(ptScreen);

				switch (nHit)
				{
				case IUI_TASKLIST:		return TDCHT_TASKLIST;
				case IUI_COLUMNHEADER:	return TDCHT_COLUMNHEADER;
				case IUI_TASK:			return TDCHT_TASK;

				case IUI_NOWHERE:
				default: // fall thru
					break;
				}
			}
		}
		break;

	default:
		ASSERT(0);
	}

	// else
	return TDCHT_NOWHERE;
}

TDC_COLUMN CTabbedToDoCtrl::ColumnHitTest(const CPoint& ptScreen) const
{
	return CToDoCtrl::ColumnHitTest(ptScreen);
}

BOOL CTabbedToDoCtrl::IsSorting() const
{
	FTC_VIEW nView = GetView();
	
	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::IsSorting();
		
	case FTCV_TASKLIST:
		// handled at end
		break;
		
	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		// handled at end
		break;
		
	default:
		ASSERT(0);
	}
	
	// all else
	return GetActiveViewData()->sort.IsSorting();
}

BOOL CTabbedToDoCtrl::IsMultiSorting() const
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::IsMultiSorting();

	case FTCV_TASKLIST:
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}

	// all else
	const TDSORT& sort = GetActiveViewData()->sort;
	
	return (sort.bMulti && sort.multi.IsSorting());
}

void CTabbedToDoCtrl::MultiSort(const TDSORTCOLUMNS& sort)
{
	ASSERT (sort.IsSorting());

	if (!sort.IsSorting())
		return;

	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::MultiSort(sort);
		break;

	case FTCV_TASKLIST:
		{
			VIEWDATA* pLVData = GetViewData(FTCV_TASKLIST);
			
			pLVData->sort.bModSinceLastSort = FALSE;
			pLVData->sort.SetSortBy(sort);
			
			TDSORTPARAMS ss(*this, pLVData->sort);

			ss.bSortChildren = FALSE;
			ss.dwTimeTrackID = 0;

			// we only sort the due today tasks high if the user has assigned them a colour
			ss.bSortDueTodayHigh = (m_crDueToday != CLR_NONE);

			m_list.SortItems(CToDoCtrl::SortFuncMulti, (DWORD)&ss);
			
			// update registry
			SaveSortState(CPreferences());

			UpdateListColumnWidths();
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}
}

void CTabbedToDoCtrl::Sort(TDC_COLUMN nBy, BOOL bAllowToggle)
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::Sort(nBy, bAllowToggle);
		break;

	case FTCV_TASKLIST:
		{
			VIEWDATA* pLVData = GetViewData(FTCV_TASKLIST);
			BOOL bAscending = pLVData->sort.single.bAscending;
			
			if (nBy != TDCC_NONE)
			{
				// first time?
				if ((bAscending == -1) || !pLVData->sort.single.IsSortingBy(nBy)) 
				{
					int nCol = GetListColumnIndex(nBy);
					TDCCOLUMN* pTDCC = GetListColumn(nCol);

					if (pTDCC)
					{
						bAscending = pTDCC->bSortAscending;
					}
					else if (CTDCCustomAttributeHelper::IsCustomColumn(nBy))
					{
						bAscending = TRUE;
					}
				}
				// if there's been a mod since last sorting then its reasonable to assume
				// that the user is not toggling direction but wants to simply resort
				// in the same direction.
				else if (bAllowToggle && !pLVData->sort.bModSinceLastSort)
				{
					bAscending = !bAscending;
				}
			}
			
			pLVData->sort.SetSortBy(nBy, bAscending);
			pLVData->sort.bModSinceLastSort = FALSE;
			
			// do the sort using whatever we can out of CToDoCtrlData
			TDSORTPARAMS ss(*this, pLVData->sort);

			ss.bSortChildren = FALSE;
			ss.dwTimeTrackID = m_dwTimeTrackTaskID;

			m_list.SortItems(CToDoCtrl::SortFunc, (DWORD)&ss);
			
			// update registry
			SaveSortState(CPreferences());

			UpdateListColumnWidths();
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		ExtensionDoAppCommand(nView, bAllowToggle ? IUI_TOGGLABLESORT : IUI_SORT);
		break;

	default:
		ASSERT(0);
	}
}

BOOL CTabbedToDoCtrl::MoveSelectedTask(TDC_MOVETASK nDirection) 
{ 
	return !InTreeView() ? FALSE : CToDoCtrl::MoveSelectedTask(nDirection); 
}

BOOL CTabbedToDoCtrl::CanMoveSelectedTask(TDC_MOVETASK nDirection) const 
{ 
	return !InTreeView() ? FALSE : CToDoCtrl::CanMoveSelectedTask(nDirection); 
}

BOOL CTabbedToDoCtrl::GotoNextTask(TDC_GOTO nDirection)
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::GotoNextTask(nDirection);

	case FTCV_TASKLIST:
		if (CanGotoNextTask(nDirection))
		{
			int nSel = GetFirstSelectedItem();

			if (nDirection == TDCG_NEXT)
				nSel++;
			else
				nSel--;

			return SelectTask(m_list.GetItemData(nSel));
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}
	
	// else
	return FALSE;
}

CRect CTabbedToDoCtrl::GetSelectedItemsRect() const
{
	CRect rInvalid(0, 0, 0, 0), rItem;
	POSITION pos = m_list.GetFirstSelectedItemPosition();

	while (pos)
	{
		int nItem = m_list.GetNextSelectedItem(pos);

		if (m_list.GetItemRect(nItem, rItem, LVIR_BOUNDS))
			rInvalid |= rItem;
	}

	return rInvalid;
}

BOOL CTabbedToDoCtrl::CanGotoNextTask(TDC_GOTO nDirection) const
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::CanGotoNextTask(nDirection);

	case FTCV_TASKLIST:
		{
			int nSel = GetFirstSelectedItem();

			if (nDirection == TDCG_NEXT)
				return (nSel >= 0 && nSel < m_list.GetItemCount() - 1);
			
			// else prev
			return (nSel > 0 && nSel <= m_list.GetItemCount() - 1);
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}
	
	// else
	return FALSE;
}

BOOL CTabbedToDoCtrl::GotoNextTopLevelTask(TDC_GOTO nDirection)
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::GotoNextTopLevelTask(nDirection);

	case FTCV_TASKLIST:
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}

	// else
	return FALSE; // not supported
}

BOOL CTabbedToDoCtrl::CanGotoNextTopLevelTask(TDC_GOTO nDirection) const
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::CanGotoNextTopLevelTask(nDirection);

	case FTCV_TASKLIST:
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}

	// else
	return FALSE; // not supported
}

void CTabbedToDoCtrl::ExpandTasks(TDC_EXPANDCOLLAPSE nWhat, BOOL bExpand)
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::ExpandTasks(nWhat, bExpand);

	case FTCV_TASKLIST:
		// no can do!
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		if (bExpand)
		{
			switch (nWhat)
			{
			case TDCEC_ALL:
				ExtensionDoAppCommand(nView, IUI_EXPANDALL);
				break;

			case TDCEC_SELECTED:
				ExtensionDoAppCommand(nView, IUI_EXPANDSELECTED);
				break;
			}
		}
		else // collapse
		{
			switch (nWhat)
			{
			case TDCEC_ALL:
				ExtensionDoAppCommand(nView, IUI_COLLAPSEALL);
				break;

			case TDCEC_SELECTED:
				ExtensionDoAppCommand(nView, IUI_COLLAPSESELECTED);
				break;
			}
		}
		break;

	default:
		ASSERT(0);
	}
}

BOOL CTabbedToDoCtrl::CanExpandTasks(TDC_EXPANDCOLLAPSE nWhat, BOOL bExpand) const 
{ 
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::CanExpandTasks(nWhat, bExpand);

	case FTCV_TASKLIST:
		// no can do!
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		if (bExpand)
		{
			switch (nWhat)
			{
			case TDCEC_ALL:
				return ExtensionCanDoAppCommand(nView, IUI_EXPANDALL);

			case TDCEC_SELECTED:
				return ExtensionCanDoAppCommand(nView, IUI_EXPANDSELECTED);
			}
		}
		else // collapse
		{
			switch (nWhat)
			{
			case TDCEC_ALL:
				return ExtensionCanDoAppCommand(nView, IUI_COLLAPSEALL);

			case TDCEC_SELECTED:
				return ExtensionCanDoAppCommand(nView, IUI_COLLAPSESELECTED);
			}
		}
		break;

	default:
		ASSERT(0);
	}

	// else
	return FALSE; // not supported
}

void CTabbedToDoCtrl::ExtensionDoAppCommand(FTC_VIEW nView, IUI_APPCOMMAND nCmd, DWORD dwExtra)
{
	IUIExtensionWindow* pExt = GetExtensionWnd(nView, FALSE);
	ASSERT(pExt);

	if (pExt)
		pExt->DoAppCommand(nCmd, dwExtra);
}

BOOL CTabbedToDoCtrl::ExtensionCanDoAppCommand(FTC_VIEW nView, IUI_APPCOMMAND nCmd, DWORD dwExtra) const
{
	const IUIExtensionWindow* pExt = GetExtensionWnd(nView);
	ASSERT(pExt);

	if (pExt)
		return pExt->CanDoAppCommand(nCmd, dwExtra);

	return FALSE;
}

void CTabbedToDoCtrl::SetFocusToTasks()
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::SetFocusToTasks();
		break;

	case FTCV_TASKLIST:
		if (GetFocus() != &m_list)
		{
			// See CToDoCtrl::SetFocusToTasks() for why 
			// we need this call
			SetFocusToComments();

			m_list.SetFocus();
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		// See CToDoCtrl::SetFocusToTasks() for why 
		// we need this call
		SetFocusToComments();

		ExtensionDoAppCommand(nView, IUI_SETFOCUS);
		break;

	default:
		ASSERT(0);
	}
}

BOOL CTabbedToDoCtrl::TasksHaveFocus() const
{ 
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::TasksHaveFocus(); 

	case FTCV_TASKLIST:
		return (::GetFocus() == m_list);

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		return ExtensionCanDoAppCommand(nView, IUI_SETFOCUS);

	default:
		ASSERT(0);
	}
	
	return FALSE;
}

int CTabbedToDoCtrl::FindTasks(const SEARCHPARAMS& params, CResultArray& aResults) const
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::FindTasks(params, aResults);

	case FTCV_TASKLIST:
		{
			for (int nItem = 0; nItem < m_list.GetItemCount(); nItem++)
			{
				DWORD dwTaskID = GetTaskID(nItem);
				SEARCHRESULT result;

				if (m_data.TaskMatches(dwTaskID, params, result))
					aResults.Add(result);
			}
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}

	return aResults.GetSize();
}


BOOL CTabbedToDoCtrl::SelectTask(CString sPart, TDC_SELECTTASK nSelect)
{
	int nFind = -1;
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::SelectTask(sPart, nSelect);

	case FTCV_TASKLIST:
		switch (nSelect)
		{
		case TDC_SELECTFIRST:
			nFind = FindListTask(sPart);
			break;
			
		case TDC_SELECTNEXT:
			nFind = FindListTask(sPart, GetFirstSelectedItem() + 1);
			break;
			
		case TDC_SELECTNEXTINCLCURRENT:
			nFind = FindListTask(sPart, GetFirstSelectedItem());
			break;
			
		case TDC_SELECTPREV:
			nFind = FindListTask(sPart, GetFirstSelectedItem() - 1, FALSE);
			break;
			
		case TDC_SELECTLAST:
			nFind = FindListTask(sPart, m_list.GetItemCount() - 1, FALSE);
			break;
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}

	// else
	if (nFind != -1)
		return SelectTask(GetTaskID(nFind));

	return FALSE;
}

int CTabbedToDoCtrl::FindListTask(const CString& sPart, int nStart, BOOL bNext)
{
	// build a search query
	SEARCHPARAMS params;
	params.aRules.Add(SEARCHPARAM(TDCA_TASKNAMEORCOMMENTS, FO_INCLUDES, sPart));

	// we need to do this manually because CListCtrl::FindItem 
	// only looks at the start of the string
	SEARCHRESULT result;

	int nFrom = nStart;
	int nTo = bNext ? m_list.GetItemCount() : -1;
	int nInc = bNext ? 1 : -1;

	for (int nItem = nFrom; nItem != nTo; nItem += nInc)
	{
		DWORD dwTaskID = GetTaskID(nItem);

		if (m_data.TaskMatches(dwTaskID, params, result))
			return nItem;
	}

	return -1; // no match
}

void CTabbedToDoCtrl::SelectNextTasksInHistory()
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::SelectNextTasksInHistory();
		break;

	case FTCV_TASKLIST:
		if (CanSelectNextTasksInHistory())
		{
			// let CToDoCtrl do it's thing
			CToDoCtrl::SelectNextTasksInHistory();

			// then update our own selection
			ResyncListSelection();
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}
}

BOOL CTabbedToDoCtrl::MultiSelectItems(const CDWordArray& aTasks, TSH_SELECT nState, BOOL bRedraw)
{
	BOOL bRes = CToDoCtrl::MultiSelectItems(aTasks, nState, bRedraw);

	// extra processing
	if (bRes)
	{
		FTC_VIEW nView = GetView();

		switch (nView)
		{
		case FTCV_TASKTREE:
		case FTCV_UNSET:
			break;

		case FTCV_TASKLIST:
			ResyncListSelection();
			break;

		case FTCV_UIEXTENSION1:
		case FTCV_UIEXTENSION2:
		case FTCV_UIEXTENSION3:
		case FTCV_UIEXTENSION4:
		case FTCV_UIEXTENSION5:
		case FTCV_UIEXTENSION6:
		case FTCV_UIEXTENSION7:
		case FTCV_UIEXTENSION8:
		case FTCV_UIEXTENSION9:
		case FTCV_UIEXTENSION10:
		case FTCV_UIEXTENSION11:
		case FTCV_UIEXTENSION12:
		case FTCV_UIEXTENSION13:
		case FTCV_UIEXTENSION14:
		case FTCV_UIEXTENSION15:
		case FTCV_UIEXTENSION16:
			break;

		default:
			ASSERT(0);
		}
	}

	return bRes;
}

void CTabbedToDoCtrl::ResyncListSelection()
{
	// save current states
	TDCSELECTIONCACHE cacheList, cacheTree;

	CacheListSelection(cacheList);
	CacheTreeSelection(cacheTree);
	
	// now update the list selection using the tree's selection
	// but the list's breadcrumbs, and save list scroll pos 
	// before restoring
	cacheTree.dwFirstVisibleTaskID = GetTaskID(m_list.GetTopIndex());
	cacheTree.aBreadcrumbs.Copy(cacheList.aBreadcrumbs);

	RestoreListSelection(cacheTree);
	
	// now check that the tree is correctly synced with us!
	CacheListSelection(cacheList);

	if (!Misc::MatchAllT(cacheTree.aSelTaskIDs, cacheList.aSelTaskIDs))
		RestoreTreeSelection(cacheList);
}


void CTabbedToDoCtrl::SelectPrevTasksInHistory()
{
	if (CanSelectPrevTasksInHistory())
	{
		// let CToDoCtrl do it's thing
		CToDoCtrl::SelectPrevTasksInHistory();

		// extra processing
		FTC_VIEW nView = GetView();

		switch (nView)
		{
		case FTCV_TASKTREE:
		case FTCV_UNSET:
			// handled above
			break;

		case FTCV_TASKLIST:
			// then update our own selection
			ResyncListSelection();
			break;

		case FTCV_UIEXTENSION1:
		case FTCV_UIEXTENSION2:
		case FTCV_UIEXTENSION3:
		case FTCV_UIEXTENSION4:
		case FTCV_UIEXTENSION5:
		case FTCV_UIEXTENSION6:
		case FTCV_UIEXTENSION7:
		case FTCV_UIEXTENSION8:
		case FTCV_UIEXTENSION9:
		case FTCV_UIEXTENSION10:
		case FTCV_UIEXTENSION11:
		case FTCV_UIEXTENSION12:
		case FTCV_UIEXTENSION13:
		case FTCV_UIEXTENSION14:
		case FTCV_UIEXTENSION15:
		case FTCV_UIEXTENSION16:
			break;

		default:
			ASSERT(0);
		}
	}
}

void CTabbedToDoCtrl::InvalidateItem(HTREEITEM hti)
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::InvalidateItem(hti);
		break;

	case FTCV_TASKLIST:
		{
			int nItem = GetListItem(hti);
			CRect rItem;

			if (GetListItemTitleRect(nItem, TDCTR_BOUNDS, rItem))
				m_list.InvalidateRect(rItem, FALSE);
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}
}

LRESULT CTabbedToDoCtrl::ScWindowProc(HWND hRealWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_RBUTTONDOWN:
		{
			// work out what got hit and make sure it's selected
			int nHit = m_list.HitTest(CPoint(lp));

			if ((nHit != -1) && !IsItemSelected(nHit))
				SelectTask(GetTaskID(nHit));
		}
		break;

	case WM_LBUTTONDOWN:
		{
			// work out what got hit
			int nSubItem = -1;

			CPoint ptHit(lp);
			int nHit = m_list.HitTest(ptHit, &nSubItem);

			if (nHit != -1 && !IsReadOnly())
			{
				TDC_COLUMN nCol = GetListColumnID(nSubItem);
				DWORD dwTaskID = GetTaskID(nHit);

				// if the user is clicking on an already multi-selected
				// item since we may need to carry out an operation on multiple items
				int nSelCount = GetSelectedCount();

				if (nSelCount > 1 && IsItemSelected(nHit))
				{
					switch (nCol)
					{
					case TDCC_DONE:
						{
							BOOL bDone = m_data.IsTaskDone(dwTaskID);
							SetSelectedTaskDone(!bDone);
							return 0; // eat it
						}
						break;

					case TDCC_CLIENT:
						{
							CRect rCheck;

							if (GetListItemTitleRect(nHit, TDCTR_CHECKBOX, rCheck) && rCheck.PtInRect(ptHit))
							{
								BOOL bDone = m_data.IsTaskDone(dwTaskID);
								SetSelectedTaskDone(!bDone);
								return 0; // eat it
							}
						}
						break;
						
					case TDCC_FLAG:
						{
							BOOL bFlagged = m_data.IsTaskFlagged(dwTaskID);
							SetSelectedTaskFlag(!bFlagged);
							return 0; // eat it
						}
						break;
					}
				}
				else if (nSelCount == 1)
				{
					// if the click was on the task title of an already singly selected item
					// we record the task ID unless the control key is down in which case
					// it really means that the user has deselected the item
					if (!Misc::KeyIsPressed(VK_CONTROL))
					{
						m_dw2ndClickItem = 0;
						
						int nSel = GetFirstSelectedItem();
						if (nHit != -1 && nHit == nSel && nCol == TDCC_CLIENT)
						{
							// unfortunately we cannot rely on the flags attribute of LVHITTESTINFO
							// to see if the user clicked on the text because LVIR_LABEL == LVIR_BOUNDS
							CRect rLabel;
							CClientDC dc(&m_list);
							CFont* pOld = NULL;

							if (m_tree.GetParentItem(GetTreeItem(nHit)) == NULL) // top level items
								pOld = (CFont*)dc.SelectObject(CFont::FromHandle(m_hFontBold));
							else
								pOld = (CFont*)dc.SelectObject(m_list.GetFont());

							GetListItemTitleRect(nHit, TDCTR_LABEL, rLabel, &dc, GetSelectedTaskTitle());
			
							if (rLabel.PtInRect(ptHit))
								m_dw2ndClickItem = m_list.GetItemData(nHit);

							// cleanup
							dc.SelectObject(pOld);
						}

						// note: we handle WM_LBUTTONUP in OnListClick() to 
						// decide whether to do a label edit
					}
				}
			}

			// because the visual state of the list selection is actually handled by
			// whether the tree selection is up to date we need to update the tree
			// selection here, because the list ctrl does it this way natively.
			LRESULT	lr = CSubclasser::ScWindowProc(hRealWnd, msg, wp, lp);
			UpdateTreeSelection();

			return lr;
		}
		break;

	case LVM_EDITLABEL:
		if (!IsReadOnly())
			EditSelectedTask(FALSE);
		return 0; // eat it

	case WM_KEYDOWN:
		{
			// if any of the cursor keys are used and nothing is currently selected
			// then we select the top/bottom item and ignore the default list ctrl processing
			LRESULT lr = CSubclasser::ScWindowProc(hRealWnd, msg, wp, lp);
			m_wKeyPress = (WORD)wp;

			if (Misc::IsCursorKey(wp))
				UpdateTreeSelection();

			return lr;
		}
		break;

	case WM_KEYUP:
		if (Misc::IsCursorKey(wp))
			UpdateControls();
		break;

	case WM_MOUSEWHEEL:
	case WM_VSCROLL:
	case WM_HSCROLL:
		if (InListView())
		{
			if (IsTaskLabelEditing())
				EndLabelEdit(FALSE);

			// extra processing for WM_HSCROLL
 			if (msg == WM_HSCROLL)
 				m_list.RedrawHeader();
		}
		break;

		
	case WM_NOTIFY:
		{
			LPNMHDR pNMHDR = (LPNMHDR)lp;
			
			switch (pNMHDR->code)
			{
			case NM_CUSTOMDRAW:
				if (pNMHDR->hwndFrom == m_list.GetHeader())
				{
					LRESULT lr = 0;
					OnListHeaderCustomDraw(pNMHDR, &lr);
					return lr;
				}
				break;
			}
		}
		break;
	}

	return CSubclasser::ScWindowProc(hRealWnd, msg, wp, lp);
}

void CTabbedToDoCtrl::UpdateTreeSelection()
{
	// update the tree selection if it needs to be
	CDWordArray aListTaskIDs, aTreeTaskIDs;
	DWORD dwTreeFocusedID, dwListFocusedID;

	GetSelectedTaskIDs(aTreeTaskIDs, dwTreeFocusedID, FALSE);
	GetSelectedListTaskIDs(aListTaskIDs, dwListFocusedID);
	
	if (!Misc::MatchAllT(aTreeTaskIDs, aListTaskIDs) || 
		(dwTreeFocusedID != dwListFocusedID))
	{
		// select tree item first then multi-select after
		HTREEITEM htiFocus = m_find.GetItem(dwListFocusedID);
		m_tree.SelectItem(htiFocus);

		MultiSelectItems(aListTaskIDs, TSHS_SELECT, FALSE);

		// reset list selection
		int nFocus = FindListTask(dwListFocusedID);
		m_list.SetItemState(nFocus, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_list.RedrawItems(nFocus, nFocus);
	}
}

BOOL CTabbedToDoCtrl::IsItemSelected(int nItem) const
{
	HTREEITEM hti = GetTreeItem(nItem);
	return hti ? Selection().HasItem(hti) : FALSE;
}

HTREEITEM CTabbedToDoCtrl::GetTreeItem(int nItem) const
{
	if (nItem < 0 || nItem >= m_list.GetItemCount())
		return NULL;

	DWORD dwID = m_list.GetItemData(nItem);
	return m_find.GetItem(dwID);
}

int CTabbedToDoCtrl::GetListItem(HTREEITEM hti) const
{
	DWORD dwID = GetTaskID(hti);
	return (dwID ? FindListTask(dwID) : -1);
}

BOOL CTabbedToDoCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	if (&m_list == pWnd)
	{
		CPoint pt(::GetMessagePos());
		m_list.ScreenToClient(&pt);

		LVHITTESTINFO lvhti = { { pt.x, pt.y }, 0, 0, 0 };
		m_list.SubItemHitTest(&lvhti);

		int nHit = lvhti.iItem;

		if (nHit >= 0)
		{
			TDC_COLUMN nCol	= GetListColumnID(lvhti.iSubItem);
			DWORD dwID = m_list.GetItemData(nHit);

			BOOL bCtrl = Misc::KeyIsPressed(VK_CONTROL);
			BOOL bShowHand = FALSE;

			switch (nCol)
			{
			case TDCC_FILEREF:
				if (bCtrl)
				{
					CString sFile = m_data.GetTaskFileRef(dwID);
					bShowHand = (!sFile.IsEmpty());
				}
				break;
				
			case TDCC_DEPENDENCY:
				if (bCtrl)
				{
					CStringArray aDepends;
					m_data.GetTaskDependencies(dwID, aDepends);
					bShowHand = aDepends.GetSize();
				}
				break;
				
			case TDCC_TRACKTIME:
				if (!IsReadOnly())
				{
					bShowHand = ((!IsItemSelected(nHit) || GetSelectedCount() == 1) && 
								 m_data.IsTaskTimeTrackable(dwID));
				}
				break;
				
			case TDCC_ICON:
			case TDCC_FLAG:
				bShowHand = (dwID && !IsReadOnly());
				break;
		
			default:
				// handle custom attributes
				if (!IsReadOnly() && CTDCCustomAttributeHelper::IsCustomColumn(nCol))
				{
					TDCCUSTOMATTRIBUTEDEFINITION attribDef;
					VERIFY (CTDCCustomAttributeHelper::GetAttributeDef(nCol, m_aCustomAttribDefs, attribDef));
					
					switch (attribDef.GetDataType())
					{
					case TDCCA_BOOL:
					case TDCCA_ICON:
						bShowHand = TRUE;
						break;
					}
				}
			}

			if (bShowHand)
			{
				::SetCursor(GraphicsMisc::HandCursor());
				return TRUE;
			}
		}
	}
		
	return CToDoCtrl::OnSetCursor(pWnd, nHitTest, message);
}

LRESULT CTabbedToDoCtrl::OnDropObject(WPARAM wParam, LPARAM lParam)
{
	if (IsReadOnly())
		return 0L;

	TLDT_DATA* pData = (TLDT_DATA*)wParam;
	CWnd* pTarget = (CWnd*)lParam;
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
	case FTCV_TASKLIST:
		// simply convert the list item into the corresponding tree
		// item and pass to base class
		if (pTarget == &m_list)
		{
			ASSERT (InListView());

			if (pData->nItem != -1)
				m_list.SetCurSel(pData->nItem);

			pData->hti = GetTreeItem(pData->nItem);
			pData->nItem = -1;
			lParam = (LPARAM)&m_tree;
		}
		return CToDoCtrl::OnDropObject(wParam, lParam);	// default handling

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}

	// else
	return 0L;
}

BOOL CTabbedToDoCtrl::GetLabelEditRect(CRect& rScreen)
{
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		return CToDoCtrl::GetLabelEditRect(rScreen);

	case FTCV_TASKLIST:
		{
			HTREEITEM htiSel = GetSelectedItem();
			int nSel = GetListItem(htiSel);
			
			// make sure item is visible
			if (m_list.EnsureVisible(GetFirstSelectedItem(), FALSE))
			{
				ScrollListClientColumnIntoView();	
				return GetListItemTitleRect(nSel, TDCTR_EDIT, rScreen);
			}
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		{
			IUIExtensionWindow* pExt = GetExtensionWnd(nView);
			ASSERT(pExt);

			return (pExt && pExt->GetLabelEditRect(rScreen));
		}
		break;

	default:
		ASSERT(0);
	}

	return FALSE;
}

BOOL CTabbedToDoCtrl::GetListItemTitleRect(int nItem, TDC_TITLERECT nArea, CRect& rect, CDC* pDC, LPCTSTR szTitle) const
{
	ASSERT (InListView());
	
	if (nItem == -1)
		return FALSE;

	OSVERSION nOsVer = COSVersion();
	
	// basic title rect
	int nColIndex = GetListColumnIndex(TDCC_CLIENT);
	const_cast<CTDCListView*>(&m_list)->GetSubItemRect(nItem, nColIndex, LVIR_LABEL, rect);

	if ((nColIndex == 0) && (nOsVer >= OSV_XP)) // right side columns in XP
		rect.left -= 4;

	BOOL bIcon = !IsColumnShowing(TDCC_ICON);
	BOOL bCheckbox = !IsColumnShowing(TDCC_DONE);

	switch (nArea)
	{
	case TDCTR_CHECKBOX:
		if (bCheckbox)
		{
			rect.right = rect.left + 16;

			// in XP, tree places checkbox at bottom of rect
			if (nOsVer <= OSV_XP)
			{
				rect.top += (rect.Height() - 16);
			}
			else // centre vertically
			{
				rect.top += ((rect.Height() - 16) / 2);
			}
			return TRUE;
		}
		break;

	case TDCTR_ICON:
		if (bIcon)
		{
			if (bCheckbox)
				rect.left += 18;

			rect.right = rect.left + 16;

			// tree places icon vertically centred in rect
			rect.top += ((rect.Height() - 16) / 2);

			return TRUE;
		}
		break;
		
	case TDCTR_LABEL:
		{
			if (bIcon)
				rect.left += 18;

 			if (bCheckbox)
 				rect.left += 18;

			if (pDC && szTitle)
			{
				int nTextExt = pDC->GetTextExtent(szTitle).cx;
				rect.right = rect.left + min(rect.Width(), nTextExt + 6 + 1);
			}

			return TRUE;
		}
		break;

	case TDCTR_BOUNDS:
		return TRUE; // nothing more to do

	case TDCTR_EDIT:
		if (GetListItemTitleRect(nItem, TDCTR_LABEL, rect)) // recursive call
		{
			CRect rClient, rInter;
			m_list.GetClientRect(rClient);
			
			if (rInter.IntersectRect(rect, rClient))
				rect = rInter;
			
			rect.top--;

			// return in screen coords
			m_list.ClientToScreen(rect);

			return TRUE;
		}
		break;
	}

	return FALSE;
}

void CTabbedToDoCtrl::OnListGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVGETINFOTIP* pLVGIT = (NMLVGETINFOTIP*)pNMHDR;
	*pResult = 0;

	int nHit = pLVGIT->iItem;

	if (nHit >= 0)
	{
		HTREEITEM hti = GetTreeItem(nHit);
		DWORD dwTaskID = m_list.GetItemData(nHit);
		ASSERT(dwTaskID);

		if (!dwTaskID)
			return;

		// we only display info tips when over the task title
		CRect rTitle;
		CClientDC dc(&m_list);
		CFont* pOld = NULL;
		
		if (m_tree.GetParentItem(hti) == NULL) // top level item
			pOld = (CFont*)dc.SelectObject(CFont::FromHandle(m_hFontBold));
		else
			pOld = (CFont*)dc.SelectObject(m_list.GetFont());
		
		GetListItemTitleRect(nHit, TDCTR_LABEL, rTitle, &dc, GetTaskTitle(dwTaskID));
		
		// cleanup
		dc.SelectObject(pOld);
	
		CPoint pt(::GetMessagePos());
		m_list.ScreenToClient(&pt);
		
		if (rTitle.PtInRect(pt))
		{
			//fabio_2005
#if _MSC_VER >= 1400
			_tcsncpy_s(pLVGIT->pszText, pLVGIT->cchTextMax, FormatInfoTip(dwTaskID), pLVGIT->cchTextMax);
#else
			_tcsncpy(pLVGIT->pszText, FormatInfoTip(dwTaskID), pLVGIT->cchTextMax);
#endif
		}
	}
}

void CTabbedToDoCtrl::UpdateSelectedTaskPath()
{
	CToDoCtrl::UpdateSelectedTaskPath();

	// extra processing
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		// handled above
		break;

	case FTCV_TASKLIST:
		// redraw the client column header
		m_list.RedrawHeaderColumn(GetListColumnIndex(TDCC_CLIENT));
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}
}

void CTabbedToDoCtrl::SaveSortState(CPreferences& prefs)
{
	// ignore this if we have no tasks
	if (GetTaskCount() == 0)
		return;

	// create a new key using the filepath
	ASSERT (GetSafeHwnd());
	
	CString sKey = GetPreferencesKey(_T("SortColState"));
	
	if (!sKey.IsEmpty())
	{
		const VIEWDATA* pLVData = GetViewData(FTCV_TASKLIST);

		prefs.WriteProfileInt(sKey, _T("ListMulti"), pLVData->sort.bMulti);

		prefs.WriteProfileInt(sKey, _T("ListColumn"), pLVData->sort.single.nBy);
		prefs.WriteProfileInt(sKey, _T("ListAscending"), pLVData->sort.single.bAscending);

		prefs.WriteProfileInt(sKey, _T("ListColumn1"), pLVData->sort.multi.col1.nBy);
		prefs.WriteProfileInt(sKey, _T("ListColumn2"), pLVData->sort.multi.col2.nBy);
		prefs.WriteProfileInt(sKey, _T("ListColumn3"), pLVData->sort.multi.col3.nBy);

		prefs.WriteProfileInt(sKey, _T("ListAscending1"), pLVData->sort.multi.col1.bAscending);
		prefs.WriteProfileInt(sKey, _T("ListAscending2"), pLVData->sort.multi.col2.bAscending);
		prefs.WriteProfileInt(sKey, _T("ListAscending3"), pLVData->sort.multi.col3.bAscending);
	}

	// base class
	CToDoCtrl::SaveSortState(prefs);
}

void CTabbedToDoCtrl::LoadSortState(const CPreferences& prefs)
{
	CString sKey = GetPreferencesKey(_T("SortColState"));
	VIEWDATA* pLVData = GetViewData(FTCV_TASKLIST);
	
	// single sort
	pLVData->sort.single.nBy = (TDC_COLUMN)prefs.GetProfileInt(sKey, _T("ListColumn"), TDCC_NONE);
	pLVData->sort.single.bAscending = prefs.GetProfileInt(sKey, _T("ListAscending"), TRUE);

	// multi sort
	pLVData->sort.bMulti = prefs.GetProfileInt(sKey, _T("ListMulti"), FALSE);
	pLVData->sort.multi.col1.nBy = (TDC_COLUMN)prefs.GetProfileInt(sKey, _T("ListColumn1"), TDCC_NONE);
	pLVData->sort.multi.col2.nBy = (TDC_COLUMN)prefs.GetProfileInt(sKey, _T("ListColumn2"), TDCC_NONE);
	pLVData->sort.multi.col3.nBy = (TDC_COLUMN)prefs.GetProfileInt(sKey, _T("ListColumn3"), TDCC_NONE);
	pLVData->sort.multi.col1.bAscending = prefs.GetProfileInt(sKey, _T("ListAscending1"), TRUE);
	pLVData->sort.multi.col2.bAscending = prefs.GetProfileInt(sKey, _T("ListAscending2"), TRUE);
	pLVData->sort.multi.col3.bAscending = prefs.GetProfileInt(sKey, _T("ListAscending3"), TRUE);

	pLVData->sort.Validate();
	pLVData->bNeedResort = pLVData->sort.IsSorting();

	CToDoCtrl::LoadSortState(prefs);
}

void CTabbedToDoCtrl::RedrawReminders() const
{ 
	FTC_VIEW nView = GetView();

	switch (nView)
	{
	case FTCV_TASKTREE:
	case FTCV_UNSET:
		CToDoCtrl::RedrawReminders();
		break;
		
	case FTCV_TASKLIST:
		if (IsColumnShowing(TDCC_REMINDER))
		{
			CListCtrl* pList = const_cast<CTDCListView*>(&m_list);
			pList->Invalidate(FALSE);
		}
		break;

	case FTCV_UIEXTENSION1:
	case FTCV_UIEXTENSION2:
	case FTCV_UIEXTENSION3:
	case FTCV_UIEXTENSION4:
	case FTCV_UIEXTENSION5:
	case FTCV_UIEXTENSION6:
	case FTCV_UIEXTENSION7:
	case FTCV_UIEXTENSION8:
	case FTCV_UIEXTENSION9:
	case FTCV_UIEXTENSION10:
	case FTCV_UIEXTENSION11:
	case FTCV_UIEXTENSION12:
	case FTCV_UIEXTENSION13:
	case FTCV_UIEXTENSION14:
	case FTCV_UIEXTENSION15:
	case FTCV_UIEXTENSION16:
		break;

	default:
		ASSERT(0);
	}
}

BOOL CTabbedToDoCtrl::IsViewSet() const 
{ 
	return (GetView() != FTCV_UNSET); 
}

BOOL CTabbedToDoCtrl::InListView() const 
{ 
	return (GetView() == FTCV_TASKLIST); 
}

BOOL CTabbedToDoCtrl::InTreeView() const 
{ 
	return (GetView() == FTCV_TASKTREE || !IsViewSet()); 
}

BOOL CTabbedToDoCtrl::InExtensionView() const
{
	return IsExtensionView(GetView());
}

BOOL CTabbedToDoCtrl::IsExtensionView(FTC_VIEW nView)
{
	return (nView >= FTCV_UIEXTENSION1 && nView <= FTCV_UIEXTENSION16);
}

BOOL CTabbedToDoCtrl::HasAnyExtensionViews() const
{
	int nView = m_aExtViews.GetSize();

	while (nView--)
	{
		if (m_aExtViews[nView] != NULL)
			return TRUE;
	}

	// else
	return FALSE;
}

void CTabbedToDoCtrl::ShowListViewTab(BOOL bVisible)
{
	// update tab visibility
	VIEWDATA* pData = GetViewData(FTCV_TASKLIST);
	ASSERT(pData);

	// update tab control
	m_tabViews.ShowViewTab(FTCV_TASKLIST, bVisible);
}

BOOL CTabbedToDoCtrl::IsListViewTabShowing() const
{
	return m_tabViews.IsViewTabShowing(FTCV_TASKLIST);
}

void CTabbedToDoCtrl::SetVisibleExtensionViews(const CStringArray& aTypeIDs)
{
	// update extension visibility
	int nExt = m_mgrUIExt.GetNumUIExtensions();

	while (nExt--)
	{
		FTC_VIEW nView = (FTC_VIEW)(FTCV_UIEXTENSION1 + nExt);

		VIEWDATA* pData = GetViewData(nView);
		ASSERT(pData);

		// update tab control
		CString sTypeID = m_mgrUIExt.GetUIExtensionTypeID(nExt);
		BOOL bVisible = (Misc::Find(aTypeIDs, sTypeID, FALSE, FALSE) != -1);

		m_tabViews.ShowViewTab(nView, bVisible);

	}
}

int CTabbedToDoCtrl::GetVisibleExtensionViews(CStringArray& aTypeIDs) const
{
	ASSERT(GetSafeHwnd());

	aTypeIDs.RemoveAll();

	int nExt = m_mgrUIExt.GetNumUIExtensions();

	while (nExt--)
	{
		FTC_VIEW nView = (FTC_VIEW)(FTCV_UIEXTENSION1 + nExt);

		if (m_tabViews.IsViewTabShowing(nView))
			aTypeIDs.Add(m_mgrUIExt.GetUIExtensionTypeID(nExt));
	}

	return aTypeIDs.GetSize();
}

BOOL CTabbedToDoCtrl::SetStyle(TDC_STYLE nStyle, BOOL bOn) 
{ 
	if (CToDoCtrl::SetStyle(nStyle, bOn))
	{
		// extra handling for extensions
		switch (nStyle)
		{
		case TDCS_READONLY:
			{
				int nView = m_aExtViews.GetSize();
				
				while (nView--)
				{
					if (m_aExtViews[nView] != NULL)
						m_aExtViews[nView]->SetReadOnly(bOn != FALSE);
				}
			}
			break;
		}

		return TRUE;
	}

	return FALSE;
}
