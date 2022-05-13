// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIFace main volume bar
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'

#define NO_CPIFACE_IMPORT
#include <string.h>
#include "psetting.h"
#include "poutput.h"
#include "cpiface.h"

#ifndef MEKKACOL

#define COLTEXT 0x07
#define COLMUTE 0x08
#define STRLS "þ\x0Fþ\x0Fþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x09þ\x09þ\x09þ\x09þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01"
#define STRRS "þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x09þ\x09þ\x09þ\x09þ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Fþ\x0F"
#define STRLL "þ\x0Fþ\x0Fþ\x0Fþ\x0Fþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x09þ\x09þ\x09þ\x09þ\x09þ\x09þ\x09þ\x09þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01"
#define STRRL "þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x09þ\x09þ\x09þ\x09þ\x09þ\x09þ\x09þ\x09þ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Fþ\x0Fþ\x0Fþ\x0F"

#else

#define COLTEXT 0xF8
#define COLMUTE 0xF7
#define STRLS "þ\xF0þ\xF0þ\xF3þ\xF3þ\xF3þ\xF3þ\xF1þ\xF1þ\xF1þ\xF1þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9"
#define STRRS "þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF1þ\xF1þ\xF1þ\xF1þ\xF3þ\xF3þ\xF3þ\xF3þ\xF0þ\xF0"
#define STRLL "þ\xF0þ\xF0þ\xF0þ\xF0þ\xF3þ\xF3þ\xF3þ\xF3þ\xF3þ\xF3þ\xF3þ\xF3þ\xF1þ\xF1þ\xF1þ\xF1þ\xF1þ\xF1þ\xF1þ\xF1þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9"
#define STRRL "þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF9þ\xF1þ\xF1þ\xF1þ\xF1þ\xF1þ\xF1þ\xF1þ\xF1þ\xF3þ\xF3þ\xF3þ\xF3þ\xF3þ\xF3þ\xF3þ\xF3þ\x0Fþ\x0Fþ\x0Fþ\x0F"

#endif

static int plMVolFirstLine;
static int plMVolHeight;
static int plMVolWidth;
static int plMVolType;

static void logvolbar(int &l, int &r)
{
  if (l>32)
    l=32+((l-32)>>1);
  if (l>48)
    l=48+((l-48)>>1);
  if (l>56)
    l=56+((l-56)>>1);
  if (l>64)
    l=64;
  if (r>32)
    r=32+((r-32)>>1);
  if (r>48)
    r=48+((r-48)>>1);
  if (r>56)
    r=56+((r-56)>>1);
  if (r>64)
    r=64;
}

static void drawpeakpower(int y, int x)
{
  short strbuf[40];
  writestring(strbuf, 0, plPause?COLMUTE:COLTEXT, " [ùùùùùùùùùùùùùùùù -- ùùùùùùùùùùùùùùùù] ", 40);
  int l,r;
  plGetRealMasterVolume(l, r);
  logvolbar(l, r);
  l=(l+2)>>2;
  r=(r+2)>>2;
  if (plPause)
  {
    writestring(strbuf, 18-l, COLMUTE, "þþþþþþþþþþþþþþþþ", l);
    writestring(strbuf, 22, COLMUTE, "þþþþþþþþþþþþþþþþ", r);
  }
  else
  {
    writestringattr(strbuf, 18-l, STRLS+32-l-l, l);
    writestringattr(strbuf, 22, STRRS, r);
  }
  displaystrattr(y, x, strbuf, 40);
  if (plMVolHeight==2)
    displaystrattr(y+1, x, strbuf, 40);
}

