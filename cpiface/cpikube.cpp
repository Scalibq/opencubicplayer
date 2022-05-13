// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface main interface code
//
// revision history: (please note changes here)
//  -cp1.7   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb_in_between   Tammo Hinrichs <opencp@gmx.net>
//    -reintegrated it into OpenCP, 'coz it r00ls ;)
//  -fd980717  Felix Domke <tmbinc@gmx.net>
//    -added WÅrfel Mode ][ (320x200)

#define NO_CPIFACE_IMPORT
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <string.h>
#include <time.h>
#include "poutput.h"
#include "pmain.h"
#include "cpiface.h"
#include "timer.h"

extern char cfDataDir[];

void memcpyintr(void *d, const void *s, unsigned long l);
#pragma aux memcpyintr parm [edi] [esi] [ecx] modify [eax] = \
"lab:" \
"  movsb" \
"  inc esi" \
"loop lab"

static unsigned char (*plWuerfel)=NULL;
char plWuerfelDirect;
static char wuerfelpal[720];
static short wuerfelpos;
unsigned long wuerfeltnext;
static unsigned long wuerfelscroll;
static unsigned short wuerfelstframes;
static unsigned short wuerfelframes;
static unsigned short wuerfelrle;
static unsigned short wuerfeldlt;
static unsigned char wuerfellowmem;
static unsigned char *wuerfelloadedframes;
static unsigned short *wuerfelframelens;
static unsigned long *wuerfelframepos;
static unsigned short *wuerfelcodelens;
static unsigned long wuerfelframe0pos;
static unsigned long wuerfelframesize;
static unsigned long wuerfelscanlines, wuerfellinelength, wuerfelversion;
static short wuerfelfile;
static unsigned char *wuerfelframebuf;
static unsigned long cfUseAnis=0xFFFFFFFF;


static int plCloseWuerfel()
{
  if (plWuerfel)
  {
    delete plWuerfel;
    plWuerfel=0;
    delete wuerfelcodelens;
    delete wuerfelframelens;
    delete wuerfelframepos;
    delete wuerfelframebuf;
    delete wuerfelloadedframes;
    wuerfelframelens=0;
    wuerfelframepos=0;
    wuerfelframebuf=0;
    wuerfelloadedframes=0;
    if (wuerfelfile)
    {
      close(wuerfelfile);
      wuerfelfile=0;
    }
    return 1;
  }
  return 0;
}

static char plLoadWuerfel()
{
  if (plWuerfel)
    plCloseWuerfel();

  int ifile=-1;

  while (cfUseAnis)
  {
    unsigned char use=rand()&31;
    if (!(cfUseAnis&(1<<use)))
      continue;

    char path[_MAX_PATH];
    strcpy(path, cfDataDir);
    strcat(path, "cpani000.dat");
    path[strlen(path)-5]+=use%10;
    path[strlen(path)-6]+=use/10;

    ifile=open(path, O_BINARY|O_RDONLY);
    if (ifile<0)
      cfUseAnis&=~(1<<use);
    else
      break;
  }
  if (ifile<0)
    return 0;

  wuerfelfile=ifile;

  unsigned char sig[8];
  if (read(ifile, sig, 8)!=8)
  {
    plCloseWuerfel();
    return 0;
  }
  if (memcmp(sig, "CPANI\x1A\x00\x00", 8))
  {
    plCloseWuerfel();
    return 0;
  }
  lseek(ifile, 32, SEEK_CUR);

  read(ifile, &wuerfelframes, 2);
  read(ifile, &wuerfelstframes, 2);

  unsigned short opt, pallen, codelenslen;

  read(ifile, &opt, 2);
  wuerfelrle=opt&1;
  wuerfeldlt=!!(opt&2);
  wuerfelframesize=(opt&4)?64000:16000;
  wuerfelscanlines=(opt&4)?200:100;
  wuerfellinelength=(opt&4)?320:160;
  wuerfelversion=!!(opt&4);

  wuerfelframelens=new unsigned short [wuerfelframes+wuerfelstframes];
  wuerfelframepos=new unsigned long [wuerfelframes+wuerfelstframes];
  wuerfelframebuf=new unsigned char [wuerfelframesize];
  wuerfelloadedframes=new unsigned char [wuerfelframes+wuerfelstframes];

  if (!wuerfelframelens||!wuerfelframepos||!wuerfelframebuf||!wuerfelloadedframes)
  {
    plCloseWuerfel();
    return 0;
  }

  lseek(ifile, 2, SEEK_CUR);
  read(ifile, &codelenslen, 2);
  wuerfelcodelens=new unsigned short [codelenslen];
  if (!wuerfelcodelens)
  {
    plCloseWuerfel();
    return 0;
  }
  read(ifile, &pallen, 2);
  read(ifile, wuerfelframelens, 2*(wuerfelframes+wuerfelstframes));

  if(wuerfelversion)
   read(ifile, wuerfelcodelens, codelenslen);
  else
   lseek(ifile, codelenslen, SEEK_CUR);

  read(ifile, wuerfelpal, pallen);

  memset(wuerfelloadedframes, 0, wuerfelframes+wuerfelstframes);
  wuerfelframepos[0]=0;
  short i;
  unsigned short maxframe=0;
  for (i=1; i<(wuerfelframes+wuerfelstframes); i++)
  {
    if (maxframe<wuerfelframelens[i-1])
      maxframe=wuerfelframelens[i-1];
    wuerfelframepos[i]=wuerfelframepos[i-1]+wuerfelframelens[i-1];
  }

  if (maxframe<wuerfelframelens[i-1])
    maxframe=wuerfelframelens[i-1];

  unsigned long framemem=wuerfelframepos[i-1]+wuerfelframelens[i-1];

  plWuerfel=new unsigned char [framemem];

  wuerfelframe0pos=tell(ifile);

  if (plWuerfel)
  {
    wuerfellowmem=0;
    // do preload, if desired!
  }
  else
  {
    for (i=0; i<wuerfelstframes; i++)
      framemem-=wuerfelframelens[i];
    plWuerfel=new unsigned char [framemem];
    if (plWuerfel)
      wuerfellowmem=1;
    else
    {
      delete wuerfelloadedframes;
      wuerfelloadedframes=0;
      wuerfellowmem=2;
      plWuerfel=new unsigned char [maxframe];
      if (!plWuerfel)
      {
        plCloseWuerfel();
        return 0;
      }
    }
  }

  return 1;
}




