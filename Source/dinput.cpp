//#define STRICT
//#define WIN32_LEAN_AND_MEAN

#include "main.h"

#define DIRECTINPUT_VERSION         0x0700
#include <dinput.h>
#include <math.h>
#include "Graphics.h"
#include "Logging.h"
#include "queue9x.cpp"

typedef HRESULT (_stdcall *DInputCreateProc)(HINSTANCE a,DWORD b,IDirectInputA** c,IUnknown* d);

static bool UseScrollWheel;
static DWORD WheelMod;

static bool ReverseMouse;

static bool MiddleMouseDown;
static DWORD MiddleMouseKey;

static bool BackgroundKeyboard;
static bool BackgroundMouse;

static bool AdjustMouseSpeed;
static double MouseSpeedMod;
static double MousePartX;
static double MousePartY;

#define MAX_KEYS (264)
static DWORD KeysDown[MAX_KEYS];

static int mouseX;
static int mouseY;

DWORD ForcingGraphicsRefresh=0;

//void _stdcall ForceGraphicsRefresh(DWORD d) { ForcingGraphicsRefresh=(d==0)?0:1; }
void GetMouse(int* x, int* y) { *x=mouseX; *y=mouseY; mouseX=0; mouseY=0; }

static BYTE LMouse=0;
static BYTE RMouse=0;
static int MPMouseX;
static int MPMouseY;

#define kDeviceType_KEYBOARD 0
#define kDeviceType_MOUSE 1

static queue<DIDEVICEOBJECTDATA> bufferedPresses;

static HKL keyboardLayout;

DWORD _stdcall KeyDown(DWORD key) {
 if ((key & 0x80000000) > 0) { // special flag to check by VK code directly
  return GetAsyncKeyState(key & 0xFFFF) & 0x8000;
 }
 key = key & 0xFFFF;
 // combined use of DINPUT states + confirmation from GetAsyncKeyState()
 if (key>MAX_KEYS) {
  return 0;
 } else {
  DWORD keyVK = 0;
  if (KeysDown[key]) { // confirm pressed state  
   keyVK = MapVirtualKeyEx(key, MAPVK_VSC_TO_VK, keyboardLayout);
   if (keyVK) KeysDown[key] = (GetAsyncKeyState(keyVK) & 0x8000);
  }
  return (KeysDown[key] > 0);
 }
}

void _stdcall TapKey(DWORD key) {
 DIDEVICEOBJECTDATA data;
 data.dwTimeStamp=0;
 data.dwSequence=0;
 data.dwOfs=key;
 data.dwData=0x80;
 bufferedPresses.push(data);
 data.dwData=0x00;
 bufferedPresses.push(data);
}

class FakeDirectInputDevice : public IDirectInputDeviceA {
private:
    IDirectInputDeviceA* RealDevice;
    DWORD DeviceType;
 ULONG Refs;
 bool formatLock;
 DIDATAFORMAT oldFormat;
public:
    /*** Constructor and misc functions ***/
    FakeDirectInputDevice(IDirectInputDevice* device,DWORD type) {
        RealDevice=device;
        DeviceType=type;
  Refs=1;
  formatLock = false;
  oldFormat.dwDataSize=0;
    }

    /*** IUnknown methods ***/
    HRESULT _stdcall QueryInterface (REFIID riid, LPVOID * ppvObj) { return RealDevice->QueryInterface(riid,ppvObj); }
    ULONG _stdcall AddRef(void) { return ++Refs; }
    ULONG _stdcall Release(void) {
  if(--Refs==0) {
   RealDevice->Release();
   delete this;
   return 0;
  } else { return Refs; }
 }

