// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <tchar.h>

#include <new>

#include "Hook.h"

struct SharedMemStruct {
    SharedMemStruct() {
        memset(this, 0, sizeof(*this));
    }

    HHOOK hookHandle32;
    HHOOK hookHandle64;

    DWORD dstThreadId;
    HWND dstWindow;
    HKL hLayout;
};

HANDLE fileMappingHandle;
SharedMemStruct *sharedStruct;
HINSTANCE hMod;

BOOL initProcess()
{
    SetLastError(ERROR_SUCCESS);
    fileMappingHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemStruct), _T("Local\\Shachar_Shemesh_TempLang"));
    if(fileMappingHandle==NULL) {
        return false;
    }
    bool created = GetLastError() == ERROR_ALREADY_EXISTS;

    sharedStruct = (SharedMemStruct *)MapViewOfFile(fileMappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemStruct));
    if(sharedStruct==NULL) {
        return false;
    }

    if( created ) {
        new(sharedStruct) SharedMemStruct();
    }

    return true;
}

void shutdownProcess()
{
    UnmapViewOfFile(sharedStruct);
    CloseHandle(fileMappingHandle);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
        hMod = hModule;
        return initProcess();
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
        break;
	case DLL_PROCESS_DETACH:
        shutdownProcess();
		break;
	}
	return TRUE;
}

LRESULT CALLBACK KeyboardProc( int code, WPARAM wParam, LPARAM lParam )
{
    OutputDebugString(_T("Point 4\n"));
    if(code>=0 && sharedStruct->hLayout!=NULL) {
        CWPSTRUCT *params = (PCWPSTRUCT)lParam;
        OutputDebugString(_T("Point 3\n"));
        if( params->hwnd == sharedStruct->dstWindow && params->message == WM_KEYDOWN && params->wParam == VK_CAPITAL ) {
            OutputDebugString(_T("Activating layout\n"));
            ActivateKeyboardLayout(sharedStruct->hLayout, KLF_SETFORPROCESS);

            sharedStruct->hLayout = NULL;

            return 1; // Do not show the message to anyone else
        }
    }

    return CallNextHookEx(sharedStruct->hookHandle32, code, wParam, lParam);
}

void switchMapping(DWORD threadId, HWND window, HKL keyboard)
{
    sharedStruct->hLayout = keyboard;
    sharedStruct->dstWindow = window;
    sharedStruct->hookHandle32 = SetWindowsHookEx(WH_CALLWNDPROC, KeyboardProc, hMod, 0);
    if(sharedStruct->hookHandle32 == NULL) {
        DWORD error = GetLastError();
        OutputDebugString(_T("Hook not installed"));
    }
    SendMessage(window, WM_KEYDOWN, VK_CAPITAL, 0);
    if(!UnhookWindowsHookEx(sharedStruct->hookHandle32)) {
        DWORD error = GetLastError();
        OutputDebugString(_T("Hook not removed"));
    }

    if( sharedStruct->hLayout )
        OutputDebugString(_T("After: no\n"));
    else
        OutputDebugString(_T("After: yes\n"));
}
