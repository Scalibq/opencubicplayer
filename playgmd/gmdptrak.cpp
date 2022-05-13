// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMDPlay track/pattern display routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <stdlib.h>
#include "gmdplay.h"
#include "poutput.h"
#include "cpiface.h"

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

static const gmdtrack *plTracks;
static const gmdpattern *plPatterns;
static const unsigned short *plOrders;
extern char plNoteStr[132][4];
static const unsigned char *curtrk;
static const unsigned char *curtrkend;
static const unsigned char *currow;
static const unsigned char *currowend;

static int startrow()
{
  if (curtrk>=curtrkend)
    return -1;
  currow=curtrk+2;
  currowend=curtrk+=2+curtrk[1];
  return currow[-2];
}

static void seektrack(int n, int c)
{
  n=(c==-1)?plPatterns[plOrders[n]].gtrack:plPatterns[plOrders[n]].tracks[c];
  curtrk=plTracks[n].ptr;
  curtrkend=plTracks[n].end;
}

static const char *getpatname(int n)
{
  return plPatterns[plOrders[n]].name;
}

static int getpatlen(int n)
{
  return plPatterns[plOrders[n]].patlen;
}

static int getcurpos()
{
  return mpGetRealPos()>>8;
}

static int getnote(short *bp, int small)
{
  const unsigned char *ptr=currow;
  while (ptr<currowend)
  {
    if (*ptr&cmdPlayNote)
    {
      unsigned char pn=*ptr++;
      if (pn&cmdPlayIns)
        ptr++;
      if (pn&cmdPlayNte)
      {
        switch (small)
        {
        case 0:
          writestring(bp, 0, (*ptr&0x80)?COLPTNOTE:COLNOTE, "CCDDEFFGGAAB"+((*ptr&~0x80)%12), 1);
          writestring(bp, 1, (*ptr&0x80)?COLPTNOTE:COLNOTE, "-#-#--#-#-#-"+((*ptr&~0x80)%12), 1);
          writestring(bp, 2, (*ptr&0x80)?COLPTNOTE:COLNOTE, "-0123456789"+((*ptr&~0x80)/12), 1);
          break;
        case 1:
          writestring(bp, 0, (*ptr&0x80)?COLPTNOTE:COLNOTE, "cCdDefFgGaAb"+((*ptr&0x7F)%12), 1);
          writestring(bp, 1, (*ptr&0x80)?COLPTNOTE:COLNOTE, "-0123456789"+((*ptr&0x7F)/12), 1);
          break;
        case 2:
          writestring(bp, 0, (*ptr&0x80)?COLPTNOTE:COLNOTE, "cCdDefFgGaAb"+((*ptr&0x7F)%12), 1);
          break;
        }
        return 1;
      }
      if (pn&cmdPlayVol)
        ptr++;
      if (pn&cmdPlayPan)
        ptr++;
      if (pn&cmdPlayDelay)
        ptr++;
    }
    else
      ptr+=2;
  }
  return 0;
}

static int getvol(short *bp)
{
  const unsigned char *ptr=currow;
  while (ptr<currowend)
  {
    if (*ptr&cmdPlayNote)
    {
      unsigned char pn=*ptr++;
      if (pn&cmdPlayIns)
        ptr++;
      if (pn&cmdPlayNte)
        ptr++;
      if (pn&cmdPlayVol)
      {
        writenum(bp, 0, COLVOL, *ptr++, 16, 2, 0);
        return 1;
      }
      if (pn&cmdPlayPan)
        ptr++;
      if (pn&cmdPlayDelay)
        ptr++;
    }
    else
      ptr+=2;
  }
  return 0;
}

static int getins(short *bp)
{
  const unsigned char *ptr=currow;
  while (ptr<currowend)
  {
    if (*ptr&cmdPlayNote)
    {
      unsigned char pn=*ptr++;
      if (pn&cmdPlayIns)
      {
        writenum(bp, 0, COLINS, *ptr++, 16, 2, 0);
        return 1;
      }
      if (pn&cmdPlayNte)
        ptr++;
      if (pn&cmdPlayVol)
        ptr++;
      if (pn&cmdPlayPan)
        ptr++;
      if (pn&cmdPlayDelay)
        ptr++;
    }
    else
      ptr+=2;
  }
  return 0;
}

