#include "precompile.h"
#include "resource.h"
#include <hash_map>

#include "../Hook/Hook.h"

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

void installHook()
{
    initSharedMem();
    HKL keyboardsList[10];
    UINT numKeyboards = GetKeyboardLayoutList( 10, keyboardsList );

    for(UINT i=0; i<numKeyboards; ++i) {
        languageMap[keyboardsList[i]] = keyboardsList[(i+1)%numKeyboards];
    }

    hookHandle = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHook, NULL, 0);
    if(hookHandle == NULL) {
        DWORD err = GetLastError();
        MessageBox(NULL, _T("Failed to register keyboard hook"), _T("Error"), MB_ICONERROR|MB_APPLMODAL);
        exit(1);
    }
}

void removeHook()
{
    UnhookWindowsHookEx(hookHandle);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,int nCmdShow)
{
    installHook();
    DialogBox(NULL, MAKEINTRESOURCE(IDD_MAIN), NULL, DialogProc);
    removeHook();

	return 0;
}
