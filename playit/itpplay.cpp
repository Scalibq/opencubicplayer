// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// ITPlayer interface routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <kb@nwn.de>
//    -added many many things to provide channel display and stuff
//    -removed some bugs which caused crashing in some situations

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pfilesel.h"
#include "mcp.h"
#include "psetting.h"
#include "binfile.h"
#include "poutput.h"
#include "err.h"
#include "deviwave.h"
#include "cpiface.h"
#include "itplay.h"

itplayerclass itplayer;

extern int plLoopMods;
extern char plCompoMode;

static itplayerclass::module mod;

static const char *modname;
static const char *composer;

static char currentmodname[_MAX_FNAME];
static char currentmodext[_MAX_EXT];
static long starttime;
static long pausetime;

void mcpDrawGStrings(short (*)[132]);
void mcpNormalize();
int mcpSetProcessKey(unsigned short);

static itplayerclass::instrument *insts;
static itplayerclass::sample *samps;

void itpInstSetup(const itplayerclass::instrument *ins, int nins, const itplayerclass::sample *smp, int nsmp, const sampleinfo *smpi, int nsmpi, int type, void (*MarkyBoy)(char *, char *));
void itpInstClear();

extern char plNoteStr[132][4];

static int itpProcessKey(unsigned short key)
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
    itpInstClear();
    itplayer.setpos(0, 0);
    if (plPause)
      starttime=pausetime;
    else
      starttime=clock();
    break;
  case 0x7300: //ctrl-left
    p=itplayer.getpos();
    pat=p>>16;
    itplayer.setpos(pat-1, 0);
    break;
  case 0x7400: //ctrl-right
    p=itplayer.getpos();
    pat=p>>16;
    itplayer.setpos(pat+1, 0);
    break;
  case 0x8D00: // ctrl-up
    p=itplayer.getpos();
    pat=p>>16;
    row=(p>>8)&0xFF;
    itplayer.setpos(pat, row-8);
    break;
  case 0x9100: //ctrl-down
    p=itplayer.getpos();
    pat=p>>16;
    row=(p>>8)&0xFF;
    itplayer.setpos(pat, row+8);
    break;
  }
  return 1;
}

static int itpLooped()
{
  return !plLoopMods&&itplayer.getloop();
}

static void itpIdle()
{
  itplayer.setloop(plLoopMods);
  if (mcpIdle)
    mcpIdle();
}

