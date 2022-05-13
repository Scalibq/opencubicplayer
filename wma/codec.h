#ifndef __CODEC_H
#define __CODEC_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "msacmdrv.h"
#include "binfile.h"
#include "binfmem.h"

class acmcodec : public binfile
{
protected:
  mbinfile memwma;
  char *memwmab;

  binfile *src;
  //DRIVERPROC DriverProc;
  long CALLBACK (*DriverProc)(DWORD, HDRVR, UINT, LPARAM, LPARAM);
  void *hnd;
  WAVEFORMATEX *fmtsrc, *fmtdst;
  ACMDRVSTREAMINSTANCE padsi;
  ACMDRVOPENDESC paod;
  ACMDRIVERDETAILS dd;
  int first;
  int dinst;

  short *buf;
  void *srcbuf;
  int buflen, srclen;
  int bufp;

  virtual errstat rawclose();
  virtual binfilepos rawread(void *, binfilepos);
  virtual binfilepos rawseek(binfilepos);

  int GetLenForSrc(int &);
  errstat InitDriver();
  errstat InitCODEC(WAVEFORMATEX *src, WAVEFORMATEX *dst);
  binfilepos Decode();

public:
  errstat open(binfile &, WAVEFORMATEX*, DRIVERPROC, void *, int buffer=0);
};

#endif
