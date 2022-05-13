// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// WMAPlay file type detection routines for fileselector
//
// revision history: (please note changes here)
//  -fd990508   Felix Domke <tmbinc@gmx.net>
//    -first release

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "asfread.h"
#include "pfilesel.h"
#include "binfile.h"

/* static int wmapReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int len)
{
  if (*(unsigned long*)buf!=0x75B22630) return -1;
  *m.modname=0;
  strcpy(m.modname, "Advanced Streaming Format");
  m.channels=0;
  m.playtime=0;
  m.modtype=mtWMA;
  return 0;
} */

static int wmapReadInfo(moduleinfostruct &m, binfile &f, const unsigned char *buf, int len)
{
  if (*(unsigned long*)buf!=0x75B22630) return 0;
  strcpy(m.modname, "ASF");
  m.modtype=mtWMA;
  asfreader asf;
  if (asf.open(f)) return 0;
  WAVEFORMATEX *pwmx;
  pwmx=(WAVEFORMATEX *)asf.ioctl(asfreader::ioctlgetformat);
  if (!pwmx) return 0;
  sprintf(m.modname, "ASF %2dkHz %s (%4dkbit/s)", pwmx->nSamplesPerSec/1000, (pwmx->nChannels==1)?"mono  ":"stereo", pwmx->nAvgBytesPerSec*8/1024);
  m.channels=pwmx->nChannels;
  m.playtime=asf.length()/pwmx->nAvgBytesPerSec;
  asf.close();
  f.seek(0);
  return 1;
}

extern "C"
{
  mdbreadnforegstruct wmapReadInfoReg = {0, wmapReadInfo};
};
