// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface text mode channel display
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


static void (*ChanDisplay)(short *buf, int len, int i);
static short plChanFirstLine;
static short plChanHeight;
static short plChanWidth;
static char plChannelType;


static void drawchannels()
{
  short buf[132];
  int i,y,x;
  int h=(plChannelType==1)?((plNLChan+1)/2):plNLChan;
  int sh=(plChannelType==1)?(plSelCh/2):plSelCh;
  int first;
  if (h>plChanHeight)
    if (sh<(plChanHeight/2))
      first=0;
    else
      if (sh>=(h-plChanHeight/2))
        first=h-plChanHeight;
      else
        first=sh-(plChanHeight-1)/2;
  else
    first=0;

  for (y=0; y<plChanHeight; y++)
  {
    char *sign=" ";
    if (!y&&first)
      sign="\x18";
    if (((y+1)==plChanHeight)&&((y+first+1)!=h))
      sign="\x19";
    if (plChannelType==1)
    {
      for (x=0; x<2; x++)
      {
        i=2*first+y*2+x;
        if (plPanType&&(y&1))
          i^=1;
        if (i<plNLChan)
        {
          if (plChanWidth==80)
          {
            writestring(buf, x*40, plMuteCh[i]?0x08:0x07, " ##:", 4);
            writestring(buf, x*40, 0x0F, (i==plSelCh)?">":sign, 1);
            writenum(buf, x*40+1, plMuteCh[i]?0x08:0x07, i+1, 10, 2);
            ChanDisplay(buf+x*40+4, 36, i);
          }
          else
          {
            writestring(buf, x*66, plMuteCh[i]?0x08:0x07, " ##:", 4);
            writestring(buf, x*66, 0x0F, (i==plSelCh)?">":sign, 1);
            writenum(buf, x*66+1, plMuteCh[i]?0x08:0x07, i+1, 10, 2);
            ChanDisplay(buf+x*66+4, 62, i);
          }
        }
        else
          writestring(buf, x*plChanWidth/2, 0, "", plChanWidth/2);
      }
    }
    else
    {
      int i=y+first;
      if ((y+first)==plSelCh)
        sign=">";
      if (plChannelType==2)
      {
        writestring(buf, 0, plMuteCh[i]?0x08:0x07, " ##:", 4);
        writestring(buf, 0, 0x0F, sign, 1);
        writenum(buf, 1, plMuteCh[i]?0x08:0x07, i+1, 10, 2);
        ChanDisplay(buf+4, (plChanWidth==80)?76:128, y+first);
      }
      else
      {
        writestring(buf, 0, plMuteCh[i]?0x08:0x07, "     ##:", 8);
        writestring(buf, 4, 0x0F, sign, 1);
        writenum(buf, 5, plMuteCh[i]?0x08:0x07, i+1, 10, 2);
        ChanDisplay(buf+8, 44, y+first);
      }
    }
    displaystrattr(plChanFirstLine+y, (plChannelType==3)?80:0, buf, plChanWidth);
  }
}

static void ChanSetWin(int, int wid, int ypos, int hgt)
{
  plChanFirstLine=ypos;
  plChanHeight=hgt;
  plChanWidth=wid;
}

static int ChanGetWin(cpitextmodequerystruct &q)
{
  if ((plChannelType==3)&&(plScrWidth!=132))
    plChannelType=0;
  if (!plNLChan)
    return 0;

  switch (plChannelType)
  {
  case 0:
    return 0;
  case 1:
    q.hgtmax=(plNLChan+1)>>1;
    q.xmode=3;
    break;
  case 2:
    q.hgtmax=plNLChan;
    q.xmode=1;
    break;
  case 3:
    q.hgtmax=plNLChan;
    q.xmode=2;
    break;
  }
  q.size=1;
  q.top=1;
  q.killprio=128;
  q.viewprio=160;
  q.hgtmin=2;
  if (q.hgtmin>q.hgtmax)
    q.hgtmin=q.hgtmax;
  return 1;
}

static void ChanDraw(int)
{
  drawchannels();
}

static int ChanIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'c': case 'C':
    cpiTextSetMode("chan");
    break;
  case 'x': case 'X':
    plChannelType=3;
    return 0;
  case 0x2d00: //alt-x
    plChannelType=2;
    return 0;
  default:
    return 0;
  }
  return 1;
}

static int ChanAProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'c': case 'C':
    plChannelType=(plChannelType+1)%4;
    cpiTextRecalc();
    break;
  default:
    return 0;
  }
  return 1;
}

static int ChanEvent(int ev)
{
  switch (ev)
  {
  case cpievInitAll:
    plChannelType=cfGetProfileInt2(cfScreenSec, "screen", "channeltype", 3, 10)&3;
    return 0;
  }
  return 1;
}

extern "C"
{
  cpitextmoderegstruct cpiTModeChan = {"chan", ChanGetWin, ChanSetWin, ChanDraw, ChanIProcessKey, ChanAProcessKey, ChanEvent};
};


void plUseChannels(void (*Display)(short *buf, int len, int i))
{
  ChanDisplay=Display;
  if (!plNLChan)
    return;
  cpiTextRegisterMode(&cpiTModeChan);
}
