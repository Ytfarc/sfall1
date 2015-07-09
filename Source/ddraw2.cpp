//#define STRICT
//#define WIN32_LEAN_AND_MEAN

#include "main.h"

#include <stdio.h>
#include "ddraw.h"
#include "Graphics.h"
#include "input.h"
#include "math.h"
#include "Version.h"
#include "vector9x.cpp"

#ifdef _DEBUG
#define D3D_DEBUG_INFO
#define DEBUGMESS(a) OutputDebugStringA(a)
#else
#define DEBUGMESS(a)
#endif

#include <d3d9.h>
#include <d3dx9.h>

typedef HRESULT (_stdcall *DDrawCreateProc)(void*, IDirectDraw**, void*);
typedef IDirect3D9* (_stdcall *D3DCreateProc)(UINT version);

#define UNUSEDFUNCTION { DEBUGMESS("Unused function called: " __FUNCTION__); return DDERR_GENERIC; }
#define SAFERELEASE(a) { if(a) { a->Release(); a=0; } }

static DWORD ResWidth;
static DWORD ResHeight;
static DWORD GPUBlt;

static bool DeviceLost=false;
static DDSURFACEDESC surfaceDesc;
static DDSURFACEDESC movieDesc;

static DWORD palette[256];

static DWORD gWidth;
static DWORD gHeight;

static int ScrollWindowKey;
static DWORD windowLeft=0;
static DWORD windowTop=0;

static DWORD ShaderVersion;

static HWND window;
IDirect3D9* d3d9=0;
IDirect3DDevice9* d3d9Device=0;
static IDirect3DTexture9* Tex=0;
static IDirect3DTexture9* sTex1=0;
static IDirect3DTexture9* sTex2=0;
static IDirect3DSurface9* sSurf1=0;
static IDirect3DSurface9* sSurf2=0;
static IDirect3DSurface9* backbuffer=0;
static IDirect3DVertexBuffer9* vBuffer;
static IDirect3DVertexBuffer9* vBuffer2;
static IDirect3DVertexBuffer9* movieBuffer;
static IDirect3DTexture9* gpuPalette;
static IDirect3DTexture9* movieTex=0;

static ID3DXEffect* gpuBltEffect;
static const char* gpuEffect=
"texture image;\n"
"texture palette;\n"
"texture head;\n"
"sampler s0 = sampler_state { texture=<image>; MAGFILTER=POINT; MINFILTER=POINT; };\n"
"sampler s1 = sampler_state { texture=<palette>; MAGFILTER=POINT; MINFILTER=POINT; };\n"
"sampler s2 = sampler_state { texture=<head>; MAGFILTER=POINT; MINFILTER=POINT; };\n"
"float2 size;\n"
"float2 corner;\n"
"float4 P0( in float2 Tex : TEXCOORD0 ) : COLOR0 {\n"
"  float3 result = tex1D(s1, tex2D(s0, Tex).a);\n"
"  return float4(result.b, result.g, result.r, 1);\n"
"}\n"
"float4 P1( in float2 Tex : TEXCOORD0 ) : COLOR0 {\n"
"  float backdrop = tex2D(s0, Tex).a;\n"
"  float3 result;\n"
"  if(abs(backdrop-(48.0/255.0))<0.001) {\n"
//"    float2 size   = float2(388.0/640.0, 200.0/480.0);\n"
//"    float2 corner = float2(126.0/640.0, 14.0/480.0);\n"
"    result = tex2D(s2, saturate((Tex-corner)/size));\n"
"  } else {\n"
"    result = tex1D(s1, backdrop);\n"
"    result = float3(result.b, result.g, result.r);\n"
"  }\n"
"  return float4(result.r, result.g, result.b, 1);\n"
"}\n"
"technique T0\n"
"{\n"
"  pass p0 { PixelShader = compile ps_2_0 P0(); }\n"
"}\n"
"technique T1\n"
"{\n"
"  pass p1 { PixelShader = compile ps_2_0 P1(); }\n"
"}\n"
;

static D3DXHANDLE gpuBltBuf;
static D3DXHANDLE gpuBltPalette;
static D3DXHANDLE gpuBltHead;
static D3DXHANDLE gpuBltHeadSize;
static D3DXHANDLE gpuBltHeadCorner;

static float rcpres[2];

DWORD GraphicsMode;

struct sShader {
 ID3DXEffect* Effect;
 bool Active;
 D3DXHANDLE ehTicks;
 DWORD mode;
 DWORD mode2;

 sShader() {
  Effect=0;
  Active=false;
  ehTicks=0;
  mode=0;
  mode2=0;
 }
};

static vector<sShader> shaders;
static vector<IDirect3DTexture9*> shaderTextures;

#define MYVERTEXFORMAT D3DFVF_XYZRHW|D3DFVF_TEX1
struct MyVertex {
    float x,y,z,w,u,v;
};

