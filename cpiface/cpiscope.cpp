// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIFace oscilloscope mode
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
#include "cpipic.h"
#include "isqrt.cpp" // igly but working

#define MAXDOTS 16384
#define MAXSAMPLEN 1280
#define MAXVIEWCHAN2 16

static unsigned long plOszRate;
static unsigned char plOszTrigger;
static unsigned char plOszMono;
static unsigned char plOszChan;
static short plSampBuf[MAXSAMPLEN];
static short scopes[MAXDOTS];
static int plScopesAmp;
static int plScopesAmp2;

static unsigned long replacbuf[640*2];

static int scopenx,scopeny;
static int scopesx,scopesy,scopetlen;
static int scopedx,scopedy;

static char scaleshift=0;
static short scaledmax;
static int scalemax;
static short scaletab[1024];

static void makescaletab(int amp, int max)
{
  for (scaleshift=0; scaleshift<6; scaleshift++)
    if ((amp>>(7-scaleshift))>max)
      break;
  scaledmax=max*80;
  scalemax=512<<scaleshift;
  int i;
  for (i=-512; i<512; i++)
  {
    int r=(i*amp)>>(16-scaleshift);
    if (r<-max)
      r=-max;
    if (r>max)
      r=max;
    scaletab[512+i]=r*80;
  }
}

static void doscale(short *buf, int len)
{
  int i;
  for (i=0; i<len; i++)
  {
    if (*buf<-scalemax)
      *buf=-scaledmax;
    else
    if (*buf>=scalemax)
      *buf=scaledmax;
    else
      *buf=scaletab[512+(*buf>>scaleshift)];
    buf++;
  }
}


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

  memset(scopes, 0, MAXDOTS*2);
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
    int chann=plNLChan;
    if (chann>(MAXVIEWCHAN2*2))
      chann=(MAXVIEWCHAN2*2);
    scopenx=2;
    scopeny=(chann+scopenx-1)/scopenx;
    scopedx=640/scopenx;
    scopedy=384/scopeny;
    scopesx=512/scopenx;
    scopesy=336/scopeny;
    scopetlen=scopesx/2;
    makescaletab(plScopesAmp*plNPChan/scopeny, scopesy/2);
  }
  else
  if (plOszChan==1)
  {
    scopenx=isqrt((plNPChan+2)/3);
    scopeny=(plNPChan+scopenx-1)/scopenx;
    scopedx=640/scopenx;
    scopedy=384/scopeny;
    scopesx=512/scopenx;
    scopesy=336/scopeny;
    scopetlen=scopesx/2;
    makescaletab(plScopesAmp*plNPChan/scopeny, scopesy/2);
  }
  else
  if (plOszChan==2)
  {
    scopenx=1;
    scopeny=plOszMono?1:2;
    scopedx=640;
    scopedy=384/scopeny;
    scopesx=640;
    scopesy=382/scopeny;
    scopetlen=scopesx/2;
    makescaletab(plScopesAmp2/scopeny, scopesy/2);
  }
  else
  {
    scopenx=1;
    scopeny=1;
    scopedx=640;
    scopedy=384;
    scopesx=640;
    scopesy=382;
    scopetlen=scopesx;
    makescaletab(plScopesAmp*plNPChan, scopesy/2);
  }

  char str[49];
  strcpy(str, "   scopes: ");
  convnum(plOszRate/scopenx, str+strlen(str), 10, 6, 1);
  strcat(str, " pix/s");
  strcat(str, ", ");
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
    if (plOszTrigger)
      strcat(str, ", triggered");
  }
  gdrawstr(4, 0, str, 48, 0x09, 0);
}

static void plotbuf(unsigned long *buf, unsigned short len)
{
  unsigned char j;
  int i;
  unsigned char *b=(unsigned char*)buf;
  unsigned char used[5];
  used[0]=used[1]=used[2]=used[3]=used[4]=0;
  for (i=0; i<len; i++, b+=4)
    used[b[2]]=1;
  for (j=0; j<5; j++)
  {
    if (!used[j])
      continue;
    plSetGraphPage(j);
    b=(unsigned char*)buf;
    for (i=0; i<len; i++, b+=4)
      if (b[2]==j)
        *(unsigned char*)(plVidMem+*(unsigned short*)b)=b[3];
  }
}

