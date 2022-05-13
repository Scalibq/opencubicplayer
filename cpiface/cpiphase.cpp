// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIFace phase graphs mode
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -doj980928  Dirk Jagdmann  <doj@cubic.org>
//    -added cpipic.h and isqrt.cpp to the #include list
//  -fd981119   Felix Domke    <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'
//  -fd981220   Felix Domke    <tmbinc@gmx.net>
//    -changes for LFB (and other faked banked-modes)

#define NO_CPIFACE_IMPORT
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include "poutput.h"
#include "cpiface.h"
#include "imsrtns.h"
#include "cpipic.h"
#include "isqrt.cpp" // ugly but working

#define HIGHLIGHT 11
#define MAXDOTS (64*128)
#define MAXSAMPLEN 1024

static unsigned long plOszRate;
static unsigned char plOszMono;
static unsigned char plOszChan;
static short plSampBuf[MAXSAMPLEN+2];
static int plScopesAmp;
static int plScopesAmp2;
static int plScopesRatio;

static unsigned long replacebuf[MAXDOTS*2];
static unsigned long sorttemp[MAXDOTS*2];
static unsigned long *replacebufpos;
static unsigned long dotbuf[MAXDOTS];
static unsigned long *dotbufpos;

static int scopenx,scopeny;
static int scopedx,scopedy;
static int scopefx,scopefy;
static int samples;


static void plPrepareScopes()
{
  int i;
  if (plOpenCPPict)
  {
    outp(0x3c8, 16);
    for (i=0; i<720; i++)
      outp(0x3c9, plOpenCPPal[i+48]);

    plSetGraphPage(0);
    memcpy((char*)plVidMem+96*640, plOpenCPPict, 4096);
    plSetGraphPage(1);
    memcpy((char*)plVidMem, plOpenCPPict+4096, 65536);
    plSetGraphPage(2);
    memcpy((char*)plVidMem, plOpenCPPict+69632, 65536);
    plSetGraphPage(3);
    memcpy((char*)plVidMem, plOpenCPPict+135168, 65536);
    plSetGraphPage(4);
    memcpy((char*)plVidMem, plOpenCPPict+200704, 45056);
  }
  else
  {
    plSetGraphPage(0);
    memset((char*)plVidMem+96*640, 0, 4096);
    plSetGraphPage(1);
    memset((char*)plVidMem, 0, 65536);
    plSetGraphPage(2);
    memset((char*)plVidMem, 0, 65536);
    plSetGraphPage(3);
    memset((char*)plVidMem, 0, 65536);
    plSetGraphPage(4);
    memset((char*)plVidMem, 0, 45056);
  }

  replacebufpos=replacebuf;
  dotbufpos=dotbuf;
}

static void plPrepareScopeScr()
{
  if ((plOszChan==2)&&!plGetMasterSample)
    plOszChan=3;
  if (((plOszChan==3)||(plOszChan==0))&&!plGetLChanSample)
    plOszChan=1;
  if ((plOszChan==1)&&!plGetPChanSample)
    plOszChan=2;
  if ((plOszChan==2)&&!plGetMasterSample)
    plOszChan=3;

  if (plOszChan==0)
  {
    scopenx=isqrt(plNLChan*2);
    scopeny=(plNLChan+scopenx-1)/scopenx;
    scopedx=640/scopenx;
    scopedy=384/scopeny;
    scopefx=(plScopesAmp*isqrt(plNLChan<<4))>>2;
    scopefy=(scopefx*plScopesRatio)>>5;
    samples=64*128/plNLChan;
    if (samples>1024)
      samples=1024;
  }
  else
  if (plOszChan==1)
  {
    scopenx=isqrt(plNPChan*2);
    scopeny=(plNPChan+scopenx-1)/scopenx;
    scopedx=640/scopenx;
    scopedy=384/scopeny;
    scopefx=(plScopesAmp*isqrt(plNPChan<<4))>>2;
    scopefy=(scopefx*plScopesRatio)>>5;
    samples=64*128/plNPChan;
    if (samples>1024)
      samples=1024;
  }
  else
  if (plOszChan==2)
  {
    scopenx=plOszMono?1:2;
    scopeny=1;
    scopedx=640/scopenx;
    scopedy=384;
    scopefx=plScopesAmp2;
    scopefy=(scopefx*plScopesRatio)>>5;
    samples=1024/scopenx;
  }
  else
  {
    scopenx=1;
    scopeny=1;
    scopedx=640;
    scopedy=384;
    scopefx=plScopesAmp*plNLChan;
    scopefy=(scopefx*plScopesRatio)>>5;
    samples=1024;
  }

  char str[49];
  strcpy(str, "   phase graphs: ");
  if (plOszChan==2)
  {
    strcat(str, "master");
    if (plOszMono)
      strcat(str, ", mono");
    else
      strcat(str, ", stereo");
  }
  else
  {
    if (plOszChan==0)
      strcat(str, "logical");
    else
    if (plOszChan==1)
      strcat(str, "physical");
    else
      strcat(str, "solo");
  }
  gdrawstr(4, 0, str, 48, 0x09, 0);
}

