#if !defined(AFX_TDCSTRUCT_H__5951FDE6_508A_4A9D_A55D_D16EB026AEF7__INCLUDED_)
#define AFX_TDCSTRUCT_H__5951FDE6_508A_4A9D_A55D_D16EB026AEF7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// tdlutil.h : header file
//

#include "tdcenum.h"
#include "tdcswitch.h"
#include "taskfile.h"
#include "tdcmapping.h"

#include "..\shared\TreeSelectionHelper.h"
#include "..\shared\TreeCtrlHelper.h"
#include "..\shared\misc.h"
#include "..\shared\datehelper.h"
#include "..\shared\encommandlineinfo.h"
#include "..\shared\enstring.h"

/////////////////////////////////////////////////////////////////////////////////////////////

typedef CMap<TDC_STYLE, TDC_STYLE, BOOL, BOOL&> CTDCStylesMap;
typedef CMap<CString, LPCTSTR, COLORREF, COLORREF&> CTDCColorMap;

/////////////////////////////////////////////////////////////////////////////////////////////

class CToDoCtrl;
class CFilteredToDoCtrl;
class CTDLImportExportMgr;

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCFINDWND
{
	TDCFINDWND(HWND ignore = NULL, BOOL incClosing = FALSE) 
		: hWndIgnore(ignore), bIncClosing(incClosing)
	{
	}

	HWND hWndIgnore;
	BOOL bIncClosing;
	CArray<HWND, HWND> aResults;
};

/////////////////////////////////////////////////////////////////////////////////////////////

const int ATTRIBLEN		= 128;
const int NEWTASKLEN	= 256;
const int COMMENTSLEN	= 1024;
const int FILEPATHSLEN	= (10 * 1024);

struct TDCSTARTUP
{
	TDCSTARTUP()
	{
		Reset();
	}

	TDCSTARTUP(const TDCSTARTUP& startup)
	{
		*this = startup;
	}

	TDCSTARTUP& operator=(const TDCSTARTUP& startup)
	{
#if _MSC_VER >= 1400
		_tcscpy_s(szFilePaths, FILEPATHSLEN, startup.szFilePaths); 
		_tcscpy_s(szNewTask, NEWTASKLEN, startup.szNewTask); 
		_tcscpy_s(szComments, COMMENTSLEN, startup.szComments); 
		_tcscpy_s(szExternalID, ATTRIBLEN, startup.szExternalID); 
		_tcscpy_s(szVersion, ATTRIBLEN, startup.szVersion);
		_tcscpy_s(szAllocTo, ATTRIBLEN, startup.szAllocTo); 
		_tcscpy_s(szAllocBy, ATTRIBLEN, startup.szAllocBy); 
		_tcscpy_s(szCategory, ATTRIBLEN, startup.szCategory); 
		_tcscpy_s(szTags, ATTRIBLEN, startup.szTags); 
		_tcscpy_s(szStatus, ATTRIBLEN, startup.szStatus); 
		_tcscpy_s(szFileRef, _MAX_PATH, startup.szFileRef); 
#else
		_tcscpy(szFilePaths, startup.szFilePaths); 
		_tcscpy(szNewTask, startup.szNewTask); 
		_tcscpy(szComments, startup.szComments); 
		_tcscpy(szExternalID, startup.szExternalID); 
		_tcscpy(szVersion, startup.szVersion);
		_tcscpy(szAllocTo, startup.szAllocTo); 
		_tcscpy(szAllocBy, startup.szAllocBy); 
		_tcscpy(szCategory, startup.szCategory); 
		_tcscpy(szTags, startup.szTags); 
		_tcscpy(szStatus, startup.szStatus); 
		_tcscpy(szFileRef, startup.szFileRef); 
#endif

		dCreateDate = startup.dCreateDate;
		dStartDate = startup.dStartDate;
		dDueDate = startup.dDueDate;
		dDoneDate = startup.dDoneDate;
		nPriority = startup.nPriority; 
		nRisk = startup.nRisk; 
		dCost = startup.dCost;
		dTimeEst = startup.dTimeEst;
		dTimeSpent = startup.dTimeSpent;
		nPercentDone = startup.nPercentDone;

		dwIDSel = startup.dwIDSel;
		dwParentID = startup.dwParentID; 
		dwSiblingID = startup.dwSiblingID; 
		dwFlags = startup.dwFlags;

		return *this;
	}


#if _MSC_VER >= 1400                        
	#define COPYTEXT(DEST, SRC, LEN)		\
		int len = min(LEN, SRC.GetLength());\
		_tcsncpy_s(DEST, LEN, SRC, len);	\
		DEST[len] = 0;
#else                                       
	#define COPYTEXT(DEST, SRC, LEN)		\
		int len = min(LEN, SRC.GetLength());\
		_tcsncpy(DEST, SRC, len);			\
		DEST[len] = 0;
#endif                                      

	static BOOL ExtractAttribute(const CEnCommandLineInfo& cmdInfo, LPCTSTR szSwitch, LPTSTR szAttrib, int nLenAttrib = ATTRIBLEN)
	{
		CString sSrc;

		if (cmdInfo.GetOption(szSwitch, sSrc))                                
		{                                                                   
			COPYTEXT(szAttrib, sSrc, nLenAttrib);
			return TRUE;
		}

		return FALSE;
	}

	TDCSTARTUP(const CEnCommandLineInfo& cmdInfo)
	{
		Reset();

		// insert default path at front
		COPYTEXT(szFilePaths, cmdInfo.m_strFileName, FILEPATHSLEN);

		// then multiple others
		if (cmdInfo.HasOption(SWITCH_TASKFILE))
		{
			if (!cmdInfo.m_strFileName.IsEmpty())
			{
#if _MSC_VER >= 1400                        
				_tcscat_s(szFilePaths, FILEPATHSLEN, _T("|")); // add delimiter
#else                                       
				_tcscat(szFilePaths, _T("|")); // add delimiter
#endif                                      
			}

			int nLen = lstrlen(szFilePaths);
			ExtractAttribute(cmdInfo, SWITCH_TASKFILE, szFilePaths + nLen, FILEPATHSLEN - nLen);
		}

		CString sValue;

		// new task
		if (ExtractAttribute(cmdInfo, SWITCH_NEWTASK, szNewTask, NEWTASKLEN))
		{
			dwFlags |= TLD_NEWTASK;

			// user can specify parentID else 0
			if (cmdInfo.GetOption(SWITCH_PARENTID, sValue))
				dwParentID = _ttoi(sValue);

			else if (cmdInfo.GetOption(SWITCH_SIBLINGID, sValue))
				dwSiblingID = _ttoi(sValue);

			// creation date
			if (cmdInfo.GetOption(SWITCH_TASKCREATEDATE, sValue))
				CDateHelper::DecodeDate(sValue, dCreateDate);
		}
		// select task
		else if (cmdInfo.GetOption(SWITCH_SELECTTASKID, sValue))
		{
			dwIDSel = _ttoi(sValue);
		}
		// or merge/import
		else if (ExtractAttribute(cmdInfo, SWITCH_IMPORT, szFilePaths, FILEPATHSLEN))
		{
			dwFlags |= TLD_IMPORTFILE;
		}
		else if (cmdInfo.GetOption(SWITCH_COMMANDID, sValue))
		{
			nCmdID = _ttoi(sValue);
		}

		// other task attribs
		ExtractAttribute(cmdInfo, SWITCH_TASKEXTID, szExternalID);	
		ExtractAttribute(cmdInfo, SWITCH_TASKCATEGORY, szCategory);	
		ExtractAttribute(cmdInfo, SWITCH_TASKSTATUS, szStatus);	
		ExtractAttribute(cmdInfo, SWITCH_TASKALLOCBY, szAllocBy);	
		ExtractAttribute(cmdInfo, SWITCH_TASKALLOCTO, szAllocTo);	
		ExtractAttribute(cmdInfo, SWITCH_TASKVERSION, szVersion);	
		ExtractAttribute(cmdInfo, SWITCH_TASKTAGS, szTags);	
		ExtractAttribute(cmdInfo, SWITCH_TASKFILEREF, szFileRef, _MAX_PATH);	
		
		// priority, risk
		if (cmdInfo.GetOption(SWITCH_TASKPRIORITY, sValue))
		{
			// handle 'n' for none
			if (sValue.CompareNoCase(_T("n")) == 0)
				nPriority = FM_NOPRIORITY;
			else
				nPriority = _ttoi(sValue);	
		}
		
		if (cmdInfo.GetOption(SWITCH_TASKRISK, sValue))
		{
			// handle 'n' for none
			if (sValue.CompareNoCase(_T("n")) == 0)
				nRisk = FM_NORISK;
			else
				nRisk = _ttoi(sValue);
		}
		
		// % completion
		if (cmdInfo.GetOption(SWITCH_TASKPERCENT, sValue))
			nPercentDone = _ttoi(sValue);	
		
		// dates
		if (cmdInfo.GetOption(SWITCH_TASKSTARTDATE, sValue))
			CDateHelper::DecodeDate(sValue, dStartDate, TRUE);
		
		if (cmdInfo.GetOption(SWITCH_TASKDUEDATE, sValue))
			CDateHelper::DecodeDate(sValue, dDueDate, TRUE);
		
		if (cmdInfo.GetOption(SWITCH_TASKDONEDATE, sValue))
			CDateHelper::DecodeDate(sValue, dDoneDate, TRUE);
		
		// times and cost
		if (cmdInfo.GetOption(SWITCH_TASKCOST, sValue))
			dCost = _ttof(sValue);	
		
		if (cmdInfo.GetOption(SWITCH_TASKTIMEEST, sValue))
			dTimeEst = _ttof(sValue);	
		
		if (cmdInfo.GetOption(SWITCH_TASKTIMESPENT, sValue))
			dTimeSpent = _ttof(sValue);	
		
		// comments replace [\][n] with [\n]
		if (cmdInfo.GetOption(SWITCH_TASKCOMMENTS, sValue))
		{
			sValue.Replace(_T("\\n"), _T("\n"));
			COPYTEXT(szComments, sValue, COMMENTSLEN);
		}

		// flags
		if (cmdInfo.HasOption(SWITCH_FORCEVISIBLE))
			dwFlags |= TLD_FORCEVISIBLE;

		if (cmdInfo.HasOption(SWITCH_NOPWORDPROMPT))
			dwFlags &= ~TLD_PASSWORDPROMPTING;

		if (cmdInfo.HasOption(SWITCH_LOGGING))
			dwFlags |= TLD_LOGGING;

		if (cmdInfo.HasOption(SWITCH_STARTEMPTY))
			dwFlags |= TLD_STARTEMPTY;

		if (cmdInfo.HasOption(SWITCH_UPGRADED))
			dwFlags |= TLD_UPGRADED;

		if (cmdInfo.HasOption(SWITCH_TASKLINK))
		{
			if (!HasFilePath())
			{
				CString sLink(cmdInfo.GetOption(SWITCH_TASKLINK));
				COPYTEXT(szFilePaths, sLink, FILEPATHSLEN);
			}
			
			dwFlags |= TLD_TASKLINK;
		}
	}

	DWORD GetTaskID() const { return dwIDSel; }
	DWORD GetParentTaskID() const { return dwParentID; }
	DWORD GetSiblingTaskID() const { return dwSiblingID; }
	DWORD GetFlags() const { return dwFlags; }

	UINT GetCommandID() const { return nCmdID; }
	BOOL HasCommandID() const { return (nCmdID != 0); }

	BOOL HasFilePath() const { return (lstrlen(szFilePaths) > 0); }
	int GetFilePaths(CStringArray& aFiles) const { return Misc::Split(szFilePaths, aFiles, '|'); }

