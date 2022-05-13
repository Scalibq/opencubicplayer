#ifndef __PMAIN_H
#define __PMAIN_H

#ifdef WIN32
#include "w32idata.h"
#endif

struct moduleinfostruct;

struct initcloseregstruct
{
  int (*Init)();
  void (*Close)();
  initcloseregstruct *next;
};

void plDosShell();
int plSystem(const char *);

void conInit();
void conSave();
void conRestore();

#if defined(DOS32) || (defined(WIN32)&&defined(NO_CPDLL_IMPORT))
extern int plScreenChanged;
#else
extern_data int plScreenChanged;
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
HINSTANCE win32GetHInstance();
#endif

extern const char *GetcfSoundSec();
extern const char *GetcfConfigSec();
extern const char *GetcfScreenSec();
extern const char *GetcfDataDir();
extern const char *GetcfCPPath();
extern const char *GetcfConfigDir();
extern const char *GetcfDataDir();
extern const char *GetcfProgramDir();
extern const char *GetcfTempDir();
extern const char *GetcfCommandLine();

#endif