static MyVertex ShaderVertices[] = {
    {-0.5,  -0.5,  0, 1, 0, 0},
    {-0.5,  479.5, 0, 1, 0, 1},
    {639.5, -0.5,  0, 1, 1, 0},
    {639.5, 479.5, 0, 1, 1, 1}
};

void GetFalloutWindowInfo(DWORD* width, DWORD* height, HWND* wnd) {
 *width=gWidth;
 *height=gHeight;
 *wnd=window;
}
int _stdcall GetShaderVersion() { return ShaderVersion; }
void _stdcall SetShaderMode(DWORD d, DWORD mode) {
 if(d>=shaders.size()||!shaders[d].Effect) return;
 if(mode&0x80000000) {
  shaders[d].mode2=mode^0x80000000;
 } else {
  shaders[d].mode=mode;
 }
}

int _stdcall LoadShader(const char* path) {
 if(GraphicsMode < 4) return -1;
 if(strstr(path, "..")) return -1;
 if(strstr(path, ":")) return -1;
 char buf[MAX_PATH];
 strcpy_s(buf, "data\\shaders\\");
 strcat_s(buf, path);
 for(DWORD d=0;d<shaders.size();d++) {
  if(!shaders[d].Effect) {
   if(FAILED(D3DXCreateEffectFromFile(d3d9Device, buf, 0, 0, 0, 0, &shaders[d].Effect, 0))) return -1;
   else return d;
  }
 }
 sShader shader=sShader();
 if(FAILED(D3DXCreateEffectFromFile(d3d9Device, buf, 0, 0, 0, 0, &shader.Effect, 0))) return -1;

 shader.Effect->SetFloatArray("rcpres", rcpres, 2);

 for(int i=1;i<128;i++) {
  char buf[MAX_PATH];
  const char* name;
  IDirect3DTexture9* tex;

  sprintf_s(buf, "texname%d", i);
  if(FAILED(shader.Effect->GetString(buf, &name))) break;
  sprintf_s(buf, "data\\art\\stex\\%s", name);
  if(FAILED(D3DXCreateTextureFromFileA(d3d9Device,buf,&tex))) continue;
  sprintf_s(buf, "tex%d", i);
  shader.Effect->SetTexture(buf, tex);
  shaderTextures.push_back(tex);
 }
 
 shader.ehTicks=shader.Effect->GetParameterByName(0, "tickcount");
 shaders.push_back(shader);
 return shaders.size()-1;
}

void _stdcall ActivateShader(DWORD d) { if(d<shaders.size()&&shaders[d].Effect) shaders[d].Active=true; }
void _stdcall DeactivateShader(DWORD d) { if(d<shaders.size()) shaders[d].Active=false; }
int _stdcall GetShaderTexture(DWORD d, DWORD id) {
 if(d>=shaders.size()||!shaders[d].Effect||id<1||id>128) return -1;
 IDirect3DBaseTexture9* tex=0;
 char buf[8];
 buf[0]='t'; buf[1]='e'; buf[2]='x';
 _itoa_s(id, &buf[3], 4, 10);
 if(FAILED(shaders[d].Effect->GetTexture(buf, &tex))) return -1;
 if(!tex) return -1;
 tex->Release();
 for(DWORD i=0;i<shaderTextures.size();i++) {
  if(shaderTextures[i]==tex) return i;
 }
 return -1;
}

void _stdcall FreeShader(DWORD d) {
 if(d<shaders.size()) {
  SAFERELEASE(shaders[d].Effect);
  shaders[d].Active=false;
 }
}

void _stdcall SetShaderInt(DWORD d, const char* param, int value) {
 if(d>=shaders.size()||!shaders[d].Effect) return;
 shaders[d].Effect->SetInt(param, value);
}

void _stdcall SetShaderFloat(DWORD d, const char* param, float value) {
 if(d>=shaders.size()||!shaders[d].Effect) return;
 shaders[d].Effect->SetFloat(param, value);
}

void _stdcall SetShaderVector(DWORD d, const char* param, float f1, float f2, float f3, float f4) {
 if(d>=shaders.size()||!shaders[d].Effect) return;
 shaders[d].Effect->SetFloatArray(param, &f1, 4);
}

void _stdcall SetShaderTexture(DWORD d, const char* param, DWORD value) {
 if(d>=shaders.size()||!shaders[d].Effect||value>=shaderTextures.size()) return;
 shaders[d].Effect->SetTexture(param, shaderTextures[value]);
}

