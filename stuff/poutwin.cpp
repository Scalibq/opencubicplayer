#ifdef WIN32
// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Routines for screen output (win32 edition)
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717  Tammo Hinrichs <kb@nwn.de>
//    -did a LOT to avoid redundant mode switching (really needed with
//     today's monitors)
//    -added color look-up table for redefining the palette
//    -added MDA output mode for all Hercules enthusiasts out there
//     (really rocks in Windows 95 background :)
//  -fd981016
//    -ported to win32 (yeah, i AM crazy :)

#define NO_POUTPUT_IMPORT

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include "err.h"
#include "pmain.h"
#include "psetting.h"
#include "imsrtns.h"
#include "poutput.h"

char *plVidMem=(char*)0xA0000;
int plLFB=0;

int plScreenChanged;

short plScrHeight;
short plScrWidth;

int plScrMode;
char plVidType;

// enum { vidNorm, vidVESA, vidMDA };

unsigned char plScrType;
unsigned short plScrRowBytes;

extern unsigned char plFont816[256][16];
extern unsigned char plFont88[256][8];
//static unsigned char lastgraphpage;
//static unsigned char pagefactor;

static int plActualMode=0xFF;

static char plpalette[256];
//static char hgcpal[16]={0,7,7,7,7,7,7,15,7,7,15,15,15,15,15,15};

#if 0
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
#endif

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
//static unsigned char mbartops[18]="\x00\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdb\xdb\xdb\xdb\xdb\xdb\xdb\xdb";

static int poutwinInit();
static int poutwinClose();
static int poutwinSetMode(int x, int y);
static int poutwinInvalidate(int x0, int y0, int x1, int x2);

static short *videomem;

static int poutwinWinWidth, poutwinWinHeight;
static int poutwinFontWidth=8, poutwinFontHeight=16;

static HWND hwin;
static HINSTANCE hinst;
static char *pixelmem;
static DWORD lastin;

static const char *classname="OpenCPWin32";

long WINAPI poutwinWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
 switch (message)
 {
  case WM_CREATE:
//   unsigned long tid;
//   CreateThread(0, 4096, newthread, 0, 0, &tid);
   break;
  case WM_DESTROY:
   PostQuitMessage(0);
   return(0);
   //break;
/*  case WM_MOUSEMOVE:
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
  {
   for(int i=0; i<buttons; i++)
    button[i]->Mouse(hWnd, LOWORD(lParam), HIWORD(lParam), wParam);
   break;
  } */
/*  case WM_KEYDOWN:
   break; */
  case WM_PAINT:
  {
    RGBQUAD pal[16]={ {0,0,0, 0},
                      {168, 0, 0, 0},
                      {0, 168, 0, 0},
                      {168, 168, 0, 0},
                      {0, 0, 168, 0},
                      {168, 0, 168, 0},
                      {0, 84, 168, 0},
                      {168, 168, 168, 0},
                      {84, 84, 84, 0},
                      {255, 84, 84, 0},
                      {84, 255, 84, 0},
                      {255, 255, 84, 0},
                      {84, 84, 255, 0},
                      {255, 84, 255, 0},
                      {84, 255, 255, 0},
                      {255, 255, 255, 0}};

    char bibuf[sizeof (BITMAPINFOHEADER)+16*4];
    BITMAPINFO &bi=*(BITMAPINFO*)&bibuf;
    memcpy(&bi.bmiColors, pal, 16*4);
    BITMAPINFOHEADER &bih=bi.bmiHeader;
    bih.biSize=sizeof(bih);
    bih.biWidth=poutwinWinWidth;
    bih.biHeight=-poutwinWinHeight;
    bih.biPlanes=1;
    bih.biBitCount=4;
    bih.biCompression=BI_RGB;
    bih.biSizeImage=0;
    bih.biXPelsPerMeter=0;
    bih.biYPelsPerMeter=0;
    bih.biClrUsed=0;
    bih.biClrImportant=0;
//    RECT r;
//    GetUpdateRect(hwin, &r, TRUE);
    HDC dc=GetDC(hwin);
    StretchDIBits(dc, 0, 0, poutwinWinWidth, poutwinWinHeight, 0, 0, poutwinWinWidth, poutwinWinHeight, pixelmem, &bi, DIB_RGB_COLORS, SRCCOPY);
//    StretchDIBits(dc, r.left, r.top, r.right-r.left, r.bottom-r.top, r.left, r.top, r.right-r.left, r.bottom-r.top, pixelmem, &bi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hwin, dc);

    break;
  }
  case WM_SIZE:
   break;
 }
 return DefWindowProc(hWnd, message, wParam, lParam);
}

