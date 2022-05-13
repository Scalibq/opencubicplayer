// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// virtual Player device for APC-gatewaying and possible DirectSound output
//
// revision history: (please note changes here)
//  -fd990814   Felix Domke <tmbinc@gmx.net>
//    -first release

// note: althought there are much more things you can do with APC, this
//       devp got the name "DirectSound", because otherwise some users
//       might get confused...

#define PRE_BUFFER
#include "imsdev.h"
#include "player.h"
#include "imsrtns.h"
#include "err.h"
#include "pmain.h"
#include "timer.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

extern "C" void * GetAddress(const char *string);
#pragma aux GetAddress parm [] caller;

            /* sync */
#pragma pack (push, 1)
struct query_s /* 0.2 */
{
  void *esp, *proc;
  long done, res, stackcopy;
};
#pragma pack (pop)

#ifdef PRE_BUFFER
static char mybuffer[128*1024]; /* this is a not-so-good-solution. i know this. anyone a better idea? */
static int buflen;
#endif

static int stimer;

extern "C"
{
  int VxDInit();

  HINSTANCE __cdecl apcLoadLibrary(const char *dll);
  void __cdecl apcFreeLibrary(HINSTANCE dll);
  void __cdecl *apcGetProcAddress(HINSTANCE dll, const char *symbol);

  extern "C" void *aapcLoadLibrary, *aapcFreeLibrary, *aapcGetProcAddress;

  int __cdecl vplrOpt();
  int __cdecl vplrRate();
  int __cdecl vplrGetBufPos();
  int __cdecl vplrGetPlayPos();
  void __cdecl vplrAdvanceTo(int a);
  long __cdecl vplrGetTimer();
  void __cdecl vplrSetOptions(int a, int b);
  int __cdecl vplrPlay(void * &a, int &b);
  void __cdecl vplrStop();
  sounddevice __cdecl *vplrGetDeviceStruct();
  int __cdecl vplrDetect(deviceinfo &card);
  int __cdecl vplrInit(const deviceinfo &card);
  void __cdecl vplrClose();

  extern void *avplrOpt, *avplrRate, *avplrGetBufPos, *avplrGetPlayPos,
       *avplrAdvanceTo, *avplrGetTimer, *avplrSetOptions,
       *avplrPlay, *avplrStop, *avplrGetDeviceStruct,
       *avplrDetect, *avplrInit, *avplrClose;

  extern query_s query;
}

static HINSTANCE hivocp;
static int loaded;

static int vxdint()
{
  int version=VxDInit();
  if (version==-1)
  {
#ifdef DEBUG
    printf("[vxdDebug] version %x indicates: VAPC.VXD not loaded!\n", version);
#endif
    return errOk;
  }

  aapcLoadLibrary=GetAddress("apcLoadLibrary");
  aapcFreeLibrary=GetAddress("apcFreeLibrary");
  aapcGetProcAddress=GetAddress("apcGetProcAddress");
  if ((!aapcLoadLibrary) || (!aapcFreeLibrary) || (!aapcGetProcAddress))
  {
#ifdef DEBUG
    printf("[vxdDebug] {%p, %p, %p} indicates: missing virtual exports.\n", aapcLoadLibrary, aapcFreeLibrary, aapcGetProcAddress);
#endif
    return errOk;   // although THIS IS an error. who cares.
  }

  hivocp=apcLoadLibrary("VOCP.DLL");
  if (!hivocp)
  {
#ifdef DEBUG
    printf("[vxdDebug] LoadLibrary(\"VOCP.DLL\")==0 indicates: VOCP.DLL could not be loaded.\n");
#endif
    return errOk;
  }

  avplrOpt=apcGetProcAddress(hivocp, "_vplrOpt");
  avplrRate=apcGetProcAddress(hivocp, "_vplrRate");
  avplrGetBufPos=apcGetProcAddress(hivocp, "_vplrGetBufPos");
  avplrGetPlayPos=apcGetProcAddress(hivocp, "_vplrGetPlayPos");
  avplrAdvanceTo=apcGetProcAddress(hivocp, "_vplrAdvanceTo");
  avplrGetTimer=apcGetProcAddress(hivocp, "_vplrGetTimer");
  avplrSetOptions=apcGetProcAddress(hivocp, "_vplrSetOptions");
  avplrPlay=apcGetProcAddress(hivocp, "_vplrPlay");
  avplrStop=apcGetProcAddress(hivocp, "_vplrStop");
  avplrGetDeviceStruct=apcGetProcAddress(hivocp, "_vplrGetDeviceStruct");
  avplrDetect=apcGetProcAddress(hivocp, "_vplrDetect");
  avplrInit=apcGetProcAddress(hivocp, "_vplrInit");
  avplrClose=apcGetProcAddress(hivocp, "_vplrClose");
  if ( (!avplrOpt) || (!avplrRate) || (!avplrGetBufPos) || (!avplrGetPlayPos) ||
       (!avplrAdvanceTo) || (!avplrGetTimer) || (!avplrSetOptions) ||
       (!avplrPlay) || (!avplrStop) || (!avplrGetDeviceStruct) ||
       (!avplrDetect) || (!avplrInit) || (!avplrClose))
  {
#ifdef DEBUG
    printf("[vxdDebug] one or more virtual symbols not found.\n");
#endif
    apcFreeLibrary(hivocp);
    return errOk;
  }

#ifdef DEBUG
  printf("[vxdDebug] ok.\n");
#endif
  loaded=!0;
  return errOk;
}