static void ResetDevice(bool CreateNew) {
 D3DPRESENT_PARAMETERS params;
 ZeroMemory(&params, sizeof(params));
 params.BackBufferCount=1;
 params.BackBufferFormat=(GraphicsMode==5)?D3DFMT_UNKNOWN:D3DFMT_X8R8G8B8;
 params.BackBufferWidth=gWidth;
 params.BackBufferHeight=gHeight;
 params.EnableAutoDepthStencil=false;
 params.MultiSampleQuality=0;
 params.MultiSampleType=D3DMULTISAMPLE_NONE;
 params.Windowed=(GraphicsMode==5);
 params.SwapEffect=D3DSWAPEFFECT_DISCARD;
 params.hDeviceWindow=window;
 params.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;

 bool software=false;

 if(CreateNew) {
  if(FAILED(d3d9->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_PUREDEVICE|D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED|D3DCREATE_FPU_PRESERVE,&params,&d3d9Device))) {
   software=true;
   d3d9->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED|D3DCREATE_FPU_PRESERVE,&params,&d3d9Device);
  }
  D3DCAPS9 caps;
  d3d9Device->GetDeviceCaps(&caps);
  ShaderVersion=((caps.PixelShaderVersion&0x0000ff00)>>8)*10 + (caps.PixelShaderVersion&0xff);

  if(GPUBlt==2) {
   if(ShaderVersion<20) GPUBlt=0;
  }

  if(GPUBlt) {
   D3DXCreateEffect(d3d9Device, gpuEffect, strlen(gpuEffect), 0, 0, 0, 0, &gpuBltEffect, 0);
   gpuBltBuf=gpuBltEffect->GetParameterByName(0, "image");
   gpuBltPalette=gpuBltEffect->GetParameterByName(0, "palette");
   gpuBltHead=gpuBltEffect->GetParameterByName(0, "head");
   gpuBltHeadSize=gpuBltEffect->GetParameterByName(0, "size");
   gpuBltHeadCorner=gpuBltEffect->GetParameterByName(0, "corner");
  }

 } else {
  d3d9Device->Reset(&params);
  if(gpuBltEffect) gpuBltEffect->OnResetDevice();
  for(DWORD d=0;d<shaders.size();d++) {
   if(shaders[d].Effect) shaders[d].Effect->OnResetDevice();
  }
 }

 ShaderVertices[1].y=ResHeight-0.5f;
 ShaderVertices[2].x=ResWidth-0.5f;
 ShaderVertices[3].y=ResHeight-0.5f;
 ShaderVertices[3].x=ResWidth-0.5f;

 d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_DYNAMIC, GPUBlt?D3DFMT_A8:D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &Tex, 0);
 d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &sTex1, 0);
 d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &sTex2, 0);

 if(GPUBlt) {
  d3d9Device->CreateTexture(256, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &gpuPalette, 0);
  gpuBltEffect->SetTexture(gpuBltBuf, Tex);
  gpuBltEffect->SetTexture(gpuBltPalette, gpuPalette);
 }

 sTex1->GetSurfaceLevel(0, &sSurf1);
 sTex2->GetSurfaceLevel(0, &sSurf2);
 d3d9Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);

 d3d9Device->CreateVertexBuffer(4*sizeof(MyVertex),D3DUSAGE_WRITEONLY|(software?D3DUSAGE_SOFTWAREPROCESSING:0),MYVERTEXFORMAT,D3DPOOL_DEFAULT,&vBuffer, 0);
 byte* VertexPointer;
 vBuffer->Lock(0,0,(void**)&VertexPointer,0);
 CopyMemory(VertexPointer,ShaderVertices,sizeof(ShaderVertices));
 vBuffer->Unlock();

 d3d9Device->CreateVertexBuffer(4*sizeof(MyVertex),D3DUSAGE_WRITEONLY|(software?D3DUSAGE_SOFTWAREPROCESSING:0),MYVERTEXFORMAT,D3DPOOL_DEFAULT,&movieBuffer, 0);

 MyVertex ShaderVertices2[4]  = {
  ShaderVertices[0],
  ShaderVertices[1],
  ShaderVertices[2],
  ShaderVertices[3]
 };

 ShaderVertices2[1].y=(float)gHeight-0.5f;
 ShaderVertices2[2].x=(float)gWidth-0.5f;
 ShaderVertices2[3].y=(float)gHeight-0.5f;
 ShaderVertices2[3].x=(float)gWidth-0.5f;

 d3d9Device->CreateVertexBuffer(4*sizeof(MyVertex),D3DUSAGE_WRITEONLY|(software?D3DUSAGE_SOFTWAREPROCESSING:0),MYVERTEXFORMAT,D3DPOOL_DEFAULT,&vBuffer2, 0);
 vBuffer2->Lock(0,0,(void**)&VertexPointer,0);
 CopyMemory(VertexPointer,ShaderVertices2,sizeof(ShaderVertices2));
 vBuffer2->Unlock();

 d3d9Device->SetFVF(MYVERTEXFORMAT);
 d3d9Device->SetTexture(0, Tex);
 d3d9Device->SetStreamSource(0, vBuffer,0, sizeof(MyVertex));

 d3d9Device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
 d3d9Device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
 d3d9Device->SetRenderState(D3DRS_ZENABLE, false);
 d3d9Device->SetRenderState(D3DRS_CULLMODE,2);
}

