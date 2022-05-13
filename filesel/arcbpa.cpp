// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// BPA "archive" reader
//
// revision history: (please note changes here)
//  -2.0a++e  Felix Domke <tmbinc@gmx.net>
//    -first release
//  -kb980717 Tammo Hinrichs <kb@nwn.de>
//    -changed some structures to fit this into the new version
//    -added _dllinfo record
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile
//  -ryg981216  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -finally fixed this damn bug (at least it should; this modification
//     helped at least the UMX reader)
//
// .BPA-files are from REMEDY's "DeathRally(TM)", and are crypted file-librarys.
// The file "MUSICS.BPA" contains the DeathRally-Songs (S3Ms) and the samples
// (XMs). DON'T PLAY THESE XMs BECAUSE THEY MIGHT CRASH THE PLAYER!
// All files have the extension .CMF (CryptedMusicFile?)
//
// The crypt-method is something with ROLs and SHIFTs. :)
// Look in the source, I can't remember exactly.. :)


#include <stdlib.h>
#include <string.h>
#include "binfstd.h"
#include "pfilesel.h"
#include "psetting.h"

unsigned char Rol(unsigned char v, int c)
{
 int a=v;
 a<<=c;
 int o;
 o=a&0xFF00;
 a&=0xFF;
 a|=(o>>8);
 return((char)a);
}

static int adbBPAScan(const char *path)
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

  long entriesinbpa;
  if (!file.eread(&entriesinbpa, 4))
  {
    file.close();
    return 0;
  }
  if((entriesinbpa<=0)||(entriesinbpa>0x10000)) return(0); // <=0 files?!?!? >65535 files?!?!?

  unsigned char pw;
  while (1)
  {
    pw=0x8B;

    unsigned long nextpos=file.tell();
    char bname[14];
    if(!file.eread(bname, 13)) { return(0); }
    for(int i=0; i<13; i++)
    {
      if(bname[i]==0) break;
      bname[i]+=pw;
      pw+=3;
    }
    long size;
    if(!file.eread(&size, 4)) { return(0); }
    if(!size) break;
    a.size=size;
    a.parent=arcref;
    a.flags=0;

    _splitpath(bname, 0, 0, name, ext);
    strupr(ext);

    if (!strcmp(ext,".CMF"))
    {
      fsConvFileName12(a.name, name, ext);
      if (!adbAdd(a))
      {
        file.close();
        return 0;
      }
    }

  }
  file.close();
  return 1;
}

static int adbBPACall(int act, const char *apath, const char *file, const char *dpath)
{
  switch (act)
  {
   case adbCallGet:
   {
    char target[_MAX_PATH];
    _makepath(target, 0, dpath, file, 0);
    sbinfile afile;
    if(afile.open(apath, sbinfile::openro)) return(0);

    long entriesinbpa, ssize=0x10F3;
    if (!afile.eread(&entriesinbpa, 4)) return 0;
    if((entriesinbpa<=0)||(entriesinbpa>0x10000)) return(0); // <=0 files?!?!? >65535 files?!?!?

    unsigned char pw;
    long size;
    while (1)
    {
     pw=0x8B;

     char bname[14];
     if(!afile.eread(bname, 13)) { return(0); }
     for(int i=0; i<13; i++)
     {
       if(bname[i]==0) break;
       bname[i]+=pw;
       pw+=3;
     }
     if(!afile.eread(&size, 4)) { return(0); }
     if(!size) return(0);
     if(stricmp(bname, file)==0) break;
     ssize+=size;
    }
    afile.seek(ssize);

    char *data;
    if((data=(char*)malloc(size))==NULL)
    {
     return(0);
    }
    sbinfile tfile;
    if(tfile.open(target, sbinfile::opencr|sbinfile::openrw)) return(0);

    afile.eread(data, size);

    {
      int co=0;
      unsigned char g=0x93;
      while(co<size)
      {
        data[co]=Rol(data[co], co%7)+g;
        g-=0x11;
        co++;
      }
    }

    tfile.write(data, size);

    free(data);

    tfile.close();
    afile.close();
    return 1;
   }
   default:
   {
    break;
   }
  }
  return 0;
}

extern "C"
{
  adbregstruct adbBPAReg = {".BPA", adbBPAScan, adbBPACall};
  char *dllinfo = "arcs _adbBPAReg";
};