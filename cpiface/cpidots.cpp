// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface note dots mode
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
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include "poutput.h"
#include "cpiface.h"
#include "cpipic.h"
#include "isqrt.cpp" // ugly but working

#define MAXPCHAN 64
#define MAXVIEWCHAN 32

static int dotsinited=0;
static int (*plGetDots)(notedotsdata *, int);

static unsigned short plDotsMiddle;
static unsigned short plDotsScale;
static unsigned char plDotsType;

static notedotsdata dotdata[MAXPCHAN];
static unsigned char dotchan[MAXPCHAN];
static unsigned short dotpos[MAXPCHAN];
static int dotvoll[MAXPCHAN];
static int dotvolr[MAXPCHAN];
static unsigned char dotcol[MAXPCHAN];
static unsigned char dothgt;
static unsigned char dotwid;
static unsigned char dotwid2;
static unsigned char dotbuf[32][96];

static unsigned char dotuse[MAXVIEWCHAN][640/32];

static unsigned char dotsqrttab[65];
static unsigned char dotcirctab[17][16];

static void drawbox(unsigned short y, signed short x)
{
  unsigned long scrpos=96*640+y*dothgt*640+x*32;
  unsigned char page=scrpos>>16;
  plSetGraphPage(page);
  scrpos=((int)plVidMem)+(scrpos&0xFFFF);
  short j;
  for (j=0; j<dothgt; j++)
  {
    memcpy((void*)scrpos, dotbuf[j]+32, 32);
    scrpos+=640;
    if ((scrpos-(int)plVidMem)>=0x10000)
    {
      scrpos-=plSetGraphPage(++page);
    }
  }
}

static void resetbox(unsigned short y, signed short x)
{
  short i;
  if (plOpenCPPict)
  {
    unsigned char *p=plOpenCPPict+y*dothgt*640+x*32;
    for (i=0; i<dothgt; i++)
    {
      memcpy(dotbuf[i]+32, p, 32);
      p+=640;
    }
  }
  else
    for (i=0; i<dothgt; i++)
      memset(dotbuf[i]+32, 0, 32);
}

static void putbar(unsigned short k, unsigned short j)
{
  int v=dotvoll[k]+dotvolr[k];
  if (v>64)
    v=64;
  unsigned char len=(v+3)>>2;
  unsigned char first=32+dotpos[k]-(len>>1)-j*32;
  short l;
  for (l=0; l<dothgt; l++)
    memset(dotbuf[l]+first, dotcol[k], len);
}

static void putstcone(unsigned short k, unsigned short j)
{
  short l;
  unsigned char pos=32+dotpos[k]-j*32;
  unsigned char lenl=(dotsqrttab[dotvoll[k]]+3)>>2;
  unsigned char lenr=(dotsqrttab[dotvolr[k]]+3)>>2;
  for (l=0; l<(dothgt>>1); l++)
  {
    if (l<lenl)
    {
      memset(dotbuf[(dothgt>>1)-1-l]+pos-lenl, dotcol[k], lenl-l);
      memset(dotbuf[(dothgt>>1)+l]+pos-lenl, dotcol[k], lenl-l);
    }
    if (l<lenr)
    {
      memset(dotbuf[(dothgt>>1)-1-l]+pos+l, dotcol[k], lenr-l);
      memset(dotbuf[(dothgt>>1)+l]+pos+l, dotcol[k], lenr-l);
    }
  }
}

static void putdot(unsigned short k, unsigned short j)
{
  short l;
  unsigned char pos=32+dotpos[k]-j*32;
  int v=dotvoll[k]+dotvolr[k];
  if (v>64)
    v=64;
  unsigned char len=(dotsqrttab[v]+3)>>2;
  for (l=0; l<(dothgt>>1); l++)
  {
    unsigned char ln=dotcirctab[len][l];
    memset(dotbuf[(dothgt>>1)-1-l]+pos-ln, dotcol[k], 2*ln);
    memset(dotbuf[(dothgt>>1)+l]+pos-ln, dotcol[k], 2*ln);
  }
}

static void putstdot(unsigned short k, unsigned short j)
{
  short l;
  unsigned char pos=32+dotpos[k]-j*32;
  unsigned char lenl=(dotsqrttab[dotvoll[k]]+3)>>2;
  unsigned char lenr=(dotsqrttab[dotvolr[k]]+3)>>2;
  for (l=0; l<(dothgt>>1); l++)
  {
    unsigned char lnl=dotcirctab[lenl][l];
    unsigned char lnr=dotcirctab[lenr][l];
    memset(dotbuf[(dothgt>>1)-1-l]+pos-lnl, dotcol[k], lnl);
    memset(dotbuf[(dothgt>>1)+l]+pos-lnl, dotcol[k], lnl);
    memset(dotbuf[(dothgt>>1)-1-l]+pos, dotcol[k], lnr);
    memset(dotbuf[(dothgt>>1)+l]+pos, dotcol[k], lnr);
  }
}