static void Present() {
 if(ScrollWindowKey!=0&&((ScrollWindowKey>0&&KeyDown((BYTE)ScrollWindowKey))
   ||(ScrollWindowKey==-1&&(KeyDown(DIK_LCONTROL)||KeyDown(DIK_RCONTROL)))
   ||(ScrollWindowKey==-2&&(KeyDown(DIK_LMENU)||KeyDown(DIK_RMENU)))
   ||(ScrollWindowKey==-3&&(KeyDown(DIK_LSHIFT)||KeyDown(DIK_RSHIFT))))) {
  int winx, winy;
  GetMouse(&winx, &winy);
  windowLeft+=winx;
  windowTop+=winy;
  RECT r, r2;
  r.left=windowLeft;
  r.right=windowLeft+gWidth;
  r.top=windowTop;
  r.bottom=windowTop+gHeight;
  AdjustWindowRect(&r, WS_OVERLAPPED|WS_CAPTION|WS_BORDER, false);
  r.right-=(r.left-windowLeft);
  r.left=windowLeft;
  r.bottom-=(r.top-windowTop);
  r.top=windowTop;
  if(GetWindowRect(GetShellWindow(), &r2)) {
   if(r.right>r2.right) {
    DWORD move=r.right-r2.right;
    r.left-=move;
    r.right-=move;
    windowLeft-=move;
   }
   if(r.left<r2.left) {
    DWORD move=r2.left-r.left;
    r.left+=move;
    r.right+=move;
    windowLeft+=move;
   }
   if(r.bottom>r2.bottom) {
    DWORD move=r.bottom-r2.bottom;
    r.top-=move;
    r.bottom-=move;
    windowTop-=move;
   }
   if(r.top<r2.top) {
    DWORD move=r2.top-r.top;
    r.top+=move;
    r.bottom+=move;
    windowTop+=move;
   }
  }
  MoveWindow(window, r.left, r.top, r.right-r.left, r.bottom-r.top, true);
 }

 if(d3d9Device->Present(0,0,0,0)==D3DERR_DEVICELOST) {
  d3d9Device->SetTexture(0, 0);
  SAFERELEASE(Tex)
  SAFERELEASE(backbuffer);
  SAFERELEASE(sSurf1);
  SAFERELEASE(sSurf2);
  SAFERELEASE(sTex1);
  SAFERELEASE(sTex2);
  SAFERELEASE(vBuffer);
  SAFERELEASE(vBuffer2);
  SAFERELEASE(movieBuffer);
  SAFERELEASE(gpuPalette);
  if(gpuBltEffect) gpuBltEffect->OnLostDevice();
  for(DWORD d=0;d<shaders.size();d++) {
   if(shaders[d].Effect) shaders[d].Effect->OnLostDevice();
  }
  DeviceLost=true;
 }
}

void RefreshGraphics() {
 if(DeviceLost) return;
 //Tex->UnlockRect(0);
 d3d9Device->BeginScene();
 //d3d9Device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
 d3d9Device->SetStreamSource(0, vBuffer, 0, sizeof(MyVertex));
 d3d9Device->SetRenderTarget(0, sSurf1);
 if(GPUBlt&&shaders.size()) {
  UINT unused;
  gpuBltEffect->Begin(&unused, 0);
  gpuBltEffect->BeginPass(0);
  d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  gpuBltEffect->EndPass();
  gpuBltEffect->End();
  d3d9Device->StretchRect(sSurf1, 0, sSurf2, 0, D3DTEXF_NONE);
  d3d9Device->SetTexture(0, sTex2);
 } else {
  d3d9Device->SetTexture(0, Tex);
 }
 for(int d=shaders.size()-1;d>=0;d--) {
  if(!shaders[d].Effect||!shaders[d].Active) continue;
//  if(shaders[d].mode2&&!(shaders[d].mode2&GetCurrentLoops())) continue;
//  if(shaders[d].mode&GetCurrentLoops()) continue;
  if(shaders[d].ehTicks) shaders[d].Effect->SetInt(shaders[d].ehTicks, GetTickCount());
  UINT passes;
  shaders[d].Effect->Begin(&passes, 0);
  for(DWORD pass=0;pass<passes;pass++) {
   shaders[d].Effect->BeginPass(pass);
   d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
   shaders[d].Effect->EndPass();
   d3d9Device->StretchRect(sSurf1, 0, sSurf2, 0, D3DTEXF_NONE);
   d3d9Device->SetTexture(0, sTex2);
  }
  shaders[d].Effect->End();
  d3d9Device->SetTexture(0, sTex2);
 }
 d3d9Device->SetStreamSource(0, vBuffer2, 0, sizeof(MyVertex));
 d3d9Device->SetRenderTarget(0, backbuffer);
 if(GPUBlt&&!shaders.size()) {
  UINT unused;
  gpuBltEffect->Begin(&unused, 0);
  gpuBltEffect->BeginPass(0);
 }
 d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
 if(GPUBlt) {
  gpuBltEffect->EndPass();
  gpuBltEffect->End();
 }
 d3d9Device->EndScene();
 Present();
}

