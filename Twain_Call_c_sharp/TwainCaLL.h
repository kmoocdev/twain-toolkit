#include "Twain.h"

int __stdcall CALL_InitInstance(HANDLE hWnd, long ProcXFerDone, long ProcEject);
int __stdcall CALL_SelectSource();
int __stdcall CALL_StringSource(const char* szProductName);
int __stdcall CALL_TWAppAquire(int ShowUI, char* pImgDir, char* pImgPrefix, 
							   FREE_IMAGE_FORMAT fiFormat, int optFlag);
int __stdcall CALL_TWAppAquireFixedName(int ShowUI, char* szFileName, int optFlag);
int __stdcall CALL_TWAppQuit();
int __stdcall CALL_ChangeCount(int count);
int __stdcall CALL_StartSetup(void);
// int __stdcall HPG3110_StartSetup(void);
int __stdcall CALL_SetProperty(TW_UINT16 cap, int val);
int __stdcall CALL_SetProperty_Frame(TW_UINT16 cap, pTW_FRAME pFrm, int itemNum);
int __stdcall CALL_SetProperty_String(TW_UINT16 cap, char* szSrc, int itemNum);
int __stdcall CALL_SetProperty_Array(TW_UINT16 cap, pTW_UINT32 pVal, int itemNum);
int __stdcall CALL_CheckCondition(void);
int __stdcall CALL_GetType(TW_UINT16 cap, pTW_UINT16 pConType, pTW_UINT16 pItemType);