static volatile int endrequest=0;

static void PeekEvents()
{
 MSG msg;
 if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
 {
  TranslateMessage(&msg);
  DispatchMessage(&msg);
 }
 Sleep(0);
}

static int poutwinInit()
{
 vgaMakePal();
 printf("poutwinInit!\n");

 hinst=win32GetHInstance();

 WNDCLASS wc;

 wc.style=CS_HREDRAW|CS_VREDRAW;
 wc.lpfnWndProc=poutwinWndProc;
 wc.cbClsExtra=0;
 wc.cbWndExtra=0;
 wc.hInstance=hinst;
// wc.hIcon=LoadIcon(hinst, MAKEINTRESOURCE(150));
 wc.hIcon=0;
 wc.hCursor=0;
 wc.hbrBackground=0;
 wc.lpszMenuName=0;
 wc.lpszClassName=classname;
 RegisterClass(&wc);

 RECT r;
 r.top=CW_USEDEFAULT;
 r.left=CW_USEDEFAULT;
 r.right=poutwinWinWidth=10;
 r.bottom=(poutwinWinHeight=10)+GetSystemMetrics(SM_CYCAPTION);
 unsigned long style=WS_CAPTION|WS_SYSMENU;
 unsigned long styleex=WS_EX_ACCEPTFILES;

 hwin=CreateWindowEx(styleex, classname, "OpenCP Win32", style, r.left, r.top, r.right, r.bottom, 0, 0, wc.hInstance, 0);
 if(!hwin)
  MessageBoxA(0, "sorry, cannot create output window...", "OpenCP", MB_TASKMODAL);

 pixelmem=new char[poutwinWinWidth*poutwinWinHeight/2];

 ShowWindow(hwin, 1);
 UpdateWindow(hwin);

// unsigned long tid;
// CreateThread(0, 4096, poutwinMessageThread, 0, 0, &tid);

 InvalidateRect(hwin, 0, FALSE);

 videomem=0;
 plSetTextMode(0);
 return(0);
}

static int poutwinClose()
{
 endrequest=1;
 DestroyWindow(hwin);
 if(pixelmem)
  delete[] pixelmem;
 UnregisterClass(classname, hinst);
 return(0);
}

static int poutwinSetMode(int x, int y)
{
  if(videomem)
    delete[] videomem;
  videomem=new short[x*y];
  if(!videomem)
    return(errAllocMem);
  memset(videomem, 0, x*y*2);
  poutwinWinWidth=x*poutwinFontWidth;
  poutwinWinHeight=y*poutwinFontHeight;
  plScrWidth=x;
  plScrHeight=y;
  plScrRowBytes=2*plScrWidth;
  if(pixelmem)
    delete[] pixelmem;
  pixelmem=new char[poutwinWinWidth*poutwinWinHeight/2];
  if(!pixelmem)
    return(errAllocMem);

//  memset(pixelmem, 0xF0, poutwinWinWidth*poutwinWinHeight/2);

  SetWindowPos(hwin, HWND_TOP, 0, 0, poutwinWinWidth+2, poutwinWinHeight+GetSystemMetrics(SM_CYCAPTION), SWP_NOMOVE);
  return(0);
 // resize window, allocate new memory etc.
}