void graphics_OnGameLoad() {
 for (DWORD d=0;d<shaders.size();d++) SAFERELEASE(shaders[d].Effect);
 shaders.clear();
}

class FakePalette2 : IDirectDrawPalette
{
private:
 ULONG Refs;
public:
 FakePalette2() { Refs=1; }
    // IUnknown methods
    HRESULT _stdcall QueryInterface(REFIID, LPVOID*) { return E_NOINTERFACE; }
    ULONG _stdcall AddRef()  { return ++Refs; }
    ULONG _stdcall Release() { 
  if(!--Refs) {
   delete this;
   return 0;
  } else return Refs;
 }

    // IDirectDrawPalette methods
    HRESULT _stdcall GetCaps(LPDWORD) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetEntries(DWORD,DWORD,DWORD,LPPALETTEENTRY) { UNUSEDFUNCTION; }
    HRESULT _stdcall Initialize(LPDIRECTDRAW,DWORD,LPPALETTEENTRY) { UNUSEDFUNCTION; }
    HRESULT _stdcall SetEntries(DWORD,DWORD b,DWORD c,LPPALETTEENTRY d) {
  if(b+c > 256||c==0) return DDERR_INVALIDPARAMS;
  if(GPUBlt) {
   if(!gpuPalette) {
    CopyMemory(&palette[b], d, c*4);
   } else {
    D3DLOCKED_RECT rect;
    if(!FAILED(gpuPalette->LockRect(0, &rect, 0, D3DLOCK_DISCARD))) {
     CopyMemory(&palette[b], d, c*4);
     CopyMemory(rect.pBits, palette, 256*4);
     gpuPalette->UnlockRect(0);
    }
   }
  } else {
   CopyMemory(&palette[b], d, c*4);
   for(DWORD i=b;i<b+c;i++) {
    //palette[i]&=0x00ffffff;
    BYTE temp=*(BYTE*)((DWORD)&palette[i]+0);
    *(BYTE*)((DWORD)&palette[i]+0)=*(BYTE*)((DWORD)&palette[i]+2);
    *(BYTE*)((DWORD)&palette[i]+2)=temp;
   }
  }
  return DD_OK;
 }
};

