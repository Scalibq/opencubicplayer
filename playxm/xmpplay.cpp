// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// XMPlay interface routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -removed all references to gmd structures to make this more flexible
//    -removed mcp "restricted" flag (theres no point in rendering XM files
//     to disk in mono if FT is able to do this in stereo anyway ;)
//    -finally, added all the screen output we all waited for since november
//     1996 :)

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "pfilesel.h"
#include "mcp.h"
#include "binfile.h"
#include "poutput.h"
#include "err.h"
#include "deviwave.h"
#include "cpiface.h"
#include "xmplay.h"


extern char plNoteStr[132][4];

extern int plLoopMods;
extern char plCompoMode;

static xmodule mod;

static const char *modname;
static const char *composer;

static char currentmodname[_MAX_FNAME];
static char currentmodext[_MAX_EXT];
static long starttime;
static long pausetime;

void mcpDrawGStrings(short (*)[132]);
void mcpNormalize();
int mcpSetProcessKey(unsigned short);

static xmpinstrument *insts;
static xmpsample *samps;

void xmTrkSetup(const xmodule &mod);
void xmpInstSetup(const xmpinstrument *ins, int nins, const xmpsample *smp, int nsmp, const sampleinfo *smpi, int nsmpi, int type, void (*MarkyBoy)(char *, char *));
void xmpInstClear();

static int xmpProcessKey(unsigned short key)
{
  if (mcpSetProcessKey(key))
    return 1;
  if (mcpProcessKey)
  {
    int ret=mcpProcessKey(key);
    if (ret==2)
      cpiResetScreen();
    if (ret)
     return 1;
  }
  int pat,row,p;
  switch (key)
  {
  case 'p': case 'P': case 0x10:
    if (plPause)
      starttime=starttime+clock()-pausetime;
    else
      pausetime=clock();
    mcpSet(-1, mcpMasterPause, plPause^=1);
    plChanChanged=1;
    break;
  case 0x7700: //ctrl-home
    xmpInstClear();
    xmpSetPos(0, 0);
    if (plPause)
      starttime=pausetime;
    else
      starttime=clock();
    break;
  case 0x7300: //ctrl-left
    p=xmpGetPos();
    pat=p>>8;
    xmpSetPos(pat-1, 0);
    break;
  case 0x7400: //ctrl-right
    p=xmpGetPos();
    pat=p>>8;
    xmpSetPos(pat+1, 0);
    break;
  case 0x8D00: // ctrl-up
    p=xmpGetPos();
    pat=p>>8;
    row=p&0xFF;
    xmpSetPos(pat, row-8);
    break;
  case 0x9100: //ctrl-down
    p=xmpGetPos();
    pat=p>>8;
    row=p&0xFF;
    xmpSetPos(pat, row+8);
    break;
  }
  return 1;
}

static int xmpLooped()
{
  return !plLoopMods&&xmpLoop();
}

static void xmpIdle()
{
  xmpSetLoop(plLoopMods);
  if (mcpIdle)
    mcpIdle();
}