static void plDrawDots()
{
  int i,j,k,n,m;

  int chann=plNLChan;
  if (chann>MAXVIEWCHAN)
    chann=MAXVIEWCHAN;
  int chan0=plSelCh-(chann/2);
  if ((chan0+chann)>=plNLChan)
    chan0=plNLChan-chann;
  if (chan0<0)
    chan0=0;

  if (plChanChanged)
    for (i=0; i<chann; i++)
    {
      if (dothgt>=16)
      {
        gdrawcharp(8, 96+(dothgt-16)/2+i*dothgt, '0'+(i+1+chan0)/10, ((i+chan0)==plSelCh)?15:plMuteCh[i+chan0]?8:7, plOpenCPPict?(plOpenCPPict-96*640):0);
        gdrawcharp(16, 96+(dothgt-16)/2+i*dothgt, '0'+(i+1+chan0)%10, ((i+chan0)==plSelCh)?15:plMuteCh[i+chan0]?8:7, plOpenCPPict?(plOpenCPPict-96*640):0);
        gdrawcharp(616, 96+(dothgt-16)/2+i*dothgt, '0'+(i+1+chan0)/10, ((i+chan0)==plSelCh)?15:plMuteCh[i+chan0]?8:7, plOpenCPPict?(plOpenCPPict-96*640):0);
        gdrawcharp(624, 96+(dothgt-16)/2+i*dothgt, '0'+(i+1+chan0)%10, ((i+chan0)==plSelCh)?15:plMuteCh[i+chan0]?8:7, plOpenCPPict?(plOpenCPPict-96*640):0);
      }
      else
      {
        gdrawchar8p(8, 96+(dothgt-8)/2+i*dothgt, '0'+(i+1+chan0)/10, ((i+chan0)==plSelCh)?15:plMuteCh[i+chan0]?8:7, plOpenCPPict?(plOpenCPPict-96*640):0);
        gdrawchar8p(16, 96+(dothgt-8)/2+i*dothgt, '0'+(i+1+chan0)%10, ((i+chan0)==plSelCh)?15:plMuteCh[i+chan0]?8:7, plOpenCPPict?(plOpenCPPict-96*640):0);
        gdrawchar8p(616, 96+(dothgt-8)/2+i*dothgt, '0'+(i+1+chan0)/10, ((i+chan0)==plSelCh)?15:plMuteCh[i+chan0]?8:7, plOpenCPPict?(plOpenCPPict-96*640):0);
        gdrawchar8p(624, 96+(dothgt-8)/2+i*dothgt, '0'+(i+1+chan0)%10, ((i+chan0)==plSelCh)?15:plMuteCh[i+chan0]?8:7, plOpenCPPict?(plOpenCPPict-96*640):0);
      }
    }

  n=plGetDots(dotdata, MAXPCHAN);

  k=0;
  for (i=0; i<n; i++)
  {
    if (dotdata[i].voll>64)
      dotdata[i].voll=64;
    if (dotdata[i].volr>64)
      dotdata[i].volr=64;
    if (!dotdata[i].voll&&!dotdata[i].volr)
    {
      dotdata[i].voll=1;
      dotdata[i].volr=1;
    }
    signed long xp=(dotdata[i].note-plDotsMiddle)*plDotsScale/1024+320;
    if ((xp<16)||(xp>614))
      continue;
    dotdata[i].note=xp;
    if (plMuteCh[dotdata[i].chan])
      dotdata[i].col=8;

    dotchan[k]=dotdata[i].chan;
    dotpos[k]=dotdata[i].note;
    dotvoll[k]=(dotdata[i].voll+1);
    dotvolr[k]=(dotdata[i].volr+1)>>1;
    dotcol[k]=dotdata[i].col;
    k++;
  }
  n=k;

  int pos;
  for (pos=0; pos<n; pos++)
    if (dotchan[pos]>=chan0)
      break;

  for (i=0; i<chann; i++)
  {
    unsigned char use[20];
    memcpy(use, dotuse[i], 20);

    for (m=pos; m<n; m++)
      if (dotchan[m]!=(i+chan0))
        break;

    for (j=1; j<19; j++)
    {
      unsigned char inited=dotuse[i][j];
      dotuse[i][j]=0;
      if (inited)
        resetbox(i, j);
      for (k=pos; k<m; k++)
      {
        if ((((dotpos[k]-dotwid2)>>5)==j)||(((dotpos[k]+dotwid2-1)>>5)==j))
        {
          dotuse[i][j]=1;
          if (!inited)
            resetbox(i, j);
          inited=1;

          switch (plDotsType)
          {
          case 0: putdot(k,j); break;
          case 1: putbar(k,j); break;
          case 2: putstcone(k,j); break;
          case 3: putstdot(k,j); break;
          }
        }
      }
      if (inited)
        drawbox(i, j);
    }
    pos=m;
  }
}

