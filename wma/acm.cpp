/*

                wma4cp, rtl-functions
                                                (c) 1999 by Felix Domke
                                                licensed in the same way
                                                as the OpenCP.


    all functions needed to run the msaudio-acm.
    a *VERY* basic emulation.

    (not *SO* basic anymore... becomes useful ;)
*/

#include <conio.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#define _USER32_
#define _KERNEL32_
#define _ADVAPI32_
#include <windows.h>
#include <mmsystem.h>

#include "pe.h"

#include "mt.h"

extern symbol_c symbol;
extern mt_c *mt;

#undef Yield

struct resource_s
{
  pe_c *module;
  IMAGE_RESOURCE_DATA_ENTRY *irde;
};

struct event_s
{
  int state;
  int autoreset;
};

int _rand()
{
  return 0;                     // microsoft, you suck so much. WHAT IS THIS RAND?!?!?
}

void null()
{
  printf("NULL called - fix that!!\n");
//  exit(0);
}

void __cdecl *_malloc(int bytes)
{
  void *d=malloc(bytes);
//  printf("malloc(%d)=%p;\n", bytes, d);
  mt->Yield();
  return d;
}

void __cdecl *_new(int bytes)
{
  if (bytes<=0)
  {
//    printf("ERROR: %p: malloc of %d bytes.\n", (&bytes)[-1], bytes);
//    exit(0);
    return 0;
  }
  void *d=malloc(bytes);
//  printf("new(%d)=%p;\n", bytes, d);
  mt->Yield();
  return d;
}

void __cdecl _delete(void *mem)
{
//  printf("delete(%x);\n", mem);
  mt->Yield();
  free(mem);
}

int __cdecl __strnicmp(char *s1, char *s2, int n)
{
//  printf("strnicmp(%s, %s, %d);\n", s1, s2, n);
  mt->Yield();
  return strnicmp(s1, s2, n);
}

void __cdecl __free(void *mem)
{
//  printf("free(%p);\n", mem);
  mt->Yield();
  free(mem);
}

float _CIexp(float x)
{
 // printf("exp(%f)=%f;\n", (float)x, (float)exp(x));
  return exp(x);
}

double _CIpow(float y, float x)
{
//  printf("pow(%f, %f)=%f;\n", (float)x, (float)y, (float)pow(x, y));
  return pow(x, y);
}

#pragma aux _CIpow parm [8087] [8087] value [8087];
#pragma aux _CIexp parm [8087] value [8087];

void InitFnc()
{
  const char *crt="MSVCRT.DLL";
  symbol.Add("_except_handler3", crt, null);
  symbol.Add("_strnicmp", crt, __strnicmp);
  symbol.Add("free", crt, __free);
  symbol.Add("_adjust_fdiv", crt, null);
  symbol.Add("_initterm", crt, null);
  symbol.Add("rand", crt, _rand);
  symbol.Add("malloc", crt, _malloc);
  symbol.Add("??2@YAPAXI@Z", crt, _new);
  symbol.Add("??3@YAXPAX@Z", crt, _delete);
  symbol.Add("_CIexp", crt, _CIexp);
  symbol.Add("_CIpow", crt, _CIpow);
  symbol.Add("sprintf", crt, sprintf);
  symbol.Add("LoadStringA", "USER32.DLL", LoadStringA);
  symbol.Add("LocalAlloc", "KERNEL32.DLL", LocalAlloc);
  symbol.Add("SetEvent", "KERNEL32.DLL", SetEvent);
  symbol.Add("GetACP", "KERNEL32.DLL", GetACP);
  symbol.Add("GlobalAlloc", "KERNEL32.DLL", GlobalAlloc);
  symbol.Add("FindResourceA", "KERNEL32.DLL", FindResourceA);
  symbol.Add("LoadResource", "KERNEL32.DLL", LoadResource);
  symbol.Add("SizeofResource", "KERNEL32.DLL", SizeofResource);
  symbol.Add("LockResource", "KERNEL32.DLL", LockResource);
  symbol.Add("CreateEventA", "KERNEL32.DLL", CreateEventA);
  symbol.Add("DisableThreadLibraryCalls", "KERNEL32.DLL", DisableThreadLibraryCalls);
  symbol.Add("GlobalFree", "KERNEL32.DLL", GlobalFree);
  symbol.Add("MultiByteToWideChar", "KERNEL32.DLL", MultiByteToWideChar);
  symbol.Add("WaitForSingleObject", "KERNEL32.DLL", WaitForSingleObject);
  symbol.Add("CreateThread", "KERNEL32.DLL", CreateThread);
  symbol.Add("LocalFree", "KERNEL32.DLL", LocalFree);
  symbol.Add("CloseHandle", "KERNEL32.DLL", CloseHandle);
  symbol.Add("DefDriverProc", "WINMM.DLL", DefDriverProc);
  symbol.Add("GetDriverModuleHandle", "WINMM.DLL", GetDriverModuleHandle);
}

