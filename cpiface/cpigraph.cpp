// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface graphic spectrum analysers
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -doj980928  Dirk Jagdmann <doj@cubic.org>
//    -changed code with fftanalyse to meet dependencies from changes in fft.cpp
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'
//  -fd981220   Felix Domke    <tmbinc@gmx.net>
//    -changes for LFB (and other faked banked-modes)

#define NO_CPIFACE_IMPORT
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include "poutput.h"
#include "cpiface.h"
#include "fft.h"

static unsigned char plStripePal1;
static unsigned char plStripePal2;

static unsigned long plAnalRate;
static unsigned short plAnalScale;

static short plStripeSpeed;
static short plAnalChan;
static short plStripePos;
static unsigned char plStripeBig;

static short plSampBuf[2048];
static unsigned short ana[1024];

static void plSetStripePals(short a, short b)
{
  plStripePal1=(a+8)%8;
  plStripePal2=(b+4)%4;
  outp(0x3c8,64);
  short i;

  switch(plStripePal2)
  {
  case 0:
    for (i=0; i<32; i++) { outp(0x3c9,2*i); outp(0x3c9,63); outp(0x3c9,0); }
    for (i=0; i<32; i++) { outp(0x3c9,63); outp(0x3c9,63-2*i); outp(0x3c9,0); }
    break;
  case 1:
    for (i=0; i<32; i++) { outp(0x3c9,0); outp(0x3c9,63); outp(0x3c9,2*i); }
    for (i=0; i<32; i++) { outp(0x3c9,0); outp(0x3c9,63-2*i); outp(0x3c9,63); }
    break;
  case 2:
    for (i=0; i<64; i++) { outp(0x3c9,63-i/2); outp(0x3c9,63-i/2); outp(0x3c9,63-i/2); }
    break;
  case 3:
    for (i=0; i<60; i++) { outp(0x3c9,63-i/2); outp(0x3c9,63-i/2); outp(0x3c9,63-i/2); }
    for (i=0; i<4; i++) { outp(0x3c9,63); outp(0x3c9,0); outp(0x3c9,0); }
    break;
  }

  switch(plStripePal1)
  {
  case 0:
    for (i=0; i<32; i++) { outp(0x3c9,0); outp(0x3c9,0); outp(0x3c9,i); }
    for (i=0; i<64; i++) { outp(0x3c9,i); outp(0x3c9,0); outp(0x3c9,31-i/2); }
    for (i=0; i<32; i++) { outp(0x3c9,63); outp(0x3c9,2*i); outp(0x3c9,0); }
    break;
  case 1:
    for (i=0; i<32; i++) { outp(0x3c9,0); outp(0x3c9,0); outp(0x3c9,i); }
    for (i=0; i<80; i++) { outp(0x3c9,4*i/5); outp(0x3c9,0); outp(0x3c9,31-2*i/5); }
    for (i=0; i<16; i++) { outp(0x3c9,63); outp(0x3c9,i*4); outp(0x3c9,0); }
    break;
  case 2:
    for (i=0; i<64; i++) { outp(0x3c9,0); outp(0x3c9,0); outp(0x3c9,i/2); }
    for (i=0; i<48; i++) { outp(0x3c9,4*i/3); outp(0x3c9,0); outp(0x3c9,31-2*i/3); }
    for (i=0; i<16; i++) { outp(0x3c9,63); outp(0x3c9,i*4); outp(0x3c9,0); }
    break;
  case 3:
    for (i=0; i<32; i++) { outp(0x3c9,0); outp(0x3c9,0); outp(0x3c9,i); }
    for (i=0; i<64; i++) { outp(0x3c9,0); outp(0x3c9,i); outp(0x3c9,31-i/2); }
    for (i=0; i<32; i++) { outp(0x3c9,2*i); outp(0x3c9,63); outp(0x3c9,2*i); }
    break;
  case 4:
    for (i=0; i<128; i++) { outp(0x3c9,i/2); outp(0x3c9,i/2); outp(0x3c9,i/2); }
    break;
  case 5:
    for (i=0; i<120; i++) { outp(0x3c9,i/2); outp(0x3c9,i/2); outp(0x3c9,i/2); }
    for (i=0; i<8; i++) { outp(0x3c9,63); outp(0x3c9,0); outp(0x3c9,0); }
    break;
  case 6:
    for (i=0; i<128; i++) { outp(0x3c9,63-i/2); outp(0x3c9,63-i/2); outp(0x3c9,63-i/2); }
    break;
  case 7:
    for (i=0; i<120; i++) { outp(0x3c9,63-i/2); outp(0x3c9,63-i/2); outp(0x3c9,63-i/2); }
    for (i=0; i<8; i++) { outp(0x3c9,63); outp(0x3c9,0); outp(0x3c9,0); }
    break;
  }
}