static void xmpDrawGStrings(short (*buf)[132])
{
  mcpDrawGStrings(buf);
  if (plScrWidth==80)
  {
    int pos=xmpGetRealPos();
    int gvol,bpm,tmp;
    xmpglobinfo gi;
    xmpGetGlobInfo(tmp,bpm,gvol);
    xmpGetGlobInfo2(gi);
    writestring(buf[1],  0, 0x09, " row: ../..  ord: .../...  tempo: ..  bpm: ...  gvol: ..� ", 58);
    writenum(buf[1],  6, 0x0F, (pos>>8)&0xFF, 16, 2, 0);
    writenum(buf[1],  9, 0x0F, mod.patlens[mod.orders[(pos>>16)&0xFF]]-1, 16, 2, 0);
    writenum(buf[1], 18, 0x0F, (pos>>16)&0xFF, 16, 3, 0);
    writenum(buf[1], 22, 0x0F, mod.nord-1, 16, 3, 0);
    writenum(buf[1], 34, 0x0F, tmp, 16, 2, 1);
    writenum(buf[1], 43, 0x0F, bpm, 10, 3, 1);
    writenum(buf[1], 54, 0x0F, gvol, 16, 2, 0);
    writestring(buf[1], 56, 0x0F, (gi.globvolslide==xfxGVSUp)?"\x18":(gi.globvolslide==xfxGVSDown)?"\x19":" ", 1);
    writestring(buf[2],  0, 0x09, " module ��������.���: ...............................               time: ..:.. ", 80);
    writestring(buf[2],  8, 0x0F, currentmodname, 8);
    writestring(buf[2], 16, 0x0F, currentmodext, 4);
    writestring(buf[2], 22, 0x0F, modname, 31);
    if (plPause)
      writestring(buf[2], 58, 0x0C, "paused", 6);
    long tim;
    if (plPause)
      tim=(pausetime-starttime)/CLK_TCK;
    else
      tim=(clock()-starttime)/CLK_TCK;
    writenum(buf[2], 74, 0x0F, (tim/60)%60, 10, 2, 1);
    writestring(buf[2], 76, 0x0F, ":", 1);
    writenum(buf[2], 77, 0x0F, tim%60, 10, 2, 0);
  }
  else
  {
    int pos=xmpGetRealPos();
    int gvol,bpm,tmp;
    xmpglobinfo gi;
    xmpGetGlobInfo(tmp,bpm,gvol);
    xmpGetGlobInfo2(gi);
    writestring(buf[1],  0, 0x09, "    row: ../..  order: .../...   tempo: ..  speed/bpm: ...   global volume: ..�  ", 81);
    writenum(buf[1],  9, 0x0F, (pos>>8)&0xFF, 16, 2, 0);
    writenum(buf[1], 12, 0x0F, mod.patlens[mod.orders[(pos>>16)&0xFF]]-1, 16, 2, 0);
    writenum(buf[1], 23, 0x0F, (pos>>16)&0xFF, 16, 3, 0);
    writenum(buf[1], 27, 0x0F, mod.nord-1, 16, 3, 0);
    writenum(buf[1], 40, 0x0F, tmp, 16, 2, 1);
    writenum(buf[1], 55, 0x0F, bpm, 10, 3, 1);
    writenum(buf[1], 76, 0x0F, gvol, 16, 2, 0);
    writestring(buf[1], 78, 0x0F, (gi.globvolslide==xfxGVSUp)?"\x18":(gi.globvolslide==xfxGVSDown)?"\x19":" ", 1);
    writestring(buf[2],  0, 0x09, "    module ��������.���: ...............................  composer: ...............................                  time: ..:..    ", 132);
    writestring(buf[2], 11, 0x0F, currentmodname, 8);
    writestring(buf[2], 19, 0x0F, currentmodext, 4);
    writestring(buf[2], 25, 0x0F, modname, 31);
    writestring(buf[2], 68, 0x0F, composer, 31);
    if (plPause)
      writestring(buf[2], 100, 0x0C, "playback paused", 15);
    long tim;
    if (plPause)
      tim=(pausetime-starttime)/CLK_TCK;
    else
      tim=(clock()-starttime)/CLK_TCK;
    writenum(buf[2], 123, 0x0F, (tim/60)%60, 10, 2, 1);
    writestring(buf[2], 125, 0x0F, ":", 1);
    writenum(buf[2], 126, 0x0F, tim%60, 10, 2, 0);
  }
}

static void xmpCloseFile()
{
  xmpStopModule();
#ifdef RESTRICTED
  mcpSet(-1, mcpGRestrict, 0);
#endif
  xmpFreeModule(mod);
}

//**********************************************************************

static void xmpMarkInsSamp(char *ins, char *smp)
{
  int i;
  for (i=0; i<plNLChan; i++)
  {
    if (!xmpChanActive(i)||plMuteCh[i])
      continue;
    int in=xmpGetChanIns(i);
    int sm=xmpGetChanSamp(i);
    ins[in-1]=((plSelCh==i)||(ins[in-1]==3))?3:2;
    smp[sm]=((plSelCh==i)||(smp[sm]==3))?3:2;
  }
}

