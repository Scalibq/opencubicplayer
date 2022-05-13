// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// NoSound Player device
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -added _dllinfo record

#include "imsdev.h"
#include "player.h"
#include "timer.h"
#include "imsrtns.h"

static unsigned long buflen;
static unsigned long bufrate;

extern "C" extern sounddevice plrNone;

static int getpos()
{
  return imuldiv(tmGetTimer(), bufrate, 65536)%buflen;
}

static void advance(int)
{
}

static long gettimer()
{
  return tmGetTimer()-imuldiv(buflen, 65536, bufrate);
}

static void qpSetOptions(int rate, int opt)
{
  unsigned char stereo=!!(opt&PLR_STEREO);
  unsigned char bit16=!!(opt&PLR_16BIT);

  if (rate<5000)
    rate=5000;

  if (rate>48000)
    rate=48000;

  bufrate=rate<<(stereo+bit16); // !!!!!!!!!!

  plrRate=rate;
  plrOpt=opt;
}

static void *thebuf;

static int qpPlay(void *&buf, int &len)
{
  thebuf=buf=new unsigned char [len];
  if (!buf)
    return 0;
  memsetd(buf, (plrOpt&PLR_SIGNEDOUT)?0:(plrOpt&PLR_16BIT)?0x80008000:0x80808080, len>>2);

  buflen=len;

  plrGetBufPos=getpos;
  plrGetPlayPos=getpos;
  plrAdvanceTo=advance;
  plrGetTimer=gettimer;

  return 1;
}

static void qpStop()
{
  delete thebuf;
}

static int qpInit(const deviceinfo &)
{
  plrSetOptions=qpSetOptions;
  plrPlay=qpPlay;
  plrStop=qpStop;
  return 1;
}

static void qpClose()
{
  plrPlay=0;
}

static int qpDetect(deviceinfo &card)
{
  card.dev=&plrNone;
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


extern "C" {
  sounddevice plrNone={SS_PLAYER, "Super High Quality Quiet Player", qpDetect, qpInit, qpClose};
  char *dllinfo="driver _plrNone";
}