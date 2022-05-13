// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMDPlay channel display routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include "gmdplay.h"
#include "poutput.h"
#include "cpiface.h"

extern char plNoteStr[132][4];

static const gmdinstrument *plChanInstr;
static const gmdsample *plChanModSamples;


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

static void drawvolbar(short *buf, int i, unsigned char st)
{
  int l,r;
  mpGetRealVolume(i, l, r);
  logvolbar(l, r);

  l=(l+4)>>3;
  r=(r+4)>>3;
  if (plPause)
    l=r=0;
  if (st)
  {
    writestring(buf, 8-l, 0x08, "þþþþþþþþ", l);
    writestring(buf, 9, 0x08, "þþþþþþþþ", r);
  }
  else
  {
    writestringattr(buf, 8-l, "þ\x0Fþ\x0Bþ\x0Bþ\x09þ\x09þ\x01þ\x01þ\x01"+16-l-l, l);
    writestringattr(buf, 9, "þ\x01þ\x01þ\x01þ\x09þ\x09þ\x0Bþ\x0Bþ\x0F", r);
  }
}

static void drawlongvolbar(short *buf, int i, unsigned char st)
{
  int l,r;
  mpGetRealVolume(i, l, r);
  logvolbar(l, r);
  l=(l+2)>>2;
  r=(r+2)>>2;
  if (plPause)
    l=r=0;
  if (st)
  {
    writestring(buf, 16-l, 0x08, "þþþþþþþþþþþþþþþþ", l);
    writestring(buf, 17, 0x08, "þþþþþþþþþþþþþþþþ", r);
  }
  else
  {
    writestringattr(buf, 16-l, "þ\x0Fþ\x0Fþ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x09þ\x09þ\x09þ\x09þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01"+32-l-l, l);
    writestringattr(buf, 17, "þ\x01þ\x01þ\x01þ\x01þ\x01þ\x01þ\x09þ\x09þ\x09þ\x09þ\x0Bþ\x0Bþ\x0Bþ\x0Bþ\x0Fþ\x0F", r);
  }
}

static char *getfxstr6(unsigned char fx)
{
  switch (fx)
  {
    case fxVolSlideUp: return "volsl\x18";
    case fxVolSlideDown: return "volsl\x19";
    case fxPanSlideRight: return "pansl\x1A";
    case fxPanSlideLeft: return "pansl\x1B";
    case fxRowVolSlideUp: return "fvols\x18";
    case fxRowVolSlideDown: return "fvols\x19";
    case fxPitchSlideUp: return "porta\x18";
    case fxPitchSlideDown: return "porta\x19";
    case fxRowPitchSlideUp: return "fport\x18";
    case fxRowPitchSlideDown: return "fport\x19";
    case fxPitchSlideToNote: return "porta\x0d";
    case fxVolVibrato: return "tremol";
    case fxTremor: return "tremor";
    case fxPitchVibrato: return "vibrat";
    case fxArpeggio: return "arpegg";
    case fxNoteCut: return " \x0e""cut ";
    case fxRetrig: return "retrig";
    case fxOffset: return "offset";
    case fxDelay: return "delay ";
    default: return 0;
  }
}

static char *getfxstr15(unsigned char fx)
{
  switch (fx)
  {
    case fxVolSlideUp: return "volume slide \x18";
    case fxVolSlideDown: return "volume slide \x19";
    case fxPanSlideRight: return "panning slide \x1A";
    case fxPanSlideLeft: return "panning slide \x1B";
    case fxRowVolSlideUp: return "fine volslide \x18";
    case fxRowVolSlideDown: return "fine volslide \x19";
    case fxPitchSlideUp: return "portamento \x18";
    case fxPitchSlideDown: return "portamento \x19";
    case fxRowPitchSlideUp: return "fine porta \x18";
    case fxRowPitchSlideDown: return "fine porta \x19";
    case fxPitchSlideToNote: return "portamento to \x0d";
    case fxTremor: return "tremor";
    case fxPitchVibrato: return "vibrato";
    case fxVolVibrato: return "tremolo";
    case fxArpeggio: return "arpeggio";
    case fxNoteCut: return "note cut";
    case fxRetrig: return "retrigger";
    case fxOffset: return "sample offset";
    case fxDelay: return "delay";
    default: return 0;
  }
}


