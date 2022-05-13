// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMDPlay interface routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pfilesel.h"
#include "mcp.h"
#include "psetting.h"
#include "binfile.h"
#include "gmdplay.h"
#include "poutput.h"
#include "err.h"
#include "plinkman.h"
#include "deviwave.h"
#include "cpiface.h"

int gmdActive;

extern int plLoopMods;

void mcpDrawGStrings(short (*)[132]);
void mcpNormalize();
int mcpSetProcessKey(unsigned short);


extern unsigned char plChanChanged;
extern char plPanType;
extern char plCompoMode;


static const char *modname;
static const char *composer;

static char currentmodname[_MAX_FNAME];
static char currentmodext[_MAX_EXT];



static long starttime;
static long pausetime;

static gmdmodule mod;
static char patlock;

static void gmdMarkInsSamp(char *ins, char *samp)
{
  int i;
  for (i=0; i<plNLChan; i++)
  {
    chaninfo ci;
    mpGetChanInfo(i, ci);

    if (!mpGetMute(i)&&mpGetChanStatus(i)&&ci.vol)
    {
      ins[ci.ins]=((plSelCh==i)||(ins[ci.ins]==3))?3:2;
      samp[ci.smp]=((plSelCh==i)||(samp[ci.smp]==3))?3:2;
    }
  }
}


static int mpLoadGen(gmdmodule &m, binfile &file, int type)
{
  char secname[20];
  strcpy(secname, "filetype ");
  ultoa(type&0xFF, secname+strlen(secname), 10);

  const char *link=cfGetProfileString(secname, "ldlink", "");
  const char *name=cfGetProfileString(secname, "loader", "");

  int hnd=lnkLink(link);
  if (hnd<0)
    return errSymMod;

  void *loadfn=lnkGetSymbol(name);
  if (!loadfn)
  {
    lnkFree(hnd);
    return errSymSym;
  }
  volatile char retval=((int (*)(gmdmodule &, binfile &))loadfn)(m, file);

  lnkFree(hnd);

  return retval;
}

void mcpSetFadePars(int i);

static unsigned long pausefadestart;
static char pausefaderelspeed;
static signed char pausefadedirect;

static void startpausefade()
{
  if (plPause)
    starttime=starttime+clock()-pausetime;

  if (pausefadedirect)
  {
    if (pausefadedirect<0)
      plPause=1;
    pausefadestart=2*clock()-CLK_TCK-pausefadestart;
  }
  else
    pausefadestart=clock();

  if (plPause)
  {
    plChanChanged=1;
    mcpSet(-1, mcpMasterPause, plPause=0);
    pausefadedirect=1;
  }
  else
    pausefadedirect=-1;
}

static void dopausefade()
{
  short i;
  if (pausefadedirect>0)
  {
    i=((signed long)clock()-pausefadestart)*64/CLK_TCK;
    if (i<0)
      i=0;
    if (i>=64)
    {
      i=64;
      pausefadedirect=0;
    }
  }
  else
  {
    i=64-((signed long)clock()-pausefadestart)*64/CLK_TCK;
    if (i>=64)
      i=64;
    if (i<=0)
    {
      i=0;
      pausefadedirect=0;
      pausetime=clock();
      mcpSet(-1, mcpMasterPause, plPause=1);
      plChanChanged=1;
      mcpSetFadePars(64);
      return;
    }
  }
  pausefaderelspeed=i;
  mcpSetFadePars(i);
}


int gmdTrkProcessKey(unsigned short key);
void gmdChanSetup(const gmdmodule &);
void gmdTrkSetup(const gmdmodule &m);
int gmdGetDots(notedotsdata *, int);
void gmdDrawPattern(char sel);

