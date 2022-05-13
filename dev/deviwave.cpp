// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Wavetable devices system
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -changed INI reading of driver symbols to _dllinfo lookup

#define NO_MCPBASE_IMPORT

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pfilesel.h"
#include "imsdev.h"
#include "player.h"
#include "psetting.h"
#include "plinkman.h"
#include "err.h"
#include "pmain.h"
#include "devigen.h"
#include "mcp.h"

int (*mcpProcessKey)(unsigned short);

devinfonode *plWaveTableDevices;
static devinfonode *curwavedev;
static devinfonode *defwavedev;

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

static void setdevice(devinfonode *&curdev, devinfonode *dev)
{
  if (curdev==dev)
    return;
  if (curdev)
  {
    if (curdev->addprocs&&curdev->addprocs->Close)
      curdev->addprocs->Close();
    mcpProcessKey=0;
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
    if (!dsym || !*dsym)
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
      mcpProcessKey=dev->addprocs->ProcessKey;
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


void mcpSetDevice(const char *name, int def)
{
  setdevice(curwavedev, getdevstr(plWaveTableDevices, name));
  if (def)
    defwavedev=curwavedev;
}

void mcpResetDevice()
{
  setdevice(curwavedev, defwavedev);
}

static int wavedevinit()
{
  if (!strlen(cfGetProfileString2(cfSoundSec, "sound", "wavetabledevices", "")))
    return errOk;
  printf("wavetabledevices:\n");
  if (!deviReadDevices(cfGetProfileString2(cfSoundSec, "sound", "wavetabledevices", ""), &plWaveTableDevices))
  {
    printf("could not install wavetable devices!\n");
    return errGen;
  }

  curwavedev=0;
  defwavedev=0;

  const char *def=cfGetProfileString("commandline_s", "w", cfGetProfileString2(cfSoundSec, "sound", "defwavetable", ""));

  if (strlen(def))
    mcpSetDevice(def, 1);
  else
    if (plWaveTableDevices)
      mcpSetDevice(plWaveTableDevices->handle, 1);

  printf("\n");

  int playrate=cfGetProfileInt("commandline_s", "r", cfGetProfileInt2(cfSoundSec, "sound", "mixrate", 44100, 10), 10);
  if (playrate<66)
    if (playrate%11)
      playrate*=1000;
    else
      playrate=playrate*11025/11;

  mcpMixOpt=0;
  if (!cfGetProfileBool("commandline_s", "8", !cfGetProfileBool2(cfSoundSec, "sound", "mix16bit", 1, 1), 1))
    mcpMixOpt|=PLR_16BIT;
  if (!cfGetProfileBool("commandline_s", "m", !cfGetProfileBool2(cfSoundSec, "sound", "mixstereo", 1, 1), 1))
    mcpMixOpt|=PLR_STEREO;
  mcpMixMaxRate=playrate;
  mcpMixProcRate=cfGetProfileInt2(cfSoundSec, "sound", "mixprocrate", 1536000, 10);
  mcpMixBufSize=cfGetProfileInt2(cfSoundSec, "sound", "mixbufsize", 100, 10)*65;
  mcpMixPoll=mcpMixBufSize;
  mcpMixMax=mcpMixBufSize;

  return errOk;
}

static void wavedevclose()
{
  setdevice(curwavedev, 0);
  while (plWaveTableDevices)
  {
    devinfonode *o=plWaveTableDevices;
    plWaveTableDevices=plWaveTableDevices->next;
    delete o;
  }
}

static int mcpReadDir(modlist &ml, unsigned short dirref, const char *mask, unsigned long opt)
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
    for (dev=plWaveTableDevices; dev; dev=dev->next)
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
        if (mdbGetModuleType(m.fileref)!=mtDEVw)
        {
          moduleinfostruct mi;
          mdbGetModuleInfo(mi, m.fileref);
          mi.flags1|=MDB_VIRTUAL;
          mi.channels=dev->dev.chan;
          strcpy(mi.modname, dev->name);
          mi.modtype=mtDEVw;
          mdbWriteModuleInfo(m.fileref, mi);
        }
        if (!mdbAppend(ml, m))
          return 0;
      }
    }
  }

  return 1;
}

static int mcpSetDev(const char *path, moduleinfostruct &, binfile *)
{
  char name[_MAX_FNAME];
  _splitpath(path, 0, 0, name, 0);
  mcpSetDevice(name, 1);
  //delay(1000); do we really need this ??? (doj)
  return 0;
}

static void mcpPrep(const char *, moduleinfostruct &info, binfile *&)
{
  mcpResetDevice();
  if (info.flags1&MDB_BIGMODULE)
    mcpSetDevice(cfGetProfileString2(cfSoundSec, "sound", "bigmodules", ""), 0);
}

extern "C"
{
  mdbreaddirregstruct mcpReadDirReg = {mcpReadDir};
  initcloseregstruct mcpDevReg={wavedevinit, wavedevclose};
  interfacestruct mcpIntr = {mcpSetDev, 0, 0};
  preprocregstruct mcpPreprocess = {mcpPrep};
  char *dllinfo = "readdirs _mcpReadDirReg; initclose _mcpDevReg; interface _mcpIntr; preprocess _mcpPreprocess";
};
