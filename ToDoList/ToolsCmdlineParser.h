// ToolsCmdlineParser.h: interface for the CToolsCmdlineParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TOOLSCMDLINEPARSER_H__D0CE619C_1C78_4BF1_9B60_CBCBD89C0B32__INCLUDED_)
#define AFX_TOOLSCMDLINEPARSER_H__D0CE619C_1C78_4BF1_9B60_CBCBD89C0B32__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>

enum CLA_TYPE
{
	CLAT_NONE,
	CLAT_PATHNAME,      
	CLAT_FILETITLE,    
	CLAT_FOLDER,        
	CLAT_FILENAME,      
	CLAT_USERFILE,
	CLAT_USERFOLDER,
	CLAT_USERTEXT,
	CLAT_USERDATE,
	CLAT_TODAYSDATE, // this gets handled internally
	CLAT_TODOLIST, 
	CLAT_SELTASKID,      
	CLAT_SELTASKTITLE,      
	CLAT_SELTASKEXTID,      
	CLAT_SELTASKCOMMENTS,      
	CLAT_SELTASKFILELINK,      
	CLAT_SELTASKALLOCBY,      
	CLAT_SELTASKALLOCTO,      
};

struct CMDLINEARG
{
	CLA_TYPE nType;
	CString sName;
	CString sLabel;
	CString sDefValue;

	CString sPlaceHolder; // for when we want to replace it
};

typedef CArray<CMDLINEARG, CMDLINEARG&> CCLArgArray;

class CToolsCmdlineParser  
{
public:
	CToolsCmdlineParser(LPCTSTR szCmdLine = NULL);
	virtual ~CToolsCmdlineParser();

	void SetCmdLine(LPCTSTR szCmdLine); // will clear previous results
	CString GetCmdLine() const { return m_sCmdLine; }

	int GetArguments(CCLArgArray& aArgs) const; // all
	int GetUserArguments(CCLArgArray& aArgs) const; // USER type only
	BOOL ReplaceArgument(CLA_TYPE nType, LPCTSTR szValue);
	BOOL ReplaceArgument(CLA_TYPE nType, DWORD dwValue);
	BOOL ReplaceArgument(LPCTSTR szName, LPCTSTR szValue); // for USER types sharing the same type

	BOOL HasArgument(CLA_TYPE nType) const;
	BOOL HasTasklistArgument() const;
	BOOL IsUserInputRequired() const;

protected:
	CCLArgArray m_aArgs, m_aUserArgs; // user args are arguments which reference CMDLINEARG::sName
	CString m_sCmdLine;

	static CMap<CString, LPCTSTR, CLA_TYPE, CLA_TYPE&> s_mapTypes;

	void ParseCmdLine();
	CLA_TYPE GetType(LPCTSTR szVarType);
	BOOL IsUserInputType(LPCTSTR szVarType);
	BOOL ReplaceArgument(int nArg, LPCTSTR szValue);
};

#endif // !defined(AFX_TOOLSCMDLINEPARSER_H__D0CE619C_1C78_4BF1_9B60_CBCBD89C0B32__INCLUDED_)
