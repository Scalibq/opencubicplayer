// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface text mode spectrum analyser
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//
//  -doj980924  Dirk Jagdmann <doj@cubic.org>
//    -changed code with fftanalyse to meet dependencies from changes in fft.cpp
//  -fd981119   Felix Domke   <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'
//  -doj981220  Dirk Jagdmann <doj@cubic.org>
//    -generation of title string changed from str() functions to sprintf()
//  -fd9903030   Felix Domke   <tmbinc@gmx.net>
//    -added "kb"-mode, blinks the keyboard-leds if you want :)
//     (useless feature, i know, but it's FUN...)

#define NO_CPIFACE_IMPORT
#include <stdio.h>
#include "poutput.h"
#include "cpiface.h"
#include "psetting.h"
#include "fft.h"
#ifdef DOS32
#include "conio.h"
#endif

#ifdef DOS32
static int kbmode=1;            // KB just stands for "KeyBoard", you know?
                                // usually NUM-, CAPS, SCROLL-lock
static int kbbit[3][2]={32, 2, 64, 4, 16, 8};  // magic, eh?? :)
                                // one value for bios, another for the keyboardport.. :)
static unsigned char oldbios;
#endif

#ifndef MEKKACOL

#define COLBACK 0x00
#define COLTITLE 0x01
#define COLTITLEH 0x09
#define COLSET0 0x090B0A
#define COLSET1 0x0C0E0A
#define COLSET2 0x070707
#define COLSET3 0x0A0A0A

#else

#define COLBACK 0xFF
#define COLTITLE 0xF9
#define COLTITLEH 0xF1
#define COLSET0 0xF1F3F2
#define COLSET1 0xF4F6F2
#define COLSET2 0xF8F8F8
#define COLSET3 0xF2F2F2

#endif

static int analactive;
static unsigned long plAnalRate;
static short plAnalFirstLine;
static short plAnalHeight;
static short plAnalWidth;
static short plAnalCol;
static unsigned short plAnalScale;

static short plAnalChan;

static short plSampBuf[2048];
static unsigned short ana[1024];

static void plDrawFFT(char sel)
{
  if ((plAnalChan==2)&&!plGetLChanSample)
    plAnalChan=0;
  if (((plAnalChan==0)||(plAnalChan==1))&&!plGetMasterSample)
    plAnalChan=2;
  if ((plAnalChan==2)&&!plGetLChanSample)
    plAnalChan=0;

  char str[80]; // contains the title string
  char *s; // pointer to temp string

  // make the *s point to the right string
  if (plAnalChan==2)
  {
     s="single channel:     ";
     sprintf(s+16, "%3i", plSelCh+1);
  }
  else
  {
    if (plAnalChan)
      s="master channel, mono";
    else
      s="master channel, stereo";
  }

  // print the title string
  sprintf( str, "  spectrum analyser, step: %3iHz, max: %5iHz, %s",
           plAnalRate>>((plAnalWidth>=128)?8:7),
           plAnalRate>>1,
           s
  );

  displaystr(plAnalFirstLine-1, 0, sel?COLTITLEH:COLTITLE, str, plAnalWidth);

  unsigned char wid=(plAnalWidth>=128)?128:64;
  unsigned char ofs=(plAnalWidth-wid)>>1;

  unsigned long col=(plAnalCol==0)?COLSET0:(plAnalCol==1)?COLSET1:(plAnalCol==2)?COLSET2:COLSET3;

  int i;
  for (i=0; i<plAnalHeight; i++)
  {
    displaystr(i+plAnalFirstLine, 0, COLBACK, "", ofs);
    displaystr(i+plAnalFirstLine, plAnalWidth-ofs, COLBACK, "", ofs);
  }
#ifdef DOS32
  int kblvl;
#endif

  if (!plAnalChan)
  {
    plGetMasterSample(plSampBuf, wid*2, plAnalRate, cpiGetSampleStereo);
    if (plAnalHeight&1)
      displaystr(plAnalFirstLine+plAnalHeight-1, ofs, COLBACK, "", plAnalWidth-2*ofs);
    unsigned short wh2=plAnalHeight>>1;
    unsigned short fl=plAnalFirstLine+wh2-1;

    fftanalyseall(ana, plSampBuf, 2, (wid==64)?7:8);
    for (i=0; i<wid; i++)
      drawbar(i+ofs, fl, wh2, (((ana[i]*plAnalScale)>>11)*wh2)>>8, col);
#ifdef DOS32
    int maxlvl=0;
    int fs=plAnalRate>>((plAnalWidth>=128)?8:7);
    for (i=0; i<wid; i++)
    {
      int weight;
      switch (kbmode)
      {
        case 1:                 // everything
          weight=256;
          break;
        case 2:                 // BASS (i.e. everything under 100Hz, i know this formula suxx, make a better one :)
          weight=(i*fs<50)?256:0;
          break;
        default:
          weight=0;
      }
      if ((ana[i]*weight)>maxlvl)
        maxlvl=ana[i]*weight;
    }
#endif
    fl+=wh2;
    fftanalyseall(ana, plSampBuf+1, 2, (wid==64)?7:8);
    for (i=0; i<wid; i++)
      drawbar(i+ofs, fl, wh2, (((ana[i]*plAnalScale)>>11)*wh2)>>8, col);
#ifdef DOS32
    for (i=0; i<wid; i++)
    {
      int weight;
      switch (kbmode)
      {
        case 1:                 // everything
          weight=256;
          break;
        case 2:                 // BASS (i.e. everything under 100Hz, i know this formula suxx, make a better one :)
          weight=(i*fs<50)?256:0;
          break;
        default:
          weight=0;
      }
      if ((ana[i]*weight)>maxlvl)
        maxlvl=ana[i]*weight;
    }
    kblvl=(maxlvl*plAnalScale)/256;
#endif
  }
  else
  {
    if (plAnalChan!=2)
      plGetMasterSample(plSampBuf, 2*wid, plAnalRate, 0);
    else
      plGetLChanSample(plSelCh, plSampBuf, 2*wid, plAnalRate, 0);
    fftanalyseall(ana, plSampBuf, 1, (wid==64)?7:8);
    for (i=0; i<wid; i++)
      drawbar(i+ofs, plAnalFirstLine+plAnalHeight-1, plAnalHeight, (((ana[i]*plAnalScale)>>11)*plAnalHeight)>>8, col);
#ifdef DOS32
    int maxlvl=0;
    int fs=plAnalRate>>((plAnalWidth>=128)?8:7);
    for (i=0; i<wid; i++)
    {
      int weight;
      switch (kbmode)
      {
        case 1:                 // everything
          weight=256;
          break;
        case 2:                 // BASS (i.e. everything under 76Hz, i know this formula suxx, make a better one :)
          weight=(i*fs<76)?256:0;
          break;
        default:
          weight=0;
      }
      if ((ana[i]*weight)>maxlvl)
        maxlvl=ana[i]*weight;
    }
    kblvl=(maxlvl*plAnalScale)/256;
#endif
  }
#ifdef DOS32
  if (kbmode)
  {
    kblvl>>=19;
    kblvl-=3;
    unsigned char bios=(*((unsigned char*)0x417))&~(kbbit[0][0]|kbbit[1][0]|kbbit[2][0]);
    int kb=0;
    for(int i=0; i<3; i++)
      if (kblvl>(1<<i))
      {
        bios|=kbbit[i][0];
        kb|=kbbit[i][1];
      }
/*    outp(0x60, 0xED);
    while (inp(0x64)&2);                        not a good idea...
    outp(0x60, kb);                     (keyboard stops scanning keys on 0edh... :)
    while (inp(0x64)&2);*/  
    *((unsigned char*)0x417)=bios;
  }
#endif
}