 /*** IDirectInputDevice8A methods ***/
 HRESULT _stdcall GetCapabilities(LPDIDEVCAPS a) { return RealDevice->GetCapabilities(a); }
 HRESULT _stdcall EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA a,LPVOID b,DWORD c) { return RealDevice->EnumObjects(a,b,c); }
 HRESULT _stdcall GetProperty(REFGUID a,DIPROPHEADER* b) { return RealDevice->GetProperty(a,b); }
 HRESULT _stdcall SetProperty(REFGUID a,const DIPROPHEADER* b) { return RealDevice->SetProperty(a,b); }
 HRESULT _stdcall Acquire(void) { return RealDevice->Acquire(); }
 HRESULT _stdcall Unacquire(void) { return RealDevice->Unacquire(); }

 //Only called for the mouse
 HRESULT _stdcall GetDeviceState(DWORD a,LPVOID b) {
  if (ForcingGraphicsRefresh) RefreshGraphics();
  if (DeviceType!=kDeviceType_MOUSE) return RealDevice->GetDeviceState(a,b);

  DIMOUSESTATE2 MouseState;
  HRESULT hr;
  int numButtons;
  if(formatLock) hr=RealDevice->GetDeviceState(sizeof(DIMOUSESTATE2), &MouseState);
  else hr=RealDevice->GetDeviceState(sizeof(DIMOUSESTATE), &MouseState);
  if(FAILED(hr)) return hr;
  if(ReverseMouse) {
   BYTE tmp=MouseState.rgbButtons[0];
   MouseState.rgbButtons[0]=MouseState.rgbButtons[1];
   MouseState.rgbButtons[1]=tmp;
  }
  if(AdjustMouseSpeed) {
   double d=((double)MouseState.lX)*MouseSpeedMod + MousePartX;
   MousePartX = modf(d, &d);
   MouseState.lX=(LONG)d;
   d=((double)MouseState.lY)*MouseSpeedMod + MousePartY;
   MousePartY = modf(d, &d);
   MouseState.lY=(LONG)d;
  }
  if(UseScrollWheel) {
   if(MouseState.lZ>0) {
    int count;
    if(WheelMod) count=MouseState.lZ/WheelMod;
    else count=1;
    while(count--) TapKey(DIK_UP);
   } else if(MouseState.lZ<0) {
    int count;
    if(WheelMod) count=(-MouseState.lZ)/WheelMod;
    else count=1;
    while(count--) TapKey(DIK_DOWN);
   }
  }
  if(MiddleMouseKey&&MouseState.rgbButtons[2]) {
   if(!MiddleMouseDown) {
    TapKey(MiddleMouseKey);
    MiddleMouseDown=true;
   }
  } else MiddleMouseDown=false;
  mouseX=MouseState.lX;
  mouseY=MouseState.lY;

  numButtons = formatLock ? 8 : 4;
  for(int i=0;i<numButtons;i++) {
/*   if ((MouseState.rgbButtons[i] & 0x80) != (KeysDown[256+i] & 0x80)) { // state changed
    MouseClickHook(i, (MouseState.rgbButtons[i] & 0x80) > 0);
   }*/
   KeysDown[256+i]=MouseState.rgbButtons[i];
  }

  memcpy(b, &MouseState, sizeof(DIMOUSESTATE));
  return 0;
 }

 //Only called for the keyboard
 HRESULT _stdcall GetDeviceData(DWORD a,DIDEVICEOBJECTDATA* b,DWORD* c,DWORD d) {
  if(DeviceType!=kDeviceType_KEYBOARD) return RealDevice->GetDeviceData(a,b,c,d);

  if(!b||bufferedPresses.empty()||(d&DIGDD_PEEK)) {
   HRESULT hr=RealDevice->GetDeviceData(a,b,c,d);
   if(FAILED(hr)||!b||!(*c)) return hr;
   for(DWORD i=0;i<*c;i++) {
    KeysDown[b[i].dwOfs]=b[i].dwData&0x80;
//    KeyPressHook(b[i].dwOfs, (b[i].dwData & 0x80) > 0, MapVirtualKeyEx(b[i].dwOfs, MAPVK_VSC_TO_VK, keyboardLayout));
   }
   return hr;
  }
  //Despite passing an array of 32 data objects, fallout cant seem to cope with a key being pressed and released in the same frame...
  //TODO: Fallouts behaviour when passing multiple keypresses makes it appear like it's expecting the DIDEVICEOBJECTDATA struct to be
  //      something other than 16 bytes. afaik, fallout uses DX3 for input, which is before the appData field was added, but it could
  //      be worth checking anyway.
  *b=bufferedPresses.front();
  bufferedPresses.pop();
  *c=1;
  return DI_OK;
 }
 HRESULT _stdcall SetDataFormat(const DIDATAFORMAT* a) {
  if(formatLock&&oldFormat.dwSize) return RealDevice->SetDataFormat(&oldFormat);
  memcpy(&oldFormat, a, sizeof(DIDATAFORMAT));
  return RealDevice->SetDataFormat(a);
 }
 HRESULT _stdcall SetEventNotification(HANDLE a) { return RealDevice->SetEventNotification(a); }
 HRESULT _stdcall SetCooperativeLevel(HWND a,DWORD b) {
  if(DeviceType==kDeviceType_KEYBOARD&&BackgroundKeyboard) b=DISCL_BACKGROUND|DISCL_NONEXCLUSIVE;
  if(DeviceType==kDeviceType_MOUSE&&BackgroundMouse) b=DISCL_BACKGROUND|DISCL_NONEXCLUSIVE;
  return RealDevice->SetCooperativeLevel(a,b);
 }
 HRESULT _stdcall GetObjectInfo(LPDIDEVICEOBJECTINSTANCEA a,DWORD b,DWORD c) { return RealDevice->GetObjectInfo(a,b,c); }
 HRESULT _stdcall GetDeviceInfo(LPDIDEVICEINSTANCEA a) { return RealDevice->GetDeviceInfo(a); }
 HRESULT _stdcall RunControlPanel(HWND a,DWORD b) { return RealDevice->RunControlPanel(a,b); }
 HRESULT _stdcall Initialize(HINSTANCE a,DWORD b,REFGUID c) { return RealDevice->Initialize(a,b,c); }
 void LockDataFormat(bool lock) {formatLock = lock; }
};

