// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Sampler device for the Gravis Ultrasound Classic/MAX/ACE
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -added _dllinfo record

#include <conio.h>
#include <stdlib.h>
#include "imsdev.h"
#include "irq.h"
#include "dpmi.h"
#include "dma.h"
#include "sampler.h"

extern "C" extern sounddevice smpUltraSound;

static unsigned short gusPort;
static unsigned char gusDMA;
static unsigned char gusIRQ;
static unsigned char gusDMA2;
static unsigned char gusIRQ2;
static unsigned char usSampControl;
static unsigned char usTimerConst;
static unsigned char usSource;


static char getcfg()
{
  char *ptr=getenv("ULTRASND");
  if (!ptr)
    return 0;
  while (*ptr==' ')
    ptr++;
  if (!ptr)
    return 0;
  gusPort=strtoul(ptr, 0, 16);
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  gusDMA=strtoul(ptr, 0, 10)&7;
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  gusDMA2=strtoul(ptr, 0, 10)&7;
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  gusIRQ=strtoul(ptr, 0, 10)&15;
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  gusIRQ2=strtoul(ptr, 0, 10)&15;
  return 1;
}

static unsigned char inpGUS(unsigned short p)
{
  return inp(gusPort+p);
}

static void delayGUS()
{
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
}

static void outpGUS(unsigned short p, unsigned char v)
{
  outp(gusPort+p,v);
}

static void outGUS(unsigned char c, unsigned char v)
{
  outp(gusPort+0x103, c);
  outp(gusPort+0x105, v);
}

static void outdGUS(unsigned char c, unsigned char v)
{
  outp(gusPort+0x103, c);
  outp(gusPort+0x105, v);
  delayGUS();
  outp(gusPort+0x105, v);
}

static void outwGUS(unsigned char c, unsigned short v)
{
  outp(gusPort+0x103, c);
  outpw(gusPort+0x104, v);
}

static unsigned char inGUS(unsigned char c)
{
  outp(gusPort+0x103, c);
  return inp(gusPort+0x105);
}

static char testGUS(unsigned short port)
{
  gusPort=port;

  outGUS(0x4C,0);

  delayGUS();
  delayGUS();

  outGUS(0x4C,1);

  delayGUS();
  delayGUS();

  outGUS(0x44,0);

  outwGUS(0x43,0);
  char v0=inpGUS(0x107);
  outwGUS(0x43,1);
  char v1=inpGUS(0x107);

  outwGUS(0x43,0);
  outpGUS(0x107,0xAA);
  outwGUS(0x43,1);
  outpGUS(0x107,0x55);

  outwGUS(0x43,0);
  char gus=inpGUS(0x107)==0xAA;

  outwGUS(0x43,v0);
  outpGUS(0x107,0xAA);
  outwGUS(0x43,v1);
  outpGUS(0x107,0x55);

  return gus;
}

static void initgus()
{
  int i;

  outGUS(0x4C,0);
  for (i=0; i<10; i++)
    delayGUS();

  outGUS(0x4C,1);
  for (i=0; i<10; i++)
    delayGUS();

  outGUS(0x41,0);
  outGUS(0x45,0);
  outGUS(0x49,0);
  outpGUS(0x100,0);

  outGUS(0xE,0xCD);

  inpGUS(0x6);
  inGUS(0x41);
  inGUS(0x49);
  inGUS(0x8F);
  inpGUS(0x100);

  for (i=0; i<32; i++)
  {
    outpGUS(0x102,i);
    outdGUS(0x00,3);
    outwGUS(0x09,0);
    outdGUS(0x0D,3);
    delayGUS();
  }

  inpGUS(0x6);
  inGUS(0x41);
  inGUS(0x49);
  inGUS(0x8F);
  inpGUS(0x100);

  outGUS(0x4C,0x07);

  unsigned char l1="\x00\x00\x01\x03\x00\x02\x00\x04\x00\x00\x00\x05\x06\x00\x00\x07"[gusIRQ]|((gusIRQ==gusIRQ2)?0x40:"\x00\x00\x08\x18\x00\x10\x00\x20\x00\x00\x00\x28\x30\x00\x00\x38"[gusIRQ2]);
  unsigned char l2="\x00\x01\x00\x02\x00\x03\x04\x05"[gusDMA]|((gusDMA==gusDMA2)?0x40:"\x00\x08\x00\x10\x00\x18\x20\x28"[gusDMA2]);

  outpGUS(0xF,5);
  outpGUS(0x0,0x0B);
  outpGUS(0xB,0);
  outpGUS(0xF,0);

  outpGUS(0x0,0x0B);
  outpGUS(0xB,l2|0x80);
  outpGUS(0x0,0x4B);
  outpGUS(0xB,l1);
  outpGUS(0x0,0x0B);
  outpGUS(0xB,l2);
  outpGUS(0x0,0x4B);
  outpGUS(0xB,l1);

  outpGUS(0x102,0);
  outpGUS(0x0,0x08);
  outpGUS(0x102,0);
}