static int plScopesKey(unsigned short key)
{
  switch (key)
  {
  case 0x8400: //ctrl-pgup
//    plOszRate=plOszRate*31/32;
//    plOszRate=(plOszRate>=512000)?256000:(plOszRate<2048)?2048:plOszRate;
    plScopesRatio=plScopesRatio*32/31;
    plScopesRatio=(plScopesRatio>=1024)?1024:(plScopesRatio<64)?64:plScopesRatio;
    break;
  case 0x7600: //ctrl-pgdn
//    plOszRate=plOszRate*32/31;
//    plOszRate=(plOszRate>=256000)?256000:(plOszRate<2048)?2048:plOszRate;
    plScopesRatio=plScopesRatio*31/32;
    plScopesRatio=(plScopesRatio>=1024)?1024:(plScopesRatio<64)?64:plScopesRatio;
    break;
  case 0x4900: //pgup
    if (plOszChan==2)
    {
      plScopesAmp2=plScopesAmp2*32/31;
      plScopesAmp2=(plScopesAmp2>=4096)?4096:(plScopesAmp2<64)?64:plScopesAmp2;
    }
    else
    {
      plScopesAmp=plScopesAmp*32/31;
      plScopesAmp=(plScopesAmp>=4096)?4096:(plScopesAmp<64)?64:plScopesAmp;
    }
    break;
  case 0x5100: //pgdn
    if (plOszChan==2)
    {
      plScopesAmp2=plScopesAmp2*31/32;
      plScopesAmp2=(plScopesAmp2>=4096)?4096:(plScopesAmp2<64)?64:plScopesAmp2;
    }
    else
    {
      plScopesAmp=plScopesAmp*31/32;
      plScopesAmp=(plScopesAmp>=4096)?4096:(plScopesAmp<64)?64:plScopesAmp;
    }
    break;
  case 0x4700: //home
    plScopesAmp=512;
    plScopesAmp2=512;
    plScopesRatio=256;
    plOszRate=44100;
    break;
  case 9: // tab
  case 0x0F00: // shift-tab
  case 0xA500:
  case 0x1800: // alt-o
    if (plOszChan==2)
    {
      plOszMono=!plOszMono;
      plPrepareScopes();
    }
    break;
  case 'b': case 'B':
    plOszChan=(plOszChan+1)%4;
    plPrepareScopes();
    plChanChanged=1;
    break;
  default:
    return 0;
  }
  plPrepareScopeScr();
  return 1;
}

static int plScopesInit()
{
  if (plVidType==vidNorm)
    return 0;
  plOszRate=44100;
  plScopesAmp=512;
  plScopesAmp2=512;
  plScopesRatio=256;
  plOszMono=0;
  return 1;
}

static void drawscope(int x, int y, const short *in, int num, unsigned char col, int step)
{
  y+=96;
  unsigned long colmask=col<<24;

  int i;
  for (i=0; i<num; i++)
  {
    int x1=x+((*in*scopefx)>>16);
    int y1=y+(((in[step]-*in)*scopefy)>>16);
    if ((x1>=0)&&(x1<640)&&(y1>=96)&&(y1<480))
      *dotbufpos++=(y1*640+x1)|colmask;
    in+=step;
  }
}

static void radix(unsigned long *dest, unsigned long *source, long n, int byte)
{
  unsigned long count[256];
  unsigned long **index=(unsigned long**)count;
  memsetd(count, 0, 256);
  int i;
  for (i=0; i<n; i++)
    count[((unsigned char*)&source[i])[byte]]++;

  unsigned long *dp=dest;
  if (byte==3)
  {
    unsigned long *ndp;
    for (i=48; i<256; i++)
    {
      ndp=dp+count[i];
      index[i]=dp;
      dp=ndp;
    }
    for (i=0; i<48; i++)
      if (i!=HIGHLIGHT)
      {
        ndp=dp+count[i];
        index[i]=dp;
        dp=ndp;
      }
    ndp=dp+count[HIGHLIGHT];
    index[HIGHLIGHT]=dp;
    dp=ndp;
  }
  else
    for (i=0; i<256; i++)
    {
      unsigned long *ndp=dp+count[i];
      index[i]=dp;
      dp=ndp;
    }
  for (i=0; i<n; i++)
    *index[((unsigned char*)&source[i])[byte]]++=source[i];
}