static int poutwinInvalidate(int x0, int y0, int x1, int y1)
{
 if(x0>=plScrWidth) x0=plScrWidth-1;
 if(y0>=plScrHeight) y0=plScrHeight-1;
 if(x1>=plScrWidth) x1=plScrWidth-1;
 if(y1>=plScrHeight) y1=plScrHeight-1;

// printf("invalidating %d %d -> %d %d\n", x0, y0, x1, y1);

 for(int y=y0; y<=y1; y++)
  for(int x=x0; x<=x1; x++)
  {
   char *m=pixelmem+((y*poutwinFontHeight)*
        poutwinWinWidth+x*poutwinFontWidth)/2;  // current pixelmem-address
   char c=videomem[y*plScrWidth+x]&0xFF;        // current char
   int a=((unsigned)videomem[y*plScrWidth+x])>>8;      // current attrib
   int fore=plpalette[a&0xF];
   int back=plpalette[(a>>4)&0xF];

   unsigned char *font=plFont816[c];
   for(int ay=0; ay<16; ay++)
   {
    for(int ax=0; ax<8; ax+=2)
    {
     *m=(*font&(1<<(7-ax)))?fore:back;
     *m++|=((*font&(1<<((7-ax)+1)))?fore:back)<<4;
    }
    m+=(poutwinWinWidth-poutwinFontWidth)/2;
    font++;
   }
  }

// if((GetTickCount()-lastin) > (1000/25)) // max 25 fps
 {
  //printf("inval.\n");
  lastin=GetTickCount();
  RECT r;
  r.left=x0*poutwinFontWidth;
  r.top=y0*poutwinFontHeight;
  r.right=(x1+1)*poutwinFontWidth-1;
  r.bottom=(y1+1)*poutwinFontHeight-1;
// printf("(%d %d %d %d)\n", r.left, r.top, r.right, r.bottom);
  InvalidateRect(hwin, &r, FALSE);
 }
 return(0);
}

void plTestEnhanced()
{
 printf("enhanced testing of the poutput-stuff..\n");
 printf("init..\n");
 poutwinInit();
 displaystr(3, 1, 0x1F, "hello...", 8);
 egetch();
 printf("close..\n");
 poutwinClose();
 printf("done..\n");
}

#ifdef WIN32
int ekbhit()            // TEMPORARY HACK!
{
 PeekEvents();
 return(kbhit());
}

int egetch()            // TEMPORARY HACK!
{
 while(!kbhit())
  PeekEvents();
 int code=getch();
 if(!code) code=getch()<<8;
 return(code);
}
#endif

#if 0
static int hgcSetTextMode()
{
  poutwinSetMode(80, 25);              // ?!?
  return 1;
}
#endif

void plSetBarFont()
{
  for (int i=0; i<=16; i++)
    for (int j=0; j<16; j++)
     plFont816["\xB5\xB6\xB7\xB8\xBD\xBE\xC6\xC7\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7"[i]][j]=((15-j)<i)?0xFE:0;

 // modify font
}

void plClearTextScreen()
{
// printf("clearing..\n");
 if(videomem)
 {
  memset(videomem, 0, plScrHeight*plScrWidth*2);
  poutwinInvalidate(0, 0, plScrWidth-1, plScrHeight-1);
 }
}


void plSetTextMode(unsigned char t)
{
  if(t=='T') { plTestEnhanced(); return; }
  int l43=0;
  plScrType=t&7;

/*  if (!plScreenChanged && plScrType==plActualMode && plScrMode==0)
  {
    plClearTextScreen();
    return;
  } */

  plScreenChanged=0;
  plScrMode=0;
  plActualMode=plScrType;

  int dx=(plScrType&8)?40:(plScrType&4)?132:80,
      dy=(plScrType&1)?((plScrType&2)?60:30):((plScrType&2)?(l43?l43:50):25);
  poutwinSetMode(dx, dy);

  plClearTextScreen();
}

int plSetGraphPage(unsigned char p)
{
  return 0;
}


void plClearGraphScreen()
{
}


void plSetGraphMode(unsigned char size)
{
}

char *convnum(unsigned long num, char *buf, unsigned char radix, unsigned short len, char clip0)
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