static void gmdDrawGStrings(short (*buf)[132])
{
  mcpDrawGStrings(buf);
  if (plScrWidth==80)
  {
    globinfo gi;
    mpGetGlobInfo(gi);
    writestring(buf[1],  0, 0x09, " row: ../..  ord: .../...  tempo: ..  bpm: ...  gvol: ..ת ", 58);
    writenum(buf[1],  6, 0x0F, gi.currow, 16, 2, 0);
    writenum(buf[1],  9, 0x0F, gi.patlen-1, 16, 2, 0);
    writenum(buf[1], 18, 0x0F, gi.curpat, 16, 3, 0);
    writenum(buf[1], 22, 0x0F, gi.patnum-1, 16, 3, 0);
    writenum(buf[1], 34, 0x0F, gi.tempo, 16, 2, 1);
    writenum(buf[1], 43, 0x0F, gi.speed, 10, 3, 1);
    writenum(buf[1], 54, 0x0F, gi.globvol, 16, 2, 0);
    writestring(buf[1], 56, 0x0F, (gi.globvolslide==fxGVSUp)?"\x18":(gi.globvolslide==fxGVSDown)?"\x19":" ", 1);
    long tim;
    if (plPause)
      tim=(pausetime-starttime)/CLK_TCK;
    else
      tim=(clock()-starttime)/CLK_TCK;

    writestring(buf[2],  0, 0x09, " module תתתתתתתת.תתת: ...............................               time: ..:.. ", 80);
    writestring(buf[2],  8, 0x0F, currentmodname, 8);
    writestring(buf[2], 16, 0x0F, currentmodext, 4);
    writestring(buf[2], 22, 0x0F, modname, 31);
    if (plPause)
      writestring(buf[2], 58, 0x0C, "paused", 6);
    writenum(buf[2], 74, 0x0F, (tim/60)%60, 10, 2, 1);
    writestring(buf[2], 76, 0x0F, ":", 1);
    writenum(buf[2], 77, 0x0F, tim%60, 10, 2, 0);
  }
  else
  {
    globinfo gi;
    mpGetGlobInfo(gi);
    writestring(buf[1],  0, 0x09, "    row: ../..  order: .../...   tempo: ..  speed/bpm: ...   global volume: ..ת  ", 81);
    writenum(buf[1],  9, 0x0F, gi.currow, 16, 2, 0);
    writenum(buf[1], 12, 0x0F, gi.patlen-1, 16, 2, 0);
    writenum(buf[1], 23, 0x0F, gi.curpat, 16, 3, 0);
    writenum(buf[1], 27, 0x0F, gi.patnum-1, 16, 3, 0);
    writenum(buf[1], 40, 0x0F, gi.tempo, 16, 2, 1);
    writenum(buf[1], 55, 0x0F, gi.speed, 10, 3, 1);
    writenum(buf[1], 76, 0x0F, gi.globvol, 16, 2, 0);
    writestring(buf[1], 78, 0x0F, (gi.globvolslide==fxGVSUp)?"\x18":(gi.globvolslide==fxGVSDown)?"\x19":" ", 1);
    long tim;
    if (plPause)
      tim=(pausetime-starttime)/CLK_TCK;
    else
      tim=(clock()-starttime)/CLK_TCK;

    writestring(buf[2],  0, 0x09, "    module תתתתתתתת.תתת: ...............................  composer: ...............................                  time: ..:..    ", 132);
    writestring(buf[2], 11, 0x0F, currentmodname, 8);
    writestring(buf[2], 19, 0x0F, currentmodext, 4);
    writestring(buf[2], 25, 0x0F, modname, 31);
    writestring(buf[2], 68, 0x0F, composer, 31);
    if (plPause)
      writestring(buf[2], 100, 0x0C, "playback paused", 15);
    writenum(buf[2], 123, 0x0F, (tim/60)%60, 10, 2, 1);
    writestring(buf[2], 125, 0x0F, ":", 1);
    writenum(buf[2], 126, 0x0F, tim%60, 10, 2, 0);
  }
}

void dgetch();
#pragma aux dgetch modify [ax] = "mov ah,7" "int 0x21"
unsigned char dkbhit();
#pragma aux dkbhit modify [ax] value [al] = "mov ah,11" "int 0x21"
void releaseslice();
#pragma aux releaseslice modify [ax] = "mov ax,1680h" "int 0x2F"

