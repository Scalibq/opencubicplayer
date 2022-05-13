#ifdef DOS32
// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Routines for screen output
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <kb@nwn.de>
//    -did a LOT to avoid redundant mode switching (really needed with
//     today's monitors)
//    -added color look-up table for redefining the palette
//    -added MDA output mode for all Hercules enthusiasts out there
//     (really rocks in Windows 95 background :)
//  -fd981220   Felix Domke    <tmbinc@gmx.net>
//    -faked in a LFB-mode (required some other changes)

#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include "dpmi.h"
#include "err.h"
#include "pmain.h"
#include "psetting.h"
#include "imsrtns.h"
#include "dpmi.h"

short plScrHeight;
short plScrWidth;

int plScrMode;
char plVidType;

int plLFB=0;
int plUseLFB=!0;

void *plLFBBase;
char *plVidMem=(char*)0xA0000;


enum { vidNorm, vidVESA, vidMDA };

unsigned char plScrType;
unsigned short plScrRowBytes;
unsigned short plScrLineBytes;

extern unsigned char plFont816[256][16];
extern unsigned char plFont88[256][8];
static unsigned char lastgraphpage;
static unsigned char pagefactor;

static char *vgatextram = (char *) 0xB8000;
static char *mdatextram = (char *) 0xB0000;
static int plActualMode=0xFF;

char getvmode();
#pragma aux getvmode modify [ax] value [al] = "pushf" "cli" "mov ah,0fh" "int 10h" "popf"

unsigned short setvesamode(unsigned short);
#pragma aux setvesamode modify [ax] parm [bx] value [ax] = "pushf" "cli" "mov ax,4f02h" "int 10h" "popf"

void setvesawpage(unsigned short);
#pragma aux setvesawpage modify [ax bx] parm [dx] = "pushf" "cli" "mov ax,4f05h" "mov bx,0" "int 10h" "popf"

void setvesarpage(unsigned short);
#pragma aux setvesarpage modify [ax bx] parm [dx] = "pushf" "cli" "mov ax,4f05h" "mov bx,1" "int 10h" "popf"

unsigned short setvesawidth(unsigned short);
#pragma aux setvesawidth modify [ax bx] parm [cx] value [ax] = "pushf" "cli" "mov ax,4f06h" "mov bx,0" "int 10h" "popf"

unsigned short testvesa();
#pragma aux testvesa modify [bx] value [ax] = "pushf" "cli" "mov ax,4f03h" "int 10h" "popf"

void load816font();
#pragma aux load816font modify [ax bl] = "pushf" "cli" "mov ax,1114h" "mov bl,0" "int 10h" "popf"

void load88font();
#pragma aux load88font modify [ax bl] = "pushf" "cli" "mov ax,1112h" "mov bl,0" "int 10h" "popf"

void disablecursor();
#pragma aux disablecursor modify [ax cx] = "pushf" "cli" "mov ax,0103h" "mov cx,2000h" "int 10h" "popf"

void disableblinking();
#pragma aux disableblinking modify [ax bx] = "pushf" "cli" "mov ax,1003h" "mov bx,0" "int 10h" "popf"

void vgamode(unsigned short n);
#pragma aux vgamode parm [ax] = "pushf" "cli" "int 10h" "popf"



static unsigned short vesagetvmb(short modenr, void __far16 *rmptr)
{
  callrmstruct rm;
  clearcallrm(rm);

  rm.x.eax=0x4F01;
  rm.x.ecx=modenr;
  rm.x.edi=(unsigned short)rmptr;
  rm.s.es=(unsigned long)rmptr>>16;
  intrrm(0x10, rm);
  return(rm.x.eax&0xFFFF);
}

static char plpalette[256];
static char hgcpal[16]={0,7,7,7,7,7,7,15,7,7,15,15,15,15,15,15};

static void hgcMakePal()
{
  for (int bg=0; bg<16; bg++)
    for (int fg=0; fg<16; fg++)
      if (hgcpal[bg]>hgcpal[fg])
        plpalette[16*bg+fg]=0x70;
      else
        if (bg&8)
          plpalette[16*bg+fg]=hgcpal[fg]^0x77;
        else
          plpalette[16*bg+fg]=hgcpal[fg];
}




