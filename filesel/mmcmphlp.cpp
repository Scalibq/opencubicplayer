// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Routines for enabling/disabling ziRCONia's MMCMP module compressor
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs
//    -added dllinfo record

#include "pmain.h"
#include "pfilesel.h"
#include "err.h"

static int mmcmp=0;

int check();
#pragma aux check value [eax] modify [eax edx] = "mov ax,4370h" "int 21h" "jc fail" "cmp eax,4352697ah" "jne fail" "mov eax,edx" "jmp ok" "fail:" "xor eax,eax" "ok:"

void endis(int);
#pragma aux endis parm [eax] = "int 21h"

static int init()
{
  int supp=check();
  if ((supp&7)==7)
  {
    mmcmp=1;
    endis(0x4372);
  }

  return errOk;
}

static void close()
{
  if (mmcmp)
    endis(0x4371);
}

void enable(char *, binfile *&)
{
  if (mmcmp)
    endis(0x4371);
}

void disable(char *, binfile *&)
{
  if (mmcmp)
    endis(0x4372);
}

extern "C"
{
  initcloseregstruct mmcmpReg = {init, close};
  fsgetfileregstruct mmcmpEnable = {enable};
  fsgetfileregstruct mmcmpDisable = {disable};

  char *dllinfo="preinitclose _mmcmpReg; pregetfile _mmcmpDisable; postgetfile _mmcmpEnable";

};