// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Sampler device for the Creative Labs Soundblaster series
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -added _dllinfo record

#include <stdlib.h>
#include <conio.h>
#include "irq.h"
#include "dma.h"
#include "imsdev.h"
#include "sampler.h"

#define SS_SB_REVSTEREO 1

extern "C" extern sounddevice smpSoundBlaster;

static signed short sbPort;
static signed char sbIRQ;
static signed char sbDMA;
static signed char sbDMA16;
static signed char sbUseVer;
static volatile char rightirq;
static unsigned char sbSource;
static unsigned char sbInSavL,sbInSavR;

static unsigned char getcfg()
{
  sbPort=-1;
  sbIRQ=-1;
  sbDMA=-1;
  sbDMA16=-1;
  sbUseVer=-1;
  char *s=getenv("BLASTER");
  if (!s)
    return 0;
  while (1)
  {
    while (*s==' ')
      s++;
    if (!*s)
      break;
    switch (*s++)
    {
    case 'a': case 'A':
      sbPort=strtoul(s, 0, 16);
      break;
    case 'i': case 'I':
      sbIRQ=strtoul(s, 0, 10);
      break;
    case 'd': case 'D':
      sbDMA=strtoul(s, 0, 10);
      break;
    case 'h': case 'H':
      sbDMA16=strtoul(s, 0, 10);
      break;
    case 't': case 'T':
      sbUseVer=strtoul(s, 0, 10);
      switch (sbUseVer)
      {
      case 6: sbUseVer=4; break;
      case 2: case 4: sbUseVer=3; break;
      case 3: sbUseVer=2; break;
      case 1: sbUseVer=1; break;
      default: sbUseVer=-1;
      }
      break;
    }
    while ((*s!=' ')&&*s)
      s++;
  }
  return 1;
}

static unsigned char inpSB(unsigned char p)
{
  return inp(sbPort+p);
}

static void outpSB(unsigned char p, unsigned char v)
{
  outp(sbPort+p,v);
}

static void outSB(unsigned char v)
{
  while (inpSB(0xC)&0x80);
  outpSB(0xC,v);
}

static unsigned char inSB()
{
  while (!(inpSB(0xE)&0x80));
  return inpSB(0xA);
}

static void initSB()
{
  outpSB(0x6,1);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  outpSB(0x6,0);
}

static void biosdelay(unsigned long c)
{
  volatile unsigned long &biosclock=*(volatile unsigned long*)0x46C;
  unsigned long t0=biosclock;
  while ((biosclock-t0)<c)
    break;
}

static void initSBp()
{
  outpSB(0x6,1);
  biosdelay(3);
  outpSB(0x6,0);
  biosdelay(3);
}

static void setrateSB(unsigned char r)
{
  outSB(0x40);
  outSB(r);
}

static void setrateSB16(unsigned short r)
{
  outSB(0x41);
  outSB(r>>8);
  outSB(r);
}

static void resetSB()
{
  inpSB(0xE);
}

static void resetSB16()
{
  inpSB(0xF);
}

static void spkroffSB()
{
  outSB(0xD3);
}

static void writeMixer(unsigned char p, unsigned char v)
{
  outpSB(0x4, p);
  outpSB(0x5, v);
}

static unsigned char readMixer(unsigned char p)
{
  outpSB(0x4, p);
  return inpSB(0x5);
}

static void test()
{
  rightirq=1;
}

static char testPort(unsigned short port, int delay)
{
  sbPort=port;
  if (delay)
    initSBp();
  else
    initSB();
  int i;
  for (i=0; i<1000; i++)
    if (inpSB(0xE)&0x80)
      return inpSB(0xA)==0xAA;
  return 0;
}

static char testirq(unsigned char irq)
{
  irqInit(irq, test, 0, 512);

  initSB();
  setrateSB(0xD3);

  rightirq=0;

  outSB(0x80);
  outSB(0x03);
  outSB(0x00);
  for (int i=0; i<0x10000; i++)
    if (rightirq)
      break;

  irqClose();

  initSB();
  resetSB();

  return rightirq;
}

static char testdma(unsigned char irq, unsigned char dma)
{
  outp(0x0A,4);
  outp(0x0A,5);
  outp(0x0A,7);

  irqInit(irq, test, 0, 512);
  dmaStart(dma, 0, 10, 0x48);

  initSB();
  setrateSB(0xD3);

  rightirq=0;

  outSB(0x14);
  outSB(0x03);
  outSB(0x00);
  for (unsigned short i=0; i!=0xFFFF; i++)
    if (rightirq)
      break;
  irqClose();
  dmaStop();

  initSB();
  resetSB();

  return rightirq;
}

static unsigned short getVersion()
{
  outSB(0xE1);
  while(1)
  {
    unsigned char verhi=inSB();
    if (verhi==0xAA)
      continue;
    return ((unsigned short)verhi<<8)+inSB();
  }
}