static int getpan(short *bp)
{
  const unsigned char *ptr=currow;
  while (ptr<currowend)
  {
    if (*ptr&cmdPlayNote)
    {
      unsigned char pn=*ptr++;
      if (pn&cmdPlayIns)
        ptr++;
      if (pn&cmdPlayNte)
        ptr++;
      if (pn&cmdPlayVol)
        ptr++;
      if (pn&cmdPlayPan)
      {
        writenum(bp, 0, COLPAN, *ptr++, 16, 2, 0);
        return 1;
      }
      if (pn&cmdPlayDelay)
        ptr++;
    }
    else
      ptr+=2;
  }
  return 0;
}

static void getgcmd(short *buf, int n)
{
  const unsigned char *ptr=currow;
  while (n&&(ptr<currowend))
  {
    switch (*ptr++)
    {
    case cmdTempo:
      writestring(buf, 0, COLSPEED, "t", 1);
      writenum(buf, 1, COLSPEED, *ptr, 16, 2, 0);
      break;
    case cmdSpeed:
      writestring(buf, 0, COLSPEED, "s", 1);
      writenum(buf, 1, COLSPEED, *ptr, 16, 2, 0);
      break;
    case cmdFineSpeed:
      writestring(buf, 0, COLSPEED, "s.", 2);
      writenum(buf, 2, COLSPEED, *ptr, 16, 1, 0);
      break;
    case cmdBreak:
      writestring(buf, 0, COLACT, "\x19", 1);
      writenum(buf, 1, COLACT, *ptr, 16, 2, 0);
      break;
    case cmdGoto:
      writestring(buf, 0, COLACT, "\x1A", 1);
      writenum(buf, 1, COLACT, *ptr, 16, 2, 0);
      break;
    case cmdPatLoop:
      writestring(buf, 0, COLACT, "pl", 2);
      writenum(buf, 2, COLACT, *ptr, 16, 1, 0);
      break;
    case cmdGlobVol:
      writestring(buf, 0, COLVOL, "v", 1);
      writenum(buf, 1, COLVOL, *ptr, 16, 2, 0);
      break;
    case cmdGlobVolSlide:
      writestring(buf, 0, COLVOL, (*(signed char*)ptr>0)?"\x18":(*(signed char*)ptr<0)?"\x19":"\x12", 1);
      writenum(buf, 1, COLVOL, abs(*(signed char*)ptr), 16, 2, 0);
      break;
    case cmdPatDelay:
      writestring(buf, 0, COLACT, "pd", 2);
      writenum(buf, 2, COLACT, *ptr, 16, 1, 0);
      break;
    default:
      buf-=4;
      n++;
    }
    ptr++;
    buf+=4;
    n--;
  }
}

