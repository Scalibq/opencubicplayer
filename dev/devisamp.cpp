// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Sampler devices system
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -changed INI reading of driver symbols to _dllinfo lookup

#define NO_SMPBASE_IMPORT

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pfilesel.h"
#include "imsdev.h"
#include "psetting.h"
#include "plinkman.h"
#include "err.h"
#include "pmain.h"
#include "devigen.h"
#include "sampler.h"

int (*smpProcessKey)(unsigned short);

unsigned char plsmpOpt;
unsigned short plsmpRate;

devinfonode *plSamplerDevices;
static devinfonode *cursampdev;
static devinfonode *defsampdev;

static devinfonode *getdevstr(devinfonode *n, const char *hnd)
{
  while (n)
  {
    if (!stricmp(n->handle, hnd))
      return n;
    n=n->next;
  }
  return 0;
}

/*
static void setdevice(devinfonode *&curdev, devinfonode *dev)
{
  if (curdev==dev)
    return;
  if (curdev)
  {
    if (curdev->addprocs&&curdev->addprocs->Close)
      curdev->addprocs->Close();
    smpProcessKey=0;
    curdev->dev.dev->Close();
  }
  curdev=0;
  if (!dev)
    return;
  printf("%s selected...\n", dev->dev.dev->name);
  if (dev->dev.dev->Init(dev->dev))
  {
    if (dev->addprocs&&dev->addprocs->Init)
      dev->addprocs->Init(dev->handle);
    if (dev->addprocs&&dev->addprocs->ProcessKey)
      smpProcessKey=dev->addprocs->ProcessKey;
    curdev=dev;
    return;
  }
  printf("device init error\n");
}
*/
static void setdevice(devinfonode *&curdev, devinfonode *dev)
{
  if (curdev==dev)
    return;
  if (curdev)
  {
    if (curdev->addprocs&&curdev->addprocs->Close)
      curdev->addprocs->Close();
    smpProcessKey=0;
    curdev->dev.dev->Close();
    if (!curdev->keep)
    {
      lnkFree(curdev->linkhand);
      curdev->linkhand=-1;
    }
  }
  curdev=0;
  if (!dev)
    return;
  if (dev->linkhand<0)
  {
    char lname[12];
    strncpy(lname,cfGetProfileString(dev->handle, "link", ""),11);
    dev->linkhand=lnkLink(lname);
    if (dev->linkhand<0)
    {
      printf("device load error\n");
      return;
    }
    dev->dev.dev=(sounddevice*)lnkGetSymbol(lnkReadInfoReg(lname,"driver"));
    if (!dev->dev.dev)
    {
      printf("device symbol error\n");
      lnkFree(dev->linkhand);
      dev->linkhand=-1;
      return;
    }
    char *dsym=lnkReadInfoReg(lname,"addprocs");
    if (*dsym)
      dev->addprocs=0;
    else
      dev->addprocs=(devaddstruct*)lnkGetSymbol(dsym);
  }

  printf("%s selected...\n", dev->name);
  if (dev->dev.dev->Init(dev->dev))
  {
    if (dev->addprocs&&dev->addprocs->Init)
      dev->addprocs->Init(dev->handle);
    if (dev->addprocs&&dev->addprocs->ProcessKey)
      smpProcessKey=dev->addprocs->ProcessKey;
    curdev=dev;
    return;
  }
  if (!curdev->keep)
  {
    lnkFree(curdev->linkhand);
    curdev->linkhand=-1;
  }
  printf("device init error\n");
}


void smpSetDevice(const char *name, int def)
{
  setdevice(cursampdev, getdevstr(plSamplerDevices, name));
  if (def)
    defsampdev=cursampdev;
}

void smpResetDevice()
{
  setdevice(cursampdev, defsampdev);
}

