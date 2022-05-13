// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Archive handler for LHA archives
//
// revision history: (please note changes here)
//  -kb980717   Felix Domke    <tmbinc@gmx.net>
//    -first release
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile
//    -corrected history ;)
//  -fd990419   Felix Domke    <tmbinc@gmx.net>
//    -fixed some small bug, fixed indention (JA LEUTE, IHR HABT JA ALLE
//     RECHT, ZWEI SPACES EIN-RYG-KEN RULT HALT DOCH MEHR...)

#include <stdlib.h>
#include <string.h>
#include "binfstd.h"
#include "pfilesel.h"
#include "psetting.h"

#pragma pack (push, 1)

struct lhaheaderstruct
{
  char size;
  char checksum;
  char id[3];
  char method;
  char id1;      // -
  long packsize;
  long unpsize;
  long time;
  short attrib;
  char filenamelength;
} aheader;

#pragma pack (pop)

static int adbLHAScan(const char *path)   // adds the contents of
                                          // path (an archive) via adbAdd();
                                          // (including the archive itself)
                                          // returns 0 for ok, 1 for error.
{
  sbinfile archive;
  if (archive.open(path, sbinfile::openro))
    return 1;

  unsigned short arcref;                 // whatever..

  {
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
  }

  // list
  arcentry a;

  while (!archive.eof())
  {
    char ext[_MAX_EXT];
    char name[_MAX_FNAME];

    if (!archive.eread(&aheader, sizeof(lhaheaderstruct))) break;
    if (aheader.id1!='-') break;
    char filename[_MAX_PATH];
    if (!archive.eread(filename, aheader.filenamelength)) break;
    filename[aheader.filenamelength]=0;
    archive.seekcur(aheader.packsize+2);

    strupr(filename);
    _splitpath(filename, 0, 0, name, ext);
    if(fsIsModule(ext) || !strcmp(name,"MOD"))
    {
      a.size=aheader.unpsize;
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

static int adbLHACall(int act,
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
      if ((mypath[strlen(mypath)-1]!='\\')&&(mypath[strlen(mypath)-1]!='/'))
        strcat(mypath, "\\");

      char myfile[_MAX_PATH];
      strcpy(myfile, file);
      strcat(myfile,"*");

      return !adbCallArc(cfGetProfileString("arcLHA", "get", "lha e %a %d %n -a"), apath, myfile, mypath);
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
  adbregstruct adbLHAReg = {".LHA", adbLHAScan, adbLHACall};
  adbregstruct adbLZHReg = {".LZH", adbLHAScan, adbLHACall};
  char *dllinfo = "arcs _adbLHAReg _adbLZHReg";
};