static void itpDrawGStrings(short (*buf)[132])
{
  mcpDrawGStrings(buf);
  if (plScrWidth==80)
  {
    int pos=itplayer.getrealpos()>>8;
    int gvol,bpm,tmp,gs;
    itplayer.getglobinfo(tmp,bpm,gvol,gs);
    writestring(buf[1],  0, 0x09, " row: ../..  ord: .../...  speed: ..  bpm: ...  gvol: ..ú ", 58);
    writenum(buf[1],  6, 0x0F, pos&0xFF, 16, 2, 0);
    writenum(buf[1],  9, 0x0F, mod.patlens[mod.orders[pos>>8]]-1, 16, 2, 0);
    writenum(buf[1], 18, 0x0F, pos>>8, 16, 3, 0);
    writenum(buf[1], 22, 0x0F, mod.nord-1, 16, 3, 0);
    writenum(buf[1], 34, 0x0F, tmp, 16, 2, 1);
    writenum(buf[1], 43, 0x0F, bpm, 10, 3, 1);
    writenum(buf[1], 54, 0x0F, gvol, 16, 2, 0);
    writestring(buf[1], 56, 0x0F, (gs==itplayerclass::ifxGVSUp)?"\x18":(gs==itplayerclass::ifxGVSDown)?"\x19":" ", 1);
    writestring(buf[2],  0, 0x09, " module úúúúúúúú.úúú: ...............................               time: ..:.. ", 80);
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
    int pos=itplayer.getrealpos()>>8;
    int gvol,bpm,tmp, gs;
    itplayer.getglobinfo(tmp,bpm,gvol,gs);
    writestring(buf[1],  0, 0x09, "    row: ../..  order: .../...   speed: ..  tempo: ...   gvol: ..ú  chan: ../..", 81);
    writenum(buf[1],  9, 0x0F, pos&0xFF, 16, 2, 0);
    writenum(buf[1], 12, 0x0F, mod.patlens[mod.orders[pos>>8]]-1, 16, 2, 0);
    writenum(buf[1], 23, 0x0F, pos>>8, 16, 3, 0);
    writenum(buf[1], 27, 0x0F, mod.nord-1, 16, 3, 0);
    writenum(buf[1], 40, 0x0F, tmp, 16, 2, 1);
    writenum(buf[1], 51, 0x0F, bpm, 10, 3, 1);
    writenum(buf[1], 63, 0x0F, gvol, 16, 2, 0);
    writestring(buf[1], 65, 0x0F, (gs==itplayerclass::ifxGVSUp)?"\x18":(gs==itplayerclass::ifxGVSDown)?"\x19":" ", 1);
    int i;
    int nch=0;
    for (i=0; i<plNPChan; i++)
      if (mcpGet(i, mcpCStatus))
        nch++;
    writenum(buf[1], 74, 0x0F, nch, 16, 2, 0);
    writenum(buf[1], 77, 0x0F, plNPChan, 16, 2, 0);
    writestring(buf[2],  0, 0x09, "    module úúúúúúúú.úúú: ...............................  composer: ...............................                  time: ..:..    ", 132);
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

static void itpCloseFile()
{
  itplayer.stop();
  mcpSet(-1, mcpGRestrict, 0);
  mod.free();
}

//**********************************************************************

static void itpMarkInsSamp(char *ins, char *smp)
{
  int i;
  for (i=0; i<plNLChan; i++)
  {
    if (plMuteCh[i])
      continue;
    for (int j=0; j<plNLChan; j++)
    {
      int lc;
      if (!itplayer.chanactive(j,lc) || lc!=i)
        continue;
      int in=itplayer.getchanins(j);
      int sm=itplayer.getchansamp(j);
      ins[in-1]=((plSelCh==i)||(ins[in-1]==3))?3:2;
      smp[sm]=((plSelCh==i)||(smp[sm]==3))?3:2;
    }
  }
}

//************************************************************************

static void logvolbar(int &l, int &r)
{
  l*=2;
  r*=2;
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
  itplayer.getrealvol(i, l, r);
  logvolbar(l, r);

  l=(l+4)>>3;
  r=(r+4)>>3;
  if (plPause)
    l=r=0;
  if (st)
  {
    writestring(buf, 8-l, 0x08, "şşşşşşşş", l);
    writestring(buf, 9, 0x08, "şşşşşşşş", r);
  }
  else
  {
    writestringattr(buf, 8-l, "ş\x0Fş\x0Bş\x0Bş\x09ş\x09ş\x01ş\x01ş\x01"+16-l-l, l);
    writestringattr(buf, 9, "ş\x01ş\x01ş\x01ş\x09ş\x09ş\x0Bş\x0Bş\x0F", r);
  }
}

static void drawlongvolbar(short *buf, int i, unsigned char st)
{
  int l,r;
  itplayer.getrealvol(i, l, r);
  logvolbar(l, r);
  l=(l+2)>>2;
  r=(r+2)>>2;
  if (plPause)
    l=r=0;
  if (st)
  {
    writestring(buf, 16-l, 0x08, "şşşşşşşşşşşşşşşş", l);
    writestring(buf, 17, 0x08, "şşşşşşşşşşşşşşşş", r);
  }
  else
  {
    writestringattr(buf, 16-l, "ş\x0Fş\x0Fş\x0Bş\x0Bş\x0Bş\x0Bş\x09ş\x09ş\x09ş\x09ş\x01ş\x01ş\x01ş\x01ş\x01ş\x01"+32-l-l, l);
    writestringattr(buf, 17, "ş\x01ş\x01ş\x01ş\x01ş\x01ş\x01ş\x09ş\x09ş\x09ş\x09ş\x0Bş\x0Bş\x0Bş\x0Bş\x0Fş\x0F", r);
  }
}

static char *fxstr3[]={0,"vl\x18","vl\x19","fv\x18","fv\x19","pt\x18",
                       "pt\x19","pt\x0d","fp\x18","fp\x19","pn\x1a","pn\x1b",
                       "tre", "trr","vib","arp","cut","ret","ofs","eps",
                       "del", "cv\x18", "cv\x19", "fc\x18", "fc\x19","p-c",
                       "p-o", "p-f", "ve0", "ve1", "pe0", "pe1", "fe0",
                       "fe1", "pbr"
                      };

static char *fxstr6[]={0, "volsl\x18","volsl\x19","fvols\x18","fvols\x19",
                       "porta\x18","porta\x19","porta\x0d","fport\x18",
                       "fport\x19","pansl\x1a","pansl\x1b","tremol","tremor",
                       "vibrat","arpegg"," \x0e""cut ","retrig","offset",
                       "envpos","delay\x0d", "chvol\x18", "chvol\x19",
                       "fchvl\x18", "fchvl\x19", "past-C", "past-O",
                       "past-F", "venv:0", "venv:1", "penv:0", "penv:1",
                       "fenv:0", "fenv:1", "panbrl"
                      };

static char *fxstr12[]={0, "volumeslide\x18","volumeslide\x19",
                        "finevolslid\x18","finevolslid\x19","portamento \x18",
                        "portamento \x19","porta to \x0d  ","fine porta \x18",
                        "fine porta \x19","pan slide \x1a ","pan slide \x1b ",
                        "tremolo     ","tremor      ","vibrato     ",
                        "arpeggio    ","note cut    ","note retrig ",
                        "sampleoffset","set env pos ","note delay  ",
                        "chanvolslid\x18","chanvolslid\x19",
                        "finechvolsl\x18","finechvolsl\x19", "past cut",
                        "past off","past fade","vol env off","vol env on",
                        "pan env off", "pan env on", "pitchenv off",
                        "pitchenv on", "panbrello"
                       };


static void drawchannel(short *buf, int len, int i)
{
  unsigned char st=plMuteCh[i];

  unsigned char tcol=st?0x08:0x0F;
  unsigned char tcold=st?0x08:0x07;
  unsigned char tcolr=st?0x08:0x0B;

  switch (len)
  {
  case 36:
    writestring(buf, 0, tcold, " úú -- --- -- --- úúúúúúúú úúúúúúúú ", 36);
    break;
  case 62:
    writestring(buf, 0, tcold, " úú                      ---ú --ú -ú ------ úúúúúúúú úúúúúúúú ", 62);
    break;
  case 128:
    writestring(buf,  0, tcold, " úú                             ³                   ³    ³   ³  ³            ³  úúúúúúúúúúúúúúúú úúúúúúúúúúúúúúúú", 128);
    break;
  case 76:
    writestring(buf,  0, tcold, " úú                             ³    ³   ³  ³            ³ úúúúúúúú úúúúúúúú", 76);
    break;
  case 44:
    writestring(buf, 0, tcold, " úú -- ---ú --ú -ú ------ úúúúúúúú úúúúúúúú ", 44);
    break;
  }

  int av=itplayer.getchanalloc(i);
  if (av)
    writenum(buf, 1, tcold, av, 16, 2, 0);


  if (!itplayer.lchanactive(i))
    return;


  itplayerclass::chaninfo ci;
  itplayer.getchaninfo(i,ci);

  char *fxstr;
  switch (len)
  {
  case 36:
    writenum(buf,  4, tcol, ci.ins, 16, 2, 0);
    writestring(buf,  7, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writenum(buf, 11, tcol, ci.vol, 16, 2, 0);
    fxstr=fxstr3[ci.fx];
    if (fxstr)
      writestring(buf, 14, tcol, fxstr, 3);
    drawvolbar(buf+18, i, st);
    break;
  case 62:
    if (ci.ins)
      if (*insts[ci.ins-1].name)
        writestring(buf,  4, tcol, insts[ci.ins-1].name, 19);
      else
      {
        writestring(buf,  4, 0x08, "(  )", 4);
        writenum(buf,  5, 0x08, ci.ins, 16, 2, 0);
      }
    writestring(buf, 25, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 28, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~ğ"+ci.pitchfx, 1);
    writenum(buf, 30, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 32, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 34, tcol, "L123456MM9ABCDERS"+(ci.pan), 1);
    writestring(buf, 35, tcol, " \x1A\x1B"+ci.panslide, 1);
    fxstr=fxstr6[ci.fx];
    if (fxstr)
      writestring(buf, 37, tcol, fxstr, 6);
    drawvolbar(buf+44, i, st);
    break;
  case 76:
    if (ci.ins)
      if (*insts[ci.ins-1].name)
        writestring(buf,  4, tcol, insts[ci.ins-1].name, 28);
      else
      {
        writestring(buf,  4, 0x08, "(  )", 4);
        writenum(buf,  5, 0x08, ci.ins, 16, 2, 0);
      }
    writestring(buf, 33, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 36, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~ğ"+ci.pitchfx, 1);
    writenum(buf, 38, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 40, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 42, tcol, "L123456MM9ABCDERS"+(ci.pan), 1);
    writestring(buf, 43, tcol, " \x1A\x1B"+ci.panslide, 1);

    fxstr=fxstr12[ci.fx];
    if (fxstr)
      writestring(buf, 45, tcol, fxstr, 12);

    drawvolbar(buf+59, i, st);
    break;
  case 128:
    if (ci.ins)
      if (*insts[ci.ins-1].name)
        writestring(buf,  4, tcol, insts[ci.ins-1].name, 28);
      else
      {
        writestring(buf,  4, 0x08, "(  )", 4);
        writenum(buf,  5, 0x08, ci.ins, 16, 2, 0);
      }
    if (ci.smp!=0xFFFF)
      if (*samps[ci.smp].name)
        writestring(buf, 34, tcol, samps[ci.smp].name, 17);
      else
      {
        writestring(buf, 34, 0x08, "(    )", 6);
        writenum(buf, 35, 0x08, ci.smp, 16, 4, 0);
      }
    writestring(buf, 53, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 56, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~ğ"+ci.pitchfx, 1);
    writenum(buf, 58, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 60, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 62, tcol, "L123456MM9ABCDERS"+(ci.pan), 1);
    writestring(buf, 63, tcol, " \x1A\x1B"+ci.panslide, 1);

    fxstr=fxstr12[ci.fx];
    if (fxstr)
      writestring(buf, 65, tcol, fxstr, 12);
    drawlongvolbar(buf+80, i, st);
    break;
  case 44:
    writenum(buf,  4, tcol, ci.ins, 16, 2, 0);
    writestring(buf,  7, ci.notehit?tcolr:tcol, plNoteStr[ci.note], 3);
    writestring(buf, 10, tcol, ci.pitchslide?" \x18\x19\x0D\x18\x19\x0D"+ci.pitchslide:" ~ğ"+ci.pitchfx, 1);
    writenum(buf, 12, tcol, ci.vol, 16, 2, 0);
    writestring(buf, 14, tcol, ci.volslide?" \x18\x19\x18\x19"+ci.volslide:" ~"+ci.volfx, 1);
    writestring(buf, 16, tcol, "L123456MM9ABCDERS"+(ci.pan), 1);
    writestring(buf, 17, tcol, " \x1A\x1B"+ci.panslide, 1);

    fxstr=fxstr6[ci.fx];
    if (fxstr)
      writestring(buf, 19, tcol, fxstr, 6);
    drawvolbar(buf+26, i, st);
    break;
  }
}

//************************************************************************

int itpGetDots(notedotsdata *d, int max)
{
  int i,j;
  int pos=0;
  for (i=0; i<plNLChan; i++)
  {
    if (pos>=max)
      break;
    j=0;
    while (pos<max)
    {
      int smp, voll, volr, note, sus;
      j=itplayer.getdotsdata(i, j, smp, note, voll, volr, sus);
      if (j==-1)
        break;
      d[pos].voll=voll;
      d[pos].volr=volr;
      d[pos].chan=i;
      d[pos].note=note;
      d[pos].col=(smp&15)+(sus?32:16);
      pos++;
    }
  }
  return pos;
}

void itpMute(int i, int m)
{
  itplayer.mutechan(i,m);
}

int itpGetLChanSample(int ch, short *buf, int len, int rate, int opt)
{
  return itplayer.getchansample(ch,buf,len,rate,opt);
}


void itTrkSetup(const itplayerclass::module &mod);


static int itpOpenFile(const char *path, moduleinfostruct &info, binfile *file)
{
  if (!mcpOpenPlayer)
    return errGen;

  if (!file)
    return errFileOpen;

  _splitpath(path, 0, 0, currentmodname, currentmodext);

  printf("loading %s%s (%ik)...\n", currentmodname, currentmodext, file->length()>>10);

  int retval=mod.load(*file);

  if (!retval)
    if (!itplayer.loadsamples(mod))
      retval=-1;

  if (retval)
    mod.free();

  mod.optimizepatlens();

  file->close();

  if (retval)
    return -1;


  mcpNormalize();
  int nch=cfGetProfileInt2(cfSoundSec, "sound", "itchan", 64, 10);
  mcpSet(-1, mcpGRestrict, 0);  // oops...
  if (!itplayer.play(mod,nch))
    retval=errPlay;

  if (retval)
  {
    mod.free();
    return retval;
  }


  insts=mod.instruments;
  samps=mod.samples;
  plNLChan=mod.nchan;
  plIsEnd=itpLooped;
  plIdle=itpIdle;
  plProcessKey=itpProcessKey;
  plDrawGStrings=itpDrawGStrings;
  plSetMute=itpMute;
  plGetLChanSample=itpGetLChanSample;
#ifdef DOS32
  plUseDots(itpGetDots);
#endif
  plUseChannels(drawchannel);
  itpInstSetup(mod.instruments, mod.ninst, mod.samples, mod.nsamp, mod.sampleinfos, mod.nsampi, 0, itpMarkInsSamp);
  itTrkSetup(mod);
  if (mod.message)
    plUseMessage(mod.message);
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
  cpifaceplayerstruct itpPlayer = {itpOpenFile, itpCloseFile};
};