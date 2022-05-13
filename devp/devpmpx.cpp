// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Player device for MPEG layer 1/2 file output
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb981118   Tammo Hinrichs <opencp@gmx.net>
//    -reduced max buffer size to 32k to avoid problems with the
//     wave/mp3 player
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile

#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "imsdev.h"
#include "player.h"
#include "imsrtns.h"
#include "binfstd.h"
#include "mpencode.h"

extern "C" extern sounddevice plrMPxWriter;

#define SS_DISK_MANUALSTART 1
#define SS_DISK_LAYER 2
#define SS_DISK_MODE 8
#define SS_DISK_FREQ 32
#define SS_DISK_RATE 128
#define SS_DISK_CRC 8192
#define SS_DISK_MODEL 16384
#define SS_DISK_EXTENSION 32768
#define SS_DISK_COPYRIGHT 65536
#define SS_DISK_ORIGINAL 131072
#define SS_DISK_EMPHASIS 262144

static long filepos;
static unsigned long buflen;
static char *playbuf;
static unsigned long bufpos;
static unsigned long bufrate;
static sbinfile file;
static unsigned char *diskcache;
static unsigned long cachelen;
static unsigned long cachepos;
static char busy;
static unsigned short playrate;
static unsigned char stereo;
static unsigned char writeerr;
static int started;
static int autostart;
static ampegencoderparams par;

static void Flush()
{
  busy=1;
  if (cachepos>(cachelen-bufrate*2))
  {
    if (!writeerr)
    {
      if (encodeframe(diskcache, cachepos)!=cachepos)
      {
        writeerr=1;
      }
    }
    if (writeerr)
    {
      printf("\a");
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

static long gettimer()
{
  return imuldiv(filepos+cachepos, 65536>>(stereo+1), playrate);
}

static void dwSetOptions(int rate, int opt)
{
  if (par.mode==3)
    opt&=~PLR_STEREO;
  else
    opt|=PLR_STEREO;
  rate=par.sampfreq;

  stereo=!!(opt&PLR_STEREO);

  opt|=PLR_16BIT|PLR_SIGNEDOUT;

  playrate=rate;
  plrRate=rate;
  plrOpt=opt;
}

static int dwPlay(void *&buf, int &len)
{
  if (len>32704)
    len=32704;
  buf=playbuf=new char [len];
  if (!buf)
    return 0;
  memsetd(buf, (plrOpt&PLR_SIGNEDOUT)?0:(plrOpt&PLR_16BIT)?0x80008000:0x80808080, len>>2);

  writeerr=0;

  started=autostart;

  cachelen=(1024*16*playrate/44100)<<stereo;
  cachepos=0;
  diskcache=new unsigned char [cachelen];
  if (!diskcache)
    return 0;

  char fn[15];
  int i;
  for (i=0; i<1000; i++)
  {
    memcpy(fn, "CPOUT000.MP0", 13);
    fn[5]+=(i/100)%10;
    fn[6]+=(i/10)%10;
    fn[7]+=(i/1)%10;
    fn[11]+=par.lay;
    if (file.open(fn, file.openro))
      break;
    file.close();
  }

  file.open(fn, file.opencr);

  if (!initencoder(file, par))
    return 0;

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

static void dwStop()
{
  plrIdle=0;

  if (!writeerr)
    encodeframe(diskcache, cachepos);

  doneencoder();
  file.close();
  delete playbuf;
}

static int dwInit(const deviceinfo &d)
{
  par.lay=((d.opt/SS_DISK_LAYER)&3)+1;
  par.mode=(d.opt/SS_DISK_MODE)&3;
  par.sampfreq=(((d.opt/SS_DISK_FREQ)&3)==1)?48000:(((d.opt/SS_DISK_FREQ)&3)==2)?32000:44100;
  par.bitrate=((d.opt/SS_DISK_RATE)&63)*8000;
  par.model=((d.opt/SS_DISK_MODEL)&1)+1;
  par.crc=(d.opt/SS_DISK_CRC)&1;
  par.extension=(d.opt/SS_DISK_EXTENSION)&1;
  par.copyright=(d.opt/SS_DISK_COPYRIGHT)&1;
  par.original=(d.opt/SS_DISK_ORIGINAL)&1;
  par.emphasis=(d.opt/SS_DISK_EMPHASIS)&3;

  autostart=!(d.opt&SS_DISK_MANUALSTART);
  plrSetOptions=dwSetOptions;
  plrPlay=dwPlay;
  plrStop=dwStop;
  return 1;
}

static void dwClose()
{
  plrPlay=0;
}

static int dwDetect(deviceinfo &card)
{
  card.dev=&plrMPxWriter;
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


#include "devigen.h"
#include "psetting.h"

static unsigned long dwGetOpt(const char *sec)
{
  unsigned long opt=0;
  if (cfGetProfileBool(sec, "diskmanual", 0, 0))
    opt|=SS_DISK_MANUALSTART;
  opt|=SS_DISK_LAYER*((cfGetProfileInt(sec, "layer", 2, 10)-1)&3);
  opt|=SS_DISK_MODE*(cfGetProfileInt(sec, "mode", 1, 10)&3);
  int freq=cfGetProfileInt(sec, "freq", 44100, 10);
  switch (freq)
  {
  case 48: case 48000: opt|=SS_DISK_FREQ*1; break;
  case 32: case 32000: opt|=SS_DISK_FREQ*2; break;
  }
  int rate=cfGetProfileInt(sec, "rate", 112000, 10);
  if (rate>=500)
    rate/=1000;
  opt|=SS_DISK_RATE*((rate>>3)&63);
  if (cfGetProfileInt(sec, "model", 2, 10)==2)
    opt|=SS_DISK_MODEL;
  if (cfGetProfileBool(sec, "crc", 0, 0))
    opt|=SS_DISK_CRC;
  if (cfGetProfileBool(sec, "extension", 0, 0))
    opt|=SS_DISK_EXTENSION;
  if (cfGetProfileBool(sec, "copyright", 0, 0))
    opt|=SS_DISK_COPYRIGHT;
  if (cfGetProfileBool(sec, "original", 0, 0))
    opt|=SS_DISK_ORIGINAL;
  opt|=SS_DISK_EMPHASIS*(cfGetProfileInt(sec, "emphasis", 0, 10)&3);
  return opt;
}

static int dwProcessKey(unsigned short key)
{
  if (key==0x1F00)
  {
    started=1;
    return 1;
  }
  return 0;
}

extern "C"
{
  sounddevice plrMPxWriter={SS_PLAYER, "MPx Writer", dwDetect, dwInit, dwClose};
  devaddstruct plrMPxAdd = {dwGetOpt, 0, 0, dwProcessKey};
  char *dllinfo = "driver _plrMPxWriter; addprocs _plrMPxAdd";
}