static void irqrout()
{
  if (inGUS(0x49)&0x40)
    outGUS(0x49, usSampControl);
}





static void usSetOptions(int rate, int opt)
{
  opt&=~SMP_16BIT;

  usSampControl=0x21|(gusDMA2&4)|((opt&SMP_SIGNEDOUT)?0x00:0x80)|((opt&SMP_STEREO)?0x02:0x00);

  if (rate<5000)
    rate=5000;

  if (rate>44100)
    rate=44100;

  usTimerConst=(617400/rate);
  rate=617400/usTimerConst;
  usTimerConst--;

  smpRate=rate;
  smpOpt=opt;
}

static void usSetSource(int source)
{
  usSource=source;
}



static int usStart(void *buf, int &len)
{
  initgus();

  irqInit(gusIRQ, irqrout, 0, 512);

  outpGUS(0x102,0);
  delayGUS();
  outpGUS(0x0,(usSource==SMP_MIC)?0x0D:(usSource==SMP_LINEIN)?0x08:0x08);//0x09);

  outGUS(0x48, usTimerConst);

  dmaStart(gusDMA2, buf, len, 0x54);

  outGUS(0x49, usSampControl);

  smpGetBufPos=dmaGetBufPos;

  return 1;
}

static void usStop()
{
  irqClose();
  outGUS(0x49,0);
  dmaStop();
  initgus();
}




static int usInit(const deviceinfo &c)
{
  if ((c.irq==-1)||(c.dma==-1))
    return 0;

  if (!testGUS(c.port))
    return 0;

  gusPort=c.port;
  gusIRQ=c.irq;
  gusIRQ2=c.irq2;
  gusDMA=c.dma;
  gusDMA2=c.dma2;

  smpSetOptions=usSetOptions;
  smpSample=usStart;
  smpStop=usStop;
  smpSetSource=usSetSource;

  smpSetOptions(65535, SMP_STEREO|SMP_16BIT);
  smpSetSource(SMP_LINEIN);

  return 1;
}

static void usClose()
{
  smpSample=0;
}

static int usDetect(deviceinfo &c)
{
  if (!getcfg())
  {
    if ((c.port==-1)||((c.irq==-1)&&(c.irq2==-1))||((c.dma==-1)&&(c.dma2==-1)))
      return 0;
    gusPort=c.port;
    gusIRQ=(c.irq==-1)?c.irq2:c.irq;
    gusIRQ2=(c.irq2==-1)?c.irq:c.irq2;
    gusDMA=(c.dma==-1)?c.dma2:c.dma;
    gusDMA2=(c.dma2==-1)?c.dma:c.dma2;
  }
  else
  {
    if (c.port!=-1)
      gusPort=c.port;
    if (c.irq!=-1)
      gusIRQ=c.irq;
    if (c.irq2!=-1)
      gusIRQ2=c.irq2;
    if (c.dma!=-1)
      gusDMA=c.dma;
    if (c.dma2!=-1)
      gusDMA2=c.dma2;
  }

  if (!testGUS(gusPort))
    return 0;
  c.dev=&smpUltraSound;
  c.port=gusPort;
  c.port2=-1;
  c.irq=(gusIRQ<gusIRQ2)?gusIRQ:gusIRQ2;
  c.irq2=(gusIRQ<gusIRQ2)?gusIRQ2:gusIRQ;
  c.dma=gusDMA;
  c.dma2=gusDMA2;
  c.subtype=-1;
  c.mem=0;
  c.chan=2;
  return 1;
}

extern "C" {
  sounddevice smpUltraSound={SS_SAMPLER, "Gravis UltraSound", usDetect, usInit, usClose};
  char *dllinfo = "driver _smpUltraSound";
}