static void plPrepareStripes()
{
  plStripePos=0;

  plSetStripePals(plStripePal1, plStripePal2);

  if (plStripeBig)
  {
    plSetGraphPage(1);
    memset((char*)plVidMem+32*1024, 128, 32*1024);
    short i;
    for (i=2; i<10; i++)
    {
      plSetGraphPage(i);
      memset((char*)plVidMem, 128, 65536);
    }
    gdrawstr(42, 1, "scale: ", 7, 0x09, 0);
    plSetGraphPage(10);
    for (i=0; i<256; i++)
    {
      short j;
      for (j=0; j<16; j++)
        *(char*)(plVidMem+32768+64+i+j*1024)=(i>>1)+128;
    }
    for (i=0; i<128; i++)
    {
      short j;
      for (j=0; j<16; j++)
        *(char*)(plVidMem+32768+352+i+j*1024)=(i>>1)+64;
    }
  }
  else
  {
    plSetGraphPage(0);
    memset((char*)plVidMem+96*640, 128, 4096);
    plSetGraphPage(1);
    memset((char*)plVidMem, 128, 65536);
    plSetGraphPage(2);
    memset((char*)plVidMem, 128, 65536);
    plSetGraphPage(3);
    memset((char*)plVidMem, 128, 38912);

    gdrawstr(24, 1, "scale: ", 7, 0x09, 0);
    short i;
    plSetGraphPage(3);
    for (i=0; i<128; i++)
    {
      short j;
      for (j=0; j<16; j++)
        *(char*)(plVidMem+384*640+64-3*65536+i+j*640)=i+128;
    }
    for (i=0; i<64; i++)
    {
      short j;
      for (j=0; j<16; j++)
        *(char*)(plVidMem+384*640+232-3*65536+i+j*640)=i+64;
    }
  }
}

static void plPrepareStripeScr()
{
  if ((plAnalChan==2)&&!plGetLChanSample)
    plAnalChan=0;
  if (((plAnalChan==0)||(plAnalChan==1))&&!plGetMasterSample)
    plAnalChan=2;
  if ((plAnalChan==2)&&!plGetLChanSample)
    plAnalChan=0;

  char str[49];
  strcpy(str, "   ");
  if (plStripeBig)
    strcat(str, "big ");
  strcat(str, "graphic spectrum analyser");
  gdrawstr(4, 0, str, 48, 0x09, 0);

  strcpy(str, "max: ");
  convnum(plAnalRate>>1, str+strlen(str), 10, 5, 1);
  strcat(str, "Hz  (");
  strcat(str, plStripeSpeed?"fast, ":"fine, ");
  strcat(str, (plAnalChan==0)?"both":(plAnalChan==1)?"mid":"chan");
  strcat(str, ")");

  if (plStripeBig)
    gdrawstr(42, 96, str, 32, 0x09, 0);
  else
    gdrawstr(24, 48, str, 32, 0x09, 0);
}

static void reduceana(unsigned short *a, short len)
{
  int max=(1<<22)/plAnalScale;
  int i;
  for (i=0; i<len; i++)
    if (*a>=max)
      *a++=255;
    else
      *a++=((*a*plAnalScale)>>15)+128;
}

