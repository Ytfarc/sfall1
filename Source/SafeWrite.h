#pragma once

void _stdcall SafeWrite8(DWORD addr, BYTE data);
void _stdcall SafeWrite16(DWORD addr, WORD data);
void _stdcall SafeWrite32(DWORD addr, DWORD data);
void _stdcall SafeWriteStr(DWORD addr, const char* data);
void HookCall(DWORD addr, void* func);
void MakeCall(DWORD addr, void* func, bool jump);
void SafeMemSet(DWORD addr, BYTE val, int len);
void BlockCall(DWORD addr);
void SafeWriteBytes(DWORD addr, BYTE* data, int count);
