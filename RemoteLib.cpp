/////////////////////////////////////////////////////////////////////////
// RemoteLib.cpp
//-----------------------------------------------------------------------
// Implementation for All-Windows-Platform RemoteLib APIs.
//-----------------------------------------------------------------------
// Author:
//
// Abin (abinn32@yahoo.com)
// Homepage: http://www.wxjindu.com/abin/
/////////////////////////////////////////////////////////////////////////

#include "RemoteLib.h"
#include "Kernel32Funcs.h"

/////////////////////////////////////////////////////////////////////////
// Process-Wide Shared Data
//-----------------------------------------------------------------------
// These data are shared among different processes, I use a mutex to
// protect them from asynchronous access.
/////////////////////////////////////////////////////////////////////////
#pragma data_seg ("SHARED")
static HHOOK g_hHook = NULL; // Hook handle.
static HMODULE g_hModule = NULL; // DLL module handle.
static wchar_t g_szDllPath[MAX_PATH + 1] = { 0 }; // DLL path, use wchar_t type for safety.
static DWORD g_dwProcError = 0; // Last error code occurred in HookProc.
static DWORD g_dwProcResult = 0; // Procedure result.
#pragma data_seg ()
#pragma comment(linker, "/section:SHARED,RWS")

/////////////////////////////////////////////////////////////////////////
// Remote Operation Type Flags
//-----------------------------------------------------------------------
// These values will be passed into SendMessage calls, as LPARAM's.
// The remote window perform appropriate operations according to LPARAM
// values. See HookProc implementation for details.
/////////////////////////////////////////////////////////////////////////
#define REMOTE_LOADLIBRARY			0xfffffff0	// Remote LoadLibrary call.
#define REMOTE_FREELIBRARY			0xfffffff1	// Remote FreeLibrary call.
#define REMOTE_GETMODULEHANDLE		0xfffffff2	// Remote GetModuleHandle call.
#define REMOTE_GETMODULEFILENAME	0xfffffff3	// Remote GetModuleFileName call.

/////////////////////////////////////////////////////////////////////////
// Module Variables
//-----------------------------------------------------------------------
// Global variables that are not shared.
/////////////////////////////////////////////////////////////////////////
HINSTANCE g_hModInstance = NULL; // Instance handle of this utility DLL.
KERNEL32FUNCS g_kernel32 = { 0 }; // Kernel32 functions

// Macro to make our code less messy.
#define KERNEL32_PROC(x)	g_kernel32.##x## = (fn##x##)::GetProcAddress(::GetModuleHandleA("Kernel32"), #x ##)

