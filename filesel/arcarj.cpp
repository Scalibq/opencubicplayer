// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Archive handler for ARJ archives
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <kb@nwn.de>
//    -added _dllinfo record
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile

#include <io.h>
#include <stdlib.h>
#include <string.h>
#include "binfstd.h"
#include "pfilesel.h"
#include "psetting.h"

static int adbARJScan(const char *path)
{
  char ext[_MAX_EXT];
  char name[_MAX_FNAME];
  char arcname[12];

  _splitpath(path, 0, 0, name, ext);
  fsConvFileName12(arcname, name, ext);

  sbinfile file;
  if (file.open(path, sbinfile::openro))
    return 1;
  arcentry a;
  memcpy(a.name, arcname, 12);
  a.size=file.length();
  a.flags=ADB_ARC;
  if (!adbAdd(a))
  {
    file.close();
    return 0;
  }
  unsigned short arcref=adbFind(arcname);

  unsigned short sig=file.getus();
  if (sig!=0xEA60)
  {
    file.close();
    return 0;
  }
  sig=file.gets();
  file.seekcur(sig+4);
  sig=file.gets();
  file.seekcur(sig);

  while (1)
  {
    sig=file.getus();
    if (sig!=0xEA60)
    {
      file.close();
      return 0;
    }
    sig=file.gets();
    if (!sig)
      break;
    file.getl();
    unsigned long flags=file.getl();
    file.getl();
    long csize, osize;
    csize=file.getl();
    osize=file.getl();
    file.seekcur(10+((flags&8)?4:0));
    char filename[13];
    char fnpos=0;
    char valid=1;
    while (1)
    {
      unsigned char c=file.getc();
      filename[fnpos++]=c;
      if (!c)
        break;
      if ((c=='/')||(c=='\\'))
      {
        fnpos=0;
        valid=1;
      }
      if (fnpos==13)
      {
	valid=0;
        fnpos--;
      }
    }
    while (file.getc());
    file.getl();
    sig=file.gets();
    file.seekcur(sig);

    if (valid&&!(flags&8)&&!(flags&0xFF0000))
    {
      strupr(filename);
      _splitpath(filename, 0, 0, name, ext);
      if (fsIsModule(ext))
      {
        a.size=osize;
        a.parent=arcref;
        a.flags=0;
        fsConvFileName12(a.name, name, ext);
        if (!adbAdd(a))
        {
          file.close();
          return 0;
        }
      }
    }
    file.seekcur(csize);
  }
  file.close();
  return 1;
}

static int adbARJCall(int act, const char *apath, const char *file, const char *dpath)
{
  switch (act)
  {
  case adbCallGet:
    return !adbCallArc(cfGetProfileString("arcARJ", "get", "arj e %a %d %n"), apath, file, dpath);
  case adbCallPut:
    return !adbCallArc(cfGetProfileString("arcARJ", "put", "arj a %a %n"), apath, file, dpath);
  case adbCallDelete:
    return !adbCallArc(cfGetProfileString("arcARJ", "delete", "arj d %a %n"), apath, file, dpath);
  case adbCallMoveTo:
    if (cfGetProfileString("arcARJ", "moveto", 0))
      return !adbCallArc(cfGetProfileString("arcARJ", "moveto", "arj m %a %n"), apath, file, dpath);
    if (!adbARJCall(adbCallPut, apath, file, dpath))
      return 0;
    unlink(file);
    return 1;
  case adbCallMoveFrom:
    if (cfGetProfileString("arcARJ", "movefrom", 0))
      return !adbCallArc(cfGetProfileString("arcARJ", "movefrom", ""), apath, file, dpath);
    if (!adbARJCall(adbCallGet, apath, file, dpath))
      return 0;
    return adbARJCall(adbCallDelete, apath, file, dpath);
  }
  return 0;
}

extern "C"
{
  adbregstruct adbARJReg = {".ARJ", adbARJScan, adbARJCall};
  char *dllinfo = "arcs _adbARJReg";
};