static void getfx(short *buf, int n)
{
  const unsigned char *ptr=currow;
  while (n&&(ptr<currowend))
  {
    if (*ptr&cmdPlayNote)
    {
      unsigned char pn=*ptr++;
      if (pn&cmdPlayIns)
        ptr++;
      if (pn&cmdPlayNte)
        ptr++;
      if (pn&cmdPlayVol)
        ptr++;
      if (pn&cmdPlayPan)
        ptr++;
      if (pn&cmdPlayDelay)
      {
        writestring(buf, 0, COLACT, "d", 1);
        writenum(buf, 1, COLACT, *ptr++, 16, 2, 0);
        buf+=3;
        n--;
      }
    }
    else
    {
      switch (*ptr++)
      {
      case cmdSpecial:
        switch (*ptr)
        {
        case cmdContVolSlide:
          writestring(buf, 0, COLVOL, "\x12""00", 3);
          break;
        case cmdContRowVolSlide:
          writestring(buf, 0, COLVOL, "ñ00", 3);
          break;
        case cmdContMixVolSlide:
          writestring(buf, 0, COLVOL, "\x12""ñ0", 3);
          break;
        case cmdContMixVolSlideUp:
          writestring(buf, 0, COLVOL, "\x18""+0", 3);
          break;
        case cmdContMixVolSlideDown:
          writestring(buf, 0, COLVOL, "\x19""-0", 3);
          break;
        case cmdContMixPitchSlideUp:
          writestring(buf, 0, COLPITCH, "\x18""+0", 3);
          break;
        case cmdContMixPitchSlideDown:
          writestring(buf, 0, COLPITCH, "\x19""-0", 3);
          break;
        }
        break;
      case cmdChannelVol:
        writestring(buf, 0, COLVOL, "V", 1);
        writenum(buf, 1, COLVOL, *ptr, 16, 2, 0);
        break;
      case cmdPitchSlideUp: case cmdPitchSlideUDMF:
        writestring(buf, 0, COLPITCH, "\x18", 1);
        writenum(buf, 1, COLPITCH, *ptr, 16, 2, 0);
        break;
      case cmdPitchSlideDown: case cmdPitchSlideDDMF:
        writestring(buf, 0, COLPITCH, "\x19", 1);
        writenum(buf, 1, COLPITCH, *ptr, 16, 2, 0);
        break;
      case cmdRowPitchSlideUp:
        writestring(buf, 0, COLPITCH, "+", 1);
        writenum(buf, 1, COLPITCH, *ptr, 16, 2, 0);
        break;
      case cmdRowPitchSlideDown:
        writestring(buf, 0, COLPITCH, "-", 1);
        writenum(buf, 1, COLPITCH, *ptr, 16, 2, 0);
        break;
      case cmdRowPitchSlideDMF:
        writestring(buf, 0, COLPITCH, ((signed char)*ptr<0)?"-":"+", 1);
        writenum(buf, 1, COLPITCH, abs((signed char)*ptr), 16, 2, 0);
        break;
      case cmdPitchSlideToNote: case cmdPitchSlideNDMF:
        writestring(buf, 0, COLPITCH, "\x0D", 1);
        writenum(buf, 1, COLPITCH, *ptr, 16, 2, 0);
        break;
      case cmdPitchVibrato:
      case cmdPitchVibratoSinDMF:
      case cmdPitchVibratoTrgDMF:
      case cmdPitchVibratoRecDMF:
        writestring(buf, 0, COLPITCH, "~", 1);
        writenum(buf, 1, COLPITCH, *ptr, 16, 2, 0);
        break;
      case cmdPitchVibratoFine:
        writestring(buf, 0, COLPITCH, "~", 1);
        writenum(buf, 1, COLPITCH, *ptr, 16, 2, 0);
        break;
      case cmdPitchVibratoSetSpeed:
        writestring(buf, 0, COLPITCH, "~\x1A", 2);
        writenum(buf, 2, COLPITCH, *ptr, 16, 1, 0);
        break;
      case cmdPitchVibratoSetWave:
        writestring(buf, 0, COLPITCH, "~=", 2);
        writestring(buf, 2, COLPITCH, "~\\*\x1C?           ~\\\x1C?"+*ptr, 1);
        break;
      case cmdArpeggio:
        writestring(buf, 0, COLPITCH, "ð", 1);
        writenum(buf, 1, COLPITCH, *ptr, 16, 2, 0);
        break;
      case cmdVolSlideUp:
      case cmdVolSlideUDMF:
        writestring(buf, 0, COLVOL, "\x18", 1);
        writenum(buf, 1, COLVOL, *ptr, 16, 2, 0);
        break;
      case cmdVolSlideDown:
      case cmdVolSlideDDMF:
        writestring(buf, 0, COLVOL, "\x19", 1);
        writenum(buf, 1, COLVOL, *ptr, 16, 2, 0);
        break;
      case cmdRowVolSlideUp:
        writestring(buf, 0, COLVOL, "+", 1);
        writenum(buf, 1, COLVOL, *ptr, 16, 2, 0);
        break;
      case cmdRowVolSlideDown:
        writestring(buf, 0, COLVOL, "-", 1);
        writenum(buf, 1, COLVOL, *ptr, 16, 2, 0);
        break;
      case cmdVolVibrato:
      case cmdVolVibratoSinDMF:
      case cmdVolVibratoTrgDMF:
      case cmdVolVibratoRecDMF:
        writestring(buf, 0, COLVOL, "~", 1);
        writenum(buf, 1, COLVOL, *ptr, 16, 2, 0);
        break;
      case cmdVolVibratoSetWave:
        writestring(buf, 0, COLVOL, "~=", 2);
        writestring(buf, 2, COLVOL, "~\\*\x1C?           ~/\x1C?"+*ptr, 1);
        break;
      case cmdTremor:
        writestring(buf, 0, COLVOL, "\xA9", 1);
        writenum(buf, 1, COLVOL, *ptr, 16, 2, 0);
        break;
      case cmdPanSlide:
        writestring(buf, 0, COLPAN, (*(signed char*)ptr>0)?"\x1A":(*(signed char*)ptr<0)?"\x1B":"\1D", 1);
        writenum(buf, 1, COLPAN, abs(*(signed char*)ptr), 16, 2, 0);
        break;
      case cmdPanSlideLDMF:
        writestring(buf, 0, COLPAN, "\x1B", 1);
        writenum(buf, 1, COLPAN, *ptr, 16, 2, 0);
        break;
      case cmdPanSlideRDMF:
        writestring(buf, 0, COLPAN, "\x1A", 1);
        writenum(buf, 1, COLPAN, *ptr, 16, 2, 0);
        break;
      case cmdPanVibratoSinDMF:
        writestring(buf, 0, COLPAN, "~", 1);
        writenum(buf, 1, COLPAN, *ptr, 16, 2, 0);
        break;
      case cmdPanSurround:
        writestring(buf, 0, COLPAN, ">\x01<", 3);
        break;
      case cmdRetrig:
        writestring(buf, 0, COLACT, "\x13", 1);
        writenum(buf, 1, COLACT, *ptr, 16, 2, 0);
        break;
      case cmdOffset:
        writestring(buf, 0, COLACT, "\x1A", 1);
        writenum(buf, 1, COLACT, *ptr, 16, 2, 0);
        break;
      case cmdOffsetEnd:
        writestring(buf, 0, COLACT, "\x1B", 1);
        writenum(buf, 1, COLACT, *ptr, 16, 2, 0);
        break;
      case cmdSetDir:
        writestring(buf, 0, COLACT, (*ptr==0)?"-->":(*ptr==1)?"<--":"<->", 3);
        break;
      case cmdSetLoop:
        writestring(buf, 0, COLACT, (*ptr==1)?"lp1":(*ptr==2)?"lp2":"lp-", 3);
        break;
      case cmdNoteCut:
        writestring(buf, 0, COLACT, "^", 1);
        writenum(buf, 1, COLACT, *ptr, 16, 2, 0);
        break;
      case cmdKeyOff:
        writestring(buf, 0, COLINS, "off", 3);
        break;
      case cmdSetEnvPos:
        writestring(buf, 0, COLINS, "\x1A", 1);
        writenum(buf, 1, COLINS, *ptr, 16, 2, 0);
        break;
      case cmdPanHeight:
        writestring(buf, 0, COLPAN, "Y", 1);
        writenum(buf, 1, COLPAN, *ptr, 16, 2, 0);
        break;
      case cmdPanDepth:
        writestring(buf, 0, COLPAN, "Z", 1);
        writenum(buf, 1, COLPAN, *ptr, 16, 2, 0);
        break;
      default:
        buf-=3;
        n++;
      }
      ptr++;
      buf+=3;
      n--;
    }
  }
}

static cpitrakdisplaystruct gmdptrkdisplay=
{
  getcurpos, getpatlen, getpatname, seektrack, startrow, getnote,
  getins, getvol, getpan, getfx, getgcmd
};


void gmdTrkSetup(const gmdmodule &mod)
{
  plPatterns=mod.patterns;
  plOrders=mod.orders;
  plTracks=mod.tracks;
  cpiTrkSetup(gmdptrkdisplay, mod.ordnum);
}