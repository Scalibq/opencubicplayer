// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface text mode instrument display
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'
//  -kb981201   Tammo Hinrichs <opencp@gmx.net>
//    -added calling plInsDisplay::Done (Memory Leak)
//  -fd981215   Felix Domke <tmbinc@gmx.net>
//    -plInsDisplay::Done should be only called when provided, not
//     if it's 0. (this caused crash with the gmi-player.)

#define NO_CPIFACE_IMPORT
#include <string.h>
#include "psetting.h"
#include "poutput.h"
#include "cpiface.h"

static insdisplaystruct plInsDisplay;

static short plInstScroll;
static short plInstFirstLine;
static short plInstLength;
static short plInstHeight;
static short plInstWidth;
static char plInstType;
static char plInstMode;

static void displayshortins(char sel)
{
  displaystr(plInstFirstLine-1, 0, sel?0x09:0x01, "   instruments (short):", 23);
  displaystr(plInstFirstLine-1, 23, 0x08, " press i to toggle mode", plInstWidth-23);
  int y,x,i;
  for (y=0; y<plInstHeight; y++)
  {
    if (y>=plInstLength)
    {
      displayvoid(y+plInstFirstLine, 0, plInstWidth);
      continue;
    }

    for (x=0; x<2; x++)
    {
      i=y+plInstScroll+x*plInstLength;
      if (i>=plInsDisplay.n40)
      {
        displayvoid(y+plInstFirstLine, x*40, 40);
        continue;
      }
      short buf[40];

      plInsDisplay.Display(buf, 40, i, plInstMode);

      displaystrattr(y+plInstFirstLine, x*40, buf, 40);
    }
  }
}

static void displayxshortins(char sel)
{
  displaystr(plInstFirstLine-1, 0, sel?0x09:0x01, "   instruments (short):", 23);
  displaystr(plInstFirstLine-1, 23, 0x08, " press i to toggle mode", plInstWidth-23);
  int y,x,i;
  for (y=0; y<plInstHeight; y++)
  {
    if (y>=plInstLength)
    {
      displayvoid(y+plInstFirstLine, 0, plInstWidth);
      continue;
    }
    for (x=0; x<4; x++)
    {
      i=y+plInstScroll+x*plInstLength;
      if (i>=plInsDisplay.n40)
      {
        displayvoid(y+plInstFirstLine, x*33, 33);
        continue;
      }

      short buf[33];
      plInsDisplay.Display(buf, 33, i, plInstMode);
      displaystrattr(y+plInstFirstLine, x*33, buf, 33);
    }
  }
}

static void displaysideins(char sel)
{
  displaystr(plInstFirstLine-1, 80, sel?0x09:0x01, "       instruments (side): ", 27);
  displaystr(plInstFirstLine-1, 107, 0x08, " press i to toggle mode", 52-27);
  int y;
  for (y=0; y<plInstHeight; y++)
  {
    if (y>=plInsDisplay.n52)
    {
      displayvoid(y+plInstFirstLine, 80, 52);
      continue;
    }

    short buf[52];
    plInsDisplay.Display(buf, 52, y+plInstScroll, plInstMode);
    displaystrattr(y+plInstFirstLine, 80, buf, 52);
  }
}

static void displaylongins(char sel)
{
  displaystr(plInstFirstLine-2, 0, sel?0x09:0x01, "   instruments (long): ", 23);
  displaystr(plInstFirstLine-2, 23, 0x08, " press i to toggle mode", 57);
  displaystr(plInstFirstLine-1, 0, 0x07, plInsDisplay.title80, 80);
  int y;
  for (y=0; y<plInstHeight; y++)
  {
    if (y>=plInsDisplay.n80)
    {
      displayvoid(y+plInstFirstLine, 0, 80);
      continue;
    }
    short buf[80];
    plInsDisplay.Display(buf, 80, y+plInstScroll, plInstMode);

    displaystrattr(y+plInstFirstLine, 0, buf, 80);
  }
}

static void displayxlongins(char sel)
{
  displaystr(plInstFirstLine-2, 0, sel?0x09:0x01, "   instruments (long): ", 23);
  displaystr(plInstFirstLine-2, 23, 0x08, " press i to toggle mode", 109);
  displaystr(plInstFirstLine-1, 0, 0x07, plInsDisplay.title132, 132);
  int y;
  for (y=0; y<plInstHeight; y++)
  {
    if (y>=plInsDisplay.n80)
    {
      displayvoid(y+plInstFirstLine, 0, 132);
      continue;
    }
    short buf[132];
    plInsDisplay.Display(buf, 132, y+plInstScroll, plInstMode);
    displaystrattr(y+plInstFirstLine, 0, buf, 132);
  }
}