struct resMSAUDIO
{
  int uid;
  char *text;
} res[]={{1, "MS-AUDIO"},
         {2, "Microsoft Audio Codec"},
         {3, "Copyright (c) 1999 Microsoft Corporation"},
         {4, "Microsoft suxx - there is no resource 4!!"},
         {5, "Compresses and decompresses audio data."},
         {10, "Microsoft Audio Codec"}};        // TODO... read this out of the pe!!

extern "C" int WINAPI LoadStringA(HINSTANCE, UINT uid, LPSTR dst, int max)
{
  for (int i=0; i<sizeof(res)/sizeof(*res); i++)
    if (res[i].uid==uid)
    {
      strncpy(dst, res[i].text, max);
      mt->Yield();
      return strlen(res[i].text);       // oder so ...
    }
//  printf("STUB: LoadStringA(..., %d, ...., %d);\n", uid, max);
  mt->Yield();
  return 0;
}

extern "C" HLOCAL WINAPI LocalAlloc(UINT flags, UINT bytes)
{
//  printf("LocalAlloc(0x%x, %d)", flags, bytes);
  void *x=malloc(bytes);
  if (!x) { /*printf("=NULL!\n"); */ return 0; }
  if (flags&LMEM_ZEROINIT) memset(x, 0, bytes);
  if (flags&LMEM_MOVEABLE) ; // printf("wow, cannot handle moveable mem!\n");
//  printf("=%p\n", x);
  mt->Yield();
  return x;
}

extern "C" int WINAPI SetEvent(void *hEvent)
{
//  printf("%p SetEvent(%p);\n", (&hEvent)[-1], hEvent);
  if (!hEvent) return 0;
  event_s *ev=(event_s *)hEvent;
  ev->state=!0;
  mt->Yield();
  return 0;
}

extern "C" UINT WINAPI GetACP()
{
//  printf("GetACP();\n");
  mt->Yield();
  return 0;
}

extern "C" HGLOBAL WINAPI GlobalAlloc(UINT flags, DWORD bytes)
{
//  printf("GlobalAlloc(0x%x, %d)=", flags, bytes);
  void *x=malloc(bytes);
  if (!x) { /* printf("=NULL!\n"); */ return 0; }
  if (flags&LMEM_ZEROINIT) memset(x, 0, bytes);
  if (flags&LMEM_MOVEABLE) ; //printf("wow, cannot handle moveable mem!\n");
//  printf("=%p\n", x);
  mt->Yield();
  return x;
}

extern "C" HRSRC WINAPI FindResource(HMODULE mod, LPCSTR name, LPCSTR type)
{
//  printf("FindResource(%p, %p, %p);\n", mod, name, type);
  if (!mod) return 0;
  mt->Yield();
  return ((pe_c*)mod)->GetResource(name, type);
}

                        /*
                           here is a problem: a "LoadResource" doesn't
                           require a "FreeResource" in Win32.
                           So, WHEN and HOW should we free this memory
                           allocated here?
                        */

