// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Player devices system
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -changed INI reading of driver symbols to _dllinfo lookup

#define NO_PLRBASE_IMPORT

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pmain.h"
#include "pfilesel.h"
#include "imsdev.h"
#include "psetting.h"
#include "plinkman.h"
#include "err.h"
#include "pmain.h"
#include "devigen.h"
#include "player.h"

int (*plrProcessKey)(unsigned short);

devinfonode *plPlayerDevices;
int plrBufSize;
static devinfonode *curplaydev;
static devinfonode *defplaydev;

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
    plrProcessKey=0;
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
      plrProcessKey=dev->addprocs->ProcessKey;
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

void plrSetDevice(const char *name, int def)
{
  setdevice(curplaydev, getdevstr(plPlayerDevices, name));
  if (def)
    defplaydev=curplaydev;
}

void plrResetDevice()
{
  setdevice(curplaydev, defplaydev);
}

static int playdevinit()
{
  if (!strlen(cfGetProfileString2(cfSoundSec, "sound", "playerdevices", "")))
    return errOk;
  printf("playerdevices:\n");
  if (!deviReadDevices(cfGetProfileString2(cfSoundSec, "sound", "playerdevices", ""), &plPlayerDevices))
  {
    printf("could not install player devices!\n");
    return errGen;
  }

  curplaydev=0;
  defplaydev=0;

  const char *def=cfGetProfileString("commandline_s", "p", cfGetProfileString2(cfSoundSec, "sound", "defplayer", ""));

  if (strlen(def))
    plrSetDevice(def, 1);
  else
    if (plPlayerDevices)
      plrSetDevice(plPlayerDevices->handle, 1);

  printf("\n");

  plrBufSize=cfGetProfileInt2(cfSoundSec, "sound", "plrbufsize", 100, 10)*65;
  return errOk;
}

static void playdevclose()
{
  setdevice(curplaydev, 0);
  while (plPlayerDevices)
  {
    devinfonode *o=plPlayerDevices;
    plPlayerDevices=plPlayerDevices->next;
    delete o;
  }
}

static int plrReadDir(modlist &ml, unsigned short dirref, const char *mask, unsigned long opt)
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
    for (dev=plPlayerDevices; dev; dev=dev->next)
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
        if (mdbGetModuleType(m.fileref)!=mtDEVp)
        {
          moduleinfostruct mi;
          mdbGetModuleInfo(mi, m.fileref);
          mi.flags1|=MDB_VIRTUAL;
          mi.channels=dev->dev.chan;
          strcpy(mi.modname, dev->name);
          mi.modtype=mtDEVp;
          mdbWriteModuleInfo(m.fileref, mi);
        }
        if (!mdbAppend(ml, m))
          return 0;
      }
    }
  }

  return 1;
}

static int plrSet(const char *path, moduleinfostruct &, binfile *)
{
  char name[_MAX_FNAME];
  _splitpath(path, 0, 0, name, 0);
  plrSetDevice(name, 1);
  //delay(1000); do we really need this ??? (doj)
  return 0;
}

static void plrPrep(const char *, moduleinfostruct &, binfile *&)
{
  plrResetDevice();
}

extern "C"
{
  mdbreaddirregstruct plrReadDirReg = {plrReadDir};
  initcloseregstruct plrDevReg={playdevinit, playdevclose};
  interfacestruct plrIntr = {plrSet, 0, 0};
  preprocregstruct plrPreprocess = {plrPrep};
  char *dllinfo = "readdirs _plrReadDirReg; initclose _plrDevReg; interface _plrIntr; preprocess _plrPreprocess";
};