static void plDisplayInstruments(char sel)
{
  if (!plInstType)
    return;

  if ((plInstScroll+plInstHeight)>plInstLength)
    plInstScroll=plInstLength-plInstHeight;
  if (plInstScroll<0)
    plInstScroll=0;

  plInsDisplay.Mark();

  switch (plInstType)
  {
  case 1:
    if (plInstWidth==132)
      displayxshortins(sel);
    else
      displayshortins(sel);
    break;
  case 2:
    if (plInstWidth==132)
      displayxlongins(sel);
    else
      displaylongins(sel);
    break;
  case 3:
    displaysideins(sel);
    break;
  }
}

static void InstSetWin(int, int wid, int ypos, int hgt)
{
  int titlehgt=(plInstType==2)?2:1;
  plInstFirstLine=ypos+titlehgt;
  plInstHeight=hgt-titlehgt;
  plInstWidth=wid;

  if (plInstType==1)
    if (plInstWidth==132)
      plInstLength=(plInsDisplay.n40+3)/4;
    else
      plInstLength=(plInsDisplay.n40+1)/2;
  else
  if (plInstType==2)
    plInstLength=plInsDisplay.n80;
  else
    plInstLength=plInsDisplay.n52;
}

static int InstGetWin(cpitextmodequerystruct &q)
{
  if ((plInstType==3)&&(plScrWidth!=132))
    plInstType=0;

  switch (plInstType)
  {
  case 0:
    return 0;
  case 1:
    q.hgtmin=2;
    q.hgtmax=1+(plInsDisplay.n40+1)/2;
    q.xmode=1;
    break;
  case 2:
    q.hgtmin=3;
    q.hgtmax=2+plInsDisplay.n80;
    q.xmode=1;
    break;
  case 3:
    q.hgtmin=2;
    q.hgtmax=1+plInsDisplay.n52;
    q.xmode=2;
    break;
  }
  q.size=1;
  q.top=1;
  q.killprio=96;
  q.viewprio=144;
  if (q.hgtmin>q.hgtmax)
    q.hgtmin=q.hgtmax;
  return 1;
}

static void InstDraw(int focus)
{
  plDisplayInstruments(focus);
}

static int InstIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'i': case 'I':
    cpiTextSetMode("inst");
    break;
  case 'x': case 'X':
    plInstType=3;
    return 0;
  case 0x2d00: //alt-x
    plInstType=1;
    return 0;
  default:
    return 0;
  }
  return 1;
}

static int InstAProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'i': case 'I':
    plInstType=(plInstType+1)%4;
    cpiTextRecalc();
    break;
  case 0x4900: //pgup
    plInstScroll--;
    break;
  case 0x5100: //pgdn
    plInstScroll++;
    break;
  case 0x8400: //ctrl-pgup
    plInstScroll-=plInstHeight;
    break;
  case 0x7600: //ctrl-pgdn
    plInstScroll+=plInstHeight;
    break;
  case 0x4700: //home
    plInstScroll=0;
    break;
  case 0x4F00: //end
    plInstScroll=plInstLength;
    break;
  case 0x1700: // alt-i
    plInsDisplay.Clear();
    break;
  case 9: // tab
  case 0x0F00: // shift-tab
  case 0xA500:
    plInstMode=!plInstMode;
    break;
  default:
    return 0;
  }
  return 1;
}

static int InstEvent(int ev)
{
  switch (ev)
  {
  case cpievInitAll:
    plInstType=cfGetProfileInt2(cfScreenSec, "screen", "insttype", 3, 10)&3;
    return 0;
  case cpievDone: case cpievDoneAll:
    if(plInsDisplay.Done) plInsDisplay.Done();
    return 0;
  }
  return 1;
}

extern "C"
{
  cpitextmoderegstruct cpiTModeInst = {"inst", InstGetWin, InstSetWin, InstDraw, InstIProcessKey, InstAProcessKey, InstEvent};
}

void plUseInstruments(insdisplaystruct &x)
{
  plInstScroll=0;
  plInsDisplay=x;
  cpiTextRegisterMode(&cpiTModeInst);
}
