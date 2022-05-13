// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Player device for WAV output
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -added _dllinfo record
//  -kb981118   Tammo Hinrichs <opencp@gmx.net>
//    -reduced max buffer size to 32k to avoid problems with the
//     wave/mp3 player

#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <string.h>
#include <stdio.h>
#include "imsdev.h"
#include "player.h"
#include "imsrtns.h"

extern "C" extern sounddevice plrDiskWriter;

#define SS_DISK_MANUALSTART 1

static long filepos;
static unsigned long buflen;
static char *playbuf;
static unsigned long bufpos;
static unsigned long bufrate;
static int file;
static unsigned char *diskcache;
static unsigned long cachelen;
static unsigned long cachepos;
static char busy;
static unsigned short playrate;
static unsigned char stereo;
static unsigned char bit16;
static unsigned char writeerr;
static int started;
static int autostart;

static void Flush()
{
  busy=1;
  if (cachepos>(cachelen/2))
  {
    if (!writeerr)
    {
      if (write(file, diskcache, cachepos)!=cachepos)
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
  busy=1;

  if (pos<bufpos)
  {
    memcpy(diskcache+cachepos, playbuf+bufpos, buflen-bufpos);
    memcpy(diskcache+cachepos+buflen-bufpos, playbuf, pos);
    cachepos+=(buflen-bufpos)+pos;
  }
  else
  {
    memcpy(diskcache+cachepos, playbuf+bufpos, pos-bufpos);
    cachepos+=pos-bufpos;
  }

  bufpos=pos;
  busy=0;
}

static long gettimer()
{
  return imuldiv(filepos+cachepos, 65536>>(stereo+bit16), playrate);
}

static void dwSetOptions(int rate, int opt)
{

  // don't even THINK about removing this line!
  if (opt&PLR_RESTRICTED)
    opt&=~PLR_STEREO;
  // don't even THINK about removing this line!
  // and after you removed it, thank me for making hacking this that easy ;)

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

  cachelen=(1024*16*playrate/44100)<<(2+stereo+bit16);
  if (cachelen<(len+1024))
    cachelen=len+1024;
  cachepos=0;
  diskcache=new unsigned char [cachelen];
  if (!diskcache)
    return 0;

  char fn[15];
  int i;
  for (i=0; i<1000; i++)
  {
    memcpy(fn, "CPOUT000.WAV", 13);
    fn[5]+=(i/100)%10;
    fn[6]+=(i/10)%10;
    fn[7]+=(i/1)%10;
    file=open(fn, O_RDONLY);
    if (file==-1)
      break;
    close(file);
  }

  file=open(fn, O_CREAT|O_WRONLY|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);

  unsigned char hdr[0x2C];
  write(file, hdr, 0x2C);

  buflen=len;
  bufpos=0;
  busy=0;
  bufrate=buflen/2;
  if (bufrate>65520)
    bufrate=65520;
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
    write(file, diskcache, cachepos);

  unsigned long wavlen=tell(file)-0x2C;

  lseek(file, 0, SEEK_SET);
  struct
  {
    char riff[4];
    unsigned long len24;
    char wave[4];
    char fmt_[4];
    unsigned long chlen;
    unsigned short form;
    unsigned short chan;
    unsigned long rate;
    unsigned long datarate;
    unsigned short bpsmp;
    unsigned short bits;
    char data[4];
    unsigned long wavlen;
  } wavhdr;

  memcpy(wavhdr.riff, "RIFF", 4);
  memcpy(wavhdr.wave, "WAVE", 4);
  memcpy(wavhdr.fmt_, "fmt ", 4);
  memcpy(wavhdr.data, "data", 4);
  wavhdr.chlen=0x10;
  wavhdr.form=1;
  wavhdr.chan=1<<stereo;
  wavhdr.rate=playrate;
  wavhdr.bits=8<<bit16;
  wavhdr.bpsmp=wavhdr.chan*wavhdr.bits/8;
  wavhdr.datarate=wavhdr.bpsmp*wavhdr.rate;
  wavhdr.wavlen=wavlen;
  wavhdr.len24=wavhdr.wavlen+0x24;
  write(file, &wavhdr, 0x2C);

  lseek(file, 0, SEEK_END);
  close(file);
  delete playbuf;
  delete diskcache;
}

static int dwInit(const deviceinfo &d)
{
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
  card.dev=&plrDiskWriter;
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
  sounddevice plrDiskWriter={SS_PLAYER, "Disk Writer", dwDetect, dwInit, dwClose};
  devaddstruct plrDiskAdd = {dwGetOpt, 0, 0, dwProcessKey};
  char *dllinfo = "driver _plrDiskWriter; addprocs _plrDiskAdd";
}