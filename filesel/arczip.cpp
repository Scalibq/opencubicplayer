// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Archive handler for ZIP archives
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <kb@nwn.de>
//    -added _dllinfo record
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile

#include <dos.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include "binfstd.h"
#include "pfilesel.h"
#include "psetting.h"

static unsigned char mdbScanBuf[1084];
static unsigned char adbScanBuf[2048];

void inflatemax(void *ob, const void *ib, unsigned long max);

static int adbZIPScan(const char *path)
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
  while (1)
  {
    struct
    {
      unsigned long sig;
      unsigned long opt;
      unsigned short method;
      unsigned long d1;
      unsigned long d2;
      unsigned long csize;
      unsigned long osize;
      unsigned short flen;
      unsigned short xlen;
    } hdr;
    if (!file.eread(&hdr, 30))
      break;
    if (hdr.sig!=0x04034B50)
      break;
    unsigned long nextpos=file.tell()+hdr.flen+hdr.xlen+hdr.csize;
    if ((hdr.flen<=12)&&!(hdr.opt&0x10000))
    {
      char filename[13];
      file.read(filename, hdr.flen);
      filename[hdr.flen]=0;
      strupr(filename);
      _splitpath(filename, 0, 0, name, ext);

      fsConvFileName12(a.name, name, ext);
      if (fsIsModule(ext))
      {
        a.size=hdr.osize;
        a.parent=arcref;
	a.flags=0;
        if (!adbAdd(a))
        {
          file.close();
          return 0;
        }
        if (fsScanInArc&&((hdr.method==0)||(hdr.method==8)))
        {
          unsigned short fileref;
          fileref=mdbGetModuleReference(a.name, a.size);
          if (fileref==0xFFFF)
          {
            file.close();
            return 0;
          }
          if (!mdbInfoRead(fileref))
          {
            memset(adbScanBuf, 0, 2048);
            file.read(adbScanBuf, (hdr.csize>2048)?2048:hdr.csize);
            if (hdr.method==8)
              inflatemax(mdbScanBuf, adbScanBuf, 1084);
            else
              memcpy(mdbScanBuf, adbScanBuf, 1084);

            moduleinfostruct mi;
            if (mdbGetModuleInfo(mi, fileref))
            {
               mdbReadMemInfo(mi, mdbScanBuf, 1084);
               mdbWriteModuleInfo(fileref, mi);
            }
          }
        }
      }
      if ((!stricmp(ext, MIF_EXT)) && (hdr.osize<65536))
      {
        char *obuffer=new char[hdr.osize],
             *cbuffer=new char[hdr.csize];
        file.read(cbuffer, hdr.csize);
        if (hdr.method==8)
          inflatemax(obuffer, cbuffer, hdr.osize);
        else
          memcpy(obuffer, cbuffer, hdr.osize);
        mifMemRead(a.name, hdr.osize, obuffer);
        delete[] obuffer;
        delete[] cbuffer;
      }
    }
    file.seek(nextpos);
  }
  file.close();
  return 1;
}


static int adbZIPCall(int act, const char *apath, const char *file, const char *dpath)
{
  switch (act)
  {
  case adbCallGet:
    return !adbCallArc(cfGetProfileString("arcZIP", "get", "pkunzip %a %d %n"), apath, file, dpath);
  case adbCallPut:
    return !adbCallArc(cfGetProfileString("arcZIP", "moveto", "pkzip %a %n"), apath, file, dpath);
  case adbCallDelete:
    if (adbCallArc(cfGetProfileString("arcZIP", "delete", "pkzip -d %a %n"), apath, file, dpath))
      return 0;
    if (cfGetProfileBool("arcZIP", "deleteempty", 0, 0))
    {
      find_t ft;
      if (_dos_findfirst(apath, _A_NORMAL, &ft))
        return 1;
      if (ft.size==22)
        unlink(apath);
    }
    return 1;
  case adbCallMoveFrom:
    if (cfGetProfileString("arcZIP", "movefrom", 0))
      return !adbCallArc(cfGetProfileString("arcZIP", "movefrom", ""), apath, file, dpath);
    if (!adbZIPCall(adbCallGet, apath, file, dpath))
      return 0;
    return adbZIPCall(adbCallDelete, apath, file, dpath);
  case adbCallMoveTo:
    if (cfGetProfileString("arcZIP", "moveto", 0))
      return !adbCallArc(cfGetProfileString("arcZIP", "moveto", "pkzip -m %a %n"), apath, file, dpath);
    if (!adbZIPCall(adbCallPut, apath, file, dpath))
      return 0;
    unlink(file);
    return 1;
  }
  return 0;
}

extern "C"
{
  adbregstruct adbZIPReg = {".ZIP", adbZIPScan, adbZIPCall};
  char *dllinfo = "arcs _adbZIPReg";
};
