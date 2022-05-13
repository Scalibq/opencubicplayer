// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// ITPlay track/pattern display code
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -added some effects to effect output

#include "poutput.h"
#include "cpiface.h"
#include "mcp.h"
#include "itplay.h"

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

extern itplayerclass itplayer;

static unsigned short *plPatLens;
static unsigned char **plPatterns;
static const unsigned short *plOrders;
static unsigned char *xmcurpat;
static int xmcurchan;
static int xmcurrow;
static int xmcurpatlen;
static unsigned char *curdata;

static int xmgetpatlen(int n)
{
  return (plOrders[n]==0xFFFF)?0:plPatLens[plOrders[n]];
}

static void xmseektrack(int n, int c)
{
  xmcurpat=plPatterns[plOrders[n]];
  xmcurchan=c;
  xmcurrow=0;
  xmcurpatlen=plPatLens[plOrders[n]];
}

static int xmstartrow()
{
  for (curdata=0; !curdata&&(xmcurrow<xmcurpatlen); xmcurrow++)
  {
    if (xmcurchan==-1)
    {
      if (*xmcurpat)
        curdata=xmcurpat;
      while (*xmcurpat)
        xmcurpat+=6;
    }
    else
    {
      while (*xmcurpat)
      {
        if (*xmcurpat==(xmcurchan+1))
          curdata=xmcurpat+1;
        xmcurpat+=6;
      }
    }
    xmcurpat++;
  }
  return curdata?(xmcurrow-1):-1;
}

static int xmgetnote(short *bp, int small)
{
  int note=curdata[0];
  if (!note)
    return 0;
  int porta=0;
  if ((curdata[3]==itplayerclass::cmdPortaNote)||(curdata[3]==itplayerclass::cmdPortaVol))
    porta=1;
  if ((curdata[2]>=itplayerclass::cmdVPortaNote)&&(curdata[2]<(itplayerclass::cmdVPortaNote+10)))
    porta=1;
  switch (small)
  {
  case 0:
    if (note>=itplayerclass::cmdNNoteFade)
      writestring(bp, 0, COLINS, (note==itplayerclass::cmdNNoteOff)?"---":(note==itplayerclass::cmdNNoteCut)?"^^^":"'''", 3);
    else
    {
      note-=itplayerclass::cmdNNote;
      writestring(bp, 0, porta?COLPTNOTE:COLNOTE, "CCDDEFFGGAAB"+(note%12), 1);
      writestring(bp, 1, porta?COLPTNOTE:COLNOTE, "-#-#--#-#-#-"+(note%12), 1);
      writestring(bp, 2, porta?COLPTNOTE:COLNOTE, "0123456789"+(note/12), 1);
    }
    break;
  case 1:
    if (note>=itplayerclass::cmdNNoteFade)
      writestring(bp, 0, COLINS, (note==itplayerclass::cmdNNoteOff)?"--":(note==itplayerclass::cmdNNoteCut)?"^^":"''", 2);
    else
    {
      note-=itplayerclass::cmdNNote;
      writestring(bp, 0, porta?COLPTNOTE:COLNOTE, "cCdDefFgGaAb"+(note%12), 1);
      writestring(bp, 1, porta?COLPTNOTE:COLNOTE, "0123456789"+(note/12), 1);
    }
    break;
  case 2:
    if (note>=itplayerclass::cmdNNoteFade)
      writestring(bp, 0, COLINS, (note==itplayerclass::cmdNNoteOff)?"-":(note==itplayerclass::cmdNNoteCut)?"^":"'", 1);
    else
    {
      note-=itplayerclass::cmdNNote;
      writestring(bp, 0, porta?COLPTNOTE:COLNOTE, "cCdDefFgGaAb"+(note%12), 1);
    }
    break;
  }
  return 1;
}

static int xmgetins(short *bp)
{
  int ins=curdata[1];
  if (!ins)
    return 0;
  writenum(bp, 0, COLINS, ins, 16, 2, 0);
  return 1;
}

static int xmgetvol(short *bp)
{
  int vol=curdata[2];
  if ((vol>=itplayerclass::cmdVVolume)&&(vol<=(itplayerclass::cmdVVolume+64)))
  {
    writenum(bp, 0, COLVOL, vol-itplayerclass::cmdVVolume, 16, 2, 0);
    return 1;
  }
  return 0;
}