static void sb1Sampler()
{
  resetSB();
  outSB(0x24);
  outSB(0xFF);
  outSB(0xFF);
  inSB();
}

static void sb2proSamplerHS()
{
  resetSB();
}

static void sb16Player8()
{
  resetSB();
  outSB(0x45);
}

static void sb16Player16()
{
  outSB(0x47);
  resetSB16();
}

static unsigned char sbTimerConst;
static unsigned char sbStereo;
static unsigned char sb16Bit;
static unsigned char sbSignedOut;
static unsigned char revstereo;

static void sbSetSource(int src)
{
  sbSource=src;
}

static void sbSetOptions(int rate, int opt)
{
  switch (sbUseVer)
  {
  case 1: case 2:
    opt&=~(SMP_STEREO|SMP_16BIT|SMP_SIGNEDOUT);
    break;
  case 3:
    opt&=~(SMP_16BIT|SMP_SIGNEDOUT);
    break;
  }
  if (revstereo)
    opt^=SMP_REVERSESTEREO;
  sbStereo=!!(opt&SMP_STEREO);
  sb16Bit=!!(opt&SMP_16BIT);
  sbSignedOut=!!(opt&SMP_SIGNEDOUT);

  unsigned long rt=rate;
  if ((sbUseVer==3)&&sbStereo)
    rt<<=1;

  if (rt<4000)
    rt=4000;
  if (sbUseVer==4)
    if (rt<5000)
      rt=5000;
  if ((sbUseVer==2)||(sbUseVer==3))
    if (rt<8000)
      rt=8000;

  if (rt>45454)
    rt=45454;
  if (sbUseVer==3)
    if (rt>43478)
      rt=43478;
  if (sbUseVer==2)
    if (rt>15151)
      rt=15151;
  if (sbUseVer==1)
    if (rt>11111)
      rt=11111;

  sbTimerConst=256-1000000/rt;
  if (sbUseVer!=4)
    rate=1000000/(256-sbTimerConst);

  if ((sbUseVer==3)&&sbStereo)
    rate>>=1;

  smpRate=rate;
  smpOpt=opt;
}

static int sbSample(void *buf, int &len)
{
  initSBp();
  resetSB();
  resetSB16();
  if (sbUseVer==4)
    setrateSB16(smpRate);
  else
  {
    setrateSB(sbTimerConst);
    setrateSB(sbTimerConst);
  }
  spkroffSB();

  void (*playproc)();
  switch (sbUseVer)
  {
  case 1:
    playproc=sb1Sampler;
    break;
  case 2: case 3:
    playproc=sb2proSamplerHS;
    break;
  case 4:
    playproc=sb16Bit?sb16Player16:sb16Player8;
    break;
  }

  irqInit(sbIRQ, playproc, 0, 512);
  dmaStart(sb16Bit?sbDMA16:sbDMA, buf, len, 0x54);

  switch (sbUseVer)
  {
  case 1:
    outSB(0x24);
    outSB(0xFF);
    outSB(0xFF);
    inSB();
    break;
  case 2: case 3:
    if (sbUseVer==3)
      outSB(sbStereo?0xA8:0xA0);
    outSB(0x48);
    outSB(0xFF);
    outSB(0xFF);
    outSB(0x98);
    inSB();
    if (sbUseVer==3)
    {
      sbInSavL=readMixer(0x0C);
      writeMixer(0x0C, (sbSource==SMP_MIC)?0x21:(sbSource==SMP_LINEIN)?0x27:0x23);
    }
    break;
  case 4:
    outSB(sb16Bit?0xBE:0xCE);
    outSB((sbStereo?0x20:0x00)|(sbSignedOut?0x10:0x00));
    outSB(0xFC);
    outSB(0xFF);
    sbInSavL=readMixer(0x3D);
    sbInSavR=readMixer(0x3E);
    writeMixer(0x3D, (sbSource==SMP_MIC)?0x01:(sbSource==SMP_LINEIN)?0x10:0x04);
    writeMixer(0x3E, (sbSource==SMP_MIC)?0x01:(sbSource==SMP_LINEIN)?0x08:0x02);
    break;
  }

  smpGetBufPos=dmaGetBufPos;

  return 1;
}



static void sbStop()
{
  switch (sbUseVer)
  {
  case 3:
    writeMixer(0x0C, sbInSavL);
    break;
  case 4:
    writeMixer(0x3D, sbInSavL);
    writeMixer(0x3E, sbInSavR);
    break;
  }
  irqClose();
  dmaStop();
  initSBp();
  resetSB();
  resetSB16();
}



