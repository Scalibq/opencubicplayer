#define SOUNDDEVICE plrDirectSound
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "imsdev.h"

extern "C" sounddevice SOUNDDEVICE;

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
  switch (iMsg)
    {
    case WM_DESTROY :
      PostQuitMessage (0) ;
      return 0 ;
    }
  return DefWindowProc(hwnd, iMsg, wParam, lParam) ;
}


int plrRate;
int plrOpt;
int (*plrGetBufPos)();
int (*plrGetPlayPos)();
void (*plrAdvanceTo)(int);
long (*plrGetTimer)();
void (*plrSetOptions)(int, int);
int (*plrPlay)(void * &, int & );
void (*plrStop)();

extern "C"
{

  int __export __cdecl vplrOpt()
  {
    return plrOpt;
  }

  int __export __cdecl vplrRate()
  {
    return plrRate;
  }


  /* to convert the __cdecls from the APC to
     watcom-calling-convention. */

  int __export __cdecl vplrGetBufPos()
  {
    return plrGetBufPos();
  }

  int __export __cdecl vplrGetPlayPos()
  {
    return plrGetPlayPos();
  }

  void __export __cdecl vplrAdvanceTo(int a)
  {
    plrAdvanceTo(a);
  }

  long __export __cdecl vplrGetTimer()
  {
    return plrGetTimer();
  }

  void __export __cdecl vplrSetOptions(int a, int b)
  {
    plrSetOptions(a, b);
  }

  int __export __cdecl vplrPlay(void * &a, int &b)
  {
    return plrPlay(a, b);
  }

  void __export __cdecl vplrStop()
  {
    plrStop();
  }

  sounddevice * __cdecl __export vplrGetDeviceStruct()
  {
    return &SOUNDDEVICE;
  }

  int __export __cdecl vplrDetect(deviceinfo &card)
  {
    int res=SOUNDDEVICE.Detect(card);
    return res;
  }

  int __export __cdecl vplrInit(const deviceinfo &card)
  {
    return SOUNDDEVICE.Init(card);
  }

  void __export __cdecl vplrClose()
  {
    SOUNDDEVICE.Close();
  }

}
