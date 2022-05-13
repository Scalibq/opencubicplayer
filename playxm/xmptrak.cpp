// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// XMPlay track/channel display routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -removed all references to gmd structures to make this more flexible

#include "poutput.h"
#include "cpiface.h"
#include "xmplay.h"

#ifndef MEKKACOL

#define COLPTNOTE 0x0A
#define COLNOTE 0x0F
#define COLPITCH 0x02
#define COLSPEED 0x02
#define COLPAN 0x05
#define COLVOL 0x09
#define COLACT 0x04
#define COLINS 0x07

#else

#define COLPTNOTE 0xF2
#define COLNOTE 0xF0
#define COLPITCH 0xFA
#define COLSPEED 0xFA
#define COLPAN 0xFD
#define COLVOL 0xF9
#define COLACT 0xFC
#define COLINS 0xF8

#endif

static unsigned short *plPatLens;
static unsigned char (**plPatterns)[5];
static const unsigned short *plOrders;
static unsigned char (*xmcurpat)[5];
static int xmcurchan;
static int xmcurrow;
static int xmcurpatlen;

static int xmgetpatlen(int n)
{
  if (plOrders[n]==0xFFFF)
    return 0;
  return plPatLens[plOrders[n]];
}

static void xmseektrack(int n, int c)
{
  xmcurpat=plPatterns[plOrders[n]]-plNLChan;
  xmcurchan=c;
  xmcurrow=-1;
  xmcurpatlen=plPatLens[plOrders[n]];
}

static int xmstartrow()
{
  xmcurrow++;
  xmcurpat+=plNLChan;
  if (xmcurrow>=xmcurpatlen)
    return -1;
  return xmcurrow;
}

static int xmgetnote(short *bp, int small)
{
  int note=xmcurpat[xmcurchan][0];
  if (!note)
    return 0;
  note--;
  int porta=0;
  if (xmcurpat[xmcurchan][3]==xmpCmdPortaNote)
    porta=1;
  if (xmcurpat[xmcurchan][3]==xmpCmdPortaVol)
    porta=1;
  if ((xmcurpat[xmcurchan][2]>>4)==xmpVCmdPortaNote)
    porta=1;
  switch (small)
  {
  case 0:
    if (note==96)
      writestring(bp, 0, COLINS, "---", 3);
    else
    {
      writestring(bp, 0, porta?COLPTNOTE:COLNOTE, "CCDDEFFGGAAB"+(note%12), 1);
      writestring(bp, 1, porta?COLPTNOTE:COLNOTE, "-#-#--#-#-#-"+(note%12), 1);
      writestring(bp, 2, porta?COLPTNOTE:COLNOTE, "01234567"+(note/12), 1);
    }
    break;
  case 1:
    if (note==96)
      writestring(bp, 0, COLINS, "--", 2);
    else
    {
      writestring(bp, 0, porta?COLPTNOTE:COLNOTE, "cCdDefFgGaAb"+(note%12), 1);
      writestring(bp, 1, porta?COLPTNOTE:COLNOTE, "01234567"+(note/12), 1);
    }
    break;
  case 2:
    if (note==96)
      writestring(bp, 0, COLINS, "-", 1);
    else
      writestring(bp, 0, porta?COLPTNOTE:COLNOTE, "cCdDefFgGaAb"+(note%12), 1);
    break;
  }
  return 1;
}

static int xmgetins(short *bp)
{
  int ins=xmcurpat[xmcurchan][1];
  if (!ins)
    return 0;
  writenum(bp, 0, COLINS, ins, 16, 2, 0);
  return 1;
}

static int xmgetvol(short *bp)
{
  int vol=xmcurpat[xmcurchan][2];
  if ((vol>=0x10)&&(vol<0x60))
  {
    writenum(bp, 0, COLVOL, vol-0x10, 16, 2, 0);
    return 1;
  }
  if (xmcurpat[xmcurchan][3]==xmpCmdVolume)
  {
    writenum(bp, 0, COLVOL, xmcurpat[xmcurchan][4], 16, 2, 0);
    return 1;
  }
  return 0;
}

static int xmgetpan(short *bp)
{
  if ((xmcurpat[xmcurchan][2]>>4)==xmpVCmdPanning)
  {
    writenum(bp, 0, COLPAN, (xmcurpat[xmcurchan][2]&0xF)*0x11, 16, 2, 0);
    return 1;
  }
  if (xmcurpat[xmcurchan][3]==xmpCmdPanning)
  {
    writenum(bp, 0, COLPAN, xmcurpat[xmcurchan][4], 16, 2, 0);
    return 1;
  }
  if (xmcurpat[xmcurchan][3]==xmpCmdSPanning)
  {
    writenum(bp, 0, COLPAN, xmcurpat[xmcurchan][4]*0x11, 16, 2, 0);
    return 1;
  }
  return 0;
}

