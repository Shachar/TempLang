// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <tchar.h>

#include <new>

#if 0
struct SharedMemStruct {
    SharedMemStruct()
    {
        hookHandle = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, );
    }

    HHOOK hookHandle;

    static LRESULT CALLBACK KeyboardProc(          int code,
    WPARAM wParam,
    LPARAM lParam
);

};

HANDLE fileMappingHandle;
SharedMemStruct *sharedStruct;

BOOL initProcess()
{
    fileMappingHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemStruct), _T("Local\\Shachar_Shemesh_TempLang"));
    if(fileMappingHandle==NULL) {
        return false;
    }

    sharedStruct = (SharedMemStruct *)MapViewOfFile(fileMappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemStruct));
    if(sharedStruct==NULL) {
        return false;
    }

    return true;
}
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
        //return initProcess();
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#if 0
void installHook()
{
    new(sharedStruct) SharedMemStruct();
}
#endif