static void vgaMakePal()
{
  int pal[16];
  char palstr[1024];
  strcpy(palstr,cfGetProfileString2(cfScreenSec, "screen", "palette", "0 1 2 3 4 5 6 7 8 9 A B C D E F"));

  int bg,fg;

  for (bg=0; bg<16; bg++)
    pal[bg]=bg;

  bg=0;
  char scol[4];
  char const *ps2=palstr;
  while (cfGetSpaceListEntry(scol, ps2, 2) && bg<16)
    pal[bg++]=strtol(scol,0,16)&0x0f;

  for (bg=0; bg<16; bg++)
    for (fg=0; fg<16; fg++)
      plpalette[16*bg+fg]=16*pal[bg]+pal[fg];

}

static unsigned char bartops[18]="\xB5\xB6\xB7\xB8\xBD\xBE\xC6\xC7\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7";
static unsigned char mbartops[18]="\x00\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdb\xdb\xdb\xdb\xdb\xdb\xdb\xdb";


static int hgcSetTextMode()
{
  static const unsigned char hgc_rtext[14]={97,80,82,15,25,6,25,25,2,13,20,20,0,0};

  int ctype=0;
  char old;
  char val=0;

  old = inp(0x3ba);
  for (int c=0;c<100;c++)
    if (inp(0x3ba)!=old) val=6;

  if (val==6) {
    int x=50000;
    char y;
    do {
      x--;
      y=inp(0x3BA);
    } while (((y & 0x80)!=0) && (x!=0));
    if (x==0) ctype=1;
    else
      switch (y & 0x70) {
        case 0x50 : { ctype=4; break; }
        case 0x10 : { ctype=3; break; }
        default   : ctype=2;
      }
  }
  if (ctype<2)
    return 0;

  outp(0x3bf, 3);
  outp(0x3b8, 8);
  for (char b=0; b<14; b++) {
    outp(0x3b4,b);
    outp(0x3b5,hgc_rtext[b]);
  }
  memsetb(mdatextram,0,4000);

  return 1;
}


void plSetBarFont()
{
  if (plVidType==vidNorm || plVidType==vidVESA)
  {
    __segment pmsel;
    void __far16 *rmptr;
    void *flatptr=dosmalloc(16, rmptr, pmsel);
    if (!flatptr)
      return;
    char hgt=(plScrType&2)?8:16;
    short i,j;
    for (i=0; i<=16; i++)
    {
      if (plScrType&2)
        for (j=0; j<8; j++)
          ((char*)flatptr)[j]=((7-j)<(i>>1))?0xFE:0;
      else
        for (j=0; j<16; j++)
          ((char*)flatptr)[j]=((15-j)<i)?0xFE:0;
      callrmstruct regs;
      regs.w.ax=0x1100;
      regs.b.bl=0;
      regs.b.bh=hgt;
      regs.w.cx=1;
      regs.w.dx="\xB5\xB6\xB7\xB8\xBD\xBE\xC6\xC7\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7"[i];
      regs.w.bp=(unsigned short)rmptr;
      regs.s.es=(unsigned long)rmptr>>16;
      intrrm(0x10, regs);
    }
    dosfree(pmsel);
  }
  else
    memcpyb(bartops,mbartops,18);
}



void plClearTextScreen()
{

  for (unsigned int i=0; i< (plScrRowBytes*plScrHeight); i+=2)
  {
    vgatextram[i]=0;
    vgatextram[i+1]=plpalette[0];
  }
}


