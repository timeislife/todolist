#include "stdafx.h"
#include "webmisc.h"
#include "misc.h"

#include <wininet.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

BOOL WebMisc::IsOnline()
{
    DWORD dwState = 0; 
    DWORD dwSize = sizeof(DWORD);
	
    return InternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize) && 
		(dwState & INTERNET_STATE_CONNECTED);
}

BOOL WebMisc::DeleteCacheEntry(LPCTSTR szURI)
{
	BOOL bSuccess = FALSE;

#if _MSC_VER >= 1400
	bSuccess = DeleteUrlCacheEntry(szURI);
#elif _UNICODE
	LPSTR szAnsiPath = Misc::WideToMultiByte(szURI);
	bSuccess = DeleteUrlCacheEntry(szAnsiPath);
	delete [] szAnsiPath;
#else
	bSuccess = DeleteUrlCacheEntry(szURI);
#endif

	return bSuccess;
}
