// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the HOOK_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// HOOK_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef HOOK_EXPORTS
#define HOOK_API __declspec(dllexport)
#else
#define HOOK_API __declspec(dllimport)
#endif

#ifdef TEMPLANG_MAIN_PROCESS
HOOK_API void switchMapping( DWORD threadId, HWND hWindow, HKL mapping );

HOOK_API void initSharedMem();
HOOK_API void hookShutdown();
#else
HOOK_API int waitLoop();
#endif

HOOK_API void ODS(const TCHAR *format, ...);