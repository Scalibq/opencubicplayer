// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// WindowsLameOut
//
//      this is just a hack, ok?
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -added _dllinfo record
//  -fd981012   Felix Domke <tmbinc@gmx.net>
//    -converted devpdisk to devpwin, using party of nb's binfile.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <mmsystem.h>

#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include "imsdev.h"
#include "player.h"
#include "imsrtns.h"

#include "binfstd.h"

sbinfile x;

HWAVEOUT wavehnd;
int blklen;
int nblk;
WAVEHDR *hdrs;
char *playbuf1;
int curbuf;
int curbuflen;

int WWOopen(int rate, int stereo, int bit16, int blen, int nb)
{

  blklen=blen;
  nblk=nb;

  hdrs=new WAVEHDR[nblk];
  if (!hdrs)
    return -1;

  playbuf1=new char [blklen*nblk];
  if (!playbuf1)
    return -1;

  WAVEFORMATEX form;
  form.wFormatTag=WAVE_FORMAT_PCM;
  form.nChannels=stereo?2:1;
  form.nSamplesPerSec=rate;
  form.wBitsPerSample=bit16?16:8;
  form.nBlockAlign=form.nChannels*form.wBitsPerSample/8;
  form.nAvgBytesPerSec=form.nSamplesPerSec*form.nBlockAlign;
  form.cbSize=0;

  if (waveOutOpen(&wavehnd, WAVE_MAPPER, &form, 0, 0, 0))
    return -1;

  int i;
  for (i=0; i<nblk; i++)
    hdrs[i].dwFlags=0;
  curbuflen=0;

  return 0;
}

int WWOclose()
{

  int i;
  while (1)
  {
    for (i=0; i<nblk; i++)
      if (!(hdrs[i].dwFlags&WHDR_DONE)&&(hdrs[i].dwFlags&WHDR_PREPARED))
        break;
    if (i==nblk)
      break;
    Sleep(50);
  }
  waveOutReset(wavehnd);

  for (i=0; i<nblk; i++)
    if (hdrs[i].dwFlags&WHDR_PREPARED)
      waveOutUnprepareHeader(wavehnd, &hdrs[i], sizeof(*hdrs));
  waveOutClose(wavehnd);

  delete hdrs;
  delete playbuf1;
  return 0;
}

int WWOwrite(const void *buf, int len)
{
  while (len)
  {
    if (!curbuflen)
    {
      int i;
      for (i=0; i<nblk; i++)
        if (hdrs[i].dwFlags&WHDR_DONE)
          waveOutUnprepareHeader(wavehnd, &hdrs[i], sizeof(*hdrs));
      for (i=0; i<nblk; i++)
        if (!(hdrs[i].dwFlags&WHDR_PREPARED))
          break;
      if (i==nblk)
      {
        Sleep(20);
        continue;
      }
      curbuf=i;
    }
    int l=blklen-curbuflen;
    if (l>len)
      l=len;
    memcpy(playbuf1+curbuf*blklen+curbuflen, buf, l);
    *(char**)&buf+=l;
    len-=l;
    curbuflen+=l;
    if (curbuflen==blklen)
    {
      hdrs[curbuf].lpData=playbuf1+curbuf*blklen;
      hdrs[curbuf].dwBufferLength=blklen;
      hdrs[curbuf].dwFlags=0;
      waveOutPrepareHeader(wavehnd, &hdrs[curbuf], sizeof(*hdrs));
      waveOutWrite(wavehnd, &hdrs[curbuf], sizeof(*hdrs));
      curbuflen=0;
    }
  }
  return 0;
}

extern "C" extern sounddevice plrWinWaveOut;

#define SS_DISK_MANUALSTART 1

static long filepos;
static unsigned long buflen;
static char *playbuf;
static unsigned long bufpos;
static unsigned long bufrate;
//static int file;
static unsigned char *diskcache;
static unsigned long cachelen;
static unsigned long cachepos;
static char busy;
static unsigned short playrate;
static unsigned char stereo;
static unsigned char bit16;
static unsigned char writeerr;
static int started;
static int autostart=1;

static void Flush()
{
  busy=1;
  if (cachepos>(cachelen-bufrate*2))
  {
    if (WWOwrite(diskcache, cachepos)!=cachepos)
    {
        writeerr=1;
    }
    filepos+=cachepos;
    cachepos=0;
  }
  busy=0;
}

static int getpos()
{
  if (busy||!started||((cachepos+bufrate)>cachelen))
    return bufpos;

  return (bufpos+bufrate)%buflen;
}

static void advance(int pos)
{
  int n=(pos-bufpos+buflen)%buflen;

  if ((bufpos+n)>buflen)
  {
    memcpy(diskcache+cachepos, playbuf+bufpos, buflen-bufpos-n);
    memcpy(diskcache+cachepos+buflen-bufpos-n, playbuf, n-buflen+bufpos+n);
  }
  else
    memcpy(diskcache+cachepos, playbuf+bufpos, n);

  cachepos+=n;

  bufpos=pos;
}

static long initticks;

static long gettimer()
{
  return imuldiv(filepos+cachepos, 65536>>(stereo+bit16), playrate);
}

static void wwoSetOptions(int rate, int opt)
{
  stereo=!!(opt&PLR_STEREO);
  bit16=!!(opt&PLR_16BIT);

  if (bit16)
    opt|=PLR_SIGNEDOUT;
  else
    opt&=~PLR_SIGNEDOUT;

  if (rate<5000)
    rate=5000;

  if (rate>64000)
    rate=64000;

  playrate=rate;
  plrRate=rate;
  plrOpt=opt;
}

static int wwoPlay(void *&buf, int &len)
{
  buf=playbuf=new char [len];
  if (!buf)
    return 0;
  memsetd(buf, (plrOpt&PLR_SIGNEDOUT)?0:(plrOpt&PLR_16BIT)?0x80008000:0x80808080, len>>2);

  writeerr=0;

  started=autostart;

  cachelen=(1024*16*playrate/44100)<<(2+stereo+bit16);
  cachepos=0;
  diskcache=new unsigned char [cachelen];
  if (!diskcache)
    return 0;

  initticks=GetTickCount();

  WWOopen(playrate, stereo, bit16, 10000, 10);

  buflen=len;
  bufpos=0;
  busy=0;
  bufrate=buflen/2;
  filepos=0;

  plrGetBufPos=getpos;
  plrGetPlayPos=getpos;
  plrAdvanceTo=advance;
  plrIdle=Flush;
  plrGetTimer=gettimer;

  return 1;
}

static void wwoStop()
{
  plrIdle=0;

  WWOwrite(diskcache, cachepos);
  WWOclose();

  delete playbuf;
}

static int wwoInit(const deviceinfo &d)
{
  plrSetOptions=wwoSetOptions;
  plrPlay=wwoPlay;
  plrStop=wwoStop;
  return 1;
}

static void wwoClose()
{
  plrPlay=0;
}

static int wwoDetect(deviceinfo &card)
{
  card.dev=&plrWinWaveOut;
  card.port=-1;
  card.port2=-1;
  card.irq=-1;
  card.irq2=-1;
  card.dma=-1;
  card.dma2=-1;
  card.subtype=-1;
  card.mem=0;
  card.chan=2;

  return 1;
}

extern "C"
{
  sounddevice plrWinWaveOut={SS_PLAYER, "Windows Waveout", wwoDetect, wwoInit, wwoClose};
  char *dllinfo = "driver _plrWinWaveOut";
}
