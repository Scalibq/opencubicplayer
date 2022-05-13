// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Sampler device for the TerraTec AudioSystem EWS64 Codec

// STILL UNDER CONSTRUCTION!

#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <ctype.h>
#include <conio.h>
#include "irq.h"
#include "dma.h"
#include "ss.h"
#include "sampler.h"

#define SS_WSS_64000 1

extern "C" extern sounddevice smpEWS;

static unsigned short gusPort;
static unsigned char gusIRQ;
static unsigned char gusIRQ2;
static unsigned char gusDMA;
static unsigned char gusDMA2;
static unsigned short wssPort;
static unsigned char wssIRQ;
static unsigned char wssDMA;
static unsigned char wssDMA2;
static unsigned char wssType;
static unsigned char gusmaxwssSetup;
static unsigned char oldmuteleft, oldmuteright, oldlineleft, oldlineright, oldcdleft, oldcdright, oldmic, oldgf1left, oldgf1right;
static unsigned char wssSpeed;
static unsigned char wssSource;
static unsigned int wssMaxRate;


inline unsigned char wssinp(unsigned short r)
{
  return inp(wssPort+r);
}

inline void wssoutp(unsigned short r, unsigned char v)
{
  outp(wssPort+r, v);
}

inline unsigned char wssin(unsigned char r)
{
  wssoutp(4, r);
  return wssinp(5);
}

inline void wssout(unsigned char r, unsigned char v)
{
  wssoutp(4, r);
  wssoutp(5, v);
}

static char wssGetCfg()
{
  return 0;
}


static void wssSetSource(int source)
{
  wssSource=source;
}


static void wssSetOptions(int rate, int opt)
{
  char stereo=!!(opt&SMP_STEREO);
  char bit16=!!(opt&SMP_16BIT);
  if (bit16)
    opt|=SMP_SIGNEDOUT;
  else
    opt&=~SMP_SIGNEDOUT;

  if (rate>wssMaxRate)
    rate=wssMaxRate;

  static unsigned short rates[16]=
  { 8000, 5513,16000,11025,27429,18900,32000,22050,
   54857,37800,64000,44100,48000,33075, 9600, 6615};

// bit   0=> base frq: 384000,264600
// bit 3-1=> frq div:  48,24,14,12,7,6,8,40

  unsigned char spd=10;
  short i;
  for (i=0; i<16; i++)
    if ((rates[i]>=rate)&&(rates[i]<rates[spd]))
      spd=i;

  rate=rates[spd];

  wssSpeed=spd|(bit16?0x40:0x00)|(stereo?0x10:0x00);

  smpRate=rate;
  smpOpt=opt;
}

static int wssSample(void *buf, int &len)
{
  wssout(0x48, wssSpeed);
  wssout(0x48, wssSpeed);
  wssinp(5);
  wssinp(5);

  while (wssinp(4)&0x80);

  wssoutp(4, 0x08);

  while (wssinp(4)!=0x08)
    wssoutp(4, 0x08);

  while (wssinp(4)!=0x0B)
    wssoutp(4, 0x0B);

  while (wssinp(5)&0x20)
    wssoutp(4, 0x0B);

  if (wssType==2)
  {
    if (wssDMA2&4)
    {
      outp(gusPort+0x106, gusmaxwssSetup&~0x10);
      outp(gusPort+0x106, gusmaxwssSetup);
    }
  }

  wssout(0x0C, 0);

  dmaStart(wssDMA2, buf, len, 0x54);

  wssout(0x0F, 0xF0);
  wssout(0x0E, 0xFF);

  wssout(0x49, ((wssDMA==wssDMA2)&&(wssType!=2))?6:2);
  wssout(0, 0x09);

  oldmuteleft=wssin(0x00);
  oldmuteright=wssin(0x01);
  oldgf1left=wssin(0x02);
  oldgf1right=wssin(0x03);
  oldcdleft=wssin(0x04);
  oldcdright=wssin(0x05);
  oldlineleft=wssin(0x12);
  oldlineright=wssin(0x13);
  oldmic=wssin(0x1A);

  unsigned char src=(wssSource==SMP_LINEIN)?0x00:(wssSource==SMP_MIC)?0x80:0xC0;
  wssout(0x00, src|0x8|(oldmuteleft&0x20));
  wssout(0x01, src|0x8|(oldmuteleft&0x20));
  wssout(0x02, oldgf1left|0x80);
  wssout(0x03, oldgf1right|0x80);
  if (wssSource!=SMP_CD)
  {
    wssout(0x04, oldcdleft|0x80);
    wssout(0x05, oldcdright|0x80);
  }
  else
  {
    wssout(0x04, oldcdleft&~0x80);
    wssout(0x05, oldcdright&~0x80);
  }
  if (wssSource!=SMP_LINEIN)
  {
    wssout(0x12, oldlineleft|0x80);
    wssout(0x13, oldlineright|0x80);
  }
  else
  {
    wssout(0x12, oldlineleft&~0x80);
    wssout(0x13, oldlineright&~0x80);
  }
  if (wssSource!=SMP_MIC)
    wssout(0x1A, (oldmic&~0x40)|0x80);
  else
    wssout(0x1A, oldmic&~0xC0);

  smpGetBufPos=dmaGetBufPos;

  return 1;
}