static void plPrepareDotsScr()
{
  char str[49];
  switch (plDotsType)
  {
  case 0: strcpy(str, "   note dots"); break;
  case 1: strcpy(str, "   note bars"); break;
  case 2: strcpy(str, "   stereo note cones"); break;
  case 3: strcpy(str, "   stereo note dots"); break;
  }

  gdrawstr(4, 0, str, 48, 0x09, 0);
}

static void plPrepareDots()
{
  int i,j;
  for (i=0; i<16; i++)
  {
    unsigned char colt=rand()%6;
    unsigned char coll=rand()%63;
    unsigned char colw=8+rand()%32;
    unsigned char r,g,b;
    switch (colt)
    {
    case 0: r=63;      g=coll;    b=0;       break;
    case 1: r=63-coll; g=63;      b=0;       break;
    case 2: r=0;       g=63;      b=coll;    break;
    case 3: r=0;       g=63-coll; b=63;      break;
    case 4: r=coll;    g=0;       b=63;      break;
    case 5: r=63;      g=0;       b=63-coll; break;
    }
    r=63-((63-r)*(64-colw)/64);
    g=63-((63-g)*(64-colw)/64);
    b=63-((63-b)*(64-colw)/64);

    plOpenCPPal[3*i+48]=r>>1;
    plOpenCPPal[3*i+49]=g>>1;
    plOpenCPPal[3*i+50]=b>>1;
    plOpenCPPal[3*i+96]=r;
    plOpenCPPal[3*i+97]=g;
    plOpenCPPal[3*i+98]=b;
  }

  memset(dotuse, 0, sizeof (dotuse));

  int chann=plNLChan;
  if (chann>MAXVIEWCHAN)
    chann=MAXVIEWCHAN;

  dothgt=(chann>24)?12:(chann>16)?16:(chann>12)?24:32;
  dotwid=32;
  dotwid2=16;

  outp(0x3c8, 16);
  for (i=48; i<768; i++)
    outp(0x3c9, plOpenCPPal[i]);

  if (plOpenCPPict)
  {
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

  for (i=0; i<65; i++)
    dotsqrttab[i]=(isqrt(256*i)+1)>>1;

  for (i=0; i<17; i++)
    for (j=0; j<16; j++)
      dotcirctab[i][j]=(j<i)?((isqrt(4*(i*i-j*(j+1))-1)+1)>>1):0;
}

static void plSetDotsType(short t)
{
  plDotsType=(t+4)%4;
}

static int plDotsKey(unsigned short key)
{
  switch (key)
  {
  case 0x4900: //pgup
    plDotsMiddle-=128;
    if (plDotsMiddle<48*256)
      plDotsMiddle=48*256;
    break;
  case 0x5100: //pgdn
    plDotsMiddle+=128;
    if (plDotsMiddle>96*256)
      plDotsMiddle=96*256;
    break;
  case 0x8400: //ctrl-pgup
    plDotsScale=plDotsScale*31/32;
    if (plDotsScale<16)
      plDotsScale=16;
    break;
  case 0x7600: //ctrl-pgdn
    plDotsScale=(plDotsScale+1)*32/31;
    if (plDotsScale>256)
      plDotsScale=256;
    break;
  case 0x4700: //home
    plDotsMiddle=72*256;
    plDotsScale=32;
    break;
  case 'n': case 'N':
    plSetDotsType(plDotsType+1);
    break;
  default:
    return 0;
  }
  plPrepareDotsScr();
  return 1;
}

static void plDotsInit()
{
  plDotsMiddle=72*256;
  plDotsScale=32;
}


static void dotDraw()
{
  cpiDrawGStrings();
  plDrawDots();
}

static void dotSetMode()
{
  plReadOpenCPPic();
  cpiSetGraphMode(0);
  plPrepareDots();
  plPrepareDotsScr();
}

static int dotIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'n': case 'N':
    cpiSetMode("dots");
    break;
  default:
    return 0;
  }
  return 1;
}

static int plDotsEvent(int)
{
  return 1;
}

static cpimoderegstruct plDotsMode = {"dots", dotSetMode, dotDraw, dotIProcessKey, plDotsKey, plDotsEvent};

void plUseDots(int (*get)(notedotsdata *, int))
{
  if (plVidType==vidNorm)
    return;
  if (!dotsinited)
  {
    dotsinited=1;
    plDotsInit();
  }
  plGetDots=get;
  cpiRegisterMode(&plDotsMode);
}