static char *vmx;

void drawgbar(unsigned long x, unsigned char h);
#pragma aux drawgbar parm [edi] [ecx] modify [eax] = \
  "mov eax,plVidMem" \
  "mov vmx,eax" \
  "add vmx,0x1000" \
  "add edi,eax" \
  "add edi,0xAD80" \
  "mov ax,0x4040" \
  "test ecx,ecx" \
  "jmp lp1e" \
"lp1:" \
    "mov word ptr [edi],ax" \
    "add ax,0x0101" \
    "sub edi,640" \
  "dec ecx" \
"lp1e:" \
  "jnz lp1" \
  \
  "jmp lp2e" \
"lp2:" \
    "mov word ptr [edi],0" \
    "sub edi,640" \
"lp2e:" \
  "cmp edi, vmx" \
  "jnb lp2"

void drawgbarb(unsigned long x, unsigned char h);
#pragma aux drawgbarb parm [edi] [ecx] modify [eax] = \
  "add edi,plVidMem" \
  "add edi,0xFC00" \
  "mov al,0x40" \
  "test ecx,ecx" \
  "jmp lp1e" \
"lp1:" \
    "mov byte ptr [edi],al" \
    "inc al" \
    "sub edi,1024" \
  "dec ecx" \
"lp1e:" \
  "jnz lp1" \
  \
  "jmp lp2e" \
"lp2:" \
    "mov byte ptr [edi],0" \
    "sub edi,1024" \
"lp2e:" \
  "cmp edi, plVidMem" \
  "jnb lp2"

