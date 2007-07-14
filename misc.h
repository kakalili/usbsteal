#ifndef MISC_H
#define MISS_H

typedef std::vector<DWORD> PIDGroup;

extern DWORD enablePrivilege();
extern void reportError(LPCSTR funcName);
extern void copierThreadFunc(LPVOID *);
extern const std::string& getConfigPath();
extern PIDGroup GetDestProcessID(LPCSTR lpcExeName);
extern std::string getSystemPath();

#endif
