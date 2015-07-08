//#define STRICT
//#define WIN32_LEAN_AND_MEAN

#include "main.h"

typedef HRESULT (_stdcall *DDrawCreateProc)(void* a, void* b, void* c);

HRESULT _stdcall FakeDirectDrawCreate(void* a, void* b, void* c) {
 char path[MAX_PATH];
 GetSystemDirectoryA(path,MAX_PATH);
 strcat_s(path, "\\ddraw.dll");
 HMODULE ddraw=LoadLibraryA(path);
 if(!ddraw||ddraw==INVALID_HANDLE_VALUE) return -1;
 DDrawCreateProc proc=(DDrawCreateProc)GetProcAddress(ddraw, "DirectDrawCreate");
 if(!proc) return -1;
 return proc(a,b,c);
}