static void drawchannel36(short *buf, int i)
{
  chaninfo ci;
  unsigned char st=mpGetMute(i);
  mpGetChanInfo(i, ci);

  unsigned char tcol=st?0x08:0x0F;
  unsigned char tcold=st?0x08:0x07;
  unsigned char tcolr=st?0x08:0x0B;

  writestring(buf, 0, tcold, " -- --- -- ------ úúúúúúúú úúúúúúúú ", 36);
  if (mpGetChanStatus(i)&&ci.vol)
  {
    writenum(buf,  1, tcol, ci.ins+1, 16, 2, 0);
    writestring(buf,  4, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writenum(buf, 8, tcol, ci.vol, 16, 2, 0);
    char *fxstr=getfxstr6(ci.fx);
    if (fxstr)
      writestring(buf, 11, tcol, fxstr, 6);
    drawvolbar(buf+18, i, st);
  }
}

static void drawchannel62(short *buf, int i)
{
  chaninfo ci;
  unsigned char st=mpGetMute(i);
  mpGetChanInfo(i, ci);

  unsigned char tcol=st?0x08:0x0F;
  unsigned char tcold=st?0x08:0x07;
  unsigned char tcolr=st?0x08:0x0B;

  writestring(buf, 0, tcold, "                        ---ú --ú -ú ------  úúúúúúúú úúúúúúúú ", 62);
  if (mpGetChanStatus(i)&&ci.vol)
  {
    if (ci.ins!=0xFF)
      if (*plChanInstr[ci.ins].name)
        writestring(buf,  1, tcol, plChanInstr[ci.ins].name, 21);
      else
      {
        writestring(buf,  1, 0x08, "(  )", 4);
        writenum(buf,  2, 0x08, ci.ins+1, 16, 2, 0);
      }
    writestring(buf, 24, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 27, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~ð"+ci.pitchfx, 1);
    writenum(buf, 29, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 31, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 33, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
    writestring(buf, 34, tcol, " \x1A\x1B"+ci.panslide, 1);
    char *fxstr=getfxstr6(ci.fx);
    if (fxstr)
      writestring(buf, 36, tcol, fxstr, 6);
    drawvolbar(buf+44, i, st);
  }
}

static void drawchannel76(short *buf, int i)
{
  chaninfo ci;
  unsigned char st=mpGetMute(i);
  mpGetChanInfo(i, ci);

  unsigned char tcol=st?0x08:0x0F;
  unsigned char tcold=st?0x08:0x07;
  unsigned char tcolr=st?0x08:0x0B;

  writestring(buf,  0, tcold, "                             ³    ³   ³  ³               ³ úúúúúúúú úúúúúúúú", 76);
  if (mpGetChanStatus(i)&&ci.vol)
  {
    if (ci.ins!=0xFF)
      if (*plChanInstr[ci.ins].name)
        writestring(buf,  1, tcol, plChanInstr[ci.ins].name, 28);
      else
      {
        writestring(buf,  1, 0x08, "(  )", 4);
        writenum(buf,  2, 0x08, ci.ins+1, 16, 2, 0);
      }
    writestring(buf, 30, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 33, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~ð"+ci.pitchfx, 1);
    writenum(buf, 35, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 37, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 39, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
    writestring(buf, 40, tcol, " \x1A\x1B"+ci.panslide, 1);

    char *fxstr=getfxstr15(ci.fx);
    if (fxstr)
      writestring(buf, 42, tcol, fxstr, 15);

    drawvolbar(buf+59, i, st);
  }
}

static void drawchannel128(short *buf, int i)
{
  chaninfo ci;
  unsigned char st=mpGetMute(i);
  mpGetChanInfo(i, ci);

  unsigned char tcol=st?0x08:0x0F;
  unsigned char tcold=st?0x08:0x07;
  unsigned char tcolr=st?0x08:0x0B;

  writestring(buf,  0, tcold, "                             ³                   ³    ³   ³  ³               ³  úúúúúúúúúúúúúúúú úúúúúúúúúúúúúúúú", 128);

  if (mpGetChanStatus(i)&&ci.vol)
  {
    if (ci.ins!=0xFF)
      if (*plChanInstr[ci.ins].name)
        writestring(buf,  1, tcol, plChanInstr[ci.ins].name, 28);
      else
      {
        writestring(buf,  1, 0x08, "(  )", 4);
        writenum(buf,  2, 0x08, ci.ins+1, 16, 2, 0);
      }
    if (ci.smp!=0xFFFF)
      if (*plChanModSamples[ci.smp].name)
        writestring(buf, 31, tcol, plChanModSamples[ci.smp].name, 17);
      else
      {
        writestring(buf, 31, 0x08, "(    )", 6);
        writenum(buf, 32, 0x08, ci.smp, 16, 4, 0);
      }
    writestring(buf, 50, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 53, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~ð"+ci.pitchfx, 1);
    writenum(buf, 55, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 57, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 59, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
    writestring(buf, 60, tcol, " \x1A\x1B"+ci.panslide, 1);

    char *fxstr=getfxstr15(ci.fx);
    if (fxstr)
      writestring(buf, 62, tcol, fxstr, 15);

    drawlongvolbar(buf+80, i, st);
  }
}

static void drawchannel44(short *buf, int i)
{
  chaninfo ci;
  unsigned char st=mpGetMute(i);
  mpGetChanInfo(i, ci);

  unsigned char tcol=st?0x08:0x0F;
  unsigned char tcold=st?0x08:0x07;
  unsigned char tcolr=st?0x08:0x0B;

  writestring(buf, 0, tcold, " --  ---ú --ú -ú ------   úúúúúúúú úúúúúúúú ", 44);
  if (mpGetChanStatus(i)&&ci.vol)
  {
    writenum(buf,  1, tcol, ci.ins+1, 16, 2, 0);
    writestring(buf,  5, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 8, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~ð"+ci.pitchfx, 1);
    writenum(buf, 10, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 12, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 14, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
    writestring(buf, 15, tcol, " \x1A\x1B"+ci.panslide, 1);

    char *fxstr=getfxstr6(ci.fx);
    if (fxstr)
      writestring(buf, 17, tcol, fxstr, 6);
    drawvolbar(buf+26, i, st);
  }
}

static void drawchannel(short *buf, int len, int i)
{
  switch (len)
  {
  case 36:
    drawchannel36(buf, i);
    break;
  case 44:
    drawchannel44(buf, i);
    break;
  case 62:
    drawchannel62(buf, i);
    break;
  case 76:
    drawchannel76(buf, i);
    break;
  case 128:
    drawchannel128(buf, i);
    break;
  }
}

void gmdChanSetup(const gmdmodule &mod)
{
  plChanInstr=mod.instruments;
  plChanModSamples=mod.modsamples;
  plUseChannels(drawchannel);
}