static void plDrawStripes()
{
  if (plPause)
    return;
  unsigned short i;
  unsigned char *sp;
  static unsigned char linebuf[1088];
  if (plStripeBig)
  {
    memset(linebuf, 128, 1088);
    if (!plAnalChan)
    {
      plGetMasterSample(plSampBuf, 1024>>plStripeSpeed, plAnalRate, cpiGetSampleStereo);

      if (plStripeSpeed)
      {
        fftanalyseall(ana, plSampBuf, 2, 9);
        reduceana(ana, 256);
        sp=linebuf+511;
        for (i=0; i<256; i++)
          *sp--=*sp--=ana[i];
        fftanalyseall(ana, plSampBuf+1, 2, 9);
        reduceana(ana, 256);
        sp=linebuf+1087;
        for (i=0; i<256; i++)
          *sp--=*sp--=ana[i];
      }
      else
      {
        fftanalyseall(ana, plSampBuf, 2, 10);
        reduceana(ana, 512);
        sp=linebuf+511;
        for (i=0; i<512; i++)
          *sp--=ana[i];
        fftanalyseall(ana, plSampBuf+1, 2, 10);
        reduceana(ana, 512);
        sp=linebuf+1087;
        for (i=0; i<512; i++)
          *sp--=ana[i];
      }
    }
    else
    {
      if (plAnalChan!=2)
        plGetMasterSample(plSampBuf, 2048>>plStripeSpeed, plAnalRate, 0);
      else
        plGetLChanSample(plSelCh, plSampBuf, 2048>>plStripeSpeed, plAnalRate, 0);
      if (plStripeSpeed)
      {
        fftanalyseall(ana, plSampBuf, 1, 10);
        reduceana(ana, 512);
        sp=linebuf+1055;
        for (i=0; i<512; i++)
          *sp--=*sp--=ana[i];
      }
      else
      {
        fftanalyseall(ana, plSampBuf, 1, 11);
        reduceana(ana, 1024);
        sp=linebuf+1055;
        for (i=0; i<1024; i++)
          *sp--=ana[i];
      }
    }

    unsigned short pos[4];
    pos[0]=plStripePos;
    pos[1]=(plStripePos+12)%1024;
    pos[2]=(plStripePos+20)%1024;
    pos[3]=(plStripePos+32)%1024;
    unsigned char pg=1;
    plSetGraphPage(1);
    sp=(unsigned char *)(plVidMem+32*1024);
    for (i=0; i<544; i++, sp+=1024)
    {
      if ((sp-(unsigned char*)plVidMem)>=0x10000)
        sp-=plSetGraphPage(++pg);
      sp[pos[0]]=(linebuf[2*i]+linebuf[2*i+1])>>1;
      sp[pos[1]]=(sp[pos[1]]>>1)+64;
      sp[pos[2]]=(sp[pos[2]]>>1)+64;
      sp[pos[3]]=(sp[pos[3]]>>1)+64;
    }
    plSetGraphPage(11);
    if (!plAnalChan)
    {
      for (i=0; i<504; i++)
        drawgbarb(i, (linebuf[511-i]-128)>>1);
      for (i=0; i<16; i++)
        drawgbarb(512-8+i, 0);
      for (i=0; i<504; i++)
        drawgbarb(512+8+i, (linebuf[1087-i]-128)>>1);
    }
    else
      for (i=0; i<1024; i++)
        drawgbarb(i, (linebuf[1055-i]-128)>>1);
    plStripePos=(plStripePos+1)%1024;
  }
  else
  {
    memset(linebuf, 128, 272);
    if (!plAnalChan)
    {
      plGetMasterSample(plSampBuf, 256>>plStripeSpeed, plAnalRate, cpiGetSampleStereo);
      if (plStripeSpeed)
      {
        fftanalyseall(ana, plSampBuf, 2, 7);
        reduceana(ana, 64);
        sp=linebuf+127;
        for (i=0; i<64; i++)
          *sp--=*sp--=ana[i];
        fftanalyseall(ana, plSampBuf+1, 2, 7);
        reduceana(ana, 64);
        sp=linebuf+271;
        for (i=0; i<64; i++)
          *sp--=*sp--=ana[i];
      }
      else
      {
        fftanalyseall(ana, plSampBuf, 2, 8);
        reduceana(ana, 128);
        sp=linebuf+127;
        for (i=0; i<128; i++)
          *sp--=ana[i];
        fftanalyseall(ana, plSampBuf+1, 2, 8);
        reduceana(ana, 128);
        sp=linebuf+271;
        for (i=0; i<128; i++)
          *sp--=ana[i];
      }
    }
    else
    {
      if (plAnalChan!=2)
        plGetMasterSample(plSampBuf, 512>>plStripeSpeed, plAnalRate, 0);
      else
        plGetLChanSample(plSelCh, plSampBuf, 512>>plStripeSpeed, plAnalRate, 0);
      if (plStripeSpeed)
      {
        fftanalyseall(ana, plSampBuf, 1, 8);
        reduceana(ana, 128);
        sp=linebuf+263;
        for (i=0; i<128; i++)
          *sp--=*sp--=ana[i];
      }
      else
      {
        fftanalyseall(ana, plSampBuf, 1, 9);
        reduceana(ana, 256);
        sp=linebuf+263;
        for (i=0; i<256; i++)
          *sp--=ana[i];
      }
    }

    unsigned char pg=0;
    plSetGraphPage(0);
    sp=(unsigned char *)(plVidMem+96*640+plStripePos);
    for (i=0; i<272; i++, sp+=640)
    {
      if ((sp-(unsigned char*)plVidMem)>=0x10000)
        sp-=plSetGraphPage(++pg);
      *sp=linebuf[i];
    }
    pg=0;
    plSetGraphPage(0);
    sp=(unsigned char *)(plVidMem+96*640+(plStripePos+12)%640);
    for (i=0; i<272; i++, sp+=640)
    {
      if ((sp-(unsigned char*)plVidMem)>=0x10000)
        sp-=plSetGraphPage(++pg);
      *sp=(*sp>>1)+64;
    }
    pg=0;
    plSetGraphPage(0);
    sp=(unsigned char *)(plVidMem+96*640+(plStripePos+20)%640);
    for (i=0; i<272; i++, sp+=640)
    {
      if ((sp-(unsigned char*)plVidMem)>=0x10000)
        sp-=plSetGraphPage(++pg);
      *sp=(*sp>>1)+64;
    }
    pg=0;
    plSetGraphPage(0);
    sp=(unsigned char *)(plVidMem+96*640+(plStripePos+32)%640);
    for (i=0; i<272; i++, sp+=640)
    {
      if ((sp-(unsigned char*)plVidMem)>=0x10000)
        sp-=plSetGraphPage(++pg);
      *sp=(*sp>>1)+64;
    }

    plSetGraphPage(4);
    if (!plAnalChan)
    {
      for (i=0; i<128; i++)
        drawgbar(48+2*i, (linebuf[127-i]-128)>>1);
      for (i=0; i<16; i++)
        drawgbar(48+256+2*i, 0);
      for (i=0; i<128; i++)
        drawgbar(48+288+2*i, (linebuf[271-i]-128)>>1);
    }
    else
      for (i=0; i<272; i++)
        drawgbar(48+2*i, (linebuf[271-i]-128)>>1);
    plStripePos=(plStripePos+1)%640;
  }
}