static int gmdProcessKey(unsigned short key)
{
  if (mcpSetProcessKey(key))
    return 1;
  unsigned short pat;
  unsigned char row;
  switch (key)
  {
  case 0x1900:
    while (!dkbhit())
    {
      if (mcpIdle)
        mcpIdle();
      releaseslice();
    }
    while (dkbhit())
      dgetch();
    break;


  case 'p': case 'P':
    startpausefade();
    break;
  case 0x10:
    pausefadedirect=0;
    if (plPause)
      starttime=starttime+clock()-pausetime;
    else
      pausetime=clock();
    mcpSet(-1, mcpMasterPause, plPause^=1);
    plChanChanged=1;
    break;
  case 0x7700: //ctrl-home
    gmdInstClear();

    mpSetPosition(0, 0);

    if (plPause)
      starttime=pausetime;
    else
      starttime=clock();
    break;
  case 0x7300: //ctrl-left
    mpGetPosition(pat, row);
    mpSetPosition(pat-1, 0);
    break;
  case 0x7400: //ctrl-right
    mpGetPosition(pat, row);
    mpSetPosition(pat+1, 0);
    break;
  case 0x8D00: // ctrl-up
    mpGetPosition(pat, row);
    mpSetPosition(pat, row-8);
    break;
  case 0x9100: //ctrl-down
    mpGetPosition(pat, row);
    mpSetPosition(pat, row+8);
    break;
  case 0x2600:
    patlock=!patlock;
    mpLockPat(patlock);
    break;
  default:
    if (mcpProcessKey)
    {
      int ret=mcpProcessKey(key);
      if (ret==2)
        cpiResetScreen();
      if (ret)
        return 1;
    }
  }
  return 1;
}

static void gmdCloseFile()
{
  gmdActive=0;
  mpStopModule();
  mpFree(mod);
}

static void gmdIdle()
{
  mpSetLoop(plLoopMods);
  if (mcpIdle)
    mcpIdle();
  if (pausefadedirect)
    dopausefade();
}

static int gmdLooped()
{
  return (!plLoopMods&&mpLooped());
}

static int gmdOpenFile(const char *path, moduleinfostruct &info, binfile *file)
{
  if (!mcpOpenPlayer)
    return errGen;

  if (!file)
    return errFileOpen;

  patlock=0;
  int i;

  _splitpath(path, 0, 0, currentmodname, currentmodext);

  printf("loading %s%s (%ik)...\n",currentmodname, currentmodext, file->length()>>10);

  int retval=mpLoadGen(mod, *file, info.modtype);

  if (!retval)
  {
    printf("preparing samples (");
    int sampsize=0;
    for (i=0; i<mod.sampnum; i++)
      sampsize+=(mod.samples[i].length)<<(!!(mod.samples[i].type&mcpSamp16Bit));
    printf("%ik)...\n", sampsize>>10);

    if (!mpReduceSamples(mod))
      retval=errAllocMem;
    else
    if (!mpLoadSamples(mod))
      retval=errAllocSamp;
    else
    {
      mpReduceMessage(mod);
      mpReduceInstruments(mod);
      mpOptimizePatLens(mod);
    }
  }
  else
  {
    mpFree(mod);
    file->close();
    return retval;
  }

  if (retval)
    mpFree(mod);

  file->close();

  if (retval)
    return retval;

  if (plCompoMode)
    mpRemoveText(mod);
  plNLChan=mod.channum;
  modname=mod.name;
  composer=mod.composer;
  plPanType=!!(mod.options&MOD_MODPAN);

  plIsEnd=gmdLooped;
  plIdle=gmdIdle;
  plProcessKey=gmdProcessKey;
  plDrawGStrings=gmdDrawGStrings;
  plSetMute=mpMute;
  plGetLChanSample=mpGetChanSample;
                                // TODO: add faked-whatever-cewl-eleet-graphik-modes here.
#ifdef DOS32
  plUseDots(gmdGetDots);
#endif
  if (mod.message)
    plUseMessage(mod.message);
  gmdInstSetup(mod.instruments, mod.instnum, mod.modsamples, mod.modsampnum, mod.samples, mod.sampnum, ((info.modtype==mtS3M)||(info.modtype==mtPTM))?1:((info.modtype==mtDMF)||(info.modtype==mt669))?2:0, gmdMarkInsSamp);
  gmdChanSetup(mod);
  gmdTrkSetup(mod);

  if (!plCompoMode)
  {
    if (!*modname)
      modname=info.modname;
    if (!*composer)
      composer=info.composer;
  }
  else
    modname=info.comment;

  mcpNormalize();
  if (!mpPlayModule(mod))
    retval=errPlay;
  plNPChan=mcpNChan;

  plGetRealMasterVolume=mcpGetRealMasterVolume;
  plGetMasterSample=mcpGetMasterSample;
  plGetPChanSample=mcpGetChanSample;

  if (retval)
  {
    mpFree(mod);
    return retval;
  }

  starttime=clock();
  plPause=0;
  mcpSet(-1, mcpMasterPause, 0);
  pausefadedirect=0;

  gmdActive=1;

  return errOk;
}

extern "C"
{
  cpifaceplayerstruct gmdPlayer = {gmdOpenFile, gmdCloseFile};

  char *dllinfo = "player _gmdPlayer";
};