	BOOL GetNewTask(CString& sValue) const { sValue = szNewTask; return !sValue.IsEmpty(); }
	BOOL GetComments(CString& sValue) const { sValue = szComments; return !sValue.IsEmpty(); }
	BOOL GetExternalID(CString& sValue) const { sValue = szExternalID; return !sValue.IsEmpty(); }
	BOOL GetVersion(CString& sValue) const { sValue = szVersion; return !sValue.IsEmpty(); }
	BOOL GetAllocBy(CString& sValue) const { sValue = szAllocBy; return !sValue.IsEmpty(); }
	BOOL GetStatus(CString& sValue) const { sValue = szStatus; return !sValue.IsEmpty(); }
	BOOL GetFileRef(CString& sValue) const { sValue = szFileRef; return !sValue.IsEmpty(); }

	BOOL GetStartDate(double& dtValue) const { dtValue = dStartDate; return (dtValue != 0); } 
	BOOL GetDueDate(double& dtValue) const { dtValue = dDueDate; return (dtValue != 0); }
	BOOL GetDoneDate(double& dtValue) const { dtValue = dDoneDate; return (dtValue != 0); } 
	BOOL GetCreationDate(double& dtValue) const { dtValue = dCreateDate; return (dtValue != 0); } 

	BOOL GetPercentDone(int& nValue) const { nValue = nPercentDone; return (nValue != -1); }
	BOOL GetPriority(int& nValue) const { nValue = nPriority; return (nValue != -1);	}
	BOOL GetRisk(int& nValue) const { nValue = nRisk; return (nValue != -1);}

	BOOL GetCost(double& dValue) const { dValue = dCost; return (dValue != 0.0); }
	BOOL GetTimeEst(double& dValue) const { dValue = dTimeEst; return (dValue != 0.0); }
	BOOL GetTimeSpent(double& dValue) const { dValue = dTimeSpent; return (dValue != 0.0); }

	int GetCategories(CStringArray& aCats) const { return Misc::Split(szCategory, aCats); }
	int GetAllocTo(CStringArray& aAllocTo) const { return Misc::Split(szAllocTo, aAllocTo); }
	int GetTags(CStringArray& aTags) const { return Misc::Split(szTags, aTags); }
	
	void Reset() 
	{ 
		szFilePaths[0] = 0; 
		szNewTask[0] = 0; 
		szComments[0] = 0; 
		szExternalID[0] = 0;
		szVersion[0] = 0;
		szAllocTo[0] = 0; 
		szAllocBy[0] = 0; 
		szCategory[0] = 0; 
		szStatus[0] = 0; 
		szTags[0] = 0; 
		szFileRef[0] = 0; 

		dCreateDate = dStartDate = dDueDate = dDoneDate = 0;
		dTimeEst = dTimeSpent = dCost = 0;
		nPercentDone = -1; // sentinel value
		nPriority = -1; // sentinel value
		nRisk = -1; // sentinel value 
		dwIDSel = 0;
		dwParentID = 0; 
		dwSiblingID = 0; 
		dwFlags = TLD_PASSWORDPROMPTING;
		nCmdID = 0;
	}