class FakeDirectInput : public IDirectInputA {
private:
    IDirectInputA* RealInput;
 ULONG Refs;
public:
    /*** Constructor ***/
    FakeDirectInput(IDirectInput* Real) { RealInput=Real; Refs=1; }
    /*** IUnknown methods ***/
    HRESULT _stdcall QueryInterface (REFIID riid, LPVOID* ppvObj) { return RealInput->QueryInterface(riid,ppvObj); }
 ULONG _stdcall AddRef(void) { return ++Refs; }
    ULONG _stdcall Release(void) {
  if(--Refs==0) {
   RealInput->Release();
   delete this;
   return 0;
  } else { return Refs; }
    }
    /*** IDirectInput8A methods ***/
    HRESULT _stdcall CreateDevice(REFGUID r,IDirectInputDeviceA** device,IUnknown* unused) {
  GUID GUID_SysMouse    = { 0x6F1D2B60, 0xD5A0, 0x11CF, { 0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00} };
  GUID GUID_SysKeyboard = { 0x6F1D2B61, 0xD5A0, 0x11CF, { 0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00} };

        if(r!=GUID_SysKeyboard&&r!=GUID_SysMouse) { 
   return RealInput->CreateDevice(r,device,unused);
  } else {
            DWORD d;
   IDirectInputDeviceA* RealDevice;
   HRESULT hr;

            if(r==GUID_SysKeyboard) d=kDeviceType_KEYBOARD;
            else d=kDeviceType_MOUSE;
            hr=RealInput->CreateDevice(r,&RealDevice,unused);
            if(hr!=DI_OK) return hr;

   FakeDirectInputDevice* fd=new FakeDirectInputDevice(RealDevice,d);
            *device = fd;
   if(d==kDeviceType_MOUSE) {
/*    if(FAILED(fd->SetDataFormat(&c_dfDIMouse2))) {
     dlogr("ERROR: Could not set mouse format to DIMOUSESTATE2. Only 4 buttons will be available!", DL_MAIN);
    } else fd->LockDataFormat(true);*/
   }
            return DI_OK;
        }
    }
    HRESULT _stdcall EnumDevices(DWORD a,LPDIENUMDEVICESCALLBACKA b,void* c,DWORD d) { return RealInput->EnumDevices(a,b,c,d); }
    HRESULT _stdcall GetDeviceStatus(REFGUID r) { return RealInput->GetDeviceStatus(r); }
    HRESULT _stdcall RunControlPanel(HWND a,DWORD b) { return RealInput->RunControlPanel(a,b); }
    HRESULT _stdcall Initialize(HINSTANCE a,DWORD b) { return RealInput->Initialize(a,b); }
};

HRESULT _stdcall FakeDirectInputCreate(HINSTANCE a,DWORD b,IDirectInputA** c,IUnknown* d) {
 ZeroMemory(KeysDown,sizeof(KeysDown));

 HMODULE dinput=LoadLibraryA("dinput.dll");
 if(!dinput||dinput==INVALID_HANDLE_VALUE) return -1;
 DInputCreateProc proc=(DInputCreateProc)GetProcAddress(dinput, "DirectInputCreateA");
 if(!proc) return -1;

 HRESULT hr=proc(a,b,c,d);
 if(FAILED(hr)) return hr;
 
 ReverseMouse=GetPrivateProfileInt("Input", "ReverseMouseButtons", 0, ini)!=0;

 UseScrollWheel=GetPrivateProfileInt("Input", "UseScrollWheel", 1, ini)!=0;
 WheelMod=GetPrivateProfileInt("Input", "ScrollMod", 0, ini);
 LONG MouseSpeed=GetPrivateProfileInt("Input", "MouseSensitivity", 100, ini);
 if(MouseSpeed!=100) {
  AdjustMouseSpeed=true;
  MouseSpeedMod=((double)MouseSpeed)/100.0;
  MousePartX=0;
  MousePartY=0;
 } else AdjustMouseSpeed=false;

 MiddleMouseKey=GetPrivateProfileInt("Input", "MiddleMouse", 0x30, ini);
 MiddleMouseDown=false;

 BackgroundKeyboard=GetPrivateProfileInt("Input", "BackgroundKeyboard", 0, ini)!=0;
 BackgroundMouse=GetPrivateProfileInt("Input", "BackgroundMouse", 0, ini)!=0;

 *c=(IDirectInputA*)new FakeDirectInput(*c);

 keyboardLayout = GetKeyboardLayout(0);

 return hr;
}
