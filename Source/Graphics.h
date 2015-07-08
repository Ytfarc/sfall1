#pragma once

extern DWORD GraphicsMode;
extern DWORD ForcingGraphicsRefresh;
void graphics_OnGameLoad();

int _stdcall GetShaderVersion();
int _stdcall LoadShader(const char*);
void _stdcall ActivateShader(DWORD);
void _stdcall DeactivateShader(DWORD);
void _stdcall FreeShader(DWORD);
void _stdcall SetShaderMode(DWORD d, DWORD mode);

void _stdcall SetShaderInt(DWORD d, const char* param, int value);
void _stdcall SetShaderFloat(DWORD d, const char* param, float value);
void _stdcall SetShaderVector(DWORD d, const char* param, float f1, float f2, float f3, float f4);

int _stdcall GetShaderTexture(DWORD d, DWORD id);
void _stdcall SetShaderTexture(DWORD d, const char* param, DWORD value);

void RefreshGraphics();
void GetFalloutWindowInfo(DWORD* width, DWORD* height, HWND* window);