/////////////////////////////////////////////////////////////////////////
// Non-Exported Functions
//-----------------------------------------------------------------------
// Internal helper function prototypes.
/////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD  ul_reason_for_call, LPVOID lpReserved);
LRESULT CALLBACK HookProcA(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK HookProcW(int code, WPARAM wParam, LPARAM lParam);
HANDLE MutexLock(DWORD dwTimeout);
BOOL MutexFree(HANDLE hMutex);
DWORD PathAccessA(HWND hTargetWnd, LPCSTR lpszDllPath, LPARAM lParam, DWORD dwTimeout);
DWORD PathAccessW(HWND hTargetWnd, LPCWSTR lpszDllPath, LPARAM lParam, DWORD dwTimeout);
DWORD ModuleAccessA(HWND hTargetWnd, LPARAM lParam, HMODULE hModule, DWORD dwTimeout, LPSTR lpszBuffer = NULL, DWORD dwBufferSize = 0);
DWORD ModuleAccessW(HWND hTargetWnd, LPARAM lParam, HMODULE hModule, DWORD dwTimeout, LPWSTR lpszBuffer = NULL, DWORD dwBufferSize = 0);

#if 0 
///////////////////////////////////////////////////////////////////////
// DllMain
//---------------------------------------------------------------------
// Obtains DLL module handle and registers unique message here.
///////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD  ul_reason_for_call, LPVOID lpReserved)
{

}
#endif

///////////////////////////////////////////////////////////////////////
// HookProcA
//---------------------------------------------------------------------
// Hook procedure for ANSI.
///////////////////////////////////////////////////////////////////////
LRESULT CALLBACK HookProcA(int code, WPARAM wParam, LPARAM lParam)
{	
	if (code == HCBT_SYSCOMMAND && (lParam == REMOTE_LOADLIBRARY || lParam == REMOTE_FREELIBRARY || lParam == REMOTE_GETMODULEHANDLE || lParam == REMOTE_GETMODULEFILENAME))
	{
		if (g_hHook)
		{
			// Remove the hook ASAP
			::UnhookWindowsHookEx(g_hHook);
			g_hHook = NULL;
		}		

		switch (lParam)
		{
		case REMOTE_LOADLIBRARY:
			g_dwProcResult = (DWORD)::LoadLibraryA((LPCSTR)g_szDllPath);
			g_dwProcError = ::GetLastError();
			break;

		case REMOTE_FREELIBRARY:
			g_dwProcResult = (DWORD)::FreeLibrary(g_hModule);
			g_dwProcError = ::GetLastError();
			break;

		case REMOTE_GETMODULEHANDLE:
			g_dwProcResult = (DWORD)::GetModuleHandleA((LPCSTR)g_szDllPath);
			g_dwProcError = ::GetLastError();
			break;

		case REMOTE_GETMODULEFILENAME:
			// copy the dll path to g_szDllPath first, this is important!
			g_szDllPath[0] = 0;
			g_dwProcResult = ::GetModuleFileNameA(g_hModule, (LPSTR)g_szDllPath, MAX_PATH);
			g_dwProcError = ::GetLastError();
			break;

		default:
			g_dwProcResult = 0;
			break;
		}

		return 1; // Returns 1 so the window won't receive this "meaningless" message
	}

	return ::CallNextHookEx(g_hHook, code, wParam, lParam);
}


///////////////////////////////////////////////////////////////////////
// HookProcW
//---------------------------------------------------------------------
// Hook procedure for wide char.
///////////////////////////////////////////////////////////////////////
LRESULT CALLBACK HookProcW(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HCBT_SYSCOMMAND && (lParam == REMOTE_LOADLIBRARY || lParam == REMOTE_FREELIBRARY || lParam == REMOTE_GETMODULEHANDLE || lParam == REMOTE_GETMODULEFILENAME))
	{
		if (g_hHook)
		{
			// Remove the hook ASAP
			::UnhookWindowsHookEx(g_hHook);
			g_hHook = NULL;
		}		

		switch (lParam)
		{
		case REMOTE_LOADLIBRARY:
			if (g_kernel32.LoadLibraryW == NULL)
			{
				g_dwProcResult = 0;
				g_dwProcError = ERROR_CALL_NOT_IMPLEMENTED;
			}
			else
			{
				g_dwProcResult = (DWORD)g_kernel32.LoadLibraryW((LPCWSTR)g_szDllPath);
				g_dwProcError = ::GetLastError();
			}
			
			break;

		case REMOTE_FREELIBRARY:
			g_dwProcResult = (DWORD)::FreeLibrary(g_hModule);
			g_dwProcError = ::GetLastError();
			break;

		case REMOTE_GETMODULEHANDLE:
			if (g_kernel32.GetModuleHandleW == NULL)
			{
				g_dwProcResult = 0;
				g_dwProcError = ERROR_CALL_NOT_IMPLEMENTED;
			}
			else
			{
				g_dwProcResult = (DWORD)g_kernel32.GetModuleHandleW((LPCWSTR)g_szDllPath);
				g_dwProcError = ::GetLastError();
			}
			break;

		case REMOTE_GETMODULEFILENAME:
			if (g_kernel32.GetModuleFileNameW == NULL)
			{
				g_dwProcResult = 0;
				g_dwProcError = ERROR_CALL_NOT_IMPLEMENTED;
			}
			else
			{
				// copy the dll path to g_szDllPath first, this is important!
				g_dwProcResult = g_kernel32.GetModuleFileNameW(g_hModule, (LPWSTR)g_szDllPath, MAX_PATH);
				g_dwProcError = ::GetLastError();
			}
			break;

		default:
			g_dwProcResult = 0;
			break;
		}

		return 1; // Returns 1 so the window won't receive this "meaningless" message
	}

	return ::CallNextHookEx(g_hHook, code, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////
// MutexLock
//---------------------------------------------------------------------
// Request ownership of the mutex to prevent asynchronous access to
// our shared global data.
///////////////////////////////////////////////////////////////////////
HANDLE MutexLock(DWORD dwTimeout)
{
	HANDLE hMutex = ::CreateMutexA(NULL, TRUE, "{D6371653-42F0-42B8-A11D-B12A4A7475F8}");
	if (hMutex == NULL)
		return NULL;

	if (::WaitForSingleObject(hMutex, dwTimeout) != WAIT_OBJECT_0)
	{
		::CloseHandle(hMutex);
		return NULL;
	}

	// Initializes shared data.
	g_szDllPath[0] = 0;
	g_hModule = NULL;
	g_dwProcError = ERROR_SUCCESS;
	g_dwProcResult = 0;
	return hMutex;
}

///////////////////////////////////////////////////////////////////////
// MutexFree
//---------------------------------------------------------------------
// Release ownership of the mutex.
///////////////////////////////////////////////////////////////////////
BOOL MutexFree(HANDLE hMutex)
{
	if (hMutex == NULL)
		return FALSE;

	// Cleanup shared data. This step is not actually necessary, but doing
	// so would never hurt, either.
	g_szDllPath[0] = 0;
	g_hModule = NULL;
	g_dwProcError = ERROR_SUCCESS;
	g_dwProcResult = 0;

	// Release mutex ownership.
	::ReleaseMutex(hMutex);
	::CloseHandle(hMutex);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////
// PathAccessA
//---------------------------------------------------------------------
// DLL path process for ANSI.
///////////////////////////////////////////////////////////////////////
DWORD PathAccessA(HWND hTargetWnd, LPCSTR lpszDllPath, LPARAM lParam, DWORD dwTimeout)
{
	DWORD dwProcID = 0;
	const DWORD TID = ::GetWindowThreadProcessId(hTargetWnd, &dwProcID);
	if (TID == 0)
	{
		::SetLastError(ERROR_INVALID_WINDOW_HANDLE);
		return 0;
	}

	if (dwProcID == ::GetCurrentProcessId())
	{
		// this is current process, the user wanna be funny?
		if (lParam == REMOTE_LOADLIBRARY)
		{
			return (DWORD)::LoadLibraryA(lpszDllPath);
		}
		else if (lParam == REMOTE_GETMODULEHANDLE)
		{
			return (DWORD)::GetModuleHandleA(lpszDllPath);
		}
		else
		{
			::SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		}
	}

	// Lock the mutex first before accessing ANY shared data!
	HANDLE hMutex = MutexLock(dwTimeout);
	if (hMutex == NULL)
	{
		::SetLastError(ERROR_LOCK_FAILED);
		return 0;
	}

	DWORD dwErrorCode = ERROR_SUCCESS;
	
	// We copy the DLL path to the shared buffer so the remote process can read it.
	::memset(g_szDllPath, 0, sizeof(g_szDllPath));
	::strncpy((LPSTR)g_szDllPath, lpszDllPath, MAX_PATH);

	g_hHook = ::SetWindowsHookEx(WH_CBT, (HOOKPROC)HookProcA, g_hModInstance, TID);
	if (g_hHook == NULL)
	{
		dwErrorCode = ::GetLastError();
		MutexFree(hMutex);
		::SetLastError(dwErrorCode);
		return 0;
	}
	
	DWORD lDummy;
	if (!::SendMessageTimeoutA(hTargetWnd, WM_SYSCOMMAND, 0, lParam, SMTO_ABORTIFHUNG | SMTO_BLOCK, dwTimeout, &lDummy))
		dwErrorCode = ERROR_TIMEOUT;
	else
		dwErrorCode = g_dwProcError;
	const DWORD RETCAL = g_dwProcResult;
	
	if (g_hHook)
	{
		// Make sure we remove the hook, in case it wasn't removed by "HookProc"
		::UnhookWindowsHookEx(g_hHook);
		g_hHook = NULL;
	}
	
	::SendMessageTimeoutA(hTargetWnd, WM_SYSCOMMAND, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, dwTimeout, &lDummy); // Force unloading this DLL
	MutexFree(hMutex);
	::SetLastError(dwErrorCode);
	return RETCAL;
}

///////////////////////////////////////////////////////////////////////
// PathAccessW
//---------------------------------------------------------------------
// DLL path process for wide char.
//////////////////////////////////////////////////////////////////
DWORD PathAccessW(HWND hTargetWnd, LPCWSTR lpszDllPath, LPARAM lParam, DWORD dwTimeout)
{
	DWORD dwProcID = 0;
	const DWORD TID = ::GetWindowThreadProcessId(hTargetWnd, &dwProcID);
	if (TID == 0)
	{
		::SetLastError(ERROR_INVALID_WINDOW_HANDLE);
		return 0;
	}

	if (dwProcID == ::GetCurrentProcessId())
	{
		// this is current process, the user wanna be funny?
		if (lParam == REMOTE_LOADLIBRARY)
		{
			if (g_kernel32.LoadLibraryW == NULL)
			{
				::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
				return 0;
			}

			return (DWORD)g_kernel32.LoadLibraryW(lpszDllPath);
		}
		else if (lParam == REMOTE_GETMODULEHANDLE)
		{
			if (g_kernel32.GetModuleHandleW == NULL)
			{
				::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
				return 0;
			}

			return (DWORD)g_kernel32.GetModuleHandleW(lpszDllPath);
		}
		else
		{
			::SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		}
	}

	// Lock the mutex first before accessing ANY shared data!
	HANDLE hMutex = MutexLock(dwTimeout);
	if (hMutex == NULL)
	{
		::SetLastError(ERROR_LOCK_FAILED);
		return 0;
	}

	DWORD dwErrorCode = ERROR_SUCCESS;
	
	// We copy the DLL path to the shared buffer so the remote process can read it.
	::memset(g_szDllPath, 0, sizeof(g_szDllPath));
	::wcsncpy((LPWSTR)g_szDllPath, lpszDllPath, MAX_PATH);

	g_hHook = ::SetWindowsHookEx(WH_CBT, (HOOKPROC)HookProcW, g_hModInstance, TID);
	if (g_hHook == NULL)
	{
		dwErrorCode = ::GetLastError();
		MutexFree(hMutex);
		::SetLastError(dwErrorCode);
		return 0;
	}
	
	DWORD lDummy;
	if (!::SendMessageTimeoutW(hTargetWnd, WM_SYSCOMMAND, 0, lParam, SMTO_ABORTIFHUNG | SMTO_BLOCK, dwTimeout, &lDummy))
		dwErrorCode = ERROR_TIMEOUT;
	else
		dwErrorCode = g_dwProcError;
	const DWORD RETCAL = g_dwProcResult;
	
	if (g_hHook)
	{
		// Make sure we remove the hook, in case it wasn't removed by "HookProc"
		::UnhookWindowsHookEx(g_hHook);
		g_hHook = NULL;
	}
	
	::SendMessageTimeoutW(hTargetWnd, WM_SYSCOMMAND, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, dwTimeout, &lDummy); // Force unloading this DLL
	MutexFree(hMutex);
	::SetLastError(dwErrorCode);
	return RETCAL;
}

DWORD ModuleAccessA(HWND hTargetWnd, LPARAM lParam, HMODULE hModule, DWORD dwTimeout, LPSTR lpszBuffer/*=NULL*/, DWORD dwBufferSize/*=0*/)
{
	if (lpszBuffer && dwBufferSize)
		lpszBuffer[0] = 0;

	DWORD dwProcID = 0;
	const DWORD TID = ::GetWindowThreadProcessId(hTargetWnd, &dwProcID);
	if (TID == 0)
	{
		::SetLastError(ERROR_INVALID_WINDOW_HANDLE);
		return 0;
	}

	if (dwProcID == ::GetCurrentProcessId())
	{
		// this is current process, the user wanna be funny?
		if (lParam == REMOTE_FREELIBRARY)
		{
			return (DWORD)::FreeLibrary(hModule);
		}
		else if (lParam == REMOTE_GETMODULEFILENAME)
		{
			return ::GetModuleFileNameA(hModule, lpszBuffer, dwBufferSize);
		}
		else
		{
			::SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		}
	}

	// Lock the mutex first before accessing ANY shared data!
	HANDLE hMutex = MutexLock(dwTimeout);
	if (hMutex == NULL)
	{
		::SetLastError(ERROR_LOCK_FAILED);
		return 0;
	}

	DWORD dwErrorCode = ERROR_SUCCESS;
	
	g_hHook = ::SetWindowsHookEx(WH_CBT, (HOOKPROC)HookProcA, g_hModInstance, TID);
	if (g_hHook == NULL)
	{
		dwErrorCode = ::GetLastError();
		MutexFree(hMutex);
		::SetLastError(dwErrorCode);
		return 0;
	}

	g_hModule = hModule;
	DWORD lDummy;
	if (!::SendMessageTimeoutA(hTargetWnd, WM_SYSCOMMAND, 0, lParam, SMTO_ABORTIFHUNG | SMTO_BLOCK, dwTimeout, &lDummy))
		dwErrorCode = ERROR_TIMEOUT;
	else
		dwErrorCode = g_dwProcError;
	DWORD dwRetCal = g_dwProcResult;

	if (g_hHook)
	{
		// Make sure we remove the hook, in case it wasn't removed by "HookProc"
		::UnhookWindowsHookEx(g_hHook);
		g_hHook = NULL;
	}

	if (lpszBuffer && dwBufferSize)
	{
		const int LEN = min(::strlen((LPCSTR)g_szDllPath), (int)dwBufferSize);
		if (LEN > 0)
		{
			::strncpy(lpszBuffer, (LPCSTR)g_szDllPath, LEN);
			lpszBuffer[LEN] = 0;
		}

		dwRetCal = (DWORD)LEN;
	}

	::SendMessageTimeoutA(hTargetWnd, WM_SYSCOMMAND, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, dwTimeout, &lDummy); // Force unloading this DLL
	MutexFree(hMutex);
	::SetLastError(dwErrorCode);
	return dwRetCal;
}

DWORD ModuleAccessW(HWND hTargetWnd, LPARAM lParam, HMODULE hModule, DWORD dwTimeout, LPWSTR lpszBuffer/*=NULL*/, DWORD dwBufferSize/*=0*/)
{
	if (lpszBuffer && dwBufferSize)
		lpszBuffer[0] = 0;

	DWORD dwProcID = 0;
	const DWORD TID = ::GetWindowThreadProcessId(hTargetWnd, &dwProcID);
	if (TID == 0)
	{
		::SetLastError(ERROR_INVALID_WINDOW_HANDLE);
		return 0;
	}

	if (dwProcID == ::GetCurrentProcessId())
	{
		// this is current process, the user wanna be funny?
		if (lParam == REMOTE_FREELIBRARY)
		{
			return (DWORD)::FreeLibrary(hModule);
		}
		else if (lParam == REMOTE_GETMODULEFILENAME)
		{
			if (g_kernel32.GetModuleFileNameW == NULL)
			{
				::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
				return 0;
			}

			return g_kernel32.GetModuleFileNameW(hModule, lpszBuffer, dwBufferSize);
		}
		else
		{
			::SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		}
	}


	// Lock the mutex first before accessing ANY shared data!
	HANDLE hMutex = MutexLock(dwTimeout);
	if (hMutex == NULL)
	{
		::SetLastError(ERROR_LOCK_FAILED);
		return 0;
	}

	DWORD dwErrorCode = ERROR_SUCCESS;
	
	g_hHook = ::SetWindowsHookEx(WH_CBT, (HOOKPROC)HookProcW, g_hModInstance, TID);
	if (g_hHook == NULL)
	{
		dwErrorCode = ::GetLastError();
		MutexFree(hMutex);
		::SetLastError(dwErrorCode);
		return 0;
	}

	g_hModule = hModule;
	DWORD lDummy;
	if (!::SendMessageTimeoutA(hTargetWnd, WM_SYSCOMMAND, 0, lParam, SMTO_ABORTIFHUNG | SMTO_BLOCK, dwTimeout, &lDummy))
		dwErrorCode = ERROR_TIMEOUT;
	else
		dwErrorCode = g_dwProcError;
	DWORD dwRetCal = g_dwProcResult;

	if (g_hHook)
	{
		// Make sure we remove the hook, in case it wasn't removed by "HookProc"
		::UnhookWindowsHookEx(g_hHook);
		g_hHook = NULL;
	}

	if (lpszBuffer && dwBufferSize)
	{
		const int LEN = min(::wcslen((LPCWSTR)g_szDllPath), (int)dwBufferSize);
		if (LEN > 0)
		{
			::wcsncpy(lpszBuffer, (LPCWSTR)g_szDllPath, LEN);
			lpszBuffer[LEN] = 0;
		}

		dwRetCal = (DWORD)LEN;
	}

	::SendMessageTimeoutA(hTargetWnd, WM_SYSCOMMAND, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, dwTimeout, &lDummy); // Force unloading this DLL
	MutexFree(hMutex);
	::SetLastError(dwErrorCode);
	return dwRetCal;
}

HMODULE __declspec(dllexport) RemoteLoadLibraryA(HWND hTargetWnd, LPCSTR lpszDllPath, DWORD dwTimeout)
{
	return (HMODULE)PathAccessA(hTargetWnd, lpszDllPath, REMOTE_LOADLIBRARY, dwTimeout);
}

HMODULE __declspec(dllexport) RemoteLoadLibraryW(HWND hTargetWnd, LPCWSTR lpszDllPath, DWORD dwTimeout)
{
	return (HMODULE)PathAccessW(hTargetWnd, lpszDllPath, REMOTE_LOADLIBRARY, dwTimeout);
}

BOOL __declspec(dllexport) RemoteFreeLibrary(HWND hTargetWnd, HMODULE hModule, DWORD dwTimeout)
{
	return (BOOL)ModuleAccessA(hTargetWnd, REMOTE_FREELIBRARY, hModule, dwTimeout);
}

HMODULE __declspec(dllexport) RemoteGetModuleHandleA(HWND hTargetWnd, LPCSTR lpszDllPath, DWORD dwTimeout)
{
	return (HMODULE)PathAccessA(hTargetWnd, lpszDllPath, REMOTE_GETMODULEHANDLE, dwTimeout);
}

HMODULE __declspec(dllexport) RemoteGetModuleHandleW(HWND hTargetWnd, LPCWSTR lpszDllPath, DWORD dwTimeout)
{
	return (HMODULE)PathAccessW(hTargetWnd, lpszDllPath, REMOTE_GETMODULEHANDLE, dwTimeout);
}

DWORD __declspec(dllexport) RemoteGetModuleFileNameA(HWND hTargetWnd, HMODULE hModule, LPSTR lpszFileName, DWORD dwSize, DWORD dwTimeout)
{
	return ModuleAccessA(hTargetWnd, REMOTE_GETMODULEFILENAME, hModule, dwTimeout, lpszFileName, dwSize);
}

DWORD __declspec(dllexport) RemoteGetModuleFileNameW(HWND hTargetWnd, HMODULE hModule, LPWSTR lpszFileName, DWORD dwSize, DWORD dwTimeout)
{
	return ModuleAccessW(hTargetWnd, REMOTE_GETMODULEFILENAME, hModule, dwTimeout, lpszFileName, dwSize);
}

DWORD __declspec(dllexport) RemoteGetModuleFileNameA(DWORD dwTargetProcessID, HMODULE hModule, LPSTR lpszFileName, DWORD dwSize)
{
	if (lpszFileName == NULL || dwSize == 0)
	{
		::SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	lpszFileName[0] = 0;	

	if (g_kernel32.CreateToolhelp32Snapshot == NULL || g_kernel32.Module32First == NULL || g_kernel32.Module32Next == NULL)
	{
		::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		return NULL;
	}

	HANDLE hModuleSnap = g_kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwTargetProcessID);
	if (hModuleSnap == (HANDLE)-1)
		return 0;
	
	int nLen = 0;
	MODULEENTRY32 me32 = { 0 };
	me32.dwSize = sizeof(me32);
	BOOL bContinue = g_kernel32.Module32First(hModuleSnap, &me32);
	while (bContinue)
	{
		if (hModule == NULL || hModule == me32.hModule)
		{
			nLen = min(::strlen(me32.szExePath), (int)dwSize);
			nLen = max(0, nLen);
			
			if (nLen > 0)
			{
				::strncpy(lpszFileName, me32.szExePath, nLen);
				lpszFileName[nLen] = 0;
			}
			break;
		}

		bContinue = g_kernel32.Module32Next(hModuleSnap, &me32);
	}

	::CloseHandle (hModuleSnap);
	if (nLen <= 0)
		::SetLastError(ERROR_DLL_NOT_FOUND);

	return (DWORD)nLen;
}

DWORD __declspec(dllexport) RemoteGetModuleFileNameW(DWORD dwTargetProcessID, HMODULE hModule, LPWSTR lpszFileName, DWORD dwSize)
{
	if (lpszFileName == NULL || dwSize == 0)
	{
		::SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	lpszFileName[0] = 0;

	if (g_kernel32.CreateToolhelp32Snapshot == NULL || g_kernel32.Module32FirstW == NULL || g_kernel32.Module32NextW == NULL)
	{
		::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		return NULL;
	}

	HANDLE hModuleSnap = g_kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwTargetProcessID);
	if (hModuleSnap == (HANDLE)-1)
		return 0;
	
	int nLen = 0;
	MODULEENTRY32W me32 = { 0 };
	me32.dwSize = sizeof(me32);
	BOOL bContinue = g_kernel32.Module32FirstW(hModuleSnap, &me32);
	while (bContinue)
	{
		if (hModule == NULL || hModule == me32.hModule)
		{
			nLen = min(::wcslen(me32.szExePath), (int)dwSize);
			nLen = max(0, nLen);
			
			if (nLen > 0)
			{
				::wcsncpy(lpszFileName, me32.szExePath, nLen);
				lpszFileName[nLen] = 0;
			}
			break;
		}

		bContinue = g_kernel32.Module32NextW(hModuleSnap, &me32);
	}

	::CloseHandle (hModuleSnap);
	if (nLen <= 0)
		::SetLastError(ERROR_DLL_NOT_FOUND);

	return (DWORD)nLen;
}
