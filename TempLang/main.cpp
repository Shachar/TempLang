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

#include "precompile.h"
#include "resource.h"
#include <hash_map>

#include "../Hook/Hook.h"

#define RUN_IDENTIFIER_ARG "Shachar Shemesh's TempLang: temporary keyboard switching"

#ifdef TEMPLANG_MAIN_PROCESS
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR ret = TRUE;
    switch(uMsg) {
    case WM_INITDIALOG:
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, false);
            break;
        }
        break;
    default:
        ret = FALSE;
    }
    return ret;
}

static HHOOK hookHandle;

stdext::hash_map<HKL, HKL> languageMap;

LRESULT CALLBACK keyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    static HKL originalMapping;
    static DWORD threadId=0;
    static HWND focusWindow;

    PKBDLLHOOKSTRUCT params = (PKBDLLHOOKSTRUCT)lParam;
    if(nCode>=0 && params->vkCode == VK_CAPITAL) {
        if( !(params->flags & LLKHF_UP) ) {
            if( threadId==0 ) {
                GUITHREADINFO guiThreadInfo;
                guiThreadInfo.cbSize = sizeof(guiThreadInfo);

                GetGUIThreadInfo( 0, &guiThreadInfo );
                focusWindow = guiThreadInfo.hwndFocus;
                threadId = GetWindowThreadProcessId( focusWindow, NULL );
                originalMapping = GetKeyboardLayout( threadId );

                switchMapping( threadId, focusWindow, languageMap[originalMapping] );
            }
        } else if( threadId!=0 ) {
            switchMapping( threadId, focusWindow, originalMapping );
            threadId = 0;
        }
        return 1;
    }

    return CallNextHookEx(hookHandle, nCode, wParam, lParam);
}

#define HELPER_EXE_NAME "TempLang Helper.exe"
void run64Helper()
{
    TCHAR path[4096];
    DWORD pathlen = GetModuleFileName(NULL, path, 4096);
    if( pathlen==0 ) {
        OutputDebugString(_T("Failed to get current module's path\n"));
        exit(1);
    } else {
        OutputDebugString(path);
        OutputDebugString(_T("\n"));
    }

    // Locate last directory separator
    TCHAR *dirend = _tcsrchr(path, _T('\\'));
    dirend++;
    memcpy(dirend, _T(HELPER_EXE_NAME), sizeof(_T(HELPER_EXE_NAME)));

    STARTUPINFO startInfo;
    memset( &startInfo, 0, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);

    PROCESS_INFORMATION procInfo;
    if( !CreateProcess(path, _T(RUN_IDENTIFIER_ARG), NULL, NULL, false, 0, NULL,
        NULL, &startInfo, &procInfo)) {
            ODS(_T("Helper process creation failed: %d\n"), GetLastError());
    } else {
        // We don't wait for it
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);
        OutputDebugString(_T("Started helper process\n"));
    }
}

void installHook()
{
    initSharedMem();
    HKL keyboardsList[10];
    UINT numKeyboards = GetKeyboardLayoutList( 10, keyboardsList );

    for(UINT i=0; i<numKeyboards; ++i) {
        languageMap[keyboardsList[i]] = keyboardsList[(i+1)%numKeyboards];
    }

    run64Helper();

    hookHandle = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHook, GetModuleHandle(NULL), 0);
    if(hookHandle == NULL) {
        DWORD err = GetLastError();
        TCHAR message[250];
        _sntprintf_s(message, 250, _T("Failed to register keyboard hook: %d"), err);
        MessageBox(NULL, message, _T("Error"), MB_ICONERROR|MB_APPLMODAL);
        exit(1);
    }
}

void removeHook()
{
    UnhookWindowsHookEx(hookHandle);
    hookShutdown();

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,int nCmdShow)
{
    installHook();
    INT_PTR res = DialogBox(NULL, MAKEINTRESOURCE(IDD_MAIN), NULL, DialogProc);
	if(res < 0) {
        DWORD error = GetLastError();
        OutputDebugString(_T("Failed to create dialog\n"));
	}
    removeHook();

	return 0;
}
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,int nCmdShow)
{
    if( _tcscmp(GetCommandLine(), _T(RUN_IDENTIFIER_ARG))==0 )
        // Not much here for 64 bit
        return waitLoop();

    OutputDebugString(GetCommandLine());
    // We were not run by our 32bit counterpart
    MessageBox(NULL, _T("This executable is a part of TempLang.\nPlease do not run it directly."),
        _T("Error"), MB_OK|MB_ICONSTOP|MB_APPLMODAL);
    return 1;
}
#endif