void plSetTextMode(unsigned char t)
{
  int l43=0;
  plScrType=t&7;

  if (!plScreenChanged && plScrType==plActualMode && plScrMode==0)
  {
    plClearTextScreen();
    return;
  }

  plScreenChanged=0;
  plScrMode=0;
  plActualMode=plScrType;

  if (plVidType==vidMDA)
  {
    plScrType=0;
    hgcSetTextMode();
    plScrWidth=80;
    plScrRowBytes=2*plScrWidth;
    plScrHeight=25;
    plLFB=!0;
    plVidMem=(char*)0xB0000;
    return;
  }


  switch (plVidType)
  {
  case vidNorm:
    plScrType&=2;
      if (getvmode()!=0x03)
        vgamode(0x03);
    if (plScrType&2)
    {
      plClearTextScreen();
      load88font();
    }
    break;

  case vidVESA:
    vgamode(0x03);                      // some video-cards don't like to
                                        // switch from a graphic-mode to
                                        // a high-res textmode directly.
    if (plScrType==7)
      if (setvesamode(0x10C)==0x004F)
        break;
      else
        plScrType=6;
    if (plScrType==6)
      if (setvesamode(0x10B)==0x004F)
        break;
      else
        if (setvesamode(0x10A)==0x004F)
        {
          outp(0x3d4,0x9);
          if ((inp(0x3D5)&0x1F)==8)
          {
            load88font();
            l43=48;
          }
          else
            l43=43;
          break;
        }
        else
          plScrType=2;
    if (plScrType==5)
      if (setvesamode(0x10C)==0x004F)
      {
        load816font();
        break;
      }
      else
        plScrType=4;
    if (plScrType==4)
      if (setvesamode(0x109)==0x004F)
        break;
      else
        plScrType=0;
    if (plScrType&1)
      if (setvesamode(0x108)!=0x004F)
        plScrType&=~1;
    if (!(plScrType&1))
      vgamode(3);
    if (plScrType&2)
      load88font();
    else
      load816font();
    break;
  };

  plLFB=!0;
  plVidMem=(char*)0xB8000;

  plScrWidth=(plScrType&8)?40:(plScrType&4)?132:80;
  plScrRowBytes=2*plScrWidth;
  plScrHeight=(*(char*)0x484)+1;
  if ((plScrHeight!=25)||(plScrHeight!=30)||(plScrHeight!=43)||(plScrHeight!=50)||(plScrHeight!=60))
    plScrHeight=(plScrType&1)?((plScrType&2)?60:30):((plScrType&2)?(l43?l43:50):25);

  disableblinking();
  disablecursor();
}

int plSetGraphPage(unsigned char p)
{
  if (p==lastgraphpage)
    return 0;
  lastgraphpage=p;
  if(plLFB)
  {
    plVidMem=(char*)plLFBBase+p*0x10000;
    return 0;
  } else
  {
    plVidMem=(char*)0xA0000;
    switch (plVidType)
    {
    case vidVESA:
      setvesawpage(p<<pagefactor);
      setvesarpage(p<<pagefactor);
      return 0x10000;
    }
  }
  return 0;
}


void plClearGraphScreen()
{
  plSetGraphPage(0);
  memset((char*)plVidMem, plpalette[0]>>4, 65536);
  plSetGraphPage(1);
  memset((char*)plVidMem, plpalette[0]>>4, 65536);
  plSetGraphPage(2);
  memset((char*)plVidMem, plpalette[0]>>4, 65536);
  plSetGraphPage(3);
  memset((char*)plVidMem, plpalette[0]>>4, 65536);
  plSetGraphPage(4);
  memset((char*)plVidMem, plpalette[0]>>4, 45056);
}

void *plRemapLFB(long phys, int size)
{
  if(plLFBBase)
    dpmiUnMapMemory(plLFBBase);
  if(!phys) return(0);
  plLFBBase=dpmiMapMemory(phys, size);
  return(plLFBBase);
}