static int sampdevinit()
{
  if (!strlen(cfGetProfileString2(cfSoundSec, "sound", "samplerdevices", "")))
    return errOk;
  printf("samplerdevices:\n");
  if (!deviReadDevices(cfGetProfileString2(cfSoundSec, "sound", "samplerdevices", ""), &plSamplerDevices))
  {
    printf("could not install sampler devices!\n");
    return errGen;
  }

  cursampdev=0;
  defsampdev=0;

  const char *def=cfGetProfileString("commandline_s", "s", cfGetProfileString2(cfSoundSec, "sound", "defsampler", ""));

  if (strlen(def))
    smpSetDevice(def, 1);
  else
    if (plSamplerDevices)
      smpSetDevice(plSamplerDevices->handle, 1);

  printf("\n");

  smpBufSize=cfGetProfileInt2(cfSoundSec, "sound", "smpbufsize", 100, 10)*65;

  int playrate=cfGetProfileInt2(cfSoundSec, "sound", "samprate", 44100, 10);
  playrate=cfGetProfileInt("commandline_s", "r", playrate, 10);
  if (playrate<65)
    if (playrate%11)
      playrate*=1000;
    else
      playrate=playrate*11025/11;

  int playopt=0;
  if (!cfGetProfileBool("commandline_s", "8", !cfGetProfileBool2(cfSoundSec, "sound", "samp16bit", 1, 1), 1))
    playopt|=SMP_16BIT;
  if (!cfGetProfileBool("commandline_s", "m", !cfGetProfileBool2(cfSoundSec, "sound", "sampstereo", 1, 1), 1))
    playopt|=SMP_STEREO;
  plsmpOpt=playopt;
  plsmpRate=playrate;

  return errOk;
}

static void sampdevclose()
{
  setdevice(cursampdev, 0);

  while (plSamplerDevices)
  {
    devinfonode *o=plSamplerDevices;
    plSamplerDevices=plSamplerDevices->next;
    delete o;
  }
}

static int smpReadDir(modlist &ml, unsigned short dirref, const char *mask, unsigned long opt)
{
  char path[_MAX_PATH];

  modlistentry m;

  if (opt&RD_PUTDSUBS)
  {
    fsConvFileName12(m.name, "@:", "");
    m.fileref=0xFFFF;
    m.dirref=dmGetDriveDir(0);
    if (!mdbAppendNew(ml, m))
      return 0;
  }

  if (opt&RD_PUTSUBS)
  {
    dmGetPath(path, dirref);
    if (!strcmp(path, "@:\\"))
    {
      fsConvFileName12(m.name, "DEVICES", "");
      dmGetPath(path, dirref);
      strcat(path, "DEVICES");
      m.dirref=dmGetPathReference(path);
      if (m.dirref==0xFFFF)
        return 0;
      m.fileref=0xFFFE;
      if (!mdbAppendNew(ml, m))
        return 0;
    }
  }

  dmGetPath(path, dirref);
  if (!strcmp(path, "@:\\DEVICES\\"))
  {
    devinfonode *dev;
    for (dev=plSamplerDevices; dev; dev=dev->next)
    {
      char hnd[9];
      strcpy(hnd, dev->handle);
      strupr(hnd);
      fsConvFileName12(m.name, hnd, ".DEV");
      if (fsMatchFileName12(m.name, mask))
      {
        m.dirref=dirref;
        m.fileref=mdbGetModuleReference(m.name, dev->dev.mem);
        if (m.fileref==0xFFFF)
          return 0;
        if (mdbGetModuleType(m.fileref)!=mtDEVs)
        {
          moduleinfostruct mi;
          mdbGetModuleInfo(mi, m.fileref);
          mi.flags1|=MDB_VIRTUAL;
          mi.channels=dev->dev.chan;
          strcpy(mi.modname, dev->name);
          mi.modtype=mtDEVs;
          mdbWriteModuleInfo(m.fileref, mi);
        }
        if (!mdbAppend(ml, m))
          return 0;
      }
    }
  }

  return 1;
}

static int smpSet(const char *path, moduleinfostruct &, binfile *)
{
  char name[_MAX_FNAME];
  _splitpath(path, 0, 0, name, 0);
  smpSetDevice(name, 1);
  //delay(1000); do we really need this ??? (doj)
  return 0;
}

static void smpPrep(const char *, moduleinfostruct &, binfile *&)
{
  smpResetDevice();
}

extern "C"
{
  initcloseregstruct smpDevReg={sampdevinit, sampdevclose};
  mdbreaddirregstruct smpReadDirReg = {smpReadDir};
  interfacestruct smpIntr = {smpSet, 0, 0};
  preprocregstruct smpPreprocess = {smpPrep};
  char *dllinfo = "readdirs _smpReadDirReg; initclose _smpDevReg; interface _smpIntr; preprocess _smpPreprocess";
};