// OpenCP Module Player
// copyright (c) '94-'99 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Archive handler for PAQ archives
//
// revision history: (please note changes here)
//  -fd990519   Felix Domke    <tmbinc@gmx.net>
//    -first release

// the "extract to path" is a pure hack, i know, but it's like this because
// the paq.exe doesn't support an other destination directory.
// but i don't want to complain, because i know that paq isn't supposed
// to be suppported somewhere else than in sds/ids-demos.
// i hope it's ok this way.

#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <dos.h>
#include "binfstd.h"
#include "pfilesel.h"
#include "psetting.h"

#pragma pack (push, 1)

struct paqhdr
{
  char tag[4];
  int files;
};

struct pfentry
{
  char  name[13];
  unsigned char flags;
  unsigned long pos;
  unsigned long len;
  unsigned long ulen;
};

#pragma pack (pop)

static int adbPAQScan(const char *path)
{
  sbinfile archive;
  if (archive.open(path, sbinfile::openro))
    return 1;

  paqhdr hdr;
  if ((!archive.eread(&hdr, sizeof(paqhdr))) || memcmp(hdr.tag, "KPAQ", 4))
  {
    archive.close();
    return 1;
  }

  unsigned short arcref;

  char arcname[12];
  char ext[_MAX_EXT];
  char name[_MAX_FNAME];
  _splitpath(path, 0, 0, name, ext);
  fsConvFileName12(arcname, name, ext);
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


  for (int f=0; f<hdr.files; f++)
  {
    pfentry entry;
    if (!archive.eread(&entry, sizeof(pfentry))) break;
    if (entry.pos>(unsigned)archive.length()) break;
    char filename[_MAX_PATH];
    strcpy(filename, entry.name);
    strupr(filename);
    _splitpath(filename, 0, 0, name, ext);
    if(fsIsModule(ext) || !strcmp(name,"MOD"))
    {
      a.size=entry.ulen;
      a.parent=arcref;
      a.flags=0;
      fsConvFileName12(a.name, name, ext);
      if(!adbAdd(a))
      {
        archive.close();
        return 0;
      }
    }
  }
  archive.close();
  return 1;
}

static int adbPAQCall(int act,
                      const char *apath,
                      const char *file,
                      const char *dpath)
{
  switch (act)
  {
    case adbCallGet:
    {
      char mypath[_MAX_PATH];
      strcpy(mypath, dpath);
      if ((mypath[strlen(mypath)-1]=='\\')||(mypath[strlen(mypath)-1]=='/'))
        mypath[strlen(mypath)-1]=0;

      char myfile[_MAX_PATH];
      strcpy(myfile, file);
      char oldpath[_MAX_PATH];
      unsigned olddrive, dummy;
      _dos_getdrive(&olddrive);
      getcwd(oldpath, _MAX_PATH);
      _dos_setdrive((*mypath)-64, &dummy);
      chdir(mypath);
      int ret=!adbCallArc(cfGetProfileString("arcPAQ", "get", "paq.exe x %a %n"), apath, myfile, mypath);
      _dos_setdrive(olddrive, &dummy);
      chdir(oldpath);
      return 0;
    }
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
  adbregstruct adbPAQReg = {".PAQ", adbPAQScan, adbPAQCall};
  char *dllinfo = "arcs _adbPAQReg";
};