static void xmgetfx(short *bp, int n)
{
  int p=0;
  int data=xmcurpat[xmcurchan][2]&0xF;
  switch (xmcurpat[xmcurchan][2]>>4)
  {
  case xmpVCmdVolSlideD:
    writestring(bp, 0, COLVOL, "\x19", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case xmpVCmdVolSlideU:
    writestring(bp, 0, COLVOL, "\x18", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case xmpVCmdFVolSlideD:
    writestring(bp, 0, COLVOL, "-", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case xmpVCmdFVolSlideU:
    writestring(bp, 0, COLVOL, "+", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case xmpVCmdVibRate:
    writestring(bp, 0, COLPITCH, "~\x1A", 2);
    writenum(bp, 2, COLPITCH, data, 16, 1, 0);
    break;
  case xmpVCmdVibDep:
    writestring(bp, 0, COLPITCH, "~", 1);
    writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    break;
  case xmpVCmdPanSlideL:
    writestring(bp, 0, COLPAN, "\x1B", 1);
    writenum(bp, 1, COLPAN, data, 16, 2, 0);
    break;
  case xmpVCmdPanSlideR:
    writestring(bp, 0, COLPAN, "\x1A", 1);
    writenum(bp, 1, COLPAN, data, 16, 2, 0);
    break;
  case xmpVCmdPortaNote:
    writestring(bp, 0, COLPITCH, "\x0D", 1);
    writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    break;
  default:
    bp-=3;
    p--;
  }
  bp+=3;
  p++;

  if (p==n)
    return;
  data=xmcurpat[xmcurchan][4];
  switch (xmcurpat[xmcurchan][3])
  {
  case xmpCmdArpeggio:
    if (data)
    {
      writestring(bp, 0, COLPITCH, "ð", 1);
      writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    }
    break;
  case xmpCmdPortaU:
    writestring(bp, 0, COLPITCH, "\x18", 1);
    writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    break;
  case xmpCmdPortaD:
    writestring(bp, 0, COLPITCH, "\x19", 1);
    writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    break;
  case xmpCmdPortaNote:
    writestring(bp, 0, COLPITCH, "\x0D", 1);
    writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    break;
  case xmpCmdVibrato:
    writestring(bp, 0, COLPITCH, "~", 1);
    writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    break;
  case xmpCmdPortaVol:
    writestring(bp, 0, COLPITCH, "\x0D", 1);
    if (!data)
      writestring(bp, 1, COLVOL, "\x12""0", 2);
    else
    if (data&0xF0)
    {
      writestring(bp, 1, COLVOL, "\x18", 1);
      writenum(bp, 2, COLVOL, data>>4, 16, 1, 0);
    }
    else
    {
      writestring(bp, 1, COLVOL, "\x19", 1);
      writenum(bp, 2, COLVOL, data&0xF, 16, 1, 0);
    }
    break;
  case xmpCmdVibVol:
    writestring(bp, 0, COLPITCH, "~", 1);
    if (!data)
      writestring(bp, 1, COLVOL, "\x12""0", 2);
    else
    if (data&0xF0)
    {
      writestring(bp, 1, COLVOL, "\x18", 1);
      writenum(bp, 2, COLVOL, data>>4, 16, 1, 0);
    }
    else
    {
      writestring(bp, 1, COLVOL, "\x19", 1);
      writenum(bp, 2, COLVOL, data&0xF, 16, 1, 0);
    }
    break;
  case xmpCmdTremolo:
    writestring(bp, 0, COLVOL, "~", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case xmpCmdOffset:
    writestring(bp, 0, COLACT, "\x1A", 1);
    writenum(bp, 1, COLACT, data, 16, 2, 0);
    break;
  case xmpCmdVolSlide:
    if (!data)
      writestring(bp, 0, COLVOL, "\x12""00", 3);
    else
    if (data&0xF0)
    {
      writestring(bp, 0, COLVOL, "\x18", 1);
      writenum(bp, 1, COLVOL, data>>4, 16, 2, 0);
    }
    else
    {
      writestring(bp, 0, COLVOL, "\x19", 1);
      writenum(bp, 1, COLVOL, data&0xF, 16, 2, 0);
    }
    break;
  case xmpCmdKeyOff:
    writestring(bp, 0, COLINS, "-", 1);
    writenum(bp, 1, COLINS, data&0xF, 16, 2, 0);
    break;
  case xmpCmdEnvPos:
    writestring(bp, 0, COLINS, "\x1A", 1);
    writenum(bp, 1, COLINS, data, 16, 2, 0);
    break;
  case xmpCmdMRetrigger: case xmpCmdRetrigger:
    writestring(bp, 0, COLACT, "\x13", 1);
    writenum(bp, 1, COLACT, data, 16, 2, 0);
    break;
  case xmpCmdTremor:
    writestring(bp, 0, COLVOL, "\xA9", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case xmpCmdNoteCut:
    writestring(bp, 0, COLACT, "^", 1);
    writenum(bp, 1, COLACT, data, 16, 2, 0);
    break;
  case xmpCmdFPortaU:
    writestring(bp, 0, COLPITCH, "+", 1);
    writenum(bp, 1, COLPITCH, data*4, 16, 2, 0);
    break;
  case xmpCmdFPortaD:
    writestring(bp, 0, COLPITCH, "-", 1);
    writenum(bp, 1, COLPITCH, data*4, 16, 2, 0);
    break;
  case xmpCmdDelayNote:
    writestring(bp, 0, COLACT, "d", 1);
    writenum(bp, 1, COLACT, data, 16, 2, 0);
    break;
  case xmpCmdFVolSlideU:
    writestring(bp, 0, COLVOL, "+", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case xmpCmdFVolSlideD:
    writestring(bp, 0, COLVOL, "-", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case xmpCmdPanSlide:
    if (!data)
      writestring(bp, 0, COLPAN, "\1D""00", 3);
    else
    if (data&0xF0)
    {
      writestring(bp, 0, COLPAN, "\x1A", 1);
      writenum(bp, 1, COLPAN, data>>4, 16, 2, 0);
    }
    else
    {
      writestring(bp, 0, COLPAN, "\x1B", 1);
      writenum(bp, 1, COLPAN, data&0xF, 16, 2, 0);
    }
    break;
  case xmpCmdXPorta:
    if ((data>>4)==1)
    {
      writestring(bp, 0, COLPITCH, "+", 1);
      writenum(bp, 1, COLPITCH, data&0xF, 16, 2, 0);
    }
    else
    if ((data>>4)==2)
    {
      writestring(bp, 0, COLPITCH, "-", 1);
      writenum(bp, 1, COLPITCH, data&0xF, 16, 2, 0);
    }
    break;
  case xmpCmdGlissando:
    if (data)
      writestring(bp, 0, COLPITCH, "\x0D=\xA9", 3);
    else
      writestring(bp, 0, COLPITCH, "\x0D=/", 3);
    break;
  case xmpCmdVibType:
    writestring(bp, 0, COLPITCH, "~=", 2);
    writestring(bp, 2, COLPITCH, "~\\\xA9?"+(data&3), 1);
    break;
  case xmpCmdTremType:
    writestring(bp, 0, COLVOL, "~=", 2);
    writestring(bp, 2, COLVOL, "~\\\xA9?"+(data&3), 1);
    break;
  case xmpCmdSFinetune:
    writestring(bp, 0, COLINS, "ft", 2);
    writenum(bp, 2, COLINS, data, 16, 1, 0);
    break;
  }
}

static void xmgetgcmd(short *bp, int n)
{
  int p=0;
  int i;
  for (i=0; i<plNLChan; i++)
  {
    if (p==n)
      break;
    int data=xmcurpat[i][4];
    switch (xmcurpat[i][3])
    {
    case xmpCmdJump:
      writestring(bp, 0, COLACT, "\x1A", 1);
      writenum(bp, 1, COLACT, data, 16, 2, 0);
      break;
    case xmpCmdBreak:
      writestring(bp, 0, COLACT, "\x19", 1);
      writenum(bp, 1, COLACT, data, 16, 2, 0);
      break;
    case xmpCmdSpeed:
      if (!data)
        writestring(bp, 0, COLACT, "end", 3);
      else
      if (data<0x20)
      {
        writestring(bp, 0, COLSPEED, "t", 1);
        writenum(bp, 1, COLSPEED, data, 16, 2, 0);
      }
      else
      {
        writestring(bp, 0, COLSPEED, "b", 1);
        writenum(bp, 1, COLSPEED, data, 16, 2, 0);
      }
      break;
    case xmpCmdGVolume:
      writestring(bp, 0, COLVOL, "v", 1);
      writenum(bp, 1, COLVOL, data, 16, 2, 0);
      break;
    case xmpCmdGVolSlide:
      if (!data)
        writestring(bp, 0, COLVOL, "\x12""00", 3);
      else
      if (data&0xF0)
      {
        writestring(bp, 0, COLVOL, "\x18", 1);
        writenum(bp, 1, COLVOL, data>>4, 16, 2, 0);
      }
      else
      {
        writestring(bp, 0, COLVOL, "\x19", 1);
        writenum(bp, 1, COLVOL, data&0xF, 16, 2, 0);
      }
      break;
    case xmpCmdPatLoop:
      writestring(bp, 0, COLACT, "pl", 2);
      writenum(bp, 2, COLACT, data, 16, 1, 0);
      break;
    case xmpCmdPatDelay:
      writestring(bp, 0, COLACT, "pd", 2);
      writenum(bp, 2, COLACT, data, 16, 1, 0);
      break;
    default:
      bp-=4;
      p--;
    }
    bp+=4;
    p++;
  }
}

static const char *xmgetpatname(int)
{
  return 0;
}

static int xmgetcurpos()
{
  return xmpGetRealPos()>>8;
}

static cpitrakdisplaystruct xmtrakdisplay=
{
  xmgetcurpos, xmgetpatlen, xmgetpatname, xmseektrack, xmstartrow, xmgetnote,
  xmgetins, xmgetvol, xmgetpan, xmgetfx, xmgetgcmd
};

void xmTrkSetup(const xmodule &mod)
{
  plPatterns=mod.patterns;
  plOrders=mod.orders;
  plPatLens=mod.patlens;
  cpiTrkSetup(xmtrakdisplay, mod.nord);
}