long plGetVesaLFBBase(int modenr)
{
  void __far16 *rmptr;
  __segment pmsel;
  struct vmistruct
  {
/*    unsigned short ModeAttributes;
    char WinAAttributes;
    char WinBAttributes;
    unsigned short WinGranularity;
    unsigned short WinSize;
    unsigned short WinASegment;
    unsigned short winBSegment;
    void *WinFuncptr;
    unsigned short BytesPerScanline;
    unsigned short XResolution;
    unsigned short YResolution;
    char XCharSize;
    char YCharSize;
    char NumberOfPlanes;
    char BitsPerPixel;
    char NumberOfBanks;
    char MemoryModel;
    char BankSize;
    char NumberOfImagePages;
    char Reserved;

    char RedMaskSize ;
    char RedFieldPosition ;
    char GreenMasksize ;
    char GreenFieldPosition;
    char Bluemasksize ;
    char Bluefieldposition ;
    char RsvdMaskSize ;
    char RsvdMasPosition ;
    char DirectColorModeInfo; */
    char d[40];

    int LinearFrameBuffer;
/*    char *OffscreenMemoryAddress;
    unsigned short int OffscreenMemory;

    char dummy[206]; */
  } *vmi=(vmistruct*)dosmalloc(256, rmptr,  pmsel);

  *vmi->d=*vmi->d;        // todo

  if(!vmi) return(0);
  if(vesagetvmb(modenr, rmptr)!=0x004F)
  {
    dosfree(pmsel);
    return(0);
  }
  dosfree(pmsel);
  return(vmi->LinearFrameBuffer);
}

void plSetGraphMode(unsigned char size)
{
  if (!plScreenChanged && plScrMode==1 && plActualMode==size)
  {
    if (!size)
      plClearGraphScreen();
    return;
  }

  plScrMode=1;
  plActualMode=size;
  plScreenChanged=0;

  int i;
  switch (plVidType)
  {
  case vidVESA:
    if (size)
    {
      plLFBBase=0;
      int phys=plGetVesaLFBBase(0x105);
      if(plUseLFB&&phys&&(setvesamode(0x105|(1<<14))==0x004F))
      {
        plRemapLFB(phys, 1024*768);
        plLFB=!0;
      }

      if(!plLFBBase)
      {
        plLFB=0;
        setvesamode(0x105);
      }

      plScrLineBytes=1024;
    }
    else
    {
      plLFBBase=0;
      int phys=plGetVesaLFBBase(0x101);

      if(plUseLFB&&phys&&(setvesamode(0x101|(1<<14))==0x004F))
      {
        plRemapLFB(phys, 640*480);
        plLFB=!0;
      }

      if(!plLFBBase)
      {
        plLFB=0;
        setvesamode(0x101);
      }

      plScrLineBytes=640;
    }
    if(!plLFB)
    {
      setvesawpage(1);                    // WHAT THE HELL IS THAT?!? :) (fd)
      *(char*)0xA0000=1;
      setvesawpage(0);
      for (i=0; i<16; i++)
        if (*(char*)(0xA0000+(1<<i)))
          break;
      pagefactor=16-i;
      setvesawpage(1);
      *(char*)0xA0000=0;
      setvesawpage(0);
    }
    break;
  }
  lastgraphpage=0;
  if(plLFB)
    plVidMem=(char*)plLFBBase;
  else
    plVidMem=(char*)0xA0000;

  plScrWidth=size?128:80;
}


void fetchfont()
{
  if (cfGetProfileString("commandline", "m", 0))
  {
    if (hgcSetTextMode())
    {
      plVidType=vidMDA;
      vgatextram=mdatextram;
      hgcMakePal();
    }
  }
  else
  {
    vgaMakePal();
    if (testvesa()==0x004f)
      plVidType=vidVESA;
    else
      plVidType=vidNorm;
  }
}


char *convnum(unsigned long num, char *buf, unsigned char radix, unsigned short len, char clip0=1)
{
  unsigned short i;
  for (i=0; i<len; i++)
  {
    buf[len-1-i]="0123456789ABCDEF"[num%radix];
    num/=radix;
  }
  buf[len]=0;
  if (clip0)
    for (i=0; i<(len-1); i++)
    {
      if (buf[i]!='0')
        break;
      buf[i]=' ';
    }
  return buf;
}