//************************************************************************

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
  xmpGetRealVolume(i, l, r);
  logvolbar(l, r);

  l=(l+4)>>3;
  r=(r+4)>>3;
  if (plPause)
    l=r=0;
  if (st)
  {
    writestring(buf, 8-l, 0x08, "��������", l);
    writestring(buf, 9, 0x08, "��������", r);
  }
  else
  {
    writestringattr(buf, 8-l, "�\x0F�\x0B�\x0B�\x09�\x09�\x01�\x01�\x01"+16-l-l, l);
    writestringattr(buf, 9, "�\x01�\x01�\x01�\x09�\x09�\x0B�\x0B�\x0F", r);
  }
}

static void drawlongvolbar(short *buf, int i, unsigned char st)
{
  int l,r;
  xmpGetRealVolume(i, l, r);
  logvolbar(l, r);
  l=(l+2)>>2;
  r=(r+2)>>2;
  if (plPause)
    l=r=0;
  if (st)
  {
    writestring(buf, 16-l, 0x08, "����������������", l);
    writestring(buf, 17, 0x08, "����������������", r);
  }
  else
  {
    writestringattr(buf, 16-l, "�\x0F�\x0F�\x0B�\x0B�\x0B�\x0B�\x09�\x09�\x09�\x09�\x01�\x01�\x01�\x01�\x01�\x01"+32-l-l, l);
    writestringattr(buf, 17, "�\x01�\x01�\x01�\x01�\x01�\x01�\x09�\x09�\x09�\x09�\x0B�\x0B�\x0B�\x0B�\x0F�\x0F", r);
  }
}



static char *getfxstr6(unsigned char fx)
{
  switch (fx)
  {
    case xfxVolSlideUp: return "volsl\x18";
    case xfxVolSlideDown: return "volsl\x19";
    case xfxRowVolSlideUp: return "fvols\x18";
    case xfxRowVolSlideDown: return "fvols\x19";
    case xfxPitchSlideUp: return "porta\x18";
    case xfxPitchSlideDown: return "porta\x19";
    case xfxPitchSlideToNote: return "porta\x0d";
    case xfxRowPitchSlideUp: return "fport\x18";
    case xfxRowPitchSlideDown: return "fport\x19";
    case xfxPanSlideRight: return "pansl\x1A";
    case xfxPanSlideLeft: return "pansl\x1B";
    case xfxVolVibrato: return "tremol";
    case xfxTremor: return "tremor";
    case xfxPitchVibrato: return "vibrat";
    case xfxArpeggio: return "arpegg";
    case xfxNoteCut: return " \x0e""cut ";
    case xfxRetrig: return "retrig";
    case xfxOffset: return "offset";
    case xfxDelay: return "\x0e""delay";
    case xfxEnvPos: return "envpos";
    case xfxSetFinetune: return "set ft";
    default: return 0;
  }
}

static char *getfxstr15(unsigned char fx)
{
  switch (fx)
  {
    case xfxVolSlideUp: return "volume slide \x18";
    case xfxVolSlideDown: return "volume slide \x19";
    case xfxPanSlideRight: return "panning slide \x1A";
    case xfxPanSlideLeft: return "panning slide \x1B";
    case xfxRowVolSlideUp: return "fine volslide \x18";
    case xfxRowVolSlideDown: return "fine volslide \x19";
    case xfxPitchSlideUp: return "portamento \x18";
    case xfxPitchSlideDown: return "portamento \x19";
    case xfxRowPitchSlideUp: return "fine porta \x18";
    case xfxRowPitchSlideDown: return "fine porta \x19";
    case xfxPitchSlideToNote: return "portamento to \x0d";
    case xfxTremor: return "tremor";
    case xfxPitchVibrato: return "vibrato";
    case xfxVolVibrato: return "tremolo";
    case xfxArpeggio: return "arpeggio";
    case xfxNoteCut: return "note cut";
    case xfxRetrig: return "retrigger";
    case xfxOffset: return "sample offset";
    case xfxDelay: return "delay";
    case xfxEnvPos: return "set env pos'n";
    case xfxSetFinetune: return "set finetune";
    default: return 0;
  }
}