class FakeSurface2 : IDirectDrawSurface
{
private:
 ULONG Refs;
 bool Primary;
 BYTE* lockTarget;
public:
 FakeSurface2(bool primary) {
  Refs=1;
  Primary=primary;
  lockTarget=new BYTE[ResWidth*ResHeight];
 }
    // IUnknown methods
    HRESULT _stdcall QueryInterface(REFIID, LPVOID *) { return E_NOINTERFACE; }
    ULONG _stdcall AddRef()  { return ++Refs; }
    ULONG _stdcall Release() {
  if(!--Refs) {
   delete lockTarget;
   delete this;
   return 0;
  } else return Refs;
 }
    // IDirectDrawSurface methods
 HRESULT _stdcall AddAttachedSurface(LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
 HRESULT _stdcall AddOverlayDirtyRect(LPRECT) { UNUSEDFUNCTION; }
    HRESULT _stdcall Blt(LPRECT a,LPDIRECTDRAWSURFACE b, LPRECT c,DWORD d, LPDDBLTFX e) {
  BYTE* lockTarget=((FakeSurface2*)b)->lockTarget;
  D3DLOCKED_RECT dRect;
  Tex->LockRect(0, &dRect, 0, 0);
  if(!GPUBlt) dRect.Pitch/=4;
  DWORD yoffset=(ResHeight-320)/2;
  DWORD xoffset=(ResWidth-640)/2;
  if(GPUBlt) {
   char* pBits=(char*)dRect.pBits;
   for(DWORD y=0;y<320;y++) {
    CopyMemory(&pBits[(y+yoffset)*dRect.Pitch + xoffset], &lockTarget[y*640], 640);
   }
   for(DWORD y=0;y<yoffset;y++)                   ZeroMemory(&pBits[y*dRect.Pitch], ResWidth);
   for(DWORD y=ResHeight-yoffset;y<ResHeight;y++) ZeroMemory(&pBits[y*dRect.Pitch], ResWidth);
   if(ResWidth>640) {
    for(DWORD y=yoffset;y<ResHeight-yoffset;y++) {
     ZeroMemory(&pBits[y*dRect.Pitch], xoffset);
     ZeroMemory(&pBits[y*dRect.Pitch + (ResWidth-xoffset)], xoffset);
    }
   }
  } else {
   for(DWORD y=0;y<320;y++) {
    for(DWORD x=0;x<640;x++) {
     ((DWORD*)dRect.pBits)[(y+yoffset)*dRect.Pitch + x + xoffset]=palette[lockTarget[y*640 + x]];
    }
   }
   for(DWORD x=0;x<ResWidth;x++) {
    for(DWORD y=0;y<yoffset;y++) ((DWORD*)dRect.pBits)[(y)*dRect.Pitch + x]=0;
    for(DWORD y=ResHeight-yoffset;y<ResHeight;y++) ((DWORD*)dRect.pBits)[(y)*dRect.Pitch + x]=0;
   }
   if(ResWidth>640) {
    for(DWORD y=yoffset;y<ResHeight-yoffset;y++) {
     for(DWORD x=0;x<xoffset;x++) ((DWORD*)dRect.pBits)[(y)*dRect.Pitch + x]=0;
     for(DWORD x=ResWidth-xoffset;x<ResWidth;x++) ((DWORD*)dRect.pBits)[(y)*dRect.Pitch + x]=0;
    }
   }
  }
  Tex->UnlockRect(0);
  if(!DeviceLost) {
   d3d9Device->SetStreamSource(0, vBuffer2, 0, sizeof(MyVertex));
   d3d9Device->SetTexture(0, Tex);
   d3d9Device->BeginScene();
   if(GPUBlt) {
    UINT unused;
    gpuBltEffect->Begin(&unused, 0);
    gpuBltEffect->BeginPass(0);
   }
   d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
   if(GPUBlt) {
    gpuBltEffect->EndPass();
    gpuBltEffect->End();
   }
   d3d9Device->EndScene();
   Present();
  }
  return DD_OK;
 }
    HRESULT _stdcall BltBatch(LPDDBLTBATCH, DWORD, DWORD) { UNUSEDFUNCTION; }
    HRESULT _stdcall BltFast(DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD) { UNUSEDFUNCTION; }
    HRESULT _stdcall DeleteAttachedSurface(DWORD,LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
    HRESULT _stdcall EnumAttachedSurfaces(LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
    HRESULT _stdcall EnumOverlayZOrders(DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
    HRESULT _stdcall Flip(LPDIRECTDRAWSURFACE, DWORD) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetBltStatus(DWORD) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetCaps(LPDDSCAPS) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetClipper(LPDIRECTDRAWCLIPPER *) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetColorKey(DWORD, LPDDCOLORKEY) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetDC(HDC *) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetFlipStatus(DWORD) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetOverlayPosition(LPLONG, LPLONG) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetPalette(LPDIRECTDRAWPALETTE *) { UNUSEDFUNCTION; }
 HRESULT _stdcall GetPixelFormat(LPDDPIXELFORMAT) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetSurfaceDesc(LPDDSURFACEDESC) { UNUSEDFUNCTION; }
    HRESULT _stdcall Initialize(LPDIRECTDRAW, LPDDSURFACEDESC) { UNUSEDFUNCTION; }
    HRESULT _stdcall IsLost() { UNUSEDFUNCTION; }
    HRESULT _stdcall Lock(LPRECT a,LPDDSURFACEDESC b,DWORD c,HANDLE d) {

  if(!Primary) *b=movieDesc;
  else *b=surfaceDesc;
  b->lpSurface=lockTarget;
  return DD_OK;
 }

    HRESULT _stdcall ReleaseDC(HDC) { UNUSEDFUNCTION; }
    HRESULT _stdcall Restore() { UNUSEDFUNCTION; }
    HRESULT _stdcall SetClipper(LPDIRECTDRAWCLIPPER) { UNUSEDFUNCTION; }
    HRESULT _stdcall SetColorKey(DWORD, LPDDCOLORKEY) { UNUSEDFUNCTION; }
    HRESULT _stdcall SetOverlayPosition(LONG, LONG) { UNUSEDFUNCTION; }
 HRESULT _stdcall SetPalette(LPDIRECTDRAWPALETTE) { return DD_OK; }
    HRESULT _stdcall Unlock(LPVOID) {
  if(Primary&&d3d9Device) {
   if(DeviceLost) {
    if(d3d9Device->TestCooperativeLevel()==D3DERR_DEVICENOTRESET) {
     ResetDevice(false);
     DeviceLost=false;
    }
   }
   if(!DeviceLost) {
    D3DLOCKED_RECT dRect;
    Tex->LockRect(0, &dRect, 0, 0);
    if(!GPUBlt) dRect.Pitch/=4;
    if(GPUBlt) {
     char* target=(char*)dRect.pBits;
     for(DWORD y=0;y<ResHeight;y++) CopyMemory(&target[y*dRect.Pitch], &lockTarget[y*ResWidth], ResWidth);
    } else {
     if(!(ResWidth%8)) {
      DWORD target=(DWORD)(&lockTarget[0]);
      DWORD palette2=(DWORD)(&palette[0]);
      dRect.Pitch=(dRect.Pitch-ResWidth)*4;
      DWORD ResWidth2=ResWidth/8;
      __asm {
       mov esi, target;
       mov edi, dRect.pBits;
       mov ebx, palette2;
       xor edx, edx;
start:
       mov ecx, ResWidth2;
start2:
       movzx eax, byte ptr ds:[esi];
       mov eax, dword ptr ds:[ebx+eax*4];
       mov dword ptr ds:[edi], eax;
       movzx eax, byte ptr ds:[esi+1];
       mov eax, dword ptr ds:[ebx+eax*4];
       mov dword ptr ds:[edi+4], eax;
       movzx eax, byte ptr ds:[esi+2];
       mov eax, dword ptr ds:[ebx+eax*4];
       mov dword ptr ds:[edi+8], eax;
       movzx eax, byte ptr ds:[esi+3];
       mov eax, dword ptr ds:[ebx+eax*4];
       mov dword ptr ds:[edi+12], eax;
       movzx eax, byte ptr ds:[esi+4];
       mov eax, dword ptr ds:[ebx+eax*4];
       mov dword ptr ds:[edi+16], eax;
       movzx eax, byte ptr ds:[esi+5];
       mov eax, dword ptr ds:[ebx+eax*4];
       mov dword ptr ds:[edi+20], eax;
       movzx eax, byte ptr ds:[esi+6];
       mov eax, dword ptr ds:[ebx+eax*4];
       mov dword ptr ds:[edi+24], eax;
       movzx eax, byte ptr ds:[esi+7];
       mov eax, dword ptr ds:[ebx+eax*4];
       mov dword ptr ds:[edi+28], eax;

       add esi, 8;
       add edi, 32;

       loop start2;
       inc edx;
       add edi, dRect.Pitch;
       cmp edx, ResHeight;
       jl start;
      }
     } else {
      DWORD pitch2, width2=0;
      WORD pitch=(WORD)dRect.Pitch;
      WORD width=(WORD)ResWidth;
      DWORD* target=((DWORD*)dRect.pBits);
      for(WORD y=0;y<ResHeight;y++) {
       pitch2=y*pitch;
       width2=y*width;
       for(DWORD x=0;x<ResWidth;x++) {
        target[pitch2+x]=palette[lockTarget[width2+x]];
       }
      }
     }
    }
    Tex->UnlockRect(0);
    RefreshGraphics();
   }
  }
  return DD_OK;
 }
    HRESULT _stdcall UpdateOverlay(LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX) { UNUSEDFUNCTION; }
    HRESULT _stdcall UpdateOverlayDisplay(DWORD) { UNUSEDFUNCTION; }
    HRESULT _stdcall UpdateOverlayZOrder(DWORD, LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
};


class FakeDirectDraw2 : IDirectDraw
{
private:
 ULONG Refs;
public:
 FakeDirectDraw2() {
  Refs=1;
 }
    // IUnknown methods
    HRESULT _stdcall QueryInterface(REFIID, LPVOID*) { return E_NOINTERFACE; }
    ULONG _stdcall AddRef()  { return ++Refs; }
    ULONG _stdcall Release() { 
  if(!--Refs) {
   for(DWORD d=0;d<shaders.size();d++) SAFERELEASE(shaders[d].Effect);
   for(DWORD d=0;d<shaderTextures.size();d++) shaderTextures[d]->Release();
   shaders.clear();
   shaderTextures.clear();
   SAFERELEASE(backbuffer);
   SAFERELEASE(sSurf1);
   SAFERELEASE(sSurf2);
   SAFERELEASE(Tex);
   SAFERELEASE(sTex1);
   SAFERELEASE(sTex2);
   SAFERELEASE(vBuffer);
   SAFERELEASE(vBuffer2);
   SAFERELEASE(d3d9Device);
   SAFERELEASE(d3d9);
   SAFERELEASE(gpuPalette);
   SAFERELEASE(gpuBltEffect);
   SAFERELEASE(movieBuffer);
   SAFERELEASE(movieTex);
   delete this;
   return 0;
  } else return Refs;
 }
    // IDirectDraw methods
    HRESULT _stdcall Compact() { UNUSEDFUNCTION; }
    HRESULT _stdcall CreateClipper(DWORD, LPDIRECTDRAWCLIPPER*, IUnknown*) { UNUSEDFUNCTION; }
 HRESULT _stdcall CreatePalette(DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE* c, IUnknown*) {
  *c=(IDirectDrawPalette*)new FakePalette2();
  return DD_OK;
 }

    HRESULT _stdcall CreateSurface(LPDDSURFACEDESC a, LPDIRECTDRAWSURFACE * b, IUnknown * c) {
  if(a->dwFlags==1&&a->ddsCaps.dwCaps==DDSCAPS_PRIMARYSURFACE) *b=(IDirectDrawSurface*)new FakeSurface2(true);
  else *b=(IDirectDrawSurface*)new FakeSurface2(false);
  return DD_OK;
 }

    HRESULT _stdcall DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
    HRESULT _stdcall EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK) { UNUSEDFUNCTION; }
    HRESULT _stdcall EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
 HRESULT _stdcall FlipToGDISurface() { UNUSEDFUNCTION; }
    HRESULT _stdcall GetCaps(LPDDCAPS, LPDDCAPS b) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetDisplayMode(LPDDSURFACEDESC) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetFourCCCodes(LPDWORD,LPDWORD) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetGDISurface(LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetMonitorFrequency(LPDWORD) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetScanLine(LPDWORD) { UNUSEDFUNCTION; }
    HRESULT _stdcall GetVerticalBlankStatus(LPBOOL) { UNUSEDFUNCTION; }
    HRESULT _stdcall Initialize(GUID *) { UNUSEDFUNCTION; }
    HRESULT _stdcall RestoreDisplayMode() { return DD_OK; }
    HRESULT _stdcall SetCooperativeLevel(HWND a, DWORD b) {
  window=a;

  if(!d3d9Device) {
   ResetDevice(true);
   CoInitialize(0);
  }

  if (GraphicsMode==5) {
   SetWindowLong(a, GWL_STYLE, WS_OVERLAPPED|WS_CAPTION|WS_BORDER);
   RECT r;
   r.left=0;
   r.right=gWidth;
   r.top=0;
   r.bottom=gHeight;
   AdjustWindowRect(&r, WS_OVERLAPPED|WS_CAPTION|WS_BORDER, false);
   r.right-=r.left;
   r.left=0;
   r.bottom-=r.top;
   r.top=0;
   SetWindowPos(a, HWND_NOTOPMOST, 0, 0, r.right, r.bottom,SWP_DRAWFRAME|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
  }
  return DD_OK;
 }
    HRESULT _stdcall SetDisplayMode(DWORD, DWORD, DWORD) { return DD_OK; }
    HRESULT _stdcall WaitForVerticalBlank(DWORD, HANDLE) { UNUSEDFUNCTION; }
};


HRESULT _stdcall FakeDirectDrawCreate2(void*, IDirectDraw** b, void*) {
 ResWidth=*(DWORD*)0x4B5B35;
 ResHeight=*(DWORD*)0x4B5B30;

 if(!d3d9) {
  d3d9=Direct3DCreate9(D3D_SDK_VERSION);
 }

 ZeroMemory(&surfaceDesc, sizeof(DDSURFACEDESC));
 surfaceDesc.dwSize=sizeof(DDSURFACEDESC);
 surfaceDesc.dwFlags=DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_PITCH;
 surfaceDesc.dwWidth=ResWidth;
 surfaceDesc.dwHeight=ResHeight;
 surfaceDesc.ddpfPixelFormat.dwRGBBitCount=16;
 surfaceDesc.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
 surfaceDesc.ddpfPixelFormat.dwRBitMask=0xf800;
 surfaceDesc.ddpfPixelFormat.dwGBitMask=0x7e0;
 surfaceDesc.ddpfPixelFormat.dwBBitMask=0x1f;
 surfaceDesc.ddpfPixelFormat.dwFlags=DDPF_RGB;
 surfaceDesc.ddsCaps.dwCaps=DDSCAPS_TEXTURE;
 surfaceDesc.lPitch=ResWidth;
 movieDesc=surfaceDesc;
 movieDesc.lPitch=640;
 movieDesc.dwHeight=320;
 movieDesc.dwWidth=640;

 gWidth=GetPrivateProfileIntA("Graphics", "GraphicsWidth", 0, ini);
 gHeight=GetPrivateProfileIntA("Graphics", "GraphicsHeight", 0, ini);
 if(!gWidth||!gHeight) {
  gWidth=ResWidth;
  gHeight=ResHeight;
 }
 GPUBlt=GetPrivateProfileIntA("Graphics", "GPUBlt", 0, ini);
 if(!GPUBlt || GPUBlt>2) GPUBlt=2; //Swap them around to keep compatibility with old ddraw.ini's
 else if(GPUBlt==2) GPUBlt=0;

 if(GraphicsMode==5) {
  ScrollWindowKey=GetPrivateProfileInt("Input", "WindowScrollKey", 0, ini);
 } else ScrollWindowKey=0;

 rcpres[0]=1.0f/(float)gWidth;
 rcpres[1]=1.0f/(float)gHeight;

 *b=(IDirectDraw*)new FakeDirectDraw2();
 return DD_OK;
}