static void plPrepareWuerfel()
{
  short i;
#pragma aux vga13 modify [ax] = "mov ax,13h" "int 10h"
  vga13();
  if(!wuerfelversion)
  {
   outp(0x3c4, 4);
   outp(0x3c5, (inp(0x3c5)&~8)|4);
   outp(0x3d4, 0x14);
   outp(0x3d5, inp(0x3d5)&~0x40);
   outp(0x3d4, 0x17);
   outp(0x3d5, inp(0x3d5)|0x40);
   outp(0x3d4, 0x09);
   outp(0x3d5, inp(0x3d5)|2);
   outpw(0x3c4, 0x0F02);
  }
  memset((char*)0xA0000, 0, 65536);
  outp(0x3c8, 16);
  for (i=0; i<720; i++)
    outp(0x3c9, wuerfelpal[i]);
  wuerfelpos=0;
  wuerfeltnext=0;
  wuerfelscroll=0;
/*
    outpw(0x3c4, 0x0302);
    memcpyintr((void*)(0xA0000+80*24), plWuerfel[wuerfelpos], 160*76/2);
    outpw(0x3c4, 0x0C02);
    memcpyintr((void*)(0xA0000+80*24), plWuerfel[wuerfelpos]+1, 160*76/2);
*/
}

static void decodrle(unsigned char *rp, unsigned short rbuflen)
{
  unsigned char *op=wuerfelframebuf;
  unsigned char *re=rp+rbuflen;
  while (rp<re)
  {
    unsigned char c=*rp++;
    if (c<=0x0F)
    {
      memset(op, *rp++, c+3);
      op+=c+3;
    }
    else
      *op++=c;
  }
}

static void decodrledlt(unsigned char *rp, unsigned short rbuflen)
{
  unsigned char *op=wuerfelframebuf;
  unsigned char *re=rp+rbuflen;
  while (rp<re)
  {
    unsigned char c=*rp++;
    if (c<=0x0E)
    {
      unsigned char c2=*rp++;
      if (c2!=0x0F)
        memset(op, c2, c+3);
      op+=c+3;
    }
    else
    {
      if (c!=0x0F)
        *op=c;
      op++;
    }
  }
}









