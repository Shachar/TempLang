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
#include <assert.h>

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

#define ALIGN64(type, name) union { type handle; UINT64 pad; } name;
struct SharedMemStruct {
    SharedMemStruct() {
        memset(this, 0, sizeof(*this));
        size = sizeof(SharedMemStruct);
    }

    unsigned int size;
    // We don't really need these: CallNextHookEx ignores the handle argument and the handlers don't unhook themselves.
    // Still, just for completeness, we leave those in.
    ALIGN64(HHOOK, hookHandle32);
    ALIGN64(HHOOK, hookHandle64);
#ifdef  _WIN64
#define HOOKHANDLE hookHandle64.handle
#else
#define HOOKHANDLE hookHandle32.handle
#endif

    ALIGN64(HWND, dstWindow);
    ALIGN64(HKL, hLayout);
    DWORD dstThreadId;
    bool running64;
    bool exit;
};

HANDLE fileMappingHandle;
SharedMemStruct *sharedStruct;
HINSTANCE hMod;
HANDLE eventEnter, eventExit, eventHookInstalled;

BOOL loadSharedMem()
{
    SetLastError(ERROR_SUCCESS);
    fileMappingHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemStruct), _T("Local\\Shachar_Shemesh_TempLang_Memory"));
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

void unloadSharedMem()
{
    UnmapViewOfFile(sharedStruct);
    CloseHandle(fileMappingHandle);
}

static void createEvents()
{
    eventEnter = CreateEvent(NULL, false, false, _T("Local\\Shachar_Shemesh_TempLang_Event_Enter"));
    if(eventEnter == NULL) {
        ODS(_T("Failed to create enter event: %ld"), GetLastError());
        return;
    }
    eventHookInstalled = CreateEvent(NULL, false, false, _T("Local\\Shachar_Shemesh_TempLang_Event_Hook"));
    if(eventHookInstalled == NULL) {
        ODS(_T("Failed to create hook installed event: %ld"), GetLastError());
        return;
    }
    eventExit = CreateEvent(NULL, false, false, _T("Local\\Shachar_Shemesh_TempLang_Event_Exit"));
    if(eventExit == NULL) {
        ODS(_T("Failed to create exit event: %ld"), GetLastError());
        return;
    }
}

void destroyEvents()
{
    CloseHandle(eventEnter);
    CloseHandle(eventHookInstalled);
    CloseHandle(eventExit);
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
    if(code>=0 && sharedStruct->hLayout.handle!=NULL) {
        CWPSTRUCT *params = (PCWPSTRUCT)lParam;
        if( params->hwnd == sharedStruct->dstWindow.handle && params->message == WM_KEYDOWN && params->wParam == VK_CAPITAL ) {
            ActivateKeyboardLayout(sharedStruct->hLayout.handle, KLF_SETFORPROCESS);

            sharedStruct->hLayout.handle = NULL;

            return 1; // Do not show the message to anyone else
        }
    }

    return CallNextHookEx(sharedStruct->HOOKHANDLE, code, wParam, lParam);
}

#ifndef _WIN64
void switchMapping(DWORD threadId, HWND window, HKL keyboard)
{
    sharedStruct->hLayout.handle = keyboard;
    sharedStruct->dstWindow.handle = window;
    sharedStruct->dstThreadId = threadId;

    if(sharedStruct->running64) {
        ODS(_T("Setting enter event threadid = %ld\n"), sharedStruct->dstThreadId);
        SetEvent(eventEnter); // Mark 64bit to start its own hook installation
    }

    sharedStruct->HOOKHANDLE = SetWindowsHookEx(WH_CALLWNDPROC, KeyboardProc, hMod, threadId);
    if(sharedStruct->HOOKHANDLE == NULL) {
        DWORD error = GetLastError();
        ODS(_T("Hook not installed error: %ld\n"), error);
    }
    if(sharedStruct->running64)
        WaitForSingleObject(eventHookInstalled, INFINITE); // Wait for 64bit to tell us its hook is installed
    ResetEvent(eventEnter);
    SendMessage(window, WM_KEYDOWN, VK_CAPITAL, 0);
    SetEvent(eventExit); // Tell 64bit to uninstall its hook
    sharedStruct->dstThreadId = 0;
    if(!UnhookWindowsHookEx(sharedStruct->HOOKHANDLE)) {
        DWORD error = GetLastError();
        ODS(_T("Hook %p not removed error: %ld\n"), sharedStruct->HOOKHANDLE, error);
    }

    if( sharedStruct->hLayout.handle )
        OutputDebugString(_T("After: no\n"));
    else
        OutputDebugString(_T("After: yes\n"));
}

void initSharedMem()
{
    new(sharedStruct) SharedMemStruct();
    createEvents();
}

void hookShutdown()
{
    // Mark 64bit process to exit
    sharedStruct->exit = true;
    SetEvent(eventEnter);
    destroyEvents();
}

#else
int waitLoop()
{
    createEvents();
    assert(sharedStruct->size == sizeof(*sharedStruct));
    sharedStruct->running64 = true;

    WaitForSingleObject(eventEnter, INFINITE);
    while(! sharedStruct->exit) {
        assert(sharedStruct->dstThreadId != 0);
        assert(sharedStruct->dstWindow.handle);

        sharedStruct->HOOKHANDLE = SetWindowsHookEx(WH_CALLWNDPROC, KeyboardProc, hMod, sharedStruct->dstThreadId);
        if(sharedStruct->HOOKHANDLE == NULL) {
            DWORD error = GetLastError();
            ODS(_T("Hook not installed error: %ld\n"), error);
        }

        // Signal the 32 bit we are ready
        SetEvent(eventHookInstalled);

        // Wait until the handler is done
        WaitForSingleObject(eventExit, INFINITE);
        if(!UnhookWindowsHookEx(sharedStruct->HOOKHANDLE)) {
            DWORD error = GetLastError();
            ODS(_T("Hook %p not removed error: %ld\n"), sharedStruct->HOOKHANDLE, error);
        }

        WaitForSingleObject(eventEnter, INFINITE);
    }

    destroyEvents();
    return 0;
}
#endif