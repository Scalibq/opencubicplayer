// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Archive handler for RAR archives, version 2
//
// revision history: (please note changes here)
//  -kb980717   Felix Domke <tmbinc@gmx.net>
//     -rewritten the old cp2dk ARCRAR.CPP
//     -first release
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile

// some lines of file are based on arcrar.cpp from niklas beisert,
// the rest (nearly everything :) was written by Felix Domke.
// move etc. are not yet supported, because i think they are totally useless
// for a player :).
// implementing them should be no problem.

#include <stdlib.h>
#include <string.h>
#include "binfstd.h"
#include "pfilesel.h"
#include "psetting.h"

struct rarheaderstruct
{
 short crc;
 char type;
 short flags;
 short size;
} *aheader;

struct rarxheaderstruct
{
  short crc;
  char type;
  short flags;
  short size;
  long addsize;
} *axheader;

struct rarfileheaderstruct
{
  short crc;
  char type;            // 0x74
  short flags;          // &0x8000
  short size;
  long packsize;
  long unpsize;
  char hostos;
  long filecrc;
  long ftime;
  char unpver;
  char method;
  short filenamesize;
  long attributes;
  char filename[];
} *afheader;

char *amem;
long nexthpos;
static int ReadNextHeader(binfile &);


static int adbRARScan(const char *path)   // adds the contents of
                                          // path (an archive) via adbAdd();
                                          // (including the archive itself)
                                          // returns 0 for ok, 1 for error.
{
  sbinfile archive;
  if(archive.open(path, sbinfile::openro))
    return(1);

  unsigned short arcref;                 // whatever..

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

  amem=(char*)malloc(65536);
  aheader=0;
  axheader=0;
  nexthpos=0;

  if(!amem)
  {
    archive.close();
    return 0;
  }

 // list
  while(ReadNextHeader(archive))
  {
   char ext[_MAX_EXT];
   char name[_MAX_FNAME];
   if(aheader->type==0x74)
   {
     afheader=(rarfileheaderstruct*)amem;
     char filename[_MAX_PATH];
     memcpy(filename, afheader->filename, afheader->filenamesize);
     filename[afheader->filenamesize]=0;
     strupr(filename);
     _splitpath(filename, 0, 0, name, ext);
     if(fsIsModule(ext))
     {
       a.size=afheader->unpsize;
       a.parent=arcref;
       a.flags=0;
       fsConvFileName12(a.name, name, ext);
       if(!adbAdd(a))
       {
         free(amem);
         archive.close();
         return 0;
       }
     }
    }
  }
  free(amem);

  archive.close();
  return(1);
}

static int adbRARCall(int act,
                      const char *apath,
                      const char *file,
                      const char *dpath)
{
 switch (act)
 {
  case adbCallGet:
  {
    // extract apath\file to dpath
    // this function is so "complex", because a filename like "foo.mod"
    // must be expanded to "sound/mods/bla/blub/foo.mod" when it's filename
    // inside the rar is "sound/mods/bla/blub/foo.mod".
    // rar e foo.rar foo.mod c:\temp doesn't work, it must be
    // rar e foo.rar sounds/mods/bla/blub/foo.mod c:\temp.

    sbinfile archive;
    if(archive.open(apath, sbinfile::openro))
      return(1);

    amem=(char*)malloc(65536);
    aheader=0;
    axheader=0;
    nexthpos=0;

    if(!amem)
    {
      archive.close();
      return 0;
    }
    while(ReadNextHeader(archive))
    {
      if(aheader->type==0x74)
      {
        afheader=(rarfileheaderstruct*)amem;
        char filename[_MAX_PATH];
        memcpy(filename, afheader->filename, afheader->filenamesize);
        filename[afheader->filenamesize]=0;
        char aext[_MAX_EXT];
        char aname[_MAX_FNAME];
        _splitpath(filename, 0, 0, aname, aext);
        char uext[_MAX_EXT];
        char uname[_MAX_FNAME];
        _splitpath(file, 0, 0, uname, uext);
        if((!stricmp(aext, uext))&&((!stricmp(aname, uname))))     // found
        {
          adbCallArc(cfGetProfileString("arcRAR2", "get", "rar e -std %a %d %n"), apath, filename, dpath);
          break;
        }
      }
    }
    archive.close();
    free(amem);
    return(1);
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

int ReadNextHeader(binfile &file)
{
  if(nexthpos==file.length()) return(0);
  file[nexthpos];
  aheader=(rarheaderstruct*)amem;
  axheader=(rarxheaderstruct*)amem;
  if(!file.eread(aheader, sizeof(rarheaderstruct))) return(0);
  if(aheader->size<sizeof(rarheaderstruct)) return(0);
  if(aheader->size-sizeof(rarheaderstruct))
  if(!file.eread((char*)aheader+sizeof(rarheaderstruct), aheader->size-sizeof(rarheaderstruct)))
    return(0);
  nexthpos=file.tell()+((axheader->flags&0x8000)?axheader->addsize:0);
  if(nexthpos>file.length()) return(0);
  return(1);
}

extern "C"
{
  adbregstruct adbRARReg = {".RAR", adbRARScan, adbRARCall};
  char *dllinfo = "arcs _adbRARReg";
};