void writenum(short *buf, unsigned short ofs, unsigned char attr, unsigned long num, unsigned char radix, unsigned short len, char clip0=1)
{
  char convbuf[20];
  char *p=(char *)buf+ofs*2;
  char *cp=convbuf+len;
  unsigned short i;
  for (i=0; i<len; i++)
  {
    *--cp="0123456789ABCDEF"[num%radix];
    num/=radix;
  }
  for (i=0; i<len; i++)
  {
    if (clip0&&(convbuf[i]=='0')&&(i!=(len-1)))
    {
      *p++=' ';
      cp++;
    }
    else
    {
      *p++=*cp++;
      clip0=0;
    }
    *p++=attr;
  }
}


void writestring(short *buf, unsigned short ofs, unsigned char attr, const char *str, unsigned short len)
{
  char *p=((char *)buf)+ofs*2;
  unsigned short i;
  for (i=0; i<len; i++)
  {
    *p++=*str;
    *p++=attr;
    if (*str)
      str++;
  }
}


void writestringattr(short *buf, unsigned short ofs, const void *str, unsigned short len)
{
  memcpyb((char *)buf+ofs*2, (void *)str, len*2);
}


void markstring(short *buf, unsigned short ofs, unsigned short len)
{
  buf+=ofs;
  short i;
  for (i=0; i<len; i++)
    *buf++^=0x8000;
}



void displaystr(unsigned short y, unsigned short x, unsigned char attr, const char *str, unsigned short len)
{
  char *p=vgatextram+(y*plScrRowBytes+x*2);
  attr=plpalette[attr];
  unsigned short i;
  for (i=0; i<len; i++)
  {
    *p++=*str;
    if (*str)
      *str++;
    *p++=attr;
  }
}


void displaystrattr(unsigned short y, unsigned short x, const short *buf, unsigned short len)
{
  char *p=vgatextram+(y*plScrRowBytes+x*2);
  char *b=(char *)buf;
  for (int i=0; i<len*2; i+=2)
  {
    p[i]=b[i];
    p[i+1]=plpalette[b[i+1]];
  }
}


void displaystrattrdi(unsigned short y, unsigned short x, const unsigned char *txt, const unsigned char *attr, unsigned short len)
{
  char *p=vgatextram+(y*plScrRowBytes+x*2);
  unsigned short i;
  for (i=0; i<len; i++)
  {
    *p++=*txt++;
    *p++=plpalette[*attr++];
  }
}


void displayvoid(unsigned short y, unsigned short x, unsigned short len)
{
  char *addr=vgatextram+y*plScrRowBytes+x*2;
  while (len--)
  {
    *addr++=0;
    *addr++=plpalette[0];
  }
}


void gdrawchar(unsigned short x, unsigned short y, unsigned char c, unsigned char f, unsigned char b)
{
  unsigned char *cp=plFont816[c];
  f=plpalette[f]&0x0f;
  b=plpalette[b]&0x0f;
  unsigned long p=y*plScrLineBytes+x;
  unsigned char pg=p>>16;
  plSetGraphPage(pg);
  char *scr=(char*)(plVidMem+(p&0xFFFF));
  short i,j;
  for (i=0; i<16; i++)
  {
    unsigned char bitmap=*cp++;
    for (j=0; j<8; j++)
    {
      if (scr>=(char*)(plVidMem+0x10000))
      {
        scr-=plSetGraphPage(++pg);
      }
      *scr++=(bitmap&128)?f:b;
      bitmap<<=1;
    }
    scr+=plScrLineBytes-8;
  }
}


void gdrawchart(unsigned short x, unsigned short y, unsigned char c, unsigned char f)
{
  unsigned char *cp=plFont816[c];
  f=plpalette[f]&0x0f;
  unsigned long p=y*plScrLineBytes+x;
  unsigned char pg=p>>16;
  plSetGraphPage(pg);
  char *scr=(char*)(plVidMem+(p&0xFFFF));
  short i,j;
  for (i=0; i<16; i++)
  {
    unsigned char bitmap=*cp++;
    for (j=0; j<8; j++)
    {
      if (scr>=(char*)plVidMem+0x10000)
      {
        scr-=plSetGraphPage(++pg);
      }
      if (bitmap&128)
        *scr=f;
      scr++;
      bitmap<<=1;
    }
    scr+=plScrLineBytes-8;
  }
}