static void AnalSetWin(int, int wid, int ypos, int hgt)
{
  plAnalFirstLine=ypos+1;
  plAnalHeight=hgt-1;
  plAnalWidth=wid;
}

static int AnalGetWin(cpitextmodequerystruct &q)
{
  if (!analactive)
    return 0;

  q.hgtmin=3;
  q.hgtmax=100;
  q.xmode=1;
  q.size=1;
  q.top=1;
  q.killprio=112;
  q.viewprio=128;
  return 1;
}

static void AnalDraw(int focus)
{
  plDrawFFT(focus);
}

static int AnalIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'a': case 'A':
    cpiTextSetMode("anal");
    break;
  case 'x': case 'X':
    analactive=1;
    return 0;
  case 0x2d00: //alt-x
    analactive=0;
    return 0;
  default:
    return 0;
  }
  return 1;
}

static int AnalAProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'a': case 'A':
    analactive=!analactive;
#ifdef DOS32
    if (kbmode && !analactive) *((unsigned char*)0x417)=oldbios;
#endif
    cpiTextRecalc();
    break;
  case 0x7600: //ctrl-pgdn
    plAnalScale=plAnalScale*31/32;
    plAnalScale=(plAnalScale>=4096)?4096:(plAnalScale<256)?256:plAnalScale;
    break;
  case 0x4900: //pgup
    plAnalRate=plAnalRate*30/32;
    plAnalRate=(plAnalRate>=64000)?64000:(plAnalRate<1024)?1024:plAnalRate;
    break;
  case 0x5100: //pgdn
    plAnalRate=plAnalRate*32/30;
    plAnalRate=(plAnalRate>=64000)?64000:(plAnalRate<1024)?1024:plAnalRate;
    break;
  case 0x4700: //home
    plAnalRate=5512;
    plAnalScale=2048;
    plAnalChan=0;
    break;
  case 0x1E00:
    plAnalChan=(plAnalChan+1)%3;
    break;
  case 9: // tab
  case 0x0F00: // shift-tab
    plAnalCol=(plAnalCol+1)%4;
    break;
  case 0xA500:
    plAnalCol=(plAnalCol+3)%4;
    break;
  default:
    return 0;
  }
  return 1;
}

static int AnalInit()
{
#ifdef DOS32
  oldbios=*((unsigned char*)0x417);
  kbmode=cfGetProfileInt2(cfScreenSec, "screen", "kbanalyser", 0, 0);
#endif
  plAnalRate=5512;
  plAnalScale=2048;
  plAnalChan=0;
  analactive=cfGetProfileBool2(cfScreenSec, "screen", "analyser", 0, 0);
  return 1;
}

static void AnalClose()
{
#ifdef DOS32
  if (kbmode) *((unsigned char*)0x417)=oldbios;
#endif
}

static int AnalCan()
{
  if (!plGetMasterSample&&!plGetLChanSample)
    return 0;
  return 1;
}

static void AnalSetMode()
{
  plSetBarFont();
}

static int AnalEvent(int ev)
{
  switch (ev)
  {
  case cpievInit: return AnalCan();
  case cpievInitAll: return AnalInit();
  case cpievDoneAll: AnalClose(); return 1;
  case cpievSetMode: AnalSetMode(); return 1;
#ifdef DOS32
  case cpievClose:
    if (kbmode && analactive) *((unsigned char*)0x417)=32;
    //  why does using oldbios instead of 32 crash?!?!? arg. -fd
    return 1;
#endif
  }
  return 1;
}

extern "C"
{
  cpitextmoderegstruct cpiTModeAnal = {"anal", AnalGetWin, AnalSetWin, AnalDraw, AnalIProcessKey, AnalAProcessKey, AnalEvent};
}
