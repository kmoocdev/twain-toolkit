// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.

#include <windows.h>         // Req. for twain.h type defs and ...
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "..\\twcommon\\twain.h"
#include "..\\twcommon\\twndebug.h"

#ifdef WIN64
#include "res_64.h"
#elif defined(WIN32)
#include "res_32.h"
#else
#include "res_16.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include "table.h"
#include "dca_glue.h"        // for function prototypes of glue code
#include "dca_type.h"        // contains function protos for this module
#include "dca_acq.h"         // contains buffered glue code function prototypes
#include "dca_app.h"
#include "special.h"
#include "twacker.h"
#include "captest.h"

BOOL InitInstanceApp(HANDLE hInstance);

// Global variables in twacker.c
extern HWND hMainWnd;							// main app window handle

#ifdef __cplusplus
}
#endif

#include "FreeImage.h"

#define TWCRC_FAILURE                 -TWRC_FAILURE          // -1 /* Application may get TW_STATUS for info on failure */
#define TWCRC_SUCCESS                  TWRC_SUCCESS          //  0


HANDLE ghWnd = NULL;

LRESULT FAR(PASCAL* PrevProc)(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
BOOL InitInstanceAppId(HANDLE hWnd);
LRESULT FAR(PASCAL* CallBackProc)(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
LRESULT FAR(PASCAL* CallBackEjectProc)(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);


BOOL InitInstanceAppId(HANDLE hWnd)
{
    // pass app particulars to glue code
    TWInitialize((HWND)hWnd);

    hMainWnd = (HWND)hWnd;

    return (TRUE);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

int __stdcall CALL_InitInstance(HANDLE hWnd, long ProcXFerDone, long ProcEject)
{
	LogMessage((char *)"CALL_InitInstance\r\n");

	OFSTRUCT of;

	memset(&of, 0, sizeof(OFSTRUCT));

	// erase the twacker.log file
	OpenFile("c:\\twacker.log", &of, OF_DELETE);
	OpenFile("c:\\twsrc.log", &of, OF_DELETE);


	// 	_ghDIB = (BYTE*) GlobalAlloc (GPTR, 3000*3000*3 );	// dglee

	if (!
		InitInstanceAppId(hWnd))
	{
		return TWCRC_FAILURE;
	}

	ghWnd = hWnd;


	// 	MessageBox(NULL,"twcall1", "t", MB_OK);

	// 	PrevProc = (LRESULT FAR (PASCAL *)(HWND,UINT,WPARAM,LPARAM))SetWindowLong((HWND)hWnd,GWL_WNDPROC,(LONG)DLLWndProc);  //2016.03.09 상위 dll 혹은 c#에서 호출 
	CallBackProc = (LRESULT FAR(PASCAL*)(HWND, UINT, WPARAM, LPARAM))ProcXFerDone;
	CallBackEjectProc = (LRESULT FAR(PASCAL*)(HWND, UINT, WPARAM, LPARAM))ProcEject;
	// 	CallBackStopProc = (LRESULT FAR (PASCAL *)(HWND,UINT,WPARAM ,LPARAM ))ProcStop;

	FreeImage_Initialise();

	return TWCRC_SUCCESS;
}