static void drawframe()
{
  memcpy(replacebufpos,dotbuf,(dotbufpos-dotbuf)*4);
  replacebufpos+=dotbufpos-dotbuf;

  int n=replacebufpos-replacebuf;

  radix(sorttemp, replacebuf, n, 3);
  radix(replacebuf, sorttemp, n, 0);
  radix(sorttemp, replacebuf, n, 1);
  radix(replacebuf, sorttemp, n, 2);

  unsigned char *b;
  int pg=-1;
  for (b=(unsigned char*)replacebuf; b<(unsigned char*)replacebufpos; b+=4)
  {
    if (pg!=b[2])
      plSetGraphPage(pg=b[2]);
    *(unsigned char*)(plVidMem+*(unsigned short*)b)=b[3];
  }

  unsigned long *bp;
  memcpy(replacebuf,dotbuf,(dotbufpos-dotbuf)*4);
  replacebufpos=replacebuf+(dotbufpos-dotbuf);
  if (plOpenCPPict)
    for (bp=replacebuf; bp<replacebufpos; bp++)
      ((unsigned char*)bp)[3]=plOpenCPPict[(*bp&0xFFFFFF)-96*640];
  else
    for (bp=replacebuf; bp<replacebufpos; bp++)
      ((unsigned char*)bp)[3]=0;

  dotbufpos=dotbuf;
}

static void plDrawScopes()
{
  int i;
  if (plOszChan==2)
  {
    plGetMasterSample(plSampBuf, samples+1, plOszRate, (plOszMono?0:cpiGetSampleStereo)|cpiGetSampleHQ);
    for (i=0; i<scopenx; i++)
      drawscope(scopedx/2+scopedx*i, scopedy/2, plSampBuf+i, samples, 15, scopenx);
  }
  else
  if (plOszChan==1)
  {
    int i;
    for (i=0; i<plNPChan; i++)
    {
      int paus=plGetPChanSample(i, plSampBuf, samples+1, plOszRate, cpiGetSampleHQ);
      drawscope((i%scopenx)*scopedx+scopedx/2, scopedy*(i/scopenx)+scopedy/2, plSampBuf, samples, paus?8:15, 1);
    }
  }
  else
  if (plOszChan==3)
  {
    plGetLChanSample(plSelCh, plSampBuf, samples+1, plOszRate, cpiGetSampleHQ);
    drawscope(scopedx/2, scopedy/2, plSampBuf, samples, plMuteCh[plSelCh]?7:15, 1);
  }
  else
  if (plOszChan==0)
  {
    int i;
    for (i=0; i<plNLChan; i++)
    {
      plGetLChanSample(i, plSampBuf, samples+1, plOszRate, cpiGetSampleHQ);
      drawscope((i%scopenx)*scopedx+scopedx/2, scopedy*(i/scopenx)+scopedy/2, plSampBuf, samples, (plSelCh==i)?plMuteCh[i]?(HIGHLIGHT&7):HIGHLIGHT:plMuteCh[i]?8:15, 1);
    }
  }
  drawframe();
}

static void scoDraw()
{
  cpiDrawGStrings();
  plDrawScopes();
}

static void scoSetMode()
{
  plReadOpenCPPic();
  cpiSetGraphMode(0);
  plPrepareScopes();
  plPrepareScopeScr();
}

static int scoCan()
{
  if (!plGetLChanSample&&!plGetPChanSample&&!plGetMasterSample)
    return 0;
  return 1;
}

static int scoIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'b': case 'B':
    cpiSetMode("phase");
    break;
  default:
    return 0;
  }
  return 1;
}

static int scoEvent(int ev)
{
  switch (ev)
  {
  case cpievInit: return scoCan();
  case cpievInitAll: return plScopesInit();
  }
  return 1;
}

extern "C"
{
  cpimoderegstruct cpiModePhase = {"phase", scoSetMode, scoDraw, scoIProcessKey, plScopesKey, scoEvent};
}