void writenum(short *buf, unsigned short ofs, unsigned char attr, unsigned long num, unsigned char radix, unsigned short len, char clip0)
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
//  printf("write %x+%d*2 bytes, %x attr,  %s, %d bytes\n", buf, ofs, attr, str, len);
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
  memcpyb(((char *)buf)+ofs*2, (void *)str, len*2);
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
  if((y*plScrWidth+x+len)>(plScrWidth*plScrHeight))
  {
   printf("illegal displaystr: %d,%d -> %d bytes, %s\n", x, y, len, str);
   return;
  }
  char *p=((char*)videomem)+(y*plScrRowBytes+x*2);
  attr=plpalette[attr];
  unsigned short i;
  int changed=0;
  for (i=0; i<len; i++)
  {
    if(*p!=*str)
     changed=1;
    *p++=*str;
    if(*str)
      str++;
    if(*p!=attr)
     changed=1;
    *p++=attr;
  }
  if(changed) poutwinInvalidate(x, y, x+len, y);
}


void displaystrattr(unsigned short y, unsigned short x, const short *buf, unsigned short len)
{
  if((y*plScrWidth+x+len)>(plScrWidth*plScrHeight))
  {
   printf("illegal displaystrattr: %d,%d -> %d bytes, %c%c%c%c\n", x, y, len, buf[0], buf[2], buf[4], buf[6]);
   return;
  }
  char *p=((char*)videomem)+(y*plScrRowBytes+x*2);
  char *b=(char *)buf;
  int changed=0;
  for (int i=0; i<len*2; i+=2)
  {
    if(p[i]!=b[i]);
     changed=1;
    p[i]=b[i];
    if(p[i+1]!=plpalette[b[i+1]])
     changed=1;
    p[i+1]=plpalette[b[i+1]];
  }
  if(changed) poutwinInvalidate(x, y, x+len, y);
}


void displaystrattrdi(unsigned short y, unsigned short x, const unsigned char *txt, const unsigned char *attr, unsigned short len)
{
  if((y*plScrWidth+x+len)>(plScrWidth*plScrHeight))
  {
   printf("illegal displaystrattrdi: %d,%d -> %d bytes, %s\n", x, y, len, txt);
   return;
  }
  char *p=((char*)videomem)+(y*plScrRowBytes+x*2);
  unsigned short i;
  int changed=0;
  for (i=0; i<len; i++)
  {
    if(*p!=*txt)
     changed=1;
    *p++=*txt++;
    if(*p!=plpalette[*attr]) changed=1;
    *p++=plpalette[*attr++];
  }
  if(changed) poutwinInvalidate(x, y, x+len, y);
}


void displayvoid(unsigned short y, unsigned short x, unsigned short len)
{
  if((y*plScrWidth+x+len)>(plScrWidth*plScrHeight))
  {
   printf("illegal displayvoid: %d,%d -> %d bytes\n", x, y, len);
   return;
  }
  char *addr=((char*)videomem)+y*plScrRowBytes+x*2;
  int changed=0;
  while (len--)
  {
    if(*addr) changed=1;
    *addr++=0;
    if((*addr)&0xF0) changed=1;
    *addr++=plpalette[0];
  }
  if(changed) poutwinInvalidate(x, y, x+len, y);
}

void gdrawchar(unsigned short x, unsigned short y, unsigned char c, unsigned char f, unsigned char b)
{
}

void gdrawchart(unsigned short x, unsigned short y, unsigned char c, unsigned char f)
{
}

void gdrawcharp(unsigned short x, unsigned short y, unsigned char c, unsigned char f, void *picp)
{
}

void gdrawchar8(unsigned short x, unsigned short y, unsigned char c, unsigned char f, unsigned char b)
{
}

void gdrawchar8t(unsigned short x, unsigned short y, unsigned char c, unsigned char f)
{
}

void gdrawchar8p(unsigned short x, unsigned short y, unsigned char c, unsigned char f, void *picp)
{
}

void gdrawstr(unsigned short y, unsigned short x, const char *str, unsigned short len, unsigned char f, unsigned char b)
{
}

void gupdatestr(unsigned short y, unsigned short x, const short *str, unsigned short len, short *old)
{
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
  char *scrptr=((char*)videomem)+(2*x+yb*plScrRowBytes);
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
  poutwinInvalidate(x, yb-yh, x, yb);
 
}

static int outInit()
{
 return poutwinInit();
}

static void outClose()
{
 poutwinClose();
}

extern "C"
{
  initcloseregstruct outReg = {outInit, outClose};
  char *dllinfo = "preinitclose _outReg";
};
#else
#error please compile under Win32.
#endif

