/*
    TempLang - allows temporary keyboard language switching.
    Copyright information is available in the Authors.txt file.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <tchar.h>

#include <new>

#include "Hook.h"

void ODS(const TCHAR *format, ...)
{
    TCHAR outputMessage[250];

    va_list arguments;
    va_start(arguments, format);

    _vsntprintf_s(outputMessage, 250, format, arguments);
    OutputDebugString(outputMessage);
}

struct SharedMemStruct {
    SharedMemStruct() {
        memset(this, 0, sizeof(*this));
    }

    // We don't really need these: CallNextHookEx ignores the handle argument and the handlers don't unhook themselves.
    // Still, just for completeness, we leave those in. 
    HHOOK hookHandle32;
    HHOOK hookHandle64;
#ifdef  _WIN64
#define HOOKHANDLE hookHandle64
#else
#define HOOKHANDLE hookHandle32
#endif

    DWORD dstThreadId;
    HWND dstWindow;
    HKL hLayout;
};

HANDLE fileMappingHandle;
SharedMemStruct *sharedStruct;
HINSTANCE hMod;

BOOL loadSharedMem()
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

    return true;
}

void initSharedMem()
{
    new(sharedStruct) SharedMemStruct();
}

void unloadSharedMem()
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
        loadSharedMem();
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
        break;
	case DLL_PROCESS_DETACH:
        unloadSharedMem();
		break;
	}
	return TRUE;
}

LRESULT CALLBACK KeyboardProc( int code, WPARAM wParam, LPARAM lParam )
{
    if(code>=0 && sharedStruct->hLayout!=NULL) {
        CWPSTRUCT *params = (PCWPSTRUCT)lParam;
        if( params->hwnd == sharedStruct->dstWindow && params->message == WM_KEYDOWN && params->wParam == VK_CAPITAL ) {
            ActivateKeyboardLayout(sharedStruct->hLayout, KLF_SETFORPROCESS);

            sharedStruct->hLayout = NULL;

            return 1; // Do not show the message to anyone else
        }
    }

    return CallNextHookEx(sharedStruct->HOOKHANDLE, code, wParam, lParam);
}

void switchMapping(DWORD threadId, HWND window, HKL keyboard)
{
    sharedStruct->hLayout = keyboard;
    _sntprintf_s(sharedStruct->strLayout, 9, _T("%08x"), (unsigned int)keyboard);
    sharedStruct->dstWindow = window;
    sharedStruct->HOOKHANDLE = SetWindowsHookEx(WH_CALLWNDPROC, KeyboardProc, hMod, threadId);
    if(sharedStruct->HOOKHANDLE == NULL) {
        DWORD error = GetLastError();
        ODS(_T("Hook not installed error: %ld\n"), error);
    }
    SendMessage(window, WM_KEYDOWN, VK_CAPITAL, 0);
    if(!UnhookWindowsHookEx(sharedStruct->HOOKHANDLE)) {
        DWORD error = GetLastError();
        ODS(_T("Hook %p not removed error: %ld\n"), sharedStruct->HOOKHANDLE, error);
    }

    if( sharedStruct->hLayout )
        OutputDebugString(_T("After: no\n"));
    else
        OutputDebugString(_T("After: yes\n"));
}
