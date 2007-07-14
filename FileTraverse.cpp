#include <windows.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <tchar.h>

/** FileTraverse.cpp: �ݹ�õ�Ŀ¼�������ļ�����Ŀ¼��
 * $Id: FileTraverse.cpp 419 2007-07-04 01:51:33Z outmatch $
 */

/** �ݹ�õ�Ŀ¼�������ļ�����Ŀ¼��
 *  @param lpszDirectory Ŀ¼��
 *  @param bIncludeSubDirectory �Ƿ������Ŀ¼
 *  @param vsFiles       �洢�ļ���
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