extern "C" HGLOBAL WINAPI LoadResource(HMODULE mod, HRSRC rsrc)
{
  if (!mod)
  {
//    printf("UNSUPPORTED: LoadResource(NULL, %p);\n", rsrc);
    return 0;
  }
  resource_s *x=new resource_s;         // todo: autofree.
  x->module=(pe_c*)mod;
  x->irde=(IMAGE_RESOURCE_DATA_ENTRY *)rsrc;
  mt->Yield();
  return x;
}

extern "C" DWORD WINAPI SizeofResource(HMODULE, HRSRC rsrc)
{
  if (!rsrc) return 0;
  mt->Yield();
  return ((IMAGE_RESOURCE_DATA_ENTRY*)rsrc)->Size;
}

extern "C" LPVOID WINAPI LockResource(HGLOBAL rsrc)
{
  if (!rsrc) return 0;
  resource_s *res=(resource_s*)rsrc;
  mt->Yield();
  return (char *)res->module->ConvertPtr(res->irde->OffsetToData);
}

extern "C" HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES, BOOL man, BOOL ini, LPCSTR name)
{
  if (name)
  {
//    printf("STUB: CreateEventA(..., %d, %d, %s);\n", man, ini, name);
    return 0;
  }

  event_s *ne=new event_s;
  if (!ne) return 0;
  
  ne->state=ini;
  ne->autoreset=!man;

//  printf("created event (%p man=%d).\n", ne, man);
  mt->Yield();
  return ne;
}

extern "C" BOOL WINAPI DisableThreadLibraryCalls(HMODULE)
{
//  printf("STUB: DisableThreadLibraryCalls(...);\n");
  mt->Yield();
  return 0;
}

extern "C" void WINAPI *LocalFree(void *addr)
{
//  printf("LocalFree(%p);\n", addr);
  free(addr);
  mt->Yield();
  return 0;
}

extern "C" int WINAPI MultiByteToWideChar(UINT cp, DWORD, LPCSTR st, int, LPWSTR d, int)
{
  for (int i=0; i<=strlen(st); i++)
    d[i]=(cp<<8)|st[i];
  mt->Yield();
  return i;
}

extern "C" DWORD WINAPI WaitForSingleObject(HANDLE hnd, DWORD)
{
//  printf("BASIC: WaitForSingleObject(%p, %d);\n", hnd, ms);
  if (!hnd) return WAIT_FAILED;
  event_s *ev=(event_s *)hnd;

  int y=0;
  while (!ev->state)
  {
//    printf("%p waits  for  %p.\n", (&hnd)[-1], hnd);
    mt->Yield();
/*    if (kbhit())
    {
      printf("**ABORTED BY HACKER**\n");
      exit(0);
    } */
  }
//  printf("...done (%p)!\n", hnd);
  if (ev->autoreset) ev->state=0;
  mt->Yield();
  return WAIT_OBJECT_0;
}

extern "C" HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES, DWORD stack, LPTHREAD_START_ROUTINE stadr, LPVOID p, DWORD, LPDWORD tid)
{
//  printf("CreateThread(..., %d, %p, %08x, %d, %p)=", stack, stadr, p, fl, tid);
  *tid=mt->CNew(stadr, p, 0, stack?stack:4096, 1);
//  printf("%d;\n", *tid);
  mt->Yield();
  return (void*)-31337;
}

extern "C" void WINAPI *GlobalFree(void *addr)
{
//  printf("GlobalFree(%p);\n", addr);
  free(addr);
  mt->Yield();
  return 0;
}

extern "C" BOOL WINAPI CloseHandle(HANDLE h)
{
//  printf("CloseHandle(%p);\n", h);
  if (!h) return 0;
  delete h;
  mt->Yield();
  return 0;
}

extern "C" LRESULT WINAPI DefDriverProc(DWORD, HDRVR, UINT, LPARAM, LPARAM) // (DWORD did, HDRVR hdrvr, UINT msg, LPARAM lparam1, LPARAM lparam2)
{
//  printf("DefDriverProc(%d, %p, %d, %d, %d);\n", did, hdrvr, msg, lparam1, lparam2);
  mt->Yield();
  return 0;
}

extern "C" HMODULE WINAPI GetDriverModuleHandle(HDRVR hdrvr)
{
  mt->Yield();
  return hdrvr;
}