void gdrawcharp(unsigned short x, unsigned short y, unsigned char c, unsigned char f, void *picp)
{
  if (!picp)
  {
    gdrawchar(x,y,c,f,0);
    return;
  }

  f=plpalette[f]&0x0f;
  unsigned char *cp=plFont816[c];
  unsigned long p=y*plScrLineBytes+x;
  char *pic=(char*)picp+p;
  unsigned char pg=p>>16;
  plSetGraphPage(pg);
  char *scr=(char*)(plVidMem+(p&0xFFFF));
  short i,j;
  for (i=0; i<16; i++)
  {
    unsigned char bitmap=*cp++;
    for (j=0; j<8; j++)
    {
      if (scr>=(char*)plVidMem+0x10000)
      {
        scr-=plSetGraphPage(++pg);
      }
      if (bitmap&128)
        *scr=f;
      else
        *scr=*pic;
      scr++;
      pic++;
      bitmap<<=1;
    }
    pic+=plScrLineBytes-8;
    scr+=plScrLineBytes-8;
  }
}


void gdrawchar8(unsigned short x, unsigned short y, unsigned char c, unsigned char f, unsigned char b)
{
  unsigned char *cp=plFont88[c];
  f=plpalette[f]&0x0f;
  b=plpalette[b]&0x0f;
  unsigned long p=y*plScrLineBytes+x;
  unsigned char pg=p>>16;
  plSetGraphPage(pg);
  char *scr=(char*)(plVidMem+(p&0xFFFF));
  short i,j;
  for (i=0; i<8; i++)
  {
    unsigned char bitmap=*cp++;
    for (j=0; j<8; j++)
    {
      if (scr>=(char*)plVidMem+0x10000)
      {
        scr-=plSetGraphPage(++pg);
      }
      *scr++=(bitmap&128)?f:b;
      bitmap<<=1;
    }
    scr+=plScrLineBytes-8;
  }
}

void gdrawchar8t(unsigned short x, unsigned short y, unsigned char c, unsigned char f)
{
  unsigned char *cp=plFont88[c];
  f=plpalette[f]&0x0f;
  unsigned long p=y*plScrLineBytes+x;
  unsigned char pg=p>>16;
  plSetGraphPage(pg);
  char *scr=(char*)(plVidMem+(p&0xFFFF));
  short i,j;
  for (i=0; i<8; i++)
  {
    unsigned char bitmap=*cp++;
    for (j=0; j<8; j++)
    {
      if (scr>=(char*)plVidMem+0x10000)
      {
        scr-=plSetGraphPage(++pg);
      }
      if (bitmap&128)
        *scr=f;
      scr++;
      bitmap<<=1;
    }
    scr+=plScrLineBytes-8;
  }
}

void gdrawchar8p(unsigned short x, unsigned short y, unsigned char c, unsigned char f, void *picp)
{
  if (!picp)
  {
    gdrawchar8(x,y,c,f,0);
    return;
  }

  f=plpalette[f]&0x0f;
  unsigned char *cp=plFont88[c];
  unsigned long p=y*plScrLineBytes+x;
  unsigned char pg=p>>16;
  plSetGraphPage(pg);
  char *scr=(char*)(plVidMem+(p&0xFFFF));
  char *pic=(char*)picp+p;
  short i,j;
  for (i=0; i<8; i++)
  {
    unsigned char bitmap=*cp++;
    for (j=0; j<8; j++)
    {
      if (scr>=((char*)plVidMem+0x10000))
      {
        scr-=plSetGraphPage(++pg);
      }
      if (bitmap&128)
        *scr=f;
      else
        *scr=*pic;
      scr++;
      pic++;
      bitmap<<=1;
    }
    scr+=plScrLineBytes-8;
    pic+=plScrLineBytes-8;
  }
}