static void drawscope(int x, int y, const short *in, short *out, int num, unsigned char col, int step)
{
  unsigned long *buf=replacbuf;
  unsigned long scrpos=(y+96)*640+x;
  unsigned char *pic=plOpenCPPict-96*640;
  unsigned long colmask=col<<24;

  int i;
  if (plOpenCPPict)
    for (i=0; i<num; i++)
    {
      *buf++=scrpos+(*out<<3);
      ((unsigned char*)buf)[-1]=pic[buf[-1]];
      *buf++=(scrpos+(*in<<3))|colmask;
      *out=*in;
      out+=step;
      in+=step;
      scrpos++;
    }
  else
    for (i=0; i<num; i++)
    {
      *buf++=scrpos+(*out<<3);
      *buf++=(scrpos+(*in<<3))|colmask;
      *out=*in;
      out+=step;
      in+=step;
      scrpos++;
    }
  plotbuf(replacbuf, buf-replacbuf);
}

static void removescope(int x, int y, short *out, int num)
{
  unsigned long *buf=replacbuf;
  unsigned long scrpos=(y+96)*640+x;
  unsigned char *pic=plOpenCPPict-96*640;

  int i;
  if (plOpenCPPict)
    for (i=0; i<num; i++)
    {
      *buf++=scrpos+(*out<<3);
      ((unsigned char*)buf)[-1]=pic[buf[-1]];
      *out++=0;
      scrpos++;
    }
  else
    for (i=0; i<num; i++)
    {
      *buf++=scrpos+(*out<<3);
      *out++=0;
      scrpos++;
    }
  plotbuf(replacbuf, buf-replacbuf);
}