static void vxdclose()
{
#ifdef DEBUG
  printf("[vxdDebug] closing.\n");
#endif
  if (loaded)
    apcFreeLibrary(hivocp);
}

void vxdSetOptions(int a, int b)
{
  vplrSetOptions(a, b);
  plrRate=vplrRate();
  plrOpt=vplrOpt();
}

int vxdGetBufPos()
{
  static int lastpos=0;
  int res=vplrGetBufPos();
  if (res==-1)
    return lastpos;
  else
    return lastpos=res;
}

int vxdGetPlayPos()
{
  static int lastpos=0;
  int res=vplrGetPlayPos();
  if (res==-1)
    return lastpos;
  else
    return lastpos=res;
}

void vxdAdvanceTo(int a)
{
  vplrAdvanceTo(a);
}

long vxdGetTimer()
{
  return vplrGetTimer();
}

int vxdPlay(void * &a, int &b)
{
#ifdef PRE_BUFFER
  a=mybuffer;
#endif
  int res=vplrPlay(a, b);
#ifdef PRE_BUFFER
  buflen=b;
  if (buflen>128*1024)
    printf("BIG FAT BUG REPORT: len is %d, which is quite more than 128k, the max.\n", buflen);
#endif
  plrGetBufPos=vxdGetBufPos;
  plrGetPlayPos=vxdGetPlayPos;
  plrAdvanceTo=vxdAdvanceTo;
  plrGetTimer=vxdGetTimer;
  return res;
}

int vxdDetect(deviceinfo &card)
{
  if (!loaded)
  {
#ifdef DEBUG
    printf("[vxdDebug] error while initializing, so we will not use the device.\n");
#endif
    return 0;
  }
  return vplrDetect(card);
}

void vxdStop()
{
  vplrStop();
}

int vxdInit(const deviceinfo &card)
{
  int res=vplrInit(card);
  if (res==-1)
    return 0;
  if (res)
  {
    plrSetOptions=vxdSetOptions;
    plrPlay=vxdPlay;
    plrStop=vxdStop;
    tmSetSecure();
    stimer=!0;
  } else
  {
    plrSetOptions=0;
    plrPlay=0;
    plrStop=0;
  }
  return res;
}

void vxdClose()
{
  if (stimer)
    tmReleaseSecure();
  if (loaded)
    vplrClose();
}

extern "C"
{
  initcloseregstruct vxdReg={vxdint, vxdclose};
  sounddevice plrVXD={SS_PLAYER, "DirectSound", vxdDetect, vxdInit, vxdClose};
  char *dllinfo = "driver _plrVXD; preinitclose _vxdReg;";
}