static int xmgetpan(short *bp)
{
  int pan=curdata[2];
  if ((pan>=itplayerclass::cmdVPanning)&&(pan<=(itplayerclass::cmdVPanning+64)))
  {
    writenum(bp, 0, COLPAN, pan-itplayerclass::cmdVPanning, 16, 2, 0);
    return 1;
  }
  if (curdata[3]==itplayerclass::cmdPanning)
  {
    writenum(bp, 0, COLPAN, (curdata[4]+1)>>2, 16, 2, 0);
    return 1;
  }
  if ((curdata[3]==itplayerclass::cmdSpecial)&&((curdata[4]>>4)==itplayerclass::cmdSPanning))
  {
    writenum(bp, 0, COLPAN, ((curdata[4]&0xF)*0x11+1)>>2, 16, 2, 0);
    return 1;
  }
  return 0;
}

static char *instfx[]={ "p-c","p-o","p-f","N:c","N:-","N:o","N:f","ve0",
                        "ve1","pe0","pe1","fe0","fe1","???","???","???" };

static void xmgetfx(short *bp, int n)
{
  int data=curdata[2];

  int p=0;
  if ((data>=itplayerclass::cmdVFVolSlU)&&(data<(itplayerclass::cmdVFVolSlU+10)))
  {
    writestring(bp, 0, COLVOL, "+", 1);
    writenum(bp, 1, COLVOL, data-itplayerclass::cmdVFVolSlU, 16, 2, 0);
  }
  else
  if ((data>=itplayerclass::cmdVFVolSlD)&&(data<(itplayerclass::cmdVFVolSlD+10)))
  {
    writestring(bp, 0, COLVOL, "-", 1);
    writenum(bp, 1, COLVOL, data-itplayerclass::cmdVFVolSlD, 16, 2, 0);
  }
  else
  if ((data>=itplayerclass::cmdVVolSlU)&&(data<(itplayerclass::cmdVVolSlU+10)))
  {
    writestring(bp, 0, COLVOL, "\x18", 1);
    writenum(bp, 1, COLVOL, data-itplayerclass::cmdVVolSlU, 16, 2, 0);
  }
  else
  if ((data>=itplayerclass::cmdVVolSlD)&&(data<(itplayerclass::cmdVVolSlD+10)))
  {
    writestring(bp, 0, COLVOL, "\x19", 1);
    writenum(bp, 1, COLVOL, data-itplayerclass::cmdVVolSlD, 16, 2, 0);
  }
  else
  if ((data>=itplayerclass::cmdVPortaNote)&&(data<(itplayerclass::cmdVPortaNote+10)))
  {
    writestring(bp, 0, COLPITCH, "\x0D", 1);
    writenum(bp, 1, COLPITCH, "\x00\x01\x04\x08\x10\x20\x40\x60\x80\xFF"[data-itplayerclass::cmdVPortaNote], 16, 2, 0);
  }
  else
  if ((data>=itplayerclass::cmdVPortaU)&&(data<(itplayerclass::cmdVPortaU+10)))
  {
    writestring(bp, 0, COLPITCH, "\x18", 1);
    writenum(bp, 1, COLPITCH, (data-itplayerclass::cmdVPortaU)*4, 16, 2, 0);
  }
  else
  if ((data>=itplayerclass::cmdVPortaD)&&(data<(itplayerclass::cmdVPortaD+10)))
  {
    writestring(bp, 0, COLPITCH, "\x19", 1);
    writenum(bp, 1, COLPITCH, (data-itplayerclass::cmdVPortaD)*4, 16, 2, 0);
  }
  else
  if ((data>=itplayerclass::cmdVVibrato)&&(data<(itplayerclass::cmdVVibrato+10)))
  {
    writestring(bp, 0, COLPITCH, "~", 1);
    writenum(bp, 1, COLPITCH, data-itplayerclass::cmdVVibrato, 16, 2, 0);
  }
  else
  {
    bp-=3;
    p--;
  }
  bp+=3;
  p++;

  if (p==n)
    return;

  data=curdata[4];
  switch (curdata[3])
  {
  case itplayerclass::cmdArpeggio:
    writestring(bp, 0, COLPITCH, "ð", 1);
    writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    break;
  case itplayerclass::cmdVibrato:
  case itplayerclass::cmdFineVib:
    writestring(bp, 0, COLPITCH, "~", 1);
    writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    break;
  case itplayerclass::cmdPanbrello:
    writestring(bp, 0, COLPAN, "~", 1);
    writenum(bp, 1, COLPAN, data, 16, 2, 0);
    break;
  case itplayerclass::cmdChanVol:
    writestring(bp, 0, COLVOL, "V", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case itplayerclass::cmdOffset:
    writestring(bp, 0, COLACT, "\x1A", 1);
    writenum(bp, 1, COLACT, data, 16, 2, 0);
    break;
  case itplayerclass::cmdRetrigger:
    writestring(bp, 0, COLACT, "\x13", 1);
    writenum(bp, 1, COLACT, data, 16, 2, 0);
    break;
  case itplayerclass::cmdTremolo:
    writestring(bp, 0, COLVOL, "~", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case itplayerclass::cmdTremor:
    writestring(bp, 0, COLVOL, "\xA9", 1);
    writenum(bp, 1, COLVOL, data, 16, 2, 0);
    break;
  case itplayerclass::cmdVolSlide:
  case itplayerclass::cmdChanVolSlide:
    if (!data)
      writestring(bp, 0, COLVOL, "\x12""00", 3);
    else
    if ((data&0x0F)==0x00)
    {
      writestring(bp, 0, COLVOL, "\x18", 1);
      writenum(bp, 1, COLVOL, data>>4, 16, 2, 0);
    }
    else
    if ((data&0xF0)==0x00)
    {
      writestring(bp, 0, COLVOL, "\x19", 1);
      writenum(bp, 1, COLVOL, data&0xF, 16, 2, 0);
    }
    else
    if ((data&0x0F)==0x0F)
    {
      writestring(bp, 0, COLVOL, "+", 1);
      writenum(bp, 1, COLVOL, data>>4, 16, 2, 0);
    }
    else
    if ((data&0xF0)==0xF0)
    {
      writestring(bp, 0, COLVOL, "-", 1);
      writenum(bp, 1, COLVOL, data&0xF, 16, 2, 0);
    }
    break;
  case itplayerclass::cmdPanSlide:
    if (!data)
      writestring(bp, 0, COLPAN, "\x1D""00", 3);
    else
    if ((data&0x0F)==0x00)
    {
      writestring(bp, 0, COLPAN, "\x1B", 1);
      writenum(bp, 1, COLPAN, data>>4, 16, 2, 0);
    }
    else
    if ((data&0xF0)==0x00)
    {
      writestring(bp, 0, COLPAN, "\x1A", 1);
      writenum(bp, 1, COLPAN, data&0xF, 16, 2, 0);
    }
    else
    if ((data&0x0F)==0x0F)
    {
      writestring(bp, 0, COLPAN, "-", 1);
      writenum(bp, 1, COLPAN, data>>4, 16, 2, 0);
    }
    else
    if ((data&0xF0)==0xF0)
    {
      writestring(bp, 0, COLPAN, "+", 1);
      writenum(bp, 1, COLPAN, data&0xF, 16, 2, 0);
    }
    break;
  case itplayerclass::cmdPortaVol:
    writestring(bp, 0, COLPITCH, "\x0D", 1);
    if (!data)
      writestring(bp, 1, COLVOL, "\x12""0", 2);
    else
    if ((data&0x0F)==0x00)
    {
      writestring(bp, 1, COLVOL, "\x18", 1);
      writenum(bp, 2, COLVOL, data>>4, 16, 1, 0);
    }
    else
    if ((data&0xF0)==0x00)
    {
      writestring(bp, 1, COLVOL, "\x19", 1);
      writenum(bp, 2, COLVOL, data&0xF, 16, 1, 0);
    }
    else
    if ((data&0x0F)==0x0F)
    {
      writestring(bp, 1, COLVOL, "+", 1);
      writenum(bp, 2, COLVOL, data>>4, 16, 1, 0);
    }
    else
    if ((data&0xF0)==0xF0)
    {
      writestring(bp, 1, COLVOL, "-", 1);
      writenum(bp, 2, COLVOL, data&0xF, 16, 1, 0);
    }
    break;
  case itplayerclass::cmdVibVol:
    writestring(bp, 0, COLPITCH, "~", 1);
    if (!data)
      writestring(bp, 1, COLVOL, "\x12""0", 2);
    else
    if ((data&0x0F)==0x00)
    {
      writestring(bp, 1, COLVOL, "\x18", 1);
      writenum(bp, 2, COLVOL, data>>4, 16, 1, 0);
    }
    else
    if ((data&0xF0)==0x00)
    {
      writestring(bp, 1, COLVOL, "\x19", 1);
      writenum(bp, 2, COLVOL, data&0xF, 16, 1, 0);
    }
    else
    if ((data&0x0F)==0x0F)
    {
      writestring(bp, 1, COLVOL, "+", 1);
      writenum(bp, 2, COLVOL, data>>4, 16, 1, 0);
    }
    else
    if ((data&0xF0)==0xF0)
    {
      writestring(bp, 1, COLVOL, "-", 1);
      writenum(bp, 2, COLVOL, data&0xF, 16, 1, 0);
    }
    break;
  case itplayerclass::cmdPortaNote:
    writestring(bp, 0, COLPITCH, "\x0D", 1);
    writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    break;
  case itplayerclass::cmdPortaU:
    if (data>=0xF0)
    {
      writestring(bp, 0, COLPITCH, "+0", 2);
      writenum(bp, 2, COLPITCH, data&0xF, 16, 1, 0);
    }
    else if (data>=0xE0)
    {
      writestring(bp, 0, COLPITCH, "+x", 2);
      writenum(bp, 2, COLPITCH, data&0xF, 16, 1, 0);
    }
    else
    {
      writestring(bp, 0, COLPITCH, "\x18", 1);
      writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    }
    break;
  case itplayerclass::cmdPortaD:
    if (data>=0xF0)
    {
      writestring(bp, 0, COLPITCH, "-0", 2);
      writenum(bp, 2, COLPITCH, data&0xF, 16, 1, 0);
    }
    else if (data>=0xE0)
    {
      writestring(bp, 0, COLPITCH, "-x", 2);
      writenum(bp, 2, COLPITCH, data&0xF, 16, 1, 0);
    }
    else
    {
      writestring(bp, 0, COLPITCH, "\x19", 1);
      writenum(bp, 1, COLPITCH, data, 16, 2, 0);
    }
    break;
  case itplayerclass::cmdSpecial:
    if (!data)
    {
      writestring(bp, 0, COLACT, "S00", 3);
      break;
    }
    data&=0xF;
    switch (curdata[4]>>4)
    {
    case itplayerclass::cmdSVibType:
      if (data>=4)
        break;
      writestring(bp, 0, COLPITCH, "~=", 2);
      writestring(bp, 2, COLPITCH, "~\\\xA9?"+data, 1);
      break;
    case itplayerclass::cmdSTremType:
      if (data>=4)
        break;
      writestring(bp, 0, COLVOL, "~=", 2);
      writestring(bp, 2, COLVOL, "~\\\xA9?"+data, 1);
      break;
    case itplayerclass::cmdSPanbrType:
      if (data>=4)
        break;
      writestring(bp, 0, COLPAN, "~=", 2);
      writestring(bp, 2, COLPAN, "~\\\xA9?"+data, 1);
      break;
    case itplayerclass::cmdSNoteCut:
      writestring(bp, 0, COLACT, "^", 1);
      writenum(bp, 1, COLACT, data, 16, 2, 0);
      break;
    case itplayerclass::cmdSNoteDelay:
      writestring(bp, 0, COLACT, "d", 1);
      writenum(bp, 1, COLACT, data, 16, 2, 0);
      break;
    case itplayerclass::cmdSSurround:
      writestring(bp, 0, COLPAN, "srd", 3);
      break;
    case itplayerclass::cmdSInstFX:
      writestring(bp, 0, COLINS, instfx[data], 3);
      break;
    case itplayerclass::cmdSOffsetHigh:
      writestring(bp, 0, COLACT, "\x1A", 1);
      writenum(bp, 1, COLACT, data, 16, 1, 0);
      writestring(bp, 2, COLACT, "x", 1);
      break;
    }
    break;
  }
}

static void xmgetgcmd(short *bp, int n)
{
  int p=0;
  while (*curdata)
  {
    if (p==n)
      break;
    int data=curdata[5];
    switch (curdata[4])
    {
    case itplayerclass::cmdSpeed:
      writestring(bp, 0, COLSPEED, "s", 1);
      writenum(bp, 1, COLSPEED, data, 16, 2, 0);
      break;
    case itplayerclass::cmdTempo:
      writestring(bp, 0, COLSPEED, "b", 1);
      if ((data>=0x20)||!data||(data==0x10))
        writenum(bp, 1, COLSPEED, data, 16, 2, 0);
      else
      {
        writestring(bp, 1, COLSPEED, "-+"+(data>>4), 1);
        writenum(bp, 2, COLSPEED, data&0xF, 16, 1, 0);
      }
      break;
    case itplayerclass::cmdJump:
      writestring(bp, 0, COLACT, "\x1A", 1);
      writenum(bp, 1, COLACT, data, 16, 2, 0);
      break;
    case itplayerclass::cmdBreak:
      writestring(bp, 0, COLACT, "\x19", 1);
      writenum(bp, 1, COLACT, data, 16, 2, 0);
      break;
    case itplayerclass::cmdGVolume:
      writestring(bp, 0, COLVOL, "v", 1);
      writenum(bp, 1, COLVOL, data, 16, 2, 0);
      break;
    case itplayerclass::cmdGVolSlide:
      if (!data)
        writestring(bp, 0, COLVOL, "\x12""00", 3);
      else
      if ((data&0x0F)==0x00)
      {
        writestring(bp, 0, COLVOL, "\x18", 1);
        writenum(bp, 1, COLVOL, data>>4, 16, 2, 0);
      }
      else
      if ((data&0xF0)==0x00)
      {
        writestring(bp, 0, COLVOL, "\x19", 1);
        writenum(bp, 1, COLVOL, data&0xF, 16, 2, 0);
      }
      else
      if ((data&0x0F)==0x0F)
      {
        writestring(bp, 0, COLVOL, "+", 1);
        writenum(bp, 1, COLVOL, data>>4, 16, 2, 0);
      }
      else
      if ((data&0xF0)==0xF0)
      {
        writestring(bp, 0, COLVOL, "-", 1);
        writenum(bp, 1, COLVOL, data&0xF, 16, 2, 0);
      }
      break;
    case itplayerclass::cmdSpecial:
      data&=0xF;
      switch (curdata[4]>>4)
      {
      case itplayerclass::cmdSPatLoop:
        writestring(bp, 0, COLACT, "pl", 2);
        writenum(bp, 2, COLACT, data, 16, 1, 0);
        break;
      case itplayerclass::cmdSPatDelayRow:
        writestring(bp, 0, COLACT, "dr", 2);
        writenum(bp, 2, COLACT, data, 16, 1, 0);
        break;
      case itplayerclass::cmdSPatDelayTick:
        writestring(bp, 0, COLACT, "dt", 2);
        writenum(bp, 2, COLACT, data, 16, 1, 0);
        break;
      default:
        bp-=4;
        p--;
      }
      break;
    default:
      bp-=4;
      p--;
    }
    bp+=4;
    p++;
    curdata+=6;
  }
}

static const char *xmgetpatname(int)
{
  return 0;
}

static int xmgetcurpos()
{
  return itplayer.getrealpos()>>8;
}

static cpitrakdisplaystruct xmtrakdisplay=
{
  xmgetcurpos, xmgetpatlen, xmgetpatname, xmseektrack, xmstartrow, xmgetnote,
  xmgetins, xmgetvol, xmgetpan, xmgetfx, xmgetgcmd
};

void itTrkSetup(const itplayerclass::module &mod)
{
  plPatterns=mod.patterns;
  plOrders=mod.orders;
  plPatLens=mod.patlens;
  cpiTrkSetup(xmtrakdisplay, mod.nord);
}