static int plScopesKey(unsigned short key)
{
  switch (key)
  {
  case 0x4900: //pgup
    plOszRate=plOszRate*31/32;
    plOszRate=(plOszRate>=512000)?256000:(plOszRate<2048)?2048:plOszRate;
    break;
  case 0x5100: //pgdn
    plOszRate=plOszRate*32/31;
    plOszRate=(plOszRate>=256000)?256000:(plOszRate<2048)?2048:plOszRate;
    break;
  case 0x8400: //ctrl-pgup
    if (plOszChan==2)
    {
      plScopesAmp2=plScopesAmp2*32/31;
      plScopesAmp2=(plScopesAmp2>=2000)?2000:(plScopesAmp2<100)?100:plScopesAmp2;
    }
    else
    {
      plScopesAmp=plScopesAmp*32/31;
      plScopesAmp=(plScopesAmp>=1000)?1000:(plScopesAmp<50)?50:plScopesAmp;
    }
    break;
  case 0x7600: //ctrl-pgdn
    if (plOszChan==2)
    {
      plScopesAmp2=plScopesAmp2*31/32;
      plScopesAmp2=(plScopesAmp2>=2000)?2000:(plScopesAmp2<100)?100:plScopesAmp2;
    }
    else
    {
      plScopesAmp=plScopesAmp*31/32;
      plScopesAmp=(plScopesAmp>=1000)?1000:(plScopesAmp<50)?50:plScopesAmp;
    }
    break;
  case 0x4700: //home
    plScopesAmp=320;
    plScopesAmp2=640;
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
    else
      plOszTrigger=!plOszTrigger;
    break;
  case 'o': case 'O':
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
  plOszTrigger=1;
  plScopesAmp=320;
  plScopesAmp2=640;
  plOszMono=0;
  return 1;
}

static void plDrawScopes()
{
  if (plOszChan==0)
  {
    int chann=(plNLChan+1)/2;
    if (chann>MAXVIEWCHAN2)
      chann=MAXVIEWCHAN2;
    int chan0=(plSelCh/2)-(chann/2);
    if ((chan0+chann)>=((plNLChan+1)/2))
      chan0=((plNLChan+1)/2)-chann;
    if (chan0<0)
      chan0=0;
    chan0*=2;
    chann*=2;

    int i;
    for (i=0; i<chann; i++)
    {
      int x=(plPanType?(((i+i+i+chan0)&2)>>1):(i&1));
      if ((i+chan0)==plNLChan)
      {
        if (plChanChanged)
        {
          gdrawchar8p(x?616:8, 96+scopedy*(i>>1)+scopedy/2-3, ' ', 0, plOpenCPPict?(plOpenCPPict-96*640):0);
          gdrawchar8p(x?624:16, 96+scopedy*(i>>1)+scopedy/2-3, ' ', 0, plOpenCPPict?(plOpenCPPict-96*640):0);
        }
        removescope((scopedx-scopesx)/2+x*scopedx, scopedy*(i/scopenx)+scopedy/2, scopes+((i&~1)|x)*scopesx, scopesx);
        break;
      }
      plGetLChanSample(i+chan0, plSampBuf, scopesx+(plOszTrigger?scopetlen:0), plOszRate/scopenx, 0);
      int paus=plMuteCh[i];
      if (plChanChanged)
      {
        gdrawchar8p(x?616:8, 96+scopedy*(i>>1)+scopedy/2-3, '0'+(i+1+chan0)/10, ((i+chan0)==plSelCh)?15:paus?8:7, plOpenCPPict?(plOpenCPPict-96*640):0);
        gdrawchar8p(x?624:16, 96+scopedy*(i>>1)+scopedy/2-3, '0'+(i+1+chan0)%10, ((i+chan0)==plSelCh)?15:paus?8:7, plOpenCPPict?(plOpenCPPict-96*640):0);
      }

      short *bp=plSampBuf;
      if (plOszTrigger)
      {
        int j;
        for (j=0; j<scopetlen; j++)
          if ((bp[0]>0)&&(bp[1]<=0)&&(bp[2]<=0))
            break;
          else
            bp++;
        if (j==scopetlen)
          bp=plSampBuf;
        else
          bp++;
      }

      doscale(bp, scopesx);

      drawscope((scopedx-scopesx)/2+x*scopedx, scopedy*(i/scopenx)+scopedy/2, bp, scopes+((i&~1)|x)*scopesx, scopesx, paus?8:15, 1);
    }
  }
  else
  if (plOszChan==1)
  {
    int i;
    for (i=0; i<plNPChan; i++)
    {
      int paus=plGetPChanSample(i, plSampBuf, scopesx+(plOszTrigger?scopetlen:0), plOszRate/scopenx, 0);
      if (paus==3)
      {
        removescope((scopedx-scopesx)/2+(i%scopenx)*scopedx, scopedy*(i/scopenx)+scopedy/2, scopes+i*scopesx, scopesx);
        continue;
      }

      short *bp=plSampBuf;
      if (plOszTrigger)
      {
        int j;
        for (j=0; j<scopetlen; j++)
          if ((bp[0]>0)&&(bp[1]<=0)&&(bp[2]<=0))
            break;
          else
            bp++;
        if (j==scopetlen)
          bp=plSampBuf;
        else
          bp++;
      }

      doscale(bp, scopesx);

      drawscope((scopedx-scopesx)/2+(i%scopenx)*scopedx, scopedy*(i/scopenx)+scopedy/2, bp, scopes+i*scopesx, scopesx, paus?8:15, 1);
    }
  }
  else
  if (plOszChan==2)
  {
    plGetMasterSample(plSampBuf, scopesx, plOszRate/scopenx, plOszMono?0:cpiGetSampleStereo);

    doscale(plSampBuf, scopesx*scopeny);

    int i;
    for (i=0; i<scopeny; i++)
      drawscope((scopedx-scopesx)/2, scopedy/2+scopedy*i, plSampBuf+i, scopes+i, scopesx, 15, scopeny);
  }
  else
  {
    plGetLChanSample(plSelCh, plSampBuf, scopesx+(plOszTrigger?scopetlen:0), plOszRate/scopenx, 0);
    char col=plMuteCh[plSelCh]?7:15;
    short *bp=plSampBuf;
    if (plOszTrigger)
    {
      int j;
      for (j=0; j<scopetlen; j++)
        if ((bp[0]>0)&&(bp[1]<=0)&&(bp[2]<=0))
          break;
        else
           bp++;
      if (j==scopetlen)
        bp=plSampBuf;
      else
        bp++;
    }

    doscale(bp, scopesx);

    drawscope((scopedx-scopesx)/2, scopedy/2, bp, scopes, scopesx, col, 1);
  }
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
  case 'o': case 'O':
    cpiSetMode("scope");
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
  cpimoderegstruct cpiModeScope = {"scope", scoSetMode, scoDraw, scoIProcessKey, plScopesKey, scoEvent};
}