	BOOL HasFlag(DWORD dwFlag) const { return Misc::HasFlag(dwFlags, dwFlag); }
	
protected:
	TCHAR szFilePaths[FILEPATHSLEN+1]; // tasklists to load/import
	DWORD dwIDSel; // task to select
	DWORD dwParentID; // parent task for new subtask
	DWORD dwSiblingID; // sibling task for new subtask
	TCHAR szNewTask[NEWTASKLEN+1];
	TCHAR szComments[COMMENTSLEN+1];
	TCHAR szExternalID[ATTRIBLEN+1]; 
	TCHAR szVersion[ATTRIBLEN+1];
	TCHAR szAllocTo[ATTRIBLEN+1];
	TCHAR szAllocBy[ATTRIBLEN+1];
	TCHAR szCategory[ATTRIBLEN+1];
	TCHAR szStatus[ATTRIBLEN+1];
	TCHAR szTags[NEWTASKLEN+1];
	TCHAR szFileRef[_MAX_PATH+1];
	int nPriority, nRisk, nPercentDone;
	double dCreateDate, dStartDate, dDueDate, dDoneDate;
	DWORD dwFlags;
	UINT nCmdID;
	double dTimeEst, dTimeSpent, dCost;
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCDUETASKNOTIFY
{
	TDCDUETASKNOTIFY(HWND hwnd, const CString sTDC, BOOL html) 
		: 
		hWndNotify(hwnd),
		sTDCPath(sTDC), 
		bHtml(html),
		pImpExpMgr(NULL)
	{
	}

	BOOL IsValid() const
	{
		if (!::IsWindow(hWndNotify))
			return FALSE;

		if (!tasks.GetTaskCount())
			return FALSE;

		if (!pImpExpMgr)
			return FALSE;

		if (sExportPath.IsEmpty())
			return FALSE;

		if (sTDCPath.IsEmpty())
			return FALSE;

		return TRUE;
	}

	CString sTDCPath;
	CString sExportPath;
	CString sStylesheet;

	CTaskFile tasks;
	BOOL bHtml;
	HWND hWndNotify;
	CTDLImportExportMgr* pImpExpMgr;
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDIRECURRENCE
{
//  nRegularity       dwSpecific1        dwSpecific2

//	TDIR_DAILY        every 'n' days     --- (0)
//	TDIR_WEEKLY       every 'n' weeks    weekdays (TDIW_...)
//	TDIR_MONTHLY      every 'n' months   day of month (1-31)
//	TDIR_YEARLY       month (1-12)       day of month (1-31)

	TDIRECURRENCE() 
		: 
		nRegularity(TDIR_ONCE), 
		dwSpecific1(1), 
		dwSpecific2(0), 
		nRecalcFrom(TDIRO_DUEDATE), 
		nReuse(TDIRO_REUSE),
		nNumRecur(-1)
	{
	}

	BOOL operator==(const TDIRECURRENCE& tr) const
	{
		return ((tr.nRegularity == nRegularity) && 
				(tr.dwSpecific1 == dwSpecific1) &&
				(tr.dwSpecific2 == dwSpecific2) && 
				(tr.nRecalcFrom == nRecalcFrom) &&
				(tr.nReuse		== nReuse) && 
				(tr.GetRegularity() == GetRegularity()) &&
				(tr.nNumRecur	== nNumRecur));
	}

	BOOL operator!=(const TDIRECURRENCE& tr) const
	{
		return !(*this == tr);
	}

	BOOL GetNextOccurence(const COleDateTime& dtFrom, COleDateTime& dtNext)
	{
		if (!CDateHelper::IsDateSet(dtFrom))
			return FALSE;

		if (nRegularity == TDIR_ONCE)
			return FALSE;

		// have we got any occurrences left
		if (nNumRecur == 0)
			return FALSE;

		dtNext = dtFrom; // starting point

		switch (nRegularity)		
		{
		case TDIR_DAY_EVERY:
		case TDIR_DAY_RECREATEAFTERNDAYS:
			if ((int)dwSpecific1 <= 0)
				return FALSE;

			// add number of days specified by dwSpecific1
			CDateHelper::OffsetDate(dtNext, (int)dwSpecific1, DHU_DAYS);
			break;

		case TDIR_DAY_WEEKDAYS:
			// add one day ensuring that the result is also a weekday
			CDateHelper::OffsetDate(dtNext, 1, DHU_WEEKDAYS);
			break;

		case TDIR_WEEK_EVERY:
			if (!dwSpecific2)
				return FALSE;
			// else fall thru

		case TDIR_WEEK_RECREATEAFTERNWEEKS:
			{
				if ((int)dwSpecific1 <= 0)
					return FALSE;

				// if no weekdays have been set we just add the specified number of weeks
				if (!dwSpecific2)
				{
					CDateHelper::OffsetDate(dtNext, (int)dwSpecific1, DHU_WEEKS);
				}
				else // go looking for the next specified weekday
				{
					// first add any weeks greater than one
					CDateHelper::OffsetDate(dtNext, (int)(dwSpecific1 - 1), DHU_WEEKS);

					// then look for the next weekday *after* this one

					// build a 2 week weekday array so we don't have to deal with 
					// wrapping around
					UINT nWeekdays[14] = 
					{
						(dwSpecific2 & DHW_SUNDAY) ? 1 : 0,
						(dwSpecific2 & DHW_MONDAY) ? 1 : 0,
						(dwSpecific2 & DHW_TUESDAY) ? 1 : 0,
						(dwSpecific2 & DHW_WEDNESDAY) ? 1 : 0,
						(dwSpecific2 & DHW_THURSDAY) ? 1 : 0,
						(dwSpecific2 & DHW_FRIDAY) ? 1 : 0,
						(dwSpecific2 & DHW_SATURDAY) ? 1 : 0,
						(dwSpecific2 & DHW_SUNDAY) ? 1 : 0,
						(dwSpecific2 & DHW_MONDAY) ? 1 : 0,
						(dwSpecific2 & DHW_TUESDAY) ? 1 : 0,
						(dwSpecific2 & DHW_WEDNESDAY) ? 1 : 0,
						(dwSpecific2 & DHW_THURSDAY) ? 1 : 0,
						(dwSpecific2 & DHW_FRIDAY) ? 1 : 0,
						(dwSpecific2 & DHW_SATURDAY) ? 1 : 0,
					};

					dtNext += 1.0; // always next day at least

					int nStart = dtFrom.GetDayOfWeek();
					int nEnd = nStart + 7; // week later

					for (int nWeekday = nStart; nWeekday < nEnd; nWeekday++)
					{
						if (nWeekdays[nWeekday] != 0)
							break;
						else
							dtNext += 1.0; 
					}
				}
			}
			break;

		case TDIR_MONTH_EVERY:
			if ((dwSpecific2 < 1) || (dwSpecific2 > 31))
				return FALSE;
			// else fall thru
			
		case TDIR_MONTH_RECREATEAFTERNMONTHS:
			{
				if ((int)dwSpecific1 <= 0)
					return FALSE;
				
				// add number of months specified by dwSpecific1
				CDateHelper::OffsetDate(dtNext, (int)dwSpecific1, DHU_MONTHS);

				// then enforce the day
				SYSTEMTIME st;
				dtNext.GetAsSystemTime(st);

				if (dwSpecific2)
					st.wDay = (WORD)dwSpecific2;
				
				// clip dates to the end of the month
				st.wDay = min(st.wDay, (WORD)CDateHelper::GetDaysInMonth(st.wMonth, st.wYear));

				dtNext = COleDateTime(st);
			}
			break;

		case TDIR_MONTH_SPECIFICDAY:
			{
				int nWhich = LOWORD(dwSpecific1);
				int nDOW = HIWORD(dwSpecific1);
				int nNumMonths = dwSpecific2;

				ASSERT(CDateHelper::IsValidDayOfMonth(nDOW, nWhich, 1));
				
				if (!CDateHelper::IsValidDayOfMonth(nDOW, nWhich, 1))
					return FALSE;
				
				// work out where we are
				int nMonth = dtNext.GetMonth();
				int nYear = dtNext.GetYear();

				// increment months
				nMonth += nNumMonths;

				if (nMonth > 12)
				{
					while (nMonth > 12)
					{
						nMonth -= 12;
						nYear++;
					}
				}

				// calculate next instance
				dtNext = CDateHelper::CalcDate(nDOW, nWhich, nMonth, nYear);
			}
			break;

		case TDIR_YEAR_EVERY:
		case TDIR_YEAR_RECREATEAFTERNYEARS:
			{
				int nDay = dwSpecific2;
				int nMonth = dwSpecific1;
				int nYear = dtNext.GetYear();

				// handle Feb 29 without spitting the dummy
				ASSERT(CDateHelper::IsValidDayInMonth(nDay, nMonth, nYear) ||
						(nDay == 29 && nMonth == 2));
				
				if (!CDateHelper::IsValidDayInMonth(nDay, nMonth, nYear) && 
					!(nDay == 29 && nMonth == 2))
				{
					return FALSE;
				}

				int nDaysInMonth = CDateHelper::GetDaysInMonth(nMonth, nYear);
				
				// see if this year would work before trying next year
				dtNext = COleDateTime(nYear, nMonth, min(nDay, nDaysInMonth), 0, 0, 0);

				// else try incrementing the year
				if (dtNext <= dtFrom)
				{
					nYear++;

					nDaysInMonth = CDateHelper::GetDaysInMonth(nMonth, nYear);
					nDay = min(nDay, nDaysInMonth);

					// calculate date
					dtNext = COleDateTime(nYear, nMonth, nDay, 0, 0, 0);
					ASSERT(dtNext > dtFrom);
				}
			}
			break;

		case TDIR_YEAR_SPECIFICDAYMONTH:
			{
				int nWhich = LOWORD(dwSpecific1);
				int nDOW = HIWORD(dwSpecific1);
				int nMonth = dwSpecific2;

				ASSERT(CDateHelper::IsValidDayOfMonth(nDOW, nWhich, nMonth));
				
				if (!CDateHelper::IsValidDayOfMonth(nDOW, nWhich, nMonth))
					return FALSE;
				
				// see if this year would work before trying next year
				int nYear = dtNext.GetYear();
				dtNext = CDateHelper::CalcDate(nDOW, nWhich, nMonth, nYear);

				// else try incrementing the year
				if (dtNext <= dtFrom)
				{
					nYear++;

					dtNext = CDateHelper::CalcDate(nDOW, nWhich, nMonth, nYear);
					ASSERT(dtNext > dtFrom);
				}
			}
			break;

		default:
			return FALSE;
		}

		// decrement the remaining occurrence count
		if (nNumRecur > 0)
			nNumRecur--;
		
		return TRUE;
	}

	CString GetRegularity(BOOL bIncOnce = TRUE) const
	{
		return GetRegularity(nRegularity, bIncOnce);
	}

	static CString GetRegularity(TDI_REGULARITY nRegularity, BOOL bIncOnce = TRUE)
	{
		enum { ONCE, DAILY, WEEKLY, MONTHLY, YEARLY, COUNT };

		static CEnString sRegularity[COUNT];
		
		switch (nRegularity)
		{
		case TDIR_DAY_WEEKDAYS:
		case TDIR_DAY_RECREATEAFTERNDAYS:
		case TDIR_DAY_EVERY:   
			if (sRegularity[DAILY].IsEmpty())
				sRegularity[DAILY].LoadString(IDS_DAILY);    
			
			return sRegularity[DAILY];
			
		case TDIR_WEEK_RECREATEAFTERNWEEKS:
		case TDIR_WEEK_EVERY:  
			if (sRegularity[WEEKLY].IsEmpty())
				sRegularity[WEEKLY].LoadString(IDS_WEEKLY);   
			
			return sRegularity[WEEKLY];
			
		case TDIR_MONTH_SPECIFICDAY:
		case TDIR_MONTH_RECREATEAFTERNMONTHS:
		case TDIR_MONTH_EVERY: 
			if (sRegularity[MONTHLY].IsEmpty())
				sRegularity[MONTHLY].LoadString(IDS_MONTHLY);  
			
			return sRegularity[MONTHLY];
			
		case TDIR_YEAR_SPECIFICDAYMONTH:
		case TDIR_YEAR_RECREATEAFTERNYEARS:
		case TDIR_YEAR_EVERY:  
			if (sRegularity[YEARLY].IsEmpty())
				sRegularity[YEARLY].LoadString(IDS_YEARLY);   
			
			return sRegularity[YEARLY];
		}
		
		// all else
		if (!bIncOnce)
			return _T("");

		if (sRegularity[ONCE].IsEmpty())
			sRegularity[ONCE].LoadString(IDS_ONCEONLY); 
			
		return sRegularity[ONCE];
	}
	
	static BOOL IsDefaultString(const CString& sRegularity)
	{
		int nReg = (int)TDIR_YEAR_EVERY + 1;
		
		while (nReg--)
		{
			if (sRegularity.CompareNoCase(GetRegularity((TDI_REGULARITY)nReg)) == 0)
				return TRUE;
		}
		
		// else
		return FALSE;
	}

	TDI_REGULARITY nRegularity;
	DWORD dwSpecific1;
	DWORD dwSpecific2;
	TDI_RECURFROMOPTION nRecalcFrom; 
	TDI_RECURREUSEOPTION nReuse;
	int nNumRecur;
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCSTATUSBARINFO
{
	TDCSTATUSBARINFO() 
		: 
		nSelCount(0), 
		dwSelTaskID(0), 
		nTimeEstUnits(0), 
		dTimeEst(0.0), 
		nTimeSpentUnits(0), 
		dTimeSpent(0.0), 
		dCost(0.0)
	{
	}

	BOOL operator==(const TDCSTATUSBARINFO& sbi) const
	{
		return (nSelCount == sbi.nSelCount &&
				dwSelTaskID == sbi.dwSelTaskID &&
				nTimeEstUnits == sbi.nTimeEstUnits &&
				dTimeEst == sbi.dTimeEst &&
				nTimeSpentUnits == sbi.nTimeSpentUnits &&
				dTimeSpent == sbi.dTimeSpent &&
				dCost == sbi.dCost);
	}

	int nSelCount;
	DWORD dwSelTaskID;
	TCHAR nTimeEstUnits;
	double dTimeEst;
	TCHAR nTimeSpentUnits;
	double dTimeSpent;
	double dCost;
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCGETTASKS
{
	TDCGETTASKS(TDC_GETTASKS filter = TDCGT_ALL, DWORD flags = 0) 
		: nFilter(filter), dwFlags(flags) 
	{ 
		InitDueByDate(); 
	}

	TDCGETTASKS(const TDCGETTASKS& filter)
	{ 
		*this = filter;
	}

	TDCGETTASKS& operator=(const TDCGETTASKS& filter)
	{
		nFilter = filter.nFilter;
		dateDueBy = filter.dateDueBy;
		dwFlags = filter.dwFlags;
		sAllocTo = filter.sAllocTo;
		aAttribs.Copy(filter.aAttribs);

		return *this;
	}

	BOOL WantAttribute(TDC_ATTRIBUTE att) const
	{
		// always want custom attributes
		if (att == TDCA_CUSTOMATTRIB)
			return TRUE;

		int nAttrib = aAttribs.GetSize();

		// if TDCGTF_CUSTOMCOLUMNS is set then
		// 'no' attributes really means 'no' attributes
		// else it means 'all attributes' 
		if (!nAttrib)
		{
			if (HasFlag(TDCGTF_USERCOLUMNS))
				return FALSE;
			else
				return TRUE;
		}

		while (nAttrib--)
		{
			if (aAttribs[nAttrib] == att)
				return TRUE;
		}

		return FALSE;
	}

	BOOL HasFlag(DWORD dwFlag) const { return Misc::HasFlag(dwFlags, dwFlag); }

	TDC_GETTASKS nFilter;
	COleDateTime dateDueBy;
	DWORD dwFlags;
	CString sAllocTo;
	CTDCAttributeArray aAttribs;

protected:
	void InitDueByDate()
	{
		// initialize filter:dateDueBy as required
		switch (nFilter)
		{
		case TDCGT_DUE:
			dateDueBy = CDateHelper::GetDate(DHD_TODAY);
			break; // done
			
		case TDCGT_DUETOMORROW:
			dateDueBy = CDateHelper::GetDate(DHD_TOMORROW);
			break;
			
		case TDCGT_DUETHISWEEK:
			dateDueBy = CDateHelper::GetDate(DHD_ENDTHISWEEK);
			break;
			
		case TDCGT_DUENEXTWEEK:
			dateDueBy = CDateHelper::GetDate(DHD_ENDNEXTWEEK);
			break;
			
		case TDCGT_DUETHISMONTH:
			dateDueBy = CDateHelper::GetDate(DHD_ENDTHISMONTH);
			break;
			
		case TDCGT_DUENEXTMONTH:
			dateDueBy = CDateHelper::GetDate(DHD_ENDNEXTMONTH);
			break;
		}
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDLSELECTION
{
	TDLSELECTION(CTreeCtrl& tree) : selection(tree), htiLastHandledLBtnDown(NULL), 
									tch(tree), nLBtnDownFlags(0), nNcLBtnDownColID(-1) {}

	CTreeSelectionHelper selection;
	CTreeCtrlHelper tch;
	HTREEITEM htiLastHandledLBtnDown;
	UINT nLBtnDownFlags;
	int nNcLBtnDownColID;

	BOOL HasIncompleteSubtasks() const
	{
		POSITION pos = selection.GetFirstItemPos();

		while (pos)
		{
			HTREEITEM hti = selection.GetNextItem(pos);

			if (ItemHasIncompleteSubtasks(hti))
				return TRUE;
		}

		return FALSE;
	}
	
	BOOL ItemHasIncompleteSubtasks(HTREEITEM hti) const
	{
		const CTreeCtrl& tree = selection.TreeCtrl();
		HTREEITEM htiChild = tree.GetChildItem(hti);

		while (htiChild)
		{
			if (tch.GetItemCheckState(htiChild) != TCHC_CHECKED || ItemHasIncompleteSubtasks(htiChild))
				return TRUE;
			
			htiChild = tree.GetNextItem(htiChild, TVGN_NEXT);
		}

		return FALSE;
	}

	int GetTaskIDs(CDWordArray& aIDs) const
	{
		aIDs.RemoveAll();
		POSITION pos = selection.GetFirstItemPos();

		while (pos)
		{
			HTREEITEM hti = selection.GetNextItem(pos);
			aIDs.Add(tch.TreeCtrl().GetItemData(hti));
		}

		return aIDs.GetSize();
	}

	BOOL SelectTasks(const CDWordArray& aIDs)
	{
		BOOL bSomeSelected = FALSE;

		selection.RemoveAll();
		
		for (int nID = 0; nID < aIDs.GetSize(); nID++)
		{
			HTREEITEM hti = tch.FindItem(aIDs[nID], NULL);

			if (hti)
			{
				selection.AddItem(hti, FALSE);
				bSomeSelected = TRUE;
			}
		}

		selection.FixupTreeSelection();

		return bSomeSelected;
	}

	BOOL HasIncompleteTasks() const
	{
		// look for first incomplete task
		POSITION pos = selection.GetFirstItemPos();

		while (pos)
		{
			HTREEITEM hti = selection.GetNextItem(pos);

			if (tch.GetItemCheckState(hti) != TCHC_CHECKED)
				return TRUE;
		}

		return FALSE;
	}

};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCSELECTIONCACHE
{
	TDCSELECTIONCACHE() : dwFocusedTaskID(0), dwFirstVisibleTaskID(0) {}

	CDWordArray aSelTaskIDs;
	DWORD dwFocusedTaskID;
	CDWordArray aBreadcrumbs;
	DWORD dwFirstVisibleTaskID;
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCCONTROL
{
	LPCTSTR szClass;
	UINT nIDCaption;
	DWORD dwStyle;
	DWORD dwExStyle; 
	int nX;
	int nY;
	int nCx;
	int nCy;
	UINT nID;
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCCOLUMN
{
	TDC_COLUMN nColID;
	UINT nIDName;
	UINT nIDLongName;
	UINT nAlignment;
	LPCTSTR szFont;
	BOOL bSymbolFont;
	BOOL bSortAscending;
};

class CTDCColumnArray : public CArray<TDCCOLUMN, TDCCOLUMN&> {};

/////////////////////////////////////////////////////////////////////////////////////////////

struct CTRLITEM
{
	UINT nCtrlID;
	UINT nLabelID;
	TDC_ATTRIBUTE nAttrib;
};

struct CUSTOMATTRIBCTRLITEM : public CTRLITEM
{
	CString sAttribID;
};

class CTDCControlArray : public CArray<CTRLITEM, CTRLITEM&> {};
class CTDCCustomControlArray : public CArray<CUSTOMATTRIBCTRLITEM, CUSTOMATTRIBCTRLITEM&> {};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCCUSTOMATTRIBUTEDEFINITION
{
	friend class CTDCCustomAttribDefinitionArray;

	TDCCUSTOMATTRIBUTEDEFINITION() 
		: 
	dwAttribType(TDCCA_STRING), 
	nColumnAlignment(DT_LEFT), 
	dwFeatures(0),
	nColID(TDCC_NONE),
	nAttribID(TDCA_NONE),
	bEnabled(TRUE)
	{

	}

	TDCCUSTOMATTRIBUTEDEFINITION(const TDCCUSTOMATTRIBUTEDEFINITION& otherDef)
	{
		*this = otherDef;
	}

	TDCCUSTOMATTRIBUTEDEFINITION& operator=(const TDCCUSTOMATTRIBUTEDEFINITION& attribDef)
	{
		dwAttribType = attribDef.dwAttribType;
		sUniqueID = attribDef.sUniqueID;
		sColumnTitle = attribDef.sColumnTitle;
		sLabel = attribDef.sLabel;
		nColumnAlignment = attribDef.nColumnAlignment;
		dwFeatures = attribDef.dwFeatures;
		nColID = attribDef.nColID;
		nAttribID = attribDef.nAttribID;
		bEnabled = attribDef.bEnabled;

		if (attribDef.IsList())
		{
			aDefaultListData.Copy(attribDef.aDefaultListData);
			
			if (attribDef.IsAutoList())
				aAutoListData.Copy(attribDef.aAutoListData);
		}

		return *this;
	}

	BOOL operator==(const TDCCUSTOMATTRIBUTEDEFINITION& attribDef) const
	{
		// NOTE: we ignore auto data which is temporary
		return ((dwAttribType == attribDef.dwAttribType) &&
				(sUniqueID.CompareNoCase(attribDef.sUniqueID) == 0) &&
				(sColumnTitle == attribDef.sColumnTitle) &&
				(sLabel == attribDef.sLabel) &&
				(nColumnAlignment == attribDef.nColumnAlignment) &&
				(dwFeatures == attribDef.dwFeatures) &&
				(nColID == attribDef.nColID) &&
				(nAttribID == attribDef.nAttribID) &&
				(bEnabled == attribDef.bEnabled) &&
				Misc::MatchAll(aDefaultListData, attribDef.aDefaultListData));
	}

	CString GetColumnTitle() const
	{
		return (sColumnTitle.IsEmpty() ? sLabel : sColumnTitle);
	}

	inline TDC_COLUMN GetColumnID() const { return nColID; }
	inline TDC_ATTRIBUTE GetAttributeID() const { return nAttribID; }
	inline DWORD GetAttributeType() const { return dwAttribType; }

	void SetAttributeType(DWORD dwType)
	{
		SetTypes((dwType & TDCCA_DATAMASK), (dwType & TDCCA_LISTMASK));
	}

	void SetDataType(DWORD dwDataType)
	{
		SetTypes(dwDataType, GetListType());
	}

	void SetListType(DWORD dwListType)
	{
		SetTypes(GetDataType(), dwListType);
	}

	inline DWORD GetDataType() const { return (dwAttribType & TDCCA_DATAMASK); }
	inline DWORD GetListType() const { return (dwAttribType & TDCCA_LISTMASK); }
	inline BOOL HasFeature(DWORD dwFeature) const { return (dwFeatures & dwFeature); }

	inline BOOL IsList() const { return (GetListType() != TDCCA_NOTALIST); }
	inline BOOL IsAutoList() const { return ((GetListType() == TDCCA_AUTOLIST) || (GetListType() == TDCCA_AUTOMULTILIST)); }

	int GetUniqueListData(CStringArray& aData) const
	{
		aData.Copy(aAutoListData);
		Misc::AddUniqueItems(aDefaultListData, aData);

		return aData.GetSize();
	}

	CString EncodeListData() const
	{
		if (!IsList())
			return _T("");

		CString sListData = Misc::FormatArray(aDefaultListData, '\n');

		if (IsAutoList())
		{
			sListData += '\t';
			sListData += Misc::FormatArray(aAutoListData, '\n');
		}

		return sListData;
	}
	
	void DecodeListData(const CString& sListData)
	{
		if (IsList() && !sListData.IsEmpty())
		{
			CString sDefData(sListData), sAutoData;

			Misc::Split(sDefData, sAutoData, '\t');
			Misc::Split(sDefData, aDefaultListData, '\n');

			if (IsAutoList())
				Misc::Split(sAutoData, aAutoListData, '\n');
		}
	}

	BOOL SupportsFeature(DWORD dwFeature) const
	{
		// sorting works on all data types
		if (dwFeature == TDCCAF_SORT)
			return TRUE;

		// calculations not supported by multi-list types
		DWORD dwListType = GetListType();

		if (dwListType == TDCCA_AUTOMULTILIST || dwListType == TDCCA_FIXEDMULTILIST)
			return FALSE;

		DWORD dwDataType = GetDataType();

		switch (dwDataType)
		{
		case TDCCA_DOUBLE:
		case TDCCA_INTEGER:
			return ((dwFeature == TDCCAF_ACCUMULATE) || 
					(dwFeature == TDCCAF_MAXIMIZE) || 
					(dwFeature == TDCCAF_MINIMIZE));
			
		case TDCCA_DATE:
			return ((dwFeature == TDCCAF_MAXIMIZE) || 
					(dwFeature == TDCCAF_MINIMIZE));
			
		case TDCCA_STRING:
		case TDCCA_BOOL:
		case TDCCA_ICON:
			break;

		default:
			ASSERT(0);
			break;
		}
		
		return FALSE;
	}

	BOOL SupportsCalculation() const
	{
		return SupportsFeature(TDCCAF_ACCUMULATE) ||
				SupportsFeature(TDCCAF_MAXIMIZE) ||
				SupportsFeature(TDCCAF_MINIMIZE);
	}

	CString GetNextListItem(const CString& sItem, BOOL bNext) const
	{
		DWORD dwListType = GetListType();

		ASSERT (dwListType != TDCCA_NOTALIST);

		switch (aDefaultListData.GetSize())
		{
		case 0:
			return sItem;

		default:
			{
				int nFind = Misc::Find(aDefaultListData, sItem);
				ASSERT((nFind != -1) || sItem.IsEmpty());

				if (bNext)
				{
					nFind++;

					if (nFind < aDefaultListData.GetSize())
						return aDefaultListData[nFind];
				}
				else // prev
				{
					if (nFind == -1)
						nFind = aDefaultListData.GetSize();

					nFind--;

					if (nFind >= 0)
						return aDefaultListData[nFind];
				}

				// all else
				return _T("");
			}
			break;
		}
	}

	CString GetImageName(const CString& sImage) const
	{
		int nTag = Misc::Find(aDefaultListData, sImage);

		CString sName, sDummy;

		if (nTag != -1)
		{
			CString sTag = aDefaultListData[nTag], sDummy;

			VERIFY(DecodeImageTag(sTag, sDummy, sName));
			//ASSERT(sImage == sDummy);
		}

		return sName;
	}

	static CString EncodeImageTag(const CString& sImage, const CString& sName) 
	{ 
		CString sTag;
		sTag.Format(_T("%s:%s"), sImage, sName);
		return sTag;
	}

	static BOOL DecodeImageTag(const CString& sTag, CString& sImage, CString& sName)
	{
		CStringArray aParts;
		int nNumParts = Misc::Split(sTag, aParts, ':');

		switch (nNumParts)
		{
		case 2:
			sName = aParts[1];
			// fall thru

		case 1:
			sImage = aParts[0];
			break;

		case 0:
			return FALSE;
		}

		return TRUE;
	}

	///////////////////////////////////////////////////////////////////////////////
	CString sUniqueID;
	CString sColumnTitle;
	CString sLabel;
	UINT nColumnAlignment;
	DWORD dwFeatures;
	CStringArray aDefaultListData;
	mutable CStringArray aAutoListData;
	BOOL bEnabled;

private:
	// these are managed internally
	DWORD dwAttribType;
	TDC_COLUMN nColID;
	TDC_ATTRIBUTE nAttribID;
	///////////////////////////////////////////////////////////////////////////////

	void SetTypes(DWORD dwDataType, DWORD dwListType)
	{
		dwAttribType = (dwDataType | ValidateListType(dwDataType, dwListType));
	}

	static DWORD ValidateListType(DWORD dwDataType, DWORD dwListType)
	{
		// ensure list-type cannot be applied to dates,
		// or only partially to icons or flags
		switch (dwDataType)
		{
		case TDCCA_DATE:
			dwListType = TDCCA_NOTALIST;
			break;

		case TDCCA_BOOL:
			if (dwListType)
			{
				if (dwListType != TDCCA_FIXEDLIST)
					dwListType = TDCCA_FIXEDLIST;
			}
			break;

		case TDCCA_ICON:
			if (dwListType)
			{
				if ((dwListType != TDCCA_FIXEDLIST) && (dwListType != TDCCA_FIXEDMULTILIST))
					dwListType = TDCCA_FIXEDLIST;
			}
			break;
		}

		return dwListType;
	}
};

class CTDCCustomAttribDefinitionArray : public CArray<TDCCUSTOMATTRIBUTEDEFINITION, TDCCUSTOMATTRIBUTEDEFINITION&>
{
public:
	int Add(TDCCUSTOMATTRIBUTEDEFINITION& newElement)
	{
		int nIndex = CArray<TDCCUSTOMATTRIBUTEDEFINITION, TDCCUSTOMATTRIBUTEDEFINITION&>::Add(newElement);
		RebuildIDs();

		return nIndex;

	};

	void InsertAt(int nIndex, TDCCUSTOMATTRIBUTEDEFINITION& newElement, int nCount = 1)
	{
		CArray<TDCCUSTOMATTRIBUTEDEFINITION, TDCCUSTOMATTRIBUTEDEFINITION&>::InsertAt(nIndex, newElement, nCount);
		RebuildIDs();
	}

	void RemoveAt(int nIndex, int nCount = 1)
	{
		CArray<TDCCUSTOMATTRIBUTEDEFINITION, TDCCUSTOMATTRIBUTEDEFINITION&>::RemoveAt(nIndex, nCount);
		RebuildIDs();
	}

protected:
	void RebuildIDs()
	{
		int nColID = TDCC_CUSTOMCOLUMN_FIRST;
		int nAttribID = TDCA_CUSTOMATTRIB_FIRST;

		for (int nAttrib = 0; nAttrib < GetSize(); nAttrib++)
		{
			TDCCUSTOMATTRIBUTEDEFINITION& attribDef = ElementAt(nAttrib);

			attribDef.nColID = (TDC_COLUMN)nColID++;
			attribDef.nAttribID = (TDC_ATTRIBUTE)nAttribID++;
		}
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCOPERATOR
{
	FIND_OPERATOR op;
	UINT nOpResID;
};

struct SEARCHPARAM
{
	friend struct SEARCHPARAMS;

	SEARCHPARAM(TDC_ATTRIBUTE a = TDCA_NONE, FIND_OPERATOR o = FO_NONE) : 
		attrib(TDCA_NONE), op(o), dValue(0), nValue(0), bAnd(TRUE), dwFlags(0), nType(FT_NONE) 
	{
		SetAttribute(a);
	}
	
	SEARCHPARAM(TDC_ATTRIBUTE a, FIND_OPERATOR o, CString s, BOOL and = TRUE, FIND_ATTRIBTYPE ft = FT_NONE) : 
		attrib(TDCA_NONE), op(o), dValue(0), nValue(0), bAnd(and), dwFlags(0), nType(ft)  
	{
		SetAttribute(a);

		if (!s.IsEmpty())
			SetValue(s);
	}
	
	SEARCHPARAM(TDC_ATTRIBUTE a, FIND_OPERATOR o, double d, BOOL and = TRUE) : 
		attrib(TDCA_NONE), op(o), dValue(0), nValue(0), bAnd(and), dwFlags(0), nType(FT_NONE)  
	{
		SetAttribute(a);

		nType = GetAttribType();

		if (nType == FT_DOUBLE || nType == FT_DATE)
			dValue = d;
	}
	
	SEARCHPARAM(TDC_ATTRIBUTE a, FIND_OPERATOR o, int n, BOOL and = TRUE) : 
		attrib(TDCA_NONE), op(o), dValue(0), nValue(0), bAnd(and), dwFlags(0), nType(FT_NONE)  
	{
		SetAttribute(a);

		nType = GetAttribType();

		if (nType == FT_INTEGER || nType == FT_BOOL)
			nValue = n;
	}

	BOOL operator==(const SEARCHPARAM& rule) const
	{ 
		// simple check
		if (IsCustomAttribute() != rule.IsCustomAttribute())
		{
			return FALSE;
		}

		// test for attribute equivalence
		if (IsCustomAttribute())
		{
			if (sCustAttribID.CompareNoCase(rule.sCustAttribID) != 0)
			{
				return FALSE;
			}
		}
		else if (attrib != rule.attrib)
		{
			return FALSE;
		}

		// test rest of attributes
		if (op == rule.op && bAnd == rule.bAnd && dwFlags == rule.dwFlags)
		{
			switch (GetAttribType())
			{
			case FT_BOOL:
				return TRUE; // handled by operator

			case FT_DATE:
			case FT_DOUBLE:
			case FT_TIME:
				return (dValue == rule.dValue);

			case FT_INTEGER:
				return (nValue == rule.nValue);

			case FT_STRING:
			case FT_DATE_REL:
				return (sValue == rule.sValue);
			}
		}

		// all else
		return FALSE;
	}

	BOOL Set(TDC_ATTRIBUTE a, FIND_OPERATOR o, CString s, BOOL and = TRUE, FIND_ATTRIBTYPE t = FT_NONE)
	{
		if (!SetAttribute(a, t))
			return FALSE;

		SetOperator(o);
		SetAnd(and);
		SetValue(s);

		return TRUE;
	}

	BOOL Set(TDC_ATTRIBUTE a, const CString& id, FIND_ATTRIBTYPE t, FIND_OPERATOR o, CString s, BOOL and = TRUE)
	{
		if (!SetCustomAttribute(a, id, t))
			return FALSE;

		SetOperator(o);
		SetAnd(and);
		SetValue(s);

		return TRUE;
	}

	BOOL SetAttribute(TDC_ATTRIBUTE a, FIND_ATTRIBTYPE t = FT_NONE)
	{
		ASSERT(!IsCustomAttribute(a));

		// custom attributes must have a custom ID
		if (IsCustomAttribute(a))
			return FALSE;

		// handle deprecated relative date attributes
		switch (attrib)
		{
		case TDCA_STARTDATE_RELATIVE:
			a = TDCA_STARTDATE;
			t = FT_DATE_REL;
			break;

		case TDCA_DUEDATE_RELATIVE:
			a = TDCA_DUEDATE;
			t = FT_DATE_REL;
			break;

		case TDCA_DONEDATE_RELATIVE:
			a = TDCA_DONEDATE;
			t = FT_DATE_REL;
			break;

		case TDCA_CREATIONDATE_RELATIVE:
			a = TDCA_CREATIONDATE;
			t = FT_DATE_REL;
			break;

		case TDCA_LASTMOD_RELATIVE:
			a = TDCA_LASTMOD;
			t = FT_DATE_REL;
			break;
		}

		attrib = a;
		nType = t;
		
		// update Attrib type
		GetAttribType();

		// handle relative dates
		if (nType == FT_DATE || nType == FT_DATE_REL) 
		{
			dwFlags = (nType == FT_DATE_REL);
		}

		ValidateOperator();
		return TRUE;
	}

	TDC_ATTRIBUTE GetAttribute() const { return attrib; }
	FIND_OPERATOR GetOperator() const { return op; }
	BOOL GetAnd() const { return bAnd; }
	DWORD GetFlags() const { return dwFlags; }
	BOOL IsCustomAttribute() const { return IsCustomAttribute(attrib); }

	BOOL SetCustomAttribute(TDC_ATTRIBUTE a, const CString& id, FIND_ATTRIBTYPE t)
	{
		ASSERT (IsCustomAttribute(a));
		ASSERT (t != FT_NONE);
		ASSERT (!id.IsEmpty());

		if (!IsCustomAttribute(a) || id.IsEmpty() || t == FT_NONE)
			return FALSE;

		attrib = a;
		nType = t;
		sCustAttribID = id;

		// handle relative dates
		if (nType == FT_DATE || nType == FT_DATE_REL) 
		{
			dwFlags = (nType == FT_DATE_REL);
		}

		ValidateOperator();
		return TRUE;
	}

	CString GetCustomAttributeID() const
	{
		ASSERT (IsCustomAttribute());
		ASSERT (!sCustAttribID.IsEmpty());

		return sCustAttribID;

	}

	BOOL SetOperator(FIND_OPERATOR o) 
	{
		if (IsValidOperator(GetAttribType(), o))
		{
			op = o;
			return TRUE;
		}

		return FALSE;
	}

	FIND_ATTRIBTYPE GetAttribType() const
	{
		if (nType == FT_NONE)
			nType = GetAttribType(attrib, dwFlags);

		ASSERT (nType != FT_NONE || 
				attrib == TDCA_NONE || 
				attrib == TDCA_SELECTION);

		return nType;
	}

	void SetAttribType(FIND_ATTRIBTYPE nAttribType)
	{
		ASSERT (nAttribType != FT_NONE);
		ASSERT (IsCustomAttribute(attrib));

		if (nAttribType != FT_NONE && IsCustomAttribute(attrib))
		{
			nType = nAttribType;

			// handle relative dates
			if (nType == FT_DATE || nType == FT_DATE_REL) 
			{
				dwFlags = (nType == FT_DATE_REL);
			}
		}
	}

	void ClearValue()
	{
		sValue.Empty();
		dValue = 0.0;
		nValue = 0;
	}

	static BOOL IsCustomAttribute(TDC_ATTRIBUTE attrib)
	{
		return (attrib >= TDCA_CUSTOMATTRIB_FIRST && attrib <= TDCA_CUSTOMATTRIB_LAST);
	}

	static FIND_ATTRIBTYPE GetAttribType(TDC_ATTRIBUTE attrib, BOOL bRelativeDate)
	{
		switch (attrib)
		{
		case TDCA_TASKNAME:
		case TDCA_TASKNAMEORCOMMENTS:
		case TDCA_ANYTEXTATTRIBUTE:
		case TDCA_ALLOCTO:
		case TDCA_ALLOCBY:
		case TDCA_STATUS:
		case TDCA_CATEGORY:
		case TDCA_VERSION:
		case TDCA_COMMENTS:
		case TDCA_FILEREF:
		case TDCA_PROJNAME:
		case TDCA_CREATEDBY:
		case TDCA_EXTERNALID: 
		case TDCA_DEPENDENCY: 
		case TDCA_TAGS: 
		case TDCA_PATH: 
			return FT_STRING;

		case TDCA_PRIORITY:
		case TDCA_COLOR:
		case TDCA_PERCENT:
		case TDCA_RISK:
		case TDCA_ID:
		case TDCA_PARENTID:
			return FT_INTEGER;

		case TDCA_TIMEEST:
		case TDCA_TIMESPENT:
			return FT_TIME;

		case TDCA_COST:
			return FT_DOUBLE;

		case TDCA_FLAG:
		case TDCA_RECENTMODIFIED:
			return FT_BOOL;

		case TDCA_STARTDATE:
		case TDCA_STARTTIME:
		case TDCA_CREATIONDATE:
		case TDCA_DONEDATE:
		case TDCA_DUEDATE:
		case TDCA_DUETIME:
		case TDCA_LASTMOD:
			return (bRelativeDate ? FT_DATE_REL : FT_DATE);

	//	case TDCA_RECURRENCE: 
	//	case TDCA_POSITION:
		}

		// custom attribute type must be set explicitly by caller

		return FT_NONE;
	}

	static BOOL IsValidOperator(FIND_ATTRIBTYPE nType, FIND_OPERATOR op)
	{
		switch (op)
		{
		case FO_NONE:
			return TRUE;

		case FO_EQUALS:
		case FO_NOT_EQUALS:
			return (nType != FT_BOOL);

		case FO_INCLUDES:
		case FO_NOT_INCLUDES:
			return (nType == FT_STRING);

		case FO_ON_OR_BEFORE:
		case FO_BEFORE:
		case FO_ON_OR_AFTER:
		case FO_AFTER:
			return (nType == FT_DATE || nType == FT_DATE_REL);

		case FO_GREATER_OR_EQUAL:
		case FO_GREATER:
		case FO_LESS_OR_EQUAL:
		case FO_LESS:
			return (nType == FT_INTEGER || nType == FT_DOUBLE || nType == FT_TIME);

		case FO_SET:
		case FO_NOT_SET:
			return TRUE;
		}

		return FALSE;
	}

	BOOL HasValidOperator() const
	{
		return IsValidOperator(GetAttribType(), op);
	}

	void ValidateOperator()
	{
		if (!HasValidOperator())
			op = FO_NONE;
	}

	BOOL OperatorIs(FIND_OPERATOR o) const
	{
		return (op == o);
	}

	BOOL AttributeIs(TDC_ATTRIBUTE a) const
	{
		return (attrib == a);
	}

	void SetAnd(BOOL and = TRUE)
	{
		bAnd = and;
	}

	void SetFlags(DWORD flags)
	{
		dwFlags = flags;
	}

	void SetValue(const CString& val)
	{
		switch (GetAttribType())
		{
		case FT_STRING:
		case FT_DATE_REL:
			sValue = val;
			break;
			
		case FT_DATE:
		case FT_DOUBLE:
		case FT_TIME:
			dValue = _ttof(val);
			break;
			
		case FT_INTEGER:
		case FT_BOOL:
			nValue = _ttoi(val);
			break;
		}
	}

	void SetValue(double val)
	{
		switch (GetAttribType())
		{
		case FT_DATE:
		case FT_DOUBLE:
		case FT_TIME:
			dValue = val;
			break;
			
		default:
			ASSERT(0);
			break;
		}
	}

	void SetValue(int val)
	{
		switch (GetAttribType())
		{
		case FT_INTEGER:
		case FT_BOOL:
			nValue = val;
			break;

		default:
			ASSERT(0);
			break;
		}
	}

	void SetValue(const COleDateTime& val)
	{
		switch (GetAttribType())
		{
		case FT_DATE:
			dValue = val;
			break;

		default:
			ASSERT(0);
			break;
		}
	}

	CString ValueAsString() const
	{
		switch (GetAttribType())
		{
		case FT_DATE:
		case FT_DOUBLE:
		case FT_TIME:
			{
				CString sVal;
				sVal.Format(_T("%.3f"), dValue);
				return sVal;
			}
			break;

		case FT_INTEGER:
		case FT_BOOL:
			{
				CString sVal;
				sVal.Format(_T("%d"), nValue);
				return sVal;
			}
			break;
		}

		// all else
		return sValue;
	}

	double ValueAsDouble() const
	{
		switch (GetAttribType())
		{
		case FT_DATE:
		case FT_TIME:
		case FT_DOUBLE:
			return dValue;

		case FT_INTEGER:
		case FT_BOOL:
			return (double)nValue;
		}

		// all else
		ASSERT(0);
		return 0.0;
	}

	int ValueAsInteger() const
	{
		switch (GetAttribType())
		{
		case FT_DATE:
		case FT_TIME:
		case FT_DOUBLE:
			return (int)dValue;

		case FT_INTEGER:
		case FT_BOOL:
			return nValue;
		}

		// all else
		ASSERT(0);
		return 0;
	}

	COleDateTime ValueAsDate() const
	{
		COleDateTime date;

		switch (GetAttribType())
		{
		case FT_DATE:
			date = dValue;
			break;

		case FT_DATE_REL:
			CDateHelper::DecodeRelativeDate(sValue, date, FALSE);
			break;

		default:
			// all else
			ASSERT(0);
			break;
		}

		return date;
	}

protected:
	TDC_ATTRIBUTE attrib;
	CString sCustAttribID;
	FIND_OPERATOR op;
	CString sValue;
	int nValue;
	double dValue;
	BOOL bAnd;
	DWORD dwFlags;

	mutable FIND_ATTRIBTYPE nType;

};
typedef CArray<SEARCHPARAM, SEARCHPARAM> CSearchParamArray;

struct SEARCHPARAMS
{
	SEARCHPARAMS() : bIgnoreDone(FALSE), bIgnoreOverDue(FALSE), bWantAllSubtasks(FALSE), bIgnoreFilteredOut(TRUE) {}
	SEARCHPARAMS(const SEARCHPARAMS& params)
	{
		*this = params;
	}

	SEARCHPARAMS& operator=(const SEARCHPARAMS& params)
	{
		bIgnoreDone = params.bIgnoreDone;
		bIgnoreOverDue = params.bIgnoreOverDue;
		bWantAllSubtasks = params.bWantAllSubtasks;
		bIgnoreFilteredOut = params.bIgnoreFilteredOut;

		aRules.Copy(params.aRules);
		aAttribDefs.Copy(params.aAttribDefs);

		return *this;
	}

	BOOL operator!=(const SEARCHPARAMS& params) const
	{
		return !(*this == params);
	}

	BOOL operator==(const SEARCHPARAMS& params) const
	{
		return Misc::MatchAllT(aRules, params.aRules, TRUE) && 
				Misc::MatchAllT(aAttribDefs, params.aAttribDefs) &&
				(bIgnoreDone == params.bIgnoreDone) && 
				(bIgnoreOverDue == params.bIgnoreOverDue) && 
				(bIgnoreFilteredOut == params.bIgnoreFilteredOut) && 
				(bWantAllSubtasks == params.bWantAllSubtasks);
	}

	void Clear()
	{
		bIgnoreDone = FALSE;
		bIgnoreOverDue = FALSE;
		bWantAllSubtasks = FALSE;
		bIgnoreFilteredOut = TRUE;

		aRules.RemoveAll();
		aAttribDefs.RemoveAll();
	}

	BOOL HasAttribute(TDC_ATTRIBUTE attrib) const
	{
		int nRule = aRules.GetSize();

		while (nRule--)
		{
			if (aRules[nRule].attrib == attrib)
				return TRUE;

			// special cases
			if (aRules[nRule].attrib == TDCA_TASKNAMEORCOMMENTS &&
				(attrib == TDCA_TASKNAME || attrib == TDCA_COMMENTS))
				return TRUE;

			else if (aRules[nRule].attrib == TDCA_ANYTEXTATTRIBUTE &&
				(attrib == TDCA_TASKNAME || attrib == TDCA_COMMENTS ||
				 attrib == TDCA_STATUS || attrib == TDCA_CATEGORY ||
				 attrib == TDCA_ALLOCBY || attrib == TDCA_ALLOCTO ||
				 attrib == TDCA_VERSION || attrib == TDCA_TAGS ||
				 attrib == TDCA_EXTERNALID))
				return TRUE;
		}

		// else
		return FALSE;
	}

	BOOL GetRuleCount() const
	{
		return aRules.GetSize();
	}

	CSearchParamArray aRules;
	CTDCCustomAttribDefinitionArray aAttribDefs;
	BOOL bIgnoreDone, bIgnoreOverDue, bWantAllSubtasks, bIgnoreFilteredOut;
};

struct SEARCHRESULT
{
	SEARCHRESULT() : dwTaskID(0), dwFlags(0) {}

	SEARCHRESULT(const SEARCHRESULT& res)
	{
		*this = res;
	}

	SEARCHRESULT& operator=(const SEARCHRESULT& res)
	{
		dwTaskID = res.dwTaskID;
		aMatched.Copy(res.aMatched);
		dwFlags = res.dwFlags;

		return *this;
	}

	BOOL HasFlag(DWORD dwFlag) const 
	{
		return ((dwFlags & dwFlag) == dwFlag) ? 1 : 0;
	}

	DWORD dwTaskID;
	DWORD dwFlags;
	CStringArray aMatched;
};

typedef CArray<SEARCHRESULT, SEARCHRESULT&> CResultArray;

/////////////////////////////////////////////////////////////////////////////////////////////

struct FTDRESULT
{
	FTDRESULT() : dwTaskID(0), pTDC(NULL), bDone(FALSE), bRef(FALSE) {}
	FTDRESULT(const SEARCHRESULT& result, const CFilteredToDoCtrl* pTaskList) 
		: 
		dwTaskID(result.dwTaskID), 
		pTDC(pTaskList), 
		bDone(result.HasFlag(RF_DONE)), 
		bRef(result.HasFlag(RF_REFERENCE)) 
	{
	}
	
	DWORD dwTaskID;
	const CFilteredToDoCtrl* pTDC;
	BOOL bDone, bRef;
};

typedef CArray<FTDRESULT, FTDRESULT&> CFTDResultsArray;

/////////////////////////////////////////////////////////////////////////////////////////////

struct FTDCFILTER
{
	FTDCFILTER() : nShow(FS_ALL), nStartBy(FD_ANY), nDueBy(FD_ANY), nPriority(FM_ANYPRIORITY), nRisk(FM_ANYRISK), 
					dwFlags(FO_ANYALLOCTO | FO_ANYCATEGORY | FO_ANYTAG), nTitleOption(FT_FILTERONTITLEONLY)
	{
		dtUserStart = dtUserDue = COleDateTime::GetCurrentTime();
	}

	FTDCFILTER(const FTDCFILTER& filter)
	{
		*this = filter;
	}

	FTDCFILTER& operator=(const FTDCFILTER& filter)
	{
		nShow = filter.nShow;
		nStartBy = filter.nStartBy;
		nDueBy = filter.nDueBy;
		dtUserStart = filter.dtUserStart;
		dtUserDue = filter.dtUserDue;
		sTitle = filter.sTitle;
		nTitleOption = filter.nTitleOption;
		nPriority = filter.nPriority;
		nRisk = filter.nRisk;
		dwFlags = filter.dwFlags;

		aAllocTo.Copy(filter.aAllocTo);
		aStatus.Copy(filter.aStatus);
		aAllocBy.Copy(filter.aAllocBy);
		aCategories.Copy(filter.aCategories);
		aVersions.Copy(filter.aVersions);
		aTags.Copy(filter.aTags);

		return *this;
	}

	BOOL operator==(const FTDCFILTER& filter) const
	{
		return FiltersMatch(*this, filter);
	}

	BOOL operator!=(const FTDCFILTER& filter) const
	{
		return !FiltersMatch(*this, filter);
	}
	
	BOOL IsSet(DWORD dwIgnore = 0) const
	{
		FTDCFILTER filterEmpty;

		return !FiltersMatch(*this, filterEmpty, dwIgnore);
	}

	BOOL HasFlag(DWORD dwFlag) const
	{
		BOOL bHasFlag = Misc::HasFlag(dwFlags, dwFlag);

		// some flags depend also on the main filter
		if (bHasFlag)
		{
			switch (dwFlag)
			{
			case FO_ANYCATEGORY:
			case FO_ANYALLOCTO:
			case FO_ANYTAG:
			case FO_HIDEPARENTS:
			case FO_HIDECOLLAPSED:
			case FO_SHOWALLSUB:
				break; // always allowed

			case FO_HIDEOVERDUE:
				bHasFlag = (nDueBy == FD_TODAY) ||
							(nDueBy == FD_TOMORROW) ||
							(nDueBy == FD_NEXTSEVENDAYS) ||
							(nDueBy == FD_ENDTHISWEEK) || 
							(nDueBy == FD_ENDNEXTWEEK) || 
							(nDueBy == FD_ENDTHISMONTH) ||
							(nDueBy == FD_ENDNEXTMONTH) ||
							(nDueBy == FD_ENDTHISYEAR) ||
							(nDueBy == FD_ENDNEXTYEAR);
				break;

			case FO_HIDEDONE:
				bHasFlag = (nShow != FS_NOTDONE && nShow != FS_DONE);
				break;

			default:
				ASSERT(0);
				break;		
			}	
		}

		return bHasFlag;
	}

	void SetFlag(DWORD dwFlag, BOOL bOn = TRUE)
	{
		if (bOn)
			dwFlags |= dwFlag;
		else
			dwFlags &= ~dwFlag;
	}

	void Reset()
	{
		*this = FTDCFILTER(); // empty filter
	}

	static BOOL FiltersMatch(const FTDCFILTER& filter1, const FTDCFILTER& filter2, DWORD dwIgnore = 0)
	{
		// simple exclusion test first
		if ((filter1.nShow != filter2.nShow) ||
			(filter1.nStartBy != filter2.nStartBy) ||
			(filter1.nDueBy != filter2.nDueBy) ||
			(filter1.nPriority != filter2.nPriority) ||
			(filter1.nRisk != filter2.nRisk) ||
			(filter1.sTitle != filter2.sTitle) ||
			((filter1.nTitleOption != filter2.nTitleOption) && !filter1.sTitle.IsEmpty()))
		{
			return FALSE;
		}

		// user-specified due date
		if (filter1.nDueBy == FD_USER) // == filter2.nDueBy
		{
			if (filter1.dtUserDue != filter2.dtUserDue)
				return FALSE;
		}

		// user-specified start date
		if (filter1.nStartBy == FD_USER) // == filter2.nStartBy
		{
			if (filter1.dtUserStart != filter2.dtUserStart)
				return FALSE;
		}

		// options
		if ((dwIgnore & FO_HIDEPARENTS) == 0)
		{
			if (filter1.HasFlag(FO_HIDEPARENTS) != filter2.HasFlag(FO_HIDEPARENTS))
				return FALSE;
		}

		if ((dwIgnore & FO_HIDEDONE) == 0)
		{
			if (filter1.HasFlag(FO_HIDEDONE) != filter2.HasFlag(FO_HIDEDONE))
				return FALSE;
		}

		if ((dwIgnore & FO_HIDEOVERDUE) == 0)
		{
			if (filter1.HasFlag(FO_HIDEOVERDUE) != filter2.HasFlag(FO_HIDEOVERDUE))
				return FALSE;
		}

		if ((dwIgnore & FO_HIDECOLLAPSED) == 0)
		{
			if (filter1.HasFlag(FO_HIDECOLLAPSED) != filter2.HasFlag(FO_HIDECOLLAPSED))
				return FALSE;
		}

		if ((dwIgnore & FO_SHOWALLSUB) == 0)
		{
			if (filter1.HasFlag(FO_SHOWALLSUB) != filter2.HasFlag(FO_SHOWALLSUB))
				return FALSE;
		}

		// compare categories disregarding dwFlags if there's not actually something to compare
		if (filter1.aCategories.GetSize() || filter2.aCategories.GetSize())
		{
			if (!Misc::MatchAll(filter1.aCategories, filter2.aCategories) ||
				filter1.HasFlag(FO_ANYCATEGORY) != filter2.HasFlag(FO_ANYCATEGORY))
			{
				return FALSE;
			}
		}

		// do the same with Alloc_To
		if (filter1.aAllocTo.GetSize() || filter2.aAllocTo.GetSize())
		{
			if (!Misc::MatchAll(filter1.aAllocTo, filter2.aAllocTo) ||
				filter1.HasFlag(FO_ANYALLOCTO) != filter2.HasFlag(FO_ANYALLOCTO))
			{
				return FALSE;
			}
		}

		// and the same with Tags
		if (filter1.aTags.GetSize() || filter2.aTags.GetSize())
		{
			if (!Misc::MatchAll(filter1.aTags, filter2.aTags) ||
				filter1.HasFlag(FO_ANYTAG) != filter2.HasFlag(FO_ANYTAG))
			{
				return FALSE;
			}
		}

		// and the same with Alloc_By
		if (filter1.aAllocBy.GetSize() || filter2.aAllocBy.GetSize())
		{
			if (!Misc::MatchAll(filter1.aAllocBy, filter2.aAllocBy))
				return FALSE;
		}

		// and the same with Status
		if (filter1.aStatus.GetSize() || filter2.aStatus.GetSize())
		{
			if (!Misc::MatchAll(filter1.aStatus, filter2.aStatus))
				return FALSE;
		}

		// and the same with Version
		if (filter1.aVersions.GetSize() || filter2.aVersions.GetSize())
		{
			if (!Misc::MatchAll(filter1.aVersions, filter2.aVersions))
				return FALSE;
		}

		return TRUE;
	}
	
	FILTER_SHOW nShow;
	FILTER_DATE nStartBy, nDueBy;
	CString sTitle;
	FILTER_TITLE nTitleOption;
	int nPriority, nRisk;
	CStringArray aCategories, aAllocTo, aTags;
	CStringArray aStatus, aAllocBy, aVersions;
	DWORD dwFlags;
	COleDateTime dtUserStart, dtUserDue;

};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDSORTCOLUMN
{
	TDSORTCOLUMN(TDC_COLUMN nSortBy = TDCC_NONE, BOOL bSortAscending = -1) 
		: nBy(nSortBy), bAscending(bSortAscending)
	{
	}

	BOOL IsSorting() const
	{
		return (nBy != TDCC_NONE);
	}

	BOOL IsSortingBy(TDC_COLUMN nSortBy) const
	{
		return (nBy == nSortBy);
	}
	
	BOOL IsSortingByCustom() const
	{
		return ((nBy >= TDCC_CUSTOMCOLUMN_FIRST) && (nBy <= TDCC_CUSTOMCOLUMN_LAST));
	}

	BOOL Verify() const
	{
		return Verify(nBy);
	}

	void Validate()
	{
		if (!Verify())
			nBy = TDCC_NONE;
	}

	static BOOL Verify(TDC_COLUMN nSortBy) 
	{
		return (((nSortBy >= TDCC_FIRST) && (nSortBy < TDCC_COUNT)) ||
				(nSortBy == TDCC_CLIENT) || (nSortBy == TDCC_NONE));
	}

	TDC_COLUMN nBy;
	BOOL bAscending;
};

struct TDSORTCOLUMNS
{
	TDSORTCOLUMNS() : col1(TDCC_NONE), col2(TDCC_NONE), col3(TDCC_NONE)
	{
	}
	
	TDSORTCOLUMNS(TDC_COLUMN nBy, BOOL bAscending = -1) 
		:  col1(nBy, bAscending), col2(TDCC_NONE), col3(TDCC_NONE)

	{
	}
	
	BOOL IsSorting() const
	{
		return col1.IsSorting();
	}
	
	BOOL IsSortingBy(TDC_COLUMN nSortBy) const
	{
		if (col1.IsSortingBy(nSortBy))
			return TRUE;

		if (col1.IsSorting())
		{
			if (col2.IsSortingBy(nSortBy))
				return TRUE;
			
			if (col2.IsSorting())
			{
				return col3.IsSortingBy(nSortBy);
			}
		}

		// else
		return FALSE;
	}

	BOOL Verify() const
	{
		return (col1.Verify() && col2.Verify() && col3.Verify());
	}

	void Validate()
	{
		col1.Validate();
		col2.Validate();
		col3.Validate();
	}

	TDSORTCOLUMN col1, col2, col3;
};

struct TDSORT
{
	TDSORT(TDC_COLUMN nBy = TDCC_NONE, BOOL bAscending = -1) 
		: 
		single(nBy, bAscending),
		multi(nBy, bAscending),
		bModSinceLastSort(FALSE), 
		bMulti(FALSE)
	{
	}

	BOOL IsSorting() const
	{
		if (bMulti)
			return multi.IsSorting();

		// else
		return single.IsSorting();
	}

	BOOL IsSortingBy(TDC_COLUMN nBy, BOOL bCheckMulti) const
	{
		if (!bMulti)
			return single.IsSortingBy(nBy);

		// else multi-sorting
		if (bCheckMulti)
			return multi.IsSortingBy(nBy);

		// else
		return FALSE;
	}

	BOOL SetSortBy(TDC_COLUMN nBy, BOOL bAscending)
	{
		single.nBy = nBy;
		single.bAscending = bAscending;
		bMulti = FALSE;

		return TRUE;
	}

	BOOL SetSortBy(const TDSORTCOLUMN& col)
	{
		return SetSortBy(col.nBy, col.bAscending);
	}

	BOOL SetSortBy(const TDSORTCOLUMNS& cols)
	{
		multi = cols;
		bMulti = TRUE;
		
		return TRUE;
	}

	BOOL Verify() const
	{
		return (single.Verify() && multi.Verify());
	}

	void Validate()
	{
		single.Validate();
		multi.Validate();
	}

	TDSORTCOLUMN single;
	TDSORTCOLUMNS multi;

	BOOL bMulti;
	BOOL bModSinceLastSort;
};

struct TDSORTPARAMS
{
	TDSORTPARAMS(const CToDoCtrl& tdctrl, const TDSORT& params)
		: 
		tdc(tdctrl), 
		sort(params),
		bSortChildren(TRUE), 
		bSortDueTodayHigh(FALSE), 
		dwTimeTrackID(0) 
	{
	}

	TDSORTPARAMS(const CToDoCtrl& tdctrl, const TDSORTCOLUMN& col)
		: 
		tdc(tdctrl), 
		bSortChildren(TRUE), 
		bSortDueTodayHigh(FALSE), 
		dwTimeTrackID(0) 
	{
		sort.SetSortBy(col);
	}		
		
	TDSORTPARAMS(const CToDoCtrl& tdctrl, const TDSORTCOLUMNS& cols)
		: 
		tdc(tdctrl), 
		bSortChildren(TRUE), 
		bSortDueTodayHigh(FALSE), 
		dwTimeTrackID(0) 
	{
		sort.SetSortBy(cols);
	}		

	TDSORT sort;
	const CToDoCtrl& tdc;
	BOOL bSortChildren;
	BOOL bSortDueTodayHigh;
	DWORD dwTimeTrackID;
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCATTRIBUTE
{
	TDC_ATTRIBUTE attrib;
	UINT nAttribResID;
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCCADATA
{
	TDCCADATA(const CString& sValue = _T("")) { Set(sValue); }
	TDCCADATA(double dValue) { Set(dValue); }
	TDCCADATA(const CStringArray& aValues) { Set(aValues); }
	TDCCADATA(int nValue) { Set(nValue); }
	TDCCADATA(const COleDateTime& dtValue) { Set(dtValue); }
	TDCCADATA(bool bValue) { Set(bValue); }

	TDCCADATA(const TDCCADATA& data)
	{
		*this = data;
	}

	TDCCADATA& operator=(const TDCCADATA& data)
	{
		sData = data.sData;
		return *this;
	}

	BOOL operator==(const TDCCADATA& data) const { return sData == data.sData; }
	BOOL operator!=(const TDCCADATA& data) const { return sData != data.sData; }

	BOOL IsEmpty() const { return sData.IsEmpty(); }

	CString AsString() const { return sData; }
	double AsDouble() const { return _ttof(sData); }
	int AsArray(CStringArray& aValues) const { return Misc::Split(sData, aValues, '\n', TRUE); }
	int AsInteger() const { return _ttoi(sData); } 
	COleDateTime AsDate() const { return _ttof(sData); }
	bool AsBool() const { return !sData.IsEmpty(); }

	void Set(const CString& sValue) { sData = sValue; }
	void Set(double dValue) { sData.Format(_T("%lf"), dValue); }
	void Set(const CStringArray& aValues) { sData = Misc::FormatArray(aValues, '\n'); }
	void Set(int nValue) { sData.Format(_T("%d"), nValue); }
	void Set(const COleDateTime& dtValue) { sData.Format(_T("%lf"), dtValue); }

	void Set(bool bValue, TCHAR nChar = 0) 
	{ 
		if (bValue)
		{
			if (nChar > 0)
				sData.Insert(0, nChar);
			else
				sData.Insert(0, '+');
		}
		else
			sData.Empty(); 
	}

protected:
	CString sData;
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TASKFILE_HEADER
{
	TASKFILE_HEADER() : bArchive(-1), bUnicode(-1), dwNextID(0), nFileFormat(-1), nFileVersion(-1) {}

	CString sXmlHeader;
	CString sProjectName;
	CString sFileName;
	CString sCheckedOutTo;
	BOOL bArchive;
	BOOL bUnicode;
	COleDateTime dtEarliestDue;
	DWORD dwNextID;
	int nFileFormat;
	int nFileVersion;
};

/////////////////////////////////////////////////////////////////////////////////////////////

struct TDCCOLEDITFILTERVISIBILITY
{
	TDCCOLEDITFILTERVISIBILITY() : nShowEditsAndFilters(TDLSA_ASCOLUMN) {}

	TDCCOLEDITFILTERVISIBILITY(const TDCCOLEDITFILTERVISIBILITY& vis)
	{
		*this = vis;
	}
	
	TDCCOLEDITFILTERVISIBILITY& operator=(const TDCCOLEDITFILTERVISIBILITY& vis)
	{
		aVisibleEdits.Copy(vis.aVisibleEdits);
		aVisibleColumns.Copy(vis.aVisibleColumns);
		aVisibleFilters.Copy(vis.aVisibleFilters);

		nShowEditsAndFilters = vis.nShowEditsAndFilters;
		
		return *this;
	}
	
	BOOL operator!=(const TDCCOLEDITFILTERVISIBILITY& vis) const
	{
		return !(*this == vis);
	}
	
	BOOL operator==(const TDCCOLEDITFILTERVISIBILITY& vis) const
	{
		return (Misc::MatchAllT(aVisibleColumns, vis.aVisibleColumns) && 
				Misc::MatchAllT(aVisibleEdits, vis.aVisibleEdits) &&
				Misc::MatchAllT(aVisibleFilters, vis.aVisibleFilters) &&
				(nShowEditsAndFilters == vis.nShowEditsAndFilters));
	}

	void Clear()
	{
		aVisibleColumns.RemoveAll();
		aVisibleEdits.RemoveAll();
		aVisibleFilters.RemoveAll();

		nShowEditsAndFilters = TDLSA_ASCOLUMN;
	}

	BOOL CheckForDiff(const TDCCOLEDITFILTERVISIBILITY& vis, 
						BOOL& bColumnChange, BOOL& bEditChange, BOOL& bFilterChange) const
	{
		bEditChange = !Misc::MatchAllT(aVisibleEdits, vis.aVisibleEdits);
		bColumnChange = !Misc::MatchAllT(aVisibleColumns, vis.aVisibleColumns);
		bFilterChange = !Misc::MatchAllT(aVisibleEdits, vis.aVisibleEdits);
		
		return (bEditChange || bColumnChange || bFilterChange);
	}

	TDL_SHOWATTRIB GetShowEditsAndFilters() const 
	{
		return nShowEditsAndFilters;
	}
	
	void SetShowEditsAndFilters(TDL_SHOWATTRIB nShow)
	{
		nShowEditsAndFilters = nShow;

		UpdateEditAndFilterVisibility();
	}

	const CTDCColumnIDArray& GetVisibleColumns() const
	{
		return aVisibleColumns;
	}

	void SetVisibleColumns(const CTDCColumnIDArray& aColumns)
	{
		aVisibleColumns.Copy(aColumns);
	}

	void SetAllColumnsVisible(BOOL bVisible = TRUE)
	{
		aVisibleColumns.RemoveAll();

		for (int nCol = TDCC_FIRST; nCol < TDCC_COUNT; nCol++)
		{
			if (IsSupportedColumn((TDC_COLUMN)nCol) && bVisible)
				aVisibleColumns.Add((TDC_COLUMN)nCol);
		}

		UpdateEditAndFilterVisibility();
	}
	
	BOOL SetAllEditsAndFiltersVisible(BOOL bVisible = TRUE)
	{
		ASSERT(nShowEditsAndFilters == TDLSA_ANY);

		if (nShowEditsAndFilters != TDLSA_ANY)
			return FALSE;

		aVisibleEdits.RemoveAll();
		aVisibleFilters.RemoveAll();
		
		if (bVisible)
		{
			GetAllEditFields(aVisibleEdits);
			GetAllFilterFields(aVisibleFilters);
		}

		return TRUE;
	}

	const CTDCAttributeArray& GetVisibleEditFields() const
	{
		return aVisibleEdits;
	}
	
	const CTDCAttributeArray& GetVisibleFilterFields() const
	{
		return aVisibleFilters;
	}

	BOOL SetVisibleEditFields(const CTDCAttributeArray& aAttrib)
	{
		// only supported for 'any' attribute
		ASSERT(nShowEditsAndFilters == TDLSA_ANY);
		
		if (nShowEditsAndFilters != TDLSA_ANY)
			return FALSE;

		// check and add attributes 
		aVisibleEdits.RemoveAll();

		int nItem = aAttrib.GetSize();
		BOOL bAnySupported = FALSE;

		while (nItem--)
		{
			if (IsSupportedEdit(aAttrib[nItem]))
			{
				aVisibleEdits.Add(aAttrib[nItem]);
				bAnySupported = TRUE;
			}
		}
		
		return bAnySupported;
	}

	BOOL SetVisibleFilterFields(const CTDCAttributeArray& aAttrib)
	{
		// only supported for 'any' attribute
		ASSERT(nShowEditsAndFilters == TDLSA_ANY);
		
		if (nShowEditsAndFilters != TDLSA_ANY)
			return FALSE;

		// check and add attributes 
		aVisibleFilters.RemoveAll();

		int nItem = aAttrib.GetSize();
		BOOL bAnySupported = FALSE;

		while (nItem--)
		{
			if (IsSupportedFilter(aAttrib[nItem]))
			{
				aVisibleFilters.Add(aAttrib[nItem]);
				bAnySupported = TRUE;
			}
		}
		
		return bAnySupported;
	}
	
	BOOL IsEditFieldVisible(TDC_ATTRIBUTE nAttrib) const
	{
		// weed out unsupported attributes
		if (!IsSupportedEdit(nAttrib))
			return FALSE;

		switch (nShowEditsAndFilters)
		{
		case TDLSA_ALL:
			return TRUE;

		case TDLSA_ASCOLUMN:
			return IsColumnVisible(TDC::MapAttributeToColumn(nAttrib));

		case TDLSA_ANY:
			return (Misc::FindT(aVisibleEdits, nAttrib) != -1);
		}

		// how did we get here?
		ASSERT(0);
		return FALSE;
	}
	
	BOOL IsFilterFieldVisible(TDC_ATTRIBUTE nAttrib) const
	{
		// weed out unsupported attributes
		if (!IsSupportedFilter(nAttrib))
			return FALSE;

		switch (nShowEditsAndFilters)
		{
		case TDLSA_ALL:
			return TRUE;

		case TDLSA_ASCOLUMN:
			return IsColumnVisible(TDC::MapAttributeToColumn(nAttrib));

		case TDLSA_ANY:
			return (Misc::FindT(aVisibleFilters, nAttrib) != -1);
		}

		// how did we get here?
		ASSERT(0);
		return FALSE;
	}

	BOOL IsColumnVisible(TDC_COLUMN nCol) const
	{
		// special cases
		switch (nCol)
		{
		case TDCC_NONE:
		case TDCC_COLOR:
			return FALSE;
		}

		// else
		return (Misc::FindT(aVisibleColumns, nCol) != -1);
	}
	
	BOOL SetEditFieldVisible(TDC_ATTRIBUTE nAttrib, BOOL bVisible = TRUE)
	{
		// only supported for 'any' attribute
		ASSERT(nShowEditsAndFilters == TDLSA_ANY);

		if (nShowEditsAndFilters != TDLSA_ANY)
			return FALSE;

		// validate attribute
		ASSERT (IsSupportedEdit(nAttrib));

		if (!IsSupportedEdit(nAttrib))
		{
			return FALSE;
		}
		else if (bVisible)
		{
			// Times cannot be shown if corresponding date column is hidden
			switch (nAttrib)
			{
			case TDCA_DUETIME:
				bVisible &= IsEditFieldVisible(TDCA_DUEDATE);
				break;
				
			case TDCA_DONETIME:
				bVisible &= IsEditFieldVisible(TDCA_DONEDATE);
				break;
				
			case TDCA_STARTTIME:
				bVisible &= IsEditFieldVisible(TDCA_STARTDATE);
				break;
			}
		}
		else // !bVisible
		{
			// Times cannot be shown if corresponding date column is hidden
			switch (nAttrib)
			{
			case TDCA_DUEDATE:
				SetEditFieldVisible(TDCA_DUETIME, FALSE);
				break;
				
			case TDCA_DONEDATE:
				SetEditFieldVisible(TDCA_DONETIME, FALSE);
				break;
				
			case TDCA_STARTDATE:
				SetEditFieldVisible(TDCA_STARTTIME, FALSE);
				break;
			}
		}

		int nFind = Misc::FindT(aVisibleEdits, nAttrib);
		BOOL bFound = (nFind != -1);
		
		if (bVisible && !bFound)
		{
			aVisibleEdits.Add(nAttrib);
		}
		else if (!bVisible && bFound)
		{
			aVisibleEdits.RemoveAt(nFind);
		}

		return TRUE;
	}
	
	BOOL SetFilterFieldVisible(TDC_ATTRIBUTE nAttrib, BOOL bVisible = TRUE)
	{
		// only supported for 'any' attribute
		ASSERT(nShowEditsAndFilters == TDLSA_ANY);

		if (nShowEditsAndFilters != TDLSA_ANY)
			return FALSE;

		// validate attribute
		ASSERT (IsSupportedFilter(nAttrib));

		if (!IsSupportedFilter(nAttrib))
			return FALSE;

		int nFind = Misc::FindT(aVisibleFilters, nAttrib);
		BOOL bFound = (nFind != -1);
		
		if (bVisible && !bFound)
		{
			aVisibleFilters.Add(nAttrib);
		}
		else if (!bVisible && bFound)
		{
			aVisibleFilters.RemoveAt(nFind);
		}

		return TRUE;
	}

	void SetColumnVisible(TDC_COLUMN nCol, BOOL bVisible = TRUE)
	{
		// special cases
		if (nCol == TDCC_NONE)
		{
			return;
		}
		else if (bVisible)
		{
			// Times cannot be shown if corresponding date column is hidden
			switch (nCol)
			{
			case TDCC_DUETIME:
				bVisible &= IsColumnVisible(TDCC_DUEDATE);
				break;
				
			case TDCC_DONETIME:
				bVisible &= IsColumnVisible(TDCC_DONEDATE);
				break;
				
			case TDCC_STARTTIME:
				bVisible &= IsColumnVisible(TDCC_STARTDATE);
				break;
			}
		}
		else // !bVisible
		{
			// Times cannot be shown if corresponding date column is hidden
			switch (nCol)
			{
			case TDCC_DUEDATE:
				SetColumnVisible(TDCC_DUETIME, FALSE);
				break;
				
			case TDCC_DONEDATE:
				SetColumnVisible(TDCC_DONETIME, FALSE);
				break;
				
			case TDCC_STARTDATE:
				SetColumnVisible(TDCC_STARTTIME, FALSE);
				break;
			}
		}

		int nFind = Misc::FindT(aVisibleColumns, nCol);
		BOOL bFound = (nFind != -1);
		
		if (bVisible && !bFound) // show
		{
			aVisibleColumns.Add(nCol);
		}
		else if (!bVisible && bFound) // hide
		{
			aVisibleColumns.RemoveAt(nFind);
		}

		UpdateEditAndFilterVisibility();
	}

	static BOOL IsSupportedEdit(TDC_ATTRIBUTE nAttrib)
	{
		// weed out unsupported attributes
		switch (nAttrib)
		{
		case TDCA_TASKNAME:
		case TDCA_COMMENTS:
		case TDCA_PROJNAME:
		case TDCA_FLAG:
		case TDCA_CREATIONDATE:
		case TDCA_CREATEDBY:
		case TDCA_POSITION:
		case TDCA_ID:
		case TDCA_LASTMOD:
			return FALSE;
		}

		// else
		return ((nAttrib >= TDCA_FIRSTATTRIBUTE) && (nAttrib < TDCA_ATTRIBUTECOUNT));
	}

	static BOOL IsSupportedFilter(TDC_ATTRIBUTE nAttrib)
	{
		switch (nAttrib)
		{
		case TDCA_DUEDATE:
		case TDCA_STARTDATE:
		case TDCA_PRIORITY:
		case TDCA_ALLOCTO:
		case TDCA_ALLOCBY:
		case TDCA_STATUS:
		case TDCA_CATEGORY:
		case TDCA_RISK:			
		case TDCA_VERSION:		
		case TDCA_TAGS:
			return TRUE;
		}

		// else
		return FALSE;
	}
	
	static BOOL IsSupportedColumn(TDC_COLUMN nColumn)
	{
		// weed out unsupported columns
		switch (nColumn)
		{
		case TDCC_COLOR:
			return FALSE;
		}

		// else
		return ((nColumn >= TDCC_FIRST) && (nColumn < TDCC_CUSTOMCOLUMN_FIRST));
	}

protected:
	CTDCColumnIDArray aVisibleColumns;
	CTDCAttributeArray aVisibleEdits;
	CTDCAttributeArray aVisibleFilters;
	TDL_SHOWATTRIB nShowEditsAndFilters;

protected:
	int UpdateEditAndFilterVisibility()
	{
		if (nShowEditsAndFilters != TDLSA_ANY)
		{
			aVisibleEdits.RemoveAll();
			
			for (int nEdit = TDCA_FIRSTATTRIBUTE; nEdit < TDCA_ATTRIBUTECOUNT; nEdit++)
			{
				if (IsEditFieldVisible((TDC_ATTRIBUTE)nEdit))
					aVisibleEdits.Add((TDC_ATTRIBUTE)nEdit);
			}

			aVisibleFilters.RemoveAll();
			
			for (int nFilter = TDCA_FIRSTATTRIBUTE; nFilter < TDCA_ATTRIBUTECOUNT; nFilter++)
			{
				if (IsFilterFieldVisible((TDC_ATTRIBUTE)nFilter))
					aVisibleFilters.Add((TDC_ATTRIBUTE)nFilter);
			}
		}
		
		return (aVisibleEdits.GetSize() || aVisibleFilters.GetSize());
	}
	
	static int GetAllEditFields(CTDCAttributeArray& aAttrib)
	{
		aAttrib.RemoveAll();
		
		for (int nAttrib = TDCA_FIRSTATTRIBUTE; nAttrib < TDCA_ATTRIBUTECOUNT; nAttrib++)
		{
			if (IsSupportedEdit((TDC_ATTRIBUTE)nAttrib))
				aAttrib.Add((TDC_ATTRIBUTE)nAttrib);
		}
		
		return aAttrib.GetSize();
	}
	
	static int GetAllFilterFields(CTDCAttributeArray& aAttrib)
	{
		aAttrib.RemoveAll();
		
		for (int nAttrib = TDCA_FIRSTATTRIBUTE; nAttrib < TDCA_ATTRIBUTECOUNT; nAttrib++)
		{
			if (IsSupportedFilter((TDC_ATTRIBUTE)nAttrib))
				aAttrib.Add((TDC_ATTRIBUTE)nAttrib);
		}
		
		return aAttrib.GetSize();
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif // AFX_TDCSTRUCT_H__5951FDE6_508A_4A9D_A55D_D16EB026AEF7__INCLUDED_
