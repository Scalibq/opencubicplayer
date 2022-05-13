// OpenCP Module Player
// copyright (c) '94-'99 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// "Archive" reader for Unreal Music Resource files
//
// revision history: (please note changes here)
//  -kb980717   Tammo Hinrichs <kb@nwn.de>
//    -first release
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile
//  -ryg981216  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -fixed the mysterious BINFILE BUG! :) (thanks to n.b. for making
//     binfile not backwards-compatible)
//  -fd991211   Felix Domke    <tmbinc@gmx.net>
//    -enabled playing of unreal mission disk-songs and kran32.it by
//     adding a new hack.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "binfstd.h"
#include "pfilesel.h"
#include "psetting.h"

#pragma pack (push, 1)

struct umxheaderstruct
{
  unsigned long tag;
  unsigned long version;    // either 0x3d (unreal) or 0x3f (some unreal data disk)
  unsigned long d2;
  unsigned long d3;
  unsigned long regoffset;
  unsigned long d4;
  unsigned long offs1;
  unsigned long d5;
  unsigned long offs2;
  unsigned long d6;
  unsigned long strangeoffset;
} aheader;

#pragma pack (pop)

static char mtype[5];
static char mname[9];

static char strbuffer[100];
static unsigned long regtag;

static volatile char regend;

static unsigned char mdbScanBuf[1084];

static int readRegStr(binfile &f)
{
  int p=0;
  char c;
  while ( p<100 && ((c=f.getc())>1) )
    strbuffer[p++]=c;

  if (!p && !c)
  {
    strbuffer[p++]=c=1;
    f.seekcur(-1);
  }

  if (p==100)
    return 0;
  if (c)
    strbuffer[p++]=c;
  strbuffer[p]=0;
  if (!c)
    regtag=f.getul();
  else
    regend=1;
  return 1;
}


static int readUmxReg(binfile &f)
{
  int state=0;
  regend=0;

  while (readRegStr(f) && !regend)
  {
    if (regtag==0x70010)
    {
      switch (state)
      {
        case 0:
          strncpy(mtype+1,strbuffer,3);
          mtype[0]='.';
          break;
        case 1:
          if (strcmp(strbuffer,"Music"))
            return 0;
          break;
        case 2:
          strncpy(mname,strbuffer,8);
          break;
      }
      state++;
    }
  }

  return 1;
}





static int adbUMXScan(const char *path)   // adds the contents of
                                          // path (an archive) via adbAdd();
                                          // (including the archive itself)
                                          // returns 0 for ok, 1 for error.
{
  sbinfile archive;
  if(archive.open(path, sbinfile::openro))
   return 1;

  unsigned short arcref;                 // whatever..

  char arcname[12];
  char ext[_MAX_EXT];
  char name[_MAX_FNAME];

  _splitpath(path, 0, 0, name, ext);
  fsConvFileName12(arcname, name, ext);

  archive.read(&aheader,sizeof(aheader));
  if (aheader.tag != 0x9e2a83c1)
  {
    archive.close();
    return 0;
  }

  arcentry a;
  memcpy(a.name, arcname, 12);
  a.size=archive.length();
  a.flags=ADB_ARC;
  if (!adbAdd(a))
  {
    archive.close();
    return 0;
  }
  arcref=adbFind(arcname);

  archive.seek(aheader.regoffset);
  if (readUmxReg(archive))
  {
    a.size=aheader.offs1-archive.tell();
    archive.seekcur(a.size>=1048576?5:4);
    archive.seekcur(aheader.version>0x3d?4:0);
    strupr(mname);
    strupr(mtype);
    if(fsIsModule(mtype))
    {
      a.parent=arcref;
      a.flags=0;
      fsConvFileName12(a.name, mname, mtype);
      if(!adbAdd(a))
      {
        archive.close();
        return 0;
      }
      if (fsScanInArc)
      {
        unsigned short fileref;
        fileref=mdbGetModuleReference(a.name, a.size);
        if (fileref==0xFFFF)
        {
          archive.close();
          return 0;
        }
        if (!mdbInfoRead(fileref))
        {
          archive.read(mdbScanBuf, (a.size>1084)?1084:a.size);

          moduleinfostruct mi;
          if (mdbGetModuleInfo(mi, fileref))
          {
            mdbReadMemInfo(mi, mdbScanBuf, 1084);
            mdbWriteModuleInfo(fileref, mi);
          }
        }
      }
    }
  }
  archive.close();
  return 1;
}



static int adbUMXCall(int act,
                      const char *apath,
                      const char *file,
                      const char *dpath)
{
 sbinfile afile;
 sbinfile dfile;
 char target[_MAX_PATH];
 unsigned long fsize;
 char *copybuf;

 switch (act)
 {
  case adbCallGet:
    printf("UMX: extracting file %s...\n", file);
    _makepath(target, 0, dpath, file, 0);
    if(afile.open(apath, sbinfile::openro))
      return 0;
    afile.read(&aheader,sizeof(aheader));
    if (aheader.tag != 0x9e2a83c1)
    {
      printf("ERROR: invalid archive");
      afile.close();
      return 0;
    }
    afile.seek(aheader.regoffset);
    if (!readUmxReg(afile))
    {
      printf("ERROR: could not read registry");
      afile.close();
      return 0;
    }

    fsize=aheader.offs1-afile.tell();
    afile.seekcur(fsize>=1048576?5:4);
    afile.seekcur(aheader.version>0x3d?4:0);
    copybuf = new char[fsize];

    if (!copybuf)
    {
      printf("ERROR: not enough memory");
      afile.close();
      return 0;
    }

    afile.read(copybuf,fsize);
    afile.close();

    if ( dfile.open(target, sbinfile::opencr|sbinfile::openrw))
    {
      printf("ERROR: couldn't create output file");
      delete copybuf;
      return (0);
    }

    dfile.write(copybuf, fsize);
    dfile.close();

    delete copybuf;

    return 1;
  case adbCallPut:
    // not implemented
    break;
  case adbCallDelete:
    // not implemented
    break;
  case adbCallMoveTo:
    // not implemented
    break;
  case adbCallMoveFrom:
    // not implemented
    break;
  }
  return 0;
}

extern "C"
{
  adbregstruct adbUMXReg = {".UMX", adbUMXScan, adbUMXCall};
  char *dllinfo = "arcs _adbUMXReg";
};