static int sbInit(const deviceinfo &card)
{
  if (!testPort(card.port, 1))
    return 0;
  resetSB();
  resetSB16();

  revstereo=!!(card.opt&SS_SB_REVSTEREO);

  sbPort=card.port;
  sbIRQ=card.irq;
  sbDMA=card.dma;
  sbDMA16=card.dma2;
  unsigned char ver=getVersion()>>8;
  if (card.subtype==-1)
    sbUseVer=ver;
  else
    sbUseVer=card.subtype;
  if (sbUseVer>ver)
    return 0;
  if ((sbUseVer==3)&&(ver==4))
    return 0;

/*
  if (!testirq(sbIRQ))
    return 0;
  if (!testdma(sbIRQ, sbDMA))
    return 0;
  if (sbUseVer==4)
    if (!testdma16(sbIRQ, sbDMA16))
      return 0;
*/

  smpSetOptions=sbSetOptions;
  smpSetSource=sbSetSource;
  smpSample=sbSample;
  smpStop=sbStop;

  smpSetOptions(65535, SMP_STEREO|SMP_16BIT);
  smpSetSource(SMP_LINEIN);

  return 1;
}

static void sbClose()
{
  smpSample=0;
}

static int sbDetect(deviceinfo &card)
{
  getcfg();
  if (card.port!=-1)
    sbPort=card.port;
  if (card.irq!=-1)
    sbIRQ=card.irq;
  if (card.dma!=-1)
    sbDMA=card.dma;
  if (card.subtype!=-1)
    sbUseVer=card.subtype;

  int i;
  if (sbPort==-1)
  {
    unsigned short ports[7]={0x220, 0x240, 0x260, 0x280, 0x210, 0x230, 0x250};
    for (i=0; i<(sizeof(ports)>>1); i++)
      if (testPort(ports[i],0))
        break;
    if (i==(sizeof(ports)>>1))
      return 0;
    sbPort=ports[i];
  }
  else
    if (!testPort(sbPort, 0))
      return 0;

  unsigned char ver=getVersion()>>8;
  if (sbUseVer==-1)
    sbUseVer=ver;
  if ((sbUseVer<1)||(sbUseVer>4))
    return 0;

  if (ver<sbUseVer)
    return 0;
  if ((ver==4)&&(sbUseVer==3))
    return 0;

  if (sbIRQ==-1)
  {
    if (sbUseVer!=4)
    {
      unsigned char irqs[5]={7, 5, 2, 3, 10};
      for (i=0; i<sizeof(irqs); i++)
        if (testirq(irqs[i]))
          break;
      if (i==sizeof(irqs))
        return 0;
      sbIRQ=irqs[i];
    }
    else
    {
      unsigned char a=readMixer(0x80);
      if (!(a&0x0F))
        return 0;
      sbIRQ=(a&2)?5:(a&4)?7:(a&1)?2:10;
    }
  }
//  else
//    if (!testirq(sbIRQ))
//      return 0;

  if (sbDMA==-1)
  {
    if (sbUseVer!=4)
    {
      unsigned char dmas[3]={1, 0, 3};
      for (i=0; i<sizeof(dmas); i++)
        if (testdma(sbIRQ, dmas[i]))
          break;
      if (i==sizeof(dmas))
        return 0;
      sbDMA=dmas[i];
    }
    else
    {
      unsigned char b=readMixer(0x81);
      if (!(b&0x0B))
        return 0;
      sbDMA=(b&2)?1:(b&1)?0:3;
    }
  }
//  else
//    if (!testdma(sbIRQ, sbDMA))
//      return 0;

  if (sbUseVer==4)
  {
    if (sbDMA16==-1)
    {
      unsigned char b=readMixer(0x81);
      if (!(b&0x0B))
        return 0;
      sbDMA16=(b&0x20)?5:(b&0x40)?6:(b&0x80)?7:sbDMA;
    }
//    else
//      if (!testdma16(sbIRQ, sbDMA16))
//        return 0;
  }
  else
    sbDMA16=-1;

  card.dev=&smpSoundBlaster;
  card.port=sbPort;
  card.port2=-1;
  card.irq=sbIRQ;
  card.irq2=-1;
  card.dma=sbDMA;
  card.dma2=sbDMA16;
  card.subtype=sbUseVer;
  card.mem=0;
  card.chan=(card.subtype<3)?1:2;

  return 1;
}


#include "devigen.h"
#include "psetting.h"

static unsigned long sbGetOpt(const char *sec)
{
  unsigned long opt=0;
  if (cfGetProfileBool(sec, "sbrevstereo", 0, 0))
    opt|=SS_SB_REVSTEREO;
  return opt;
}

extern "C" {
  sounddevice smpSoundBlaster={SS_SAMPLER, "SoundBlaster", sbDetect, sbInit, sbClose};
  devaddstruct smpSBAdd = {sbGetOpt, 0, 0, 0};
  char *dllinfo = "driver _smpSoundBlaster; addprocs _smpSBAdd";
}