void gdrawstr(unsigned short y, unsigned short x, const char *str, unsigned short len, unsigned char f, unsigned char b)
{
  unsigned long p=16*y*plScrLineBytes+x*8;
  f=plpalette[f]&0x0f;
  b=plpalette[b]&0x0f;
  plSetGraphPage(p>>16);
  char *sp=(char*)plVidMem+(p&0xFFFF);
  short i,j,k;
  for (i=0; i<16; i++)
  {
    const char *s=str;
    for (k=0; k<len; k++)
    {
      unsigned char bitmap=plFont816[*s][i];
      for (j=0; j<8; j++)
      {
        *sp++=(bitmap&128)?f:b;
        bitmap<<=1;
      }
      if (*s)
        s++;
    }
    sp+=plScrLineBytes-8*len;
  }
}

void gupdatestr(unsigned short y, unsigned short x, const short *str, unsigned short len, short *old)
{
  unsigned long p=16*y*plScrLineBytes+x*8;
  plSetGraphPage(p>>16);
  char *sp=(char*)plVidMem+(p&0xFFFF);
  short i,j,k;
  for (k=0; k<len; k++, str++, old++)
    if (*str!=*old)
    {
      *old=*str;
      unsigned char *bitmap0=plFont816[((unsigned char*)str)[0]];
      unsigned char f=plpalette[((unsigned char*)str)[1]]&0x0F;
      unsigned char b=plpalette[((unsigned char*)str)[1]]>>4;
      for (i=0; i<16; i++)
      {
        unsigned char bitmap=bitmap0[i];
        for (j=0; j<8; j++)
        {
          *sp++=(bitmap&128)?f:b;
          bitmap<<=1;
        }
        sp+=plScrLineBytes-8;
      }
      sp-=16*plScrLineBytes-8;
    }
    else
      sp+=8;
}

void drawbar(unsigned short x, unsigned short yb, unsigned short yh, unsigned long hgt, unsigned long c)
{
  if (hgt>((yh*16)-4))
    hgt=(yh*16)-4;
  char buf[60];
  short i;
  for (i=0; i<yh; i++)
  {
    if (hgt>=16)
    {
      buf[i]=bartops[16];
      hgt-=16;
    }
    else
    {
      buf[i]=bartops[hgt];
      hgt=0;
    }
  }
  char *scrptr=vgatextram+(2*x+yb*plScrRowBytes);
  short yh1=(yh+2)/3;
  short yh2=(yh+yh1+1)/2;
  for (i=0; i<yh1; i++, scrptr-=plScrRowBytes)
  {
    scrptr[0]=buf[i];
    scrptr[1]=plpalette[c&0xFF];
  }
  c>>=8;
  for (i=yh1; i<yh2; i++, scrptr-=plScrRowBytes)
  {
    scrptr[0]=buf[i];
    scrptr[1]=plpalette[c&0xFF];
  }
  c>>=8;
  for (i=yh2; i<yh; i++, scrptr-=plScrRowBytes)
  {
    scrptr[0]=buf[i];
    scrptr[1]=plpalette[c&0xFF];
  }
}

static int outInit()
{
  plUseLFB=cfGetProfileBool2(cfScreenSec, "screen", "uselfb", !0, 0);
  fetchfont();
  if (plVidType==vidMDA)
    printf("using MDA/Hercules card for output\n");
  if (plVidType==vidNorm)
    printf("you have to install a vesa driver for the extended text and graphic modes!\n");
  return errOk;
}

static void outClose()
{
  if (plVidType==vidMDA)
    memsetb(mdatextram,0,4000);
  plRemapLFB(0, 0);             // unmap all memory
}

char xekbhit();
#pragma aux xekbhit modify [ah] value [al] = "mov ah,11h" "int 16h" "mov al,0" "jz nohit" "inc al" "nohit:"
unsigned short xegetch();
#pragma aux xegetch value [ax] = "mov ah,10h" "int 16h"

int ekbhit()
{
 return(xekbhit());
}

int egetch()
{
 return(xegetch());
}


extern "C"
{
  initcloseregstruct outReg = {outInit, outClose};

  char *dllinfo = "preinitclose _outReg";
};
#else
#error please compile under DOS32.
#endif