static void wuerfelDraw()
{

  if (tmGetTimer()<(wuerfeltnext+(wuerfelversion?wuerfelcodelens[wuerfelpos]:3072)))
    return;

  wuerfeltnext=tmGetTimer();
  short i;

  if(!wuerfelversion)
  {
   outp(0x3c4, 4);
   outp(0x3c5, inp(0x3c5)&~8);
  }

  if (wuerfeldlt)
    plWuerfelDirect=0;

  if (wuerfelpos<wuerfelstframes)
  {
    plWuerfelDirect=0;
    wuerfelscroll=wuerfelscanlines;
  }

  unsigned char *curframe;
  unsigned short framelen=wuerfelframelens[wuerfelpos];
  if (wuerfellowmem==2)
  {
    lseek(wuerfelfile, wuerfelframe0pos+wuerfelframepos[wuerfelpos], SEEK_SET);
    read(wuerfelfile, plWuerfel, framelen);
    curframe=plWuerfel;
  }
  else
  if (wuerfellowmem==1)
  {
    if (wuerfelpos<wuerfelstframes)
    {
      lseek(wuerfelfile, wuerfelframe0pos+wuerfelframepos[wuerfelpos], SEEK_SET);
      read(wuerfelfile, plWuerfel, framelen);
      curframe=plWuerfel;
    }
    else
    {
      curframe=plWuerfel+wuerfelframepos[wuerfelpos];
      if (!wuerfelloadedframes[wuerfelpos])
      {
        lseek(wuerfelfile, wuerfelframe0pos+wuerfelframepos[wuerfelpos], SEEK_SET);
        read(wuerfelfile, curframe, framelen);
        wuerfelloadedframes[wuerfelpos]=1;
      }
    }
  }
  else
  {
    curframe=plWuerfel+wuerfelframepos[wuerfelpos];
    if (!wuerfelloadedframes[wuerfelpos])
    {
      lseek(wuerfelfile, wuerfelframe0pos+wuerfelframepos[wuerfelpos], SEEK_SET);
      read(wuerfelfile, curframe, framelen);
      wuerfelloadedframes[wuerfelpos]=1;
    }
  }

  if (wuerfeldlt)
    decodrledlt(curframe, framelen);
  else
  if (wuerfelrle)
    decodrle(curframe, framelen);
  else
    memcpy(wuerfelframebuf, curframe, framelen);

/*  while(inp(0x3da)&8);
  while(!(inp(0x3da)&8));*/
  for (i=0; i<wuerfelscroll; i++)
  {
    if(!wuerfelversion)
    {
      outpw(0x3c4, 0x0302);
      memcpyintr((void*)(0xA0000+80*(100+i-wuerfelscroll)), wuerfelframebuf+i*160, 80);
      outpw(0x3c4, 0x0C02);
      memcpyintr((void*)(0xA0000+80*(100+i-wuerfelscroll)), wuerfelframebuf+1+i*160, 80);
    } else
    {
      memcpy((void*)(0xA0000+320*(wuerfelscanlines+i-wuerfelscroll)), wuerfelframebuf+i*320, 320);
    }
  }

  if (wuerfelscroll<wuerfelscanlines)
    wuerfelscroll+=(wuerfelversion?2:1);
  if (wuerfelpos<wuerfelstframes)
    wuerfelpos++;
  else
    wuerfelpos=wuerfelstframes+(wuerfelpos-wuerfelstframes+(plWuerfelDirect?(wuerfelframes-1):1))%wuerfelframes;

  if(!wuerfelversion)
  {
    outp(0x3c4, 4);
    outp(0x3c5, inp(0x3c5)|8);
  }
}



static int wuerfelKey(unsigned short key)
{
  switch (key)
  {
  case 0x9700: //alt-home
  case 0x4700: //home
    break;
  case 9: // tab
  case 0x0F00: // shift-tab
  case 0xA500:
    plWuerfelDirect=!plWuerfelDirect;
    break;
  case 'w':
    plLoadWuerfel();
    plPrepareWuerfel();
    return 1;
  default:
    return 0;
  }
  return 1;
}



static void wuerfelSetMode()
{
  plPrepareWuerfel();
  plScreenChanged=1;
}



static int wuerfelEvent(int ev)
{
  switch (ev)
  {
  case cpievInitAll: return /* plVidType!=vidMDA */ !0;
  case cpievInit: return plLoadWuerfel();
  case cpievDoneAll: plCloseWuerfel();
  }
  return 1;
}



static int wuerfelIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'w': 
    cpiSetMode("wuerfel2");
    break;
  default:
    return 0;
  }
  return 1;
}



extern "C"
{
  cpimoderegstruct cpiModeWuerfel = {"wuerfel2", wuerfelSetMode, wuerfelDraw, wuerfelIProcessKey, wuerfelKey, wuerfelEvent};
}