static void strSetMode()
{
  cpiSetGraphMode(plStripeBig);
  plPrepareStripes();
  plPrepareStripeScr();
}

static int plStripeKey(unsigned short key)
{
  switch (key)
  {
  case 0x4900: //pgup
    plAnalRate=plAnalRate*30/32;
    plAnalRate=(plAnalRate>=64000)?64000:(plAnalRate<1024)?1024:plAnalRate;
    break;
  case 0x8400: //ctrl-pgup
    plAnalScale=plAnalScale*32/31;
    plAnalScale=(plAnalScale>=4096)?4096:(plAnalScale<256)?256:plAnalScale;
    break;
  case 0x5100: //pgdn
    plAnalRate=plAnalRate*32/30;
    plAnalRate=(plAnalRate>=64000)?64000:(plAnalRate<1024)?1024:plAnalRate;
    break;
  case 0x7600: //ctrl-pgdn
    plAnalScale=plAnalScale*31/32;
    plAnalScale=(plAnalScale>=4096)?4096:(plAnalScale<256)?256:plAnalScale;
    break;
  case 0x4700: //home
    plAnalRate=5512;
    plAnalScale=2048;
    plAnalChan=0;
    break;
  case 9: // tab
    plSetStripePals(plStripePal1+1, plStripePal2);
    break;
  case 0xA500: // alt-tab
    plSetStripePals(plStripePal1-1, plStripePal2);
    break;
  case 0x0F00: // shift-tab
    plSetStripePals(plStripePal1, plStripePal2+1);
    break;
  case 0x2200: // alt-g
    plStripeSpeed=!plStripeSpeed;
    break;
  case 'g':
    plAnalChan=(plAnalChan+1)%3;
    break;
  case 'G':
    plStripeBig=!plStripeBig;
    strSetMode();
    break;
  default:
    return 0;
  }
  plPrepareStripeScr();
  return 1;
}

static int plStripeInit()
{
  if (plVidType==vidNorm)
    return 0;
  plAnalRate=5512;
  plAnalScale=2048;
  plAnalChan=0;
  plStripeSpeed=0;
  return 1;
}

static void strDraw()
{
  cpiDrawGStrings();
  plDrawStripes();
}

static int strCan()
{
  if (!plGetLChanSample&&!plGetMasterSample)
    return 0;
  return 1;
}

static int strIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'g': case 'G':
    plStripeBig=(key=='G');
    cpiSetMode("graph");
    break;
  default:
    return 0;
  }
  return 1;
}

static int strEvent(int ev)
{
  switch (ev)
  {
  case cpievInit: return strCan();
  case cpievInitAll: return plStripeInit();
  }
  return 1;
}

extern "C"
{
  cpimoderegstruct cpiModeGraph = {"graph", strSetMode, strDraw, strIProcessKey, plStripeKey, strEvent};
}