static void drawbigpeakpower(int y, int x)
{
  short strbuf[80];
  writestring(strbuf, 0, plPause?COLMUTE:COLTEXT, "   [ùùùùùùùùùùùùùùùùùùùùùùùùùùùùùùùù -=ðð=- ùùùùùùùùùùùùùùùùùùùùùùùùùùùùùùùù]   ", 80);
  int l,r;
  plGetRealMasterVolume(l, r);
  logvolbar(l, r);
  l=(l+1)>>1;
  r=(r+1)>>1;
  if (plPause)
  {
    writestring(strbuf, 36-l, COLMUTE, "þþþþþþþþþþþþþþþþþþþþþþþþþþþþþþþþ", l);
    writestring(strbuf, 44, COLMUTE, "þþþþþþþþþþþþþþþþþþþþþþþþþþþþþþþþ", r);
  }
  else
  {
    writestringattr(strbuf, 36-l, STRLL+64-l-l, l);
    writestringattr(strbuf, 44, STRRL, r);
  }
  displaystrattr(y, x, strbuf, 80);
  if (plMVolHeight==2)
    displaystrattr(y+1, x, strbuf, 80);
}

static void MVolDraw(int)
{
  if (plMVolType==2)
  {
    displaystr(plMVolFirstLine, 80, COLTEXT, "", 8);
    displaystr(plMVolFirstLine, 128, COLTEXT, "", 4);
    if (plMVolHeight==2)
    {
      displaystr(plMVolFirstLine+1, 80, COLTEXT, "", 8);
      displaystr(plMVolFirstLine+1, 128, COLTEXT, "", 4);
    }
    drawpeakpower(plMVolFirstLine, 88);
  }
  else
  {
    int l=(plMVolWidth==132)?26:20;
    displaystr(plMVolFirstLine, 0, plPause?COLMUTE:COLTEXT, "  peak power level:", l);
    displaystr(plMVolFirstLine, plMVolWidth-l, COLTEXT, "", l);
    if (plMVolHeight==2)
    {
      displaystr(plMVolFirstLine+1, 0, COLTEXT, "", l);
      displaystr(plMVolFirstLine+1, plMVolWidth-l, COLTEXT, "", l);
    }
    if (plMVolWidth==132)
      drawbigpeakpower(plMVolFirstLine, l);
    else
      drawpeakpower(plMVolFirstLine, l);
  }
}

static void MVolSetWin(int, int wid, int ypos, int hgt)
{
  plMVolFirstLine=ypos;
  plMVolHeight=hgt;
  plMVolWidth=wid;
}

static int MVolGetWin(cpitextmodequerystruct &q)
{
  if ((plMVolType==2)&&(plScrWidth!=132))
    plMVolType=0;
  int pplheight=(plScrHeight>30)?2:1;

  switch (plMVolType)
  {
  case 0:
    return 0;
  case 1:
    q.xmode=3;
    break;
  case 2:
    q.xmode=2;
    break;
  }
  q.size=0;
  q.top=1;
  q.killprio=128;
  q.viewprio=176;
  q.hgtmax=pplheight;
  q.hgtmin=pplheight;
  return 1;
}

static int MVolIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'v': case 'V':
    cpiTextSetMode("mvol");
    break;
  case 'x': case 'X':
    plMVolType=plNLChan?2:1;
    return 0;
  case 0x2d00: //alt-x
    plMVolType=1;
    return 0;
  default:
    return 0;
  }
  return 1;
}

static int MVolAProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'v': case 'V':
    plMVolType=(plMVolType+1)%3;
    cpiTextRecalc();
    break;
  default:
    return 0;
  }
  return 1;
}

static int MVolCan()
{
  return !!plGetRealMasterVolume;
}

static int MVolEvent(int ev)
{
  switch (ev)
  {
  case cpievInit: return MVolCan();
  case cpievInitAll:
    plMVolType=cfGetProfileInt2(cfScreenSec, "screen", "mvoltype", 2, 10)%3;
    return 1;
  }
  return 1;
}

extern "C"
{
  cpitextmoderegstruct cpiTModeMVol = {"mvol", MVolGetWin, MVolSetWin, MVolDraw, MVolIProcessKey, MVolAProcessKey, MVolEvent};
};