static void wssStop()
{
  wssout(0x00, oldmuteleft);
  wssout(0x01, oldmuteright);
  wssout(0x02, oldgf1left);
  wssout(0x03, oldgf1right);
  wssout(0x04, oldcdleft);
  wssout(0x05, oldcdright);
  wssout(0x12, oldlineleft);
  wssout(0x13, oldlineright);
  wssout(0x1A, oldmic);

  wssout(0x0A, 0);
//  wssinp(6);
//  wssoutp(6, 0);
  wssout(0x49, ((wssDMA==wssDMA2)&&(wssType!=2))?4:0);
  wssout(0, 0x09);
  dmaStop();
}

static void waste_time(unsigned int val)
{
  int i,j;
  for (i=0; i<val; i++)
    for (j=0; j<1000; j++);
}

static int wssInit(const deviceinfo &card)
{
  if ((card.subtype!=-1)&&((card.subtype<0)||(card.subtype>2)))
    return 0;

  if (card.opt&SS_WSS_64000)
    wssMaxRate=64000;
  else
    wssMaxRate=48000;
  if (card.subtype==-1)
    wssType=0;
  else
    wssType=card.subtype;
  wssIRQ=gusIRQ=card.irq;
  gusIRQ2=card.irq2;
  wssDMA=gusDMA2=card.dma;
  wssDMA2=gusDMA=(card.dma2==-1)?card.dma:card.dma2;
  wssPort=card.port-(wssType?4:0);
  gusPort=card.port2;

  if (wssType==2)
  {
    // setup normal gus...

    gusmaxwssSetup=((wssPort-0x30C)>>4)|0x40;
    if (wssDMA2&4)
      gusmaxwssSetup|=0x10;
    if (wssDMA&4)
      gusmaxwssSetup|=0x20;
    outp(gusPort+0x106, gusmaxwssSetup);
    waste_time(100);
  }
/*
  if (wssType==0)
  {
    int cfg;
    switch (card.dma)
    {
    case 0: cfg=0x01; break;
    case 1: cfg=0x02; break;
    case 3: cfg=0x03; break;
    default:
      return 0;
    }
    wssoutp(0, cfg);
  }
*/

//  wssinp(6);
//  wssoutp(6, 0);

  unsigned char ver;
  int i;
  for (i=0; i<1000; i++)
  {
    if (wssinp(4)&0x80)
      waste_time(1);
    else
    {
      wssout(0x0C, 0);
      ver=wssinp(4)&0x0F;
      if ((ver>=1)&&(ver<15))
        break;
    }
  }

  if (i==1000)
    return 0;

  wssout(0x0C, 0);
  ver=wssinp(5);
  wssoutp(5, 0);
  if (wssinp(5)!=ver)
    return 0;

  if (wssinp(4)&0x80)
    return 0;

  wssout(0x49, ((wssDMA==wssDMA2)&&(wssType!=2))?4:0);
  wssoutp(4, 0x09);

  wssout(0x0C, 0);

  smpSetOptions=wssSetOptions;
  smpSample=wssSample;
  smpStop=wssStop;
  smpSetSource=wssSetSource;

  smpSetOptions(65535, SMP_STEREO|SMP_16BIT);
  smpSetSource(SMP_LINEIN);

  return 1;
}

static void wssClose()
{
  smpSample=0;
}

static int wssDetect(deviceinfo &card)
{
  int type=card.subtype;
  if (type==-1)
  {
    if (!wssGetCfg())
      type=0;
    else
      type=wssType;
  }

  if (type==0)
  {
    if ((card.port==-1)||(card.dma==-1))
      return 0;
  }
  else
  {
    if (!wssGetCfg())
    {
      if ((card.port==-1)||(card.dma==-1))
        return 0;
    }
    else
      if (wssType!=type)
        return 0;
  }

  card.dev=&smpEWS;
  if (card.port==-1)
    card.port=wssPort;
  if (card.dma==-1)
    card.dma=wssDMA;
  card.subtype=wssType;
  if (type==2)
  {
    if (card.port2==-1)
      card.port2=gusPort;
    if (card.dma2==-1)
      card.dma2=wssDMA2;
  }
  else
  {
    card.port2=-1;
    card.dma2=-1;
  }
  card.irq=-1;
  card.irq2=-1;
  card.mem=0;
  card.chan=2;

  return 1;
}



extern "C" {
  sounddevice smpEWS={SS_SAMPLER, "AudioSystem EWS64 Codec", wssDetect, wssInit, wssClose};
  char *dllinfo="driver _smpEWS"
}