static void drawchannel(short *buf, int len, int i)
{
  unsigned char st=plMuteCh[i];

  unsigned char tcol=st?0x08:0x0F;
  unsigned char tcold=st?0x08:0x07;
  unsigned char tcolr=st?0x08:0x0B;

  switch (len)
  {
  case 36:
    writestring(buf, 0, tcold, " -- --- -- ------ �������� �������� ", 36);
    break;
  case 62:
    writestring(buf, 0, tcold, "                        ---� --� -� ------  �������� �������� ", 62);
    break;
  case 128:
    writestring(buf,  0, tcold, "                             �                   �    �   �  �               �  ���������������� ����������������", 128);
    break;
  case 76:
    writestring(buf,  0, tcold, "                             �    �   �  �               � �������� ��������", 76);
    break;
  case 44:
    writestring(buf, 0, tcold, " --  ---� --� -� ------   �������� �������� ", 44);
    break;
  }

  if (!xmpChanActive(i))
    return;

  int ins=xmpGetChanIns(i);
  int smp=xmpGetChanSamp(i);
  xmpchaninfo ci;
  xmpGetChanInfo(i,ci);
  char *fxstr;
  switch (len)
  {
  case 36:
    writenum(buf,  1, tcol, ins, 16, 2, 0);
    writestring(buf,  4, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writenum(buf, 8, tcol, ci.vol, 16, 2, 0);
    fxstr=getfxstr6(ci.fx);
    if (fxstr)
      writestring(buf, 11, tcol, fxstr, 6);
    drawvolbar(buf+18, i, st);
    break;
  case 62:
    if (ins)
      if (*insts[ins-1].name)
        writestring(buf,  1, tcol, insts[ins-1].name, 21);
      else
      {
        writestring(buf,  1, 0x08, "(  )", 4);
        writenum(buf,  2, 0x08, ins, 16, 2, 0);
      }
    writestring(buf, 24, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 27, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~�"+ci.pitchfx, 1);
    writenum(buf, 29, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 31, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 33, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
    writestring(buf, 34, tcol, " \x1A\x1B"+ci.panslide, 1);
    fxstr=getfxstr6(ci.fx);
    if (fxstr)
      writestring(buf, 36, tcol, fxstr, 6);
    drawvolbar(buf+44, i, st);
    break;
  case 76:
    if (ins)
      if (*insts[ins-1].name)
        writestring(buf,  1, tcol, insts[ins-1].name, 28);
      else
      {
        writestring(buf,  1, 0x08, "(  )", 4);
        writenum(buf,  2, 0x08, ins, 16, 2, 0);
      }
    writestring(buf, 30, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 33, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~�"+ci.pitchfx, 1);
    writenum(buf, 35, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 37, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 39, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
    writestring(buf, 40, tcol, " \x1A\x1B"+ci.panslide, 1);

    fxstr=getfxstr15(ci.fx);
    if (fxstr)
      writestring(buf, 42, tcol, fxstr, 15);

    drawvolbar(buf+59, i, st);
    break;
  case 128:
    if (ins)
      if (*insts[ins-1].name)
        writestring(buf,  1, tcol, insts[ins-1].name, 28);
      else
      {
        writestring(buf,  1, 0x08, "(  )", 4);
        writenum(buf,  2, 0x08, ins, 16, 2, 0);
      }
    if (smp!=0xFFFF)
      if (*samps[smp].name)
        writestring(buf, 31, tcol, samps[smp].name, 17);
      else
      {
        writestring(buf, 31, 0x08, "(    )", 6);
        writenum(buf, 32, 0x08, smp, 16, 4, 0);
      }
    writestring(buf, 50, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 53, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~�"+ci.pitchfx, 1);
    writenum(buf, 55, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 57, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 59, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
    writestring(buf, 60, tcol, " \x1A\x1B"+ci.panslide, 1);

    fxstr=getfxstr15(ci.fx);
    if (fxstr)
      writestring(buf, 62, tcol, fxstr, 15);
    drawlongvolbar(buf+80, i, st);
    break;
  case 44:
    writenum(buf,  1, tcol, xmpGetChanIns(i), 16, 2, 0);
    writestring(buf,  5, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 8, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~�"+ci.pitchfx, 1);
    writenum(buf, 10, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 12, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 14, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
    writestring(buf, 15, tcol, " \x1A\x1B"+ci.panslide, 1);

    fxstr=getfxstr6(ci.fx);
    if (fxstr)
      writestring(buf, 17, tcol, fxstr, 6);
    drawvolbar(buf+26, i, st);
    break;
  }
}

//************************************************************************

static int xmpGetDots(notedotsdata *d, int max)
{
  int pos=0;
  int i;
  for (i=0; i<plNLChan; i++)
  {
    if (pos>=max)
      break;
    int smp,frq,voll,volr,sus;
    if (!xmpGetDotsData(i, smp, frq, voll, volr, sus))
      continue;
    d[pos].voll=voll;
    d[pos].volr=volr;
    d[pos].chan=i;
    d[pos].note=frq;
    d[pos].col=(sus?32:16)+(smp&15);
    pos++;
  }
  return pos;
}




static int xmpOpenFile(const char *path, moduleinfostruct &info, binfile *file)
{
  if (!mcpOpenPlayer)
    return errGen;

  if (!file)
    return errFileOpen;

  _splitpath(path, 0, 0, currentmodname, currentmodext);

  printf("loading %s%s (%ik)...\n", currentmodname, currentmodext, file->length()>>10);

  int (*loader)(xmodule &, binfile &)=0;
  switch (info.modtype)
  {
    case mtXM  : loader=xmpLoadModule; break;
    case mtMOD : loader=xmpLoadMOD;    break;
    case mtMODt: loader=xmpLoadMODt;   break;
    case mtMODd: loader=xmpLoadMODd;   break;
    case mtM31 : loader=xmpLoadM31;    break;
    case mtM15 : loader=xmpLoadM15;    break;
    case mtM15t: loader=xmpLoadM15t;   break;
    case mtWOW : loader=xmpLoadWOW;    break;
    case mtMXM : loader=xmpLoadMXM;    break;
    case mtMODf: loader=xmpLoadMODf;   break;
  }
  if (!loader)
    return errFormStruc;

  int retval=loader(mod, *file);

  if (!retval)
    if (!xmpLoadSamples(mod))
      retval=-1;

  if (retval)
    xmpFreeModule(mod);

  xmpOptimizePatLens(mod);

  file->close();

  if (retval)
    return -1;

  mcpNormalize();
  if (!xmpPlayModule(mod))
    retval=errPlay;

  if (retval)
  {
    xmpFreeModule(mod);
    return retval;
  }

  insts=mod.instruments;
  samps=mod.samples;
  plNLChan=mod.nchan;

  plIsEnd=xmpLooped;
  plIdle=xmpIdle;
  plProcessKey=xmpProcessKey;
  plDrawGStrings=xmpDrawGStrings;
  plSetMute=xmpMute;
  plGetLChanSample=xmpGetLChanSample;

#ifdef DOS32
  plUseDots(xmpGetDots);
#endif

  plUseChannels(drawchannel);

  xmpInstSetup(mod.instruments, mod.ninst, mod.samples, mod.nsamp, mod.sampleinfos, mod.nsampi, 0, xmpMarkInsSamp);
  xmTrkSetup(mod);

  plNPChan=mcpNChan;
  modname=mod.name;
  composer="";
  if (!plCompoMode)
  {
    if (!*modname)
      modname=info.modname;
    composer=info.composer;
  }
  else
    modname=info.comment;

  plGetRealMasterVolume=mcpGetRealMasterVolume;
  plGetMasterSample=mcpGetMasterSample;
  plGetPChanSample=mcpGetChanSample;


  starttime=clock();
  plPause=0;
  mcpSet(-1, mcpMasterPause, 0);

  return errOk;
}

extern "C"
{
  cpifaceplayerstruct xmpPlayer = {xmpOpenFile, xmpCloseFile};
};