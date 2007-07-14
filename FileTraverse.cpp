#include <windows.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <tchar.h>

/** FileTraverse.cpp: 递归得到目录下所有文件名和目录名
 * $Id: FileTraverse.cpp 419 2007-07-04 01:51:33Z outmatch $
 */

/** 递归得到目录下所有文件名和目录名
 *  @param lpszDirectory 目录名
 *  @param bIncludeSubDirectory 是否包括子目录
 *  @param vsFiles       存储文件名
 */
HRESULT   EnumDirectory(LPCTSTR   lpszDirectory,BOOL   bIncludeSubDirectory, std::vector<std::string>& vsFiles)  
{  
	HANDLE hFindHandle;  
	WIN32_FIND_DATA wfd;  

	CHAR sz[MAX_PATH];  
	::GetCurrentDirectory(MAX_PATH,sz);  

	CHAR szFindFilter[MAX_PATH];  
	_tmakepath(szFindFilter,NULL,lpszDirectory,_T("*"),_T("*"));  

	hFindHandle   =   ::FindFirstFile(szFindFilter,&wfd);  
	if(hFindHandle   ==   INVALID_HANDLE_VALUE)  
	{  
		return E_FAIL;  
	}  

	do  
	{  
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && bIncludeSubDirectory)  
		{  
			if(_tcscmp(wfd.cFileName,_T(".."))   &&  
					_tcscmp(wfd.cFileName,_T(".")))  
			{  

				CHAR szNext[MAX_PATH];  
				_tmakepath(szNext,NULL,lpszDirectory,wfd.cFileName,NULL);
				
				EnumDirectory(szNext,bIncludeSubDirectory, vsFiles);  
			}  
		}
		std::string s = lpszDirectory;
		if(s[s.size()-1] != '\\') {
			s += '\\';
		}
		s += wfd.cFileName;
		vsFiles.push_back(s);
	} while(::FindNextFile(hFindHandle,&wfd));  

	::FindClose(hFindHandle);  
	::SetCurrentDirectory(sz);  
	return   NOERROR;  
}
