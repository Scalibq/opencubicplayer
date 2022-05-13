// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Interface routines for Line/Mic input player
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "pfilesel.h"
#include "poutput.h"
#include "sampler.h"
#include "psetting.h"
#include "imsdev.h"
#include "devigen.h"
#include "devisamp.h"
#include "cpiface.h"

static char cdpMode;

static void cdaDrawGStrings(short (*buf)[132])
{
  if (plScrWidth==80)
  {
    writestring(buf[0], 0, 0x09, "  mode: ..........  ", 80);
    writestring(buf[0], 8, 0x0F, cdpMode?"microphone":"line-in", 10);
    writestring(buf[1], 0, 0, "", 80);
    writestring(buf[2], 0, 0, "", 80);
  }
  else
  {
    writestring(buf[0], 0, 0x09, "    mode: ..........  ", 132);
    writestring(buf[0], 10, 0x0F, cdpMode?"microphone":"line-in", 10);
    writestring(buf[1], 0, 0, "", 132);
    writestring(buf[2], 0, 0, "", 132);
  }
}




static void cdaCloseFile()
{
  smpCloseSampler();
}




static int inpProcessKey(unsigned short key)
{
  if (smpProcessKey)
  {
    int ret=smpProcessKey(key);
    if (ret==2)
      cpiResetScreen();
    if (ret)
      return 1;
  }
  return 0;
}





static int inpOpenFile(const char *path, moduleinfostruct &, binfile *)
{
  char name[_MAX_FNAME];
  char ext[_MAX_FNAME];

  _splitpath(path, 0, 0, name, ext);

  if (stricmp(name, "DEFAULT"))
    smpSetDevice(name, 0);

  if (!smpSample)
    return -1;

  cdpMode=strcmp(ext, ".MIC")?1:0;

  smpSetSource(cdpMode?SMP_LINEIN:SMP_MIC);

  plDrawGStrings=cdaDrawGStrings;
  plGetMasterSample=smpGetMasterSample;
  plGetRealMasterVolume=smpGetRealMasterVolume;
  plProcessKey=inpProcessKey;

  smpSetOptions(plsmpRate, plsmpOpt);

  void *buf;
  int len;
  if (!smpOpenSampler(buf, len, smpBufSize))
    return -1;

  return 0;
}

static int inpReadDir(modlist &ml, unsigned short dirref, const char *mask, unsigned long opt)
{
  if (!plSamplerDevices)
    return 1;

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
      fsConvFileName12(m.name, "INPUTS", "");
      dmGetPath(path, dirref);
      strcat(path, "INPUTS");
      m.dirref=dmGetPathReference(path);
      if (m.dirref==0xFFFF)
        return 0;
      m.fileref=0xFFFE;
      if (!mdbAppend(ml, m))
        return 0;
    }
  }

  dmGetPath(path, dirref);
  if (!strcmp(path, "@:\\INPUTS\\"))
  {
    devinfonode *dev;
    for (dev=plSamplerDevices; dev; dev=dev->next)
    {
      char hnd[9];
      strcpy(hnd, dev->handle);
      strupr(hnd);
      fsConvFileName12(m.name, hnd, ".LIN");
      if (fsMatchFileName12(m.name, mask))
      {
        m.dirref=dirref;
        m.fileref=mdbGetModuleReference(m.name, dev->ihandle);
        if (m.fileref==0xFFFF)
          return 0;
        if (mdbGetModuleType(m.fileref)!=mtINP)
        {
          moduleinfostruct mi;
          mdbGetModuleInfo(mi, m.fileref);
          mi.flags1|=MDB_VIRTUAL;
          mi.channels=2;
          strcpy(mi.modname, dev->dev.dev->name);
          mi.modtype=mtINP;
          mdbWriteModuleInfo(m.fileref, mi);
        }
        if (!mdbAppend(ml, m))
          return 0;
      }
      strcpy(hnd, dev->handle);
      strupr(hnd);
      fsConvFileName12(m.name, hnd, ".MIC");
      if (fsMatchFileName12(m.name, mask))
      {
        m.dirref=dirref;
        m.fileref=mdbGetModuleReference(m.name, dev->ihandle);
        if (m.fileref==0xFFFF)
          return 0;
        if (mdbGetModuleType(m.fileref)!=mtINP)
        {
          moduleinfostruct mi;
          mdbGetModuleInfo(mi, m.fileref);
          mi.flags1|=MDB_VIRTUAL;
          mi.channels=1;
          strcpy(mi.modname, dev->dev.dev->name);
          mi.modtype=mtINP;
          mdbWriteModuleInfo(m.fileref, mi);
        }
        if (!mdbAppend(ml, m))
          return 0;
      }
    }

    fsConvFileName12(m.name, "DEFAULT", ".LIN");
    if (fsMatchFileName12(m.name, mask))
    {
      m.dirref=dirref;
      m.fileref=mdbGetModuleReference(m.name, 0);
      if (m.fileref==0xFFFF)
        return 0;
      if (mdbGetModuleType(m.fileref)!=mtINP)
      {
        moduleinfostruct mi;
        mdbGetModuleInfo(mi, m.fileref);
        mi.flags1|=MDB_VIRTUAL;
        mi.channels=2;
        strcpy(mi.modname, "Default Line-In");
        mi.modtype=mtINP;
        mdbWriteModuleInfo(m.fileref, mi);
      }
      if (!mdbAppend(ml, m))
        return 0;
    }
    fsConvFileName12(m.name, "DEFAULT", ".MIC");
    if (fsMatchFileName12(m.name, mask))
    {
      m.dirref=dirref;
      m.fileref=mdbGetModuleReference(m.name, 0);
      if (m.fileref==0xFFFF)
        return 0;
      if (mdbGetModuleType(m.fileref)!=mtINP)
      {
        moduleinfostruct mi;
        mdbGetModuleInfo(mi, m.fileref);
        mi.flags1|=MDB_VIRTUAL;
        mi.channels=1;
        strcpy(mi.modname, "Default Microphone");
        mi.modtype=mtINP;
        mdbWriteModuleInfo(m.fileref, mi);
      }
      if (!mdbAppend(ml, m))
        return 0;
    }
  }

  return 1;
}

extern "C"
{
  mdbreaddirregstruct inpReadDirReg = {inpReadDir};
  cpifaceplayerstruct inpPlayer = {inpOpenFile, cdaCloseFile};
  char *dllinfo="player _inpPlayer; readdirs _inpReadDirReg";
};
