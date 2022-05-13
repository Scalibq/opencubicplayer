// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMIPlay interface routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kbwhenever Tammo Hinrichs <opencp@gmx.net>
//    -changed path searching for ULTRASND.INI and patch files
//    -removed strange error which executed interface even in case of errors


#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pfilesel.h"
#include "mcp.h"
#include "psetting.h"
#include "binfile.h"
#include "gmiplay.h"
#include "poutput.h"
#include "err.h"
#include "deviwave.h"
#include "cpiface.h"

#define MAXCHAN 64

extern int plLoopMods;

void mcpDrawGStrings(short (*)[132]);
void mcpNormalize();
int mcpSetProcessKey(unsigned short);

void gmiClearInst();

extern unsigned char plChanChanged;
extern char plPanType;
extern char plCompoMode;



static const char *modname;
static const char *composer;

static char currentmodname[_MAX_FNAME];
static char currentmodext[_MAX_EXT];

static long starttime;
static long pausetime;


static midifile mid;
void gmiInsSetup(const midifile &);
void gmiChanSetup(const midifile &mid);
int gmiGetDots(notedotsdata *, int);

static void gmiDrawGStrings(short (*buf)[132])
{
  mcpDrawGStrings(buf);
  if (plScrWidth==80)
  {
    mglobinfo gi;
    midGetGlobInfo(gi);
    writestring(buf[1],  0, 0x09, " pos: ......../........  spd: ....", 57);
    writenum(buf[1], 6, 0x0F, gi.curtick, 16, 8, 0);
    writenum(buf[1], 15, 0x0F, gi.ticknum-1, 16, 8, 0);
    writenum(buf[1], 30, 0x0F, gi.speed, 16, 4, 1);
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
    mglobinfo gi;
    midGetGlobInfo(gi);
    writestring(buf[1],  0, 0x09, "   position: ......../........  speed: ....", 80);
    writenum(buf[1], 13, 0x0F, gi.curtick, 16, 8, 0);
    writenum(buf[1], 22, 0x0F, gi.ticknum-1, 16, 8, 0);
    writenum(buf[1], 39, 0x0F, gi.speed, 16, 4, 1);
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

static int gmiProcessKey(unsigned short key)
{
  if (mcpSetProcessKey(key))
    return 1;
  switch (key)
  {
  case 'p': case 'P':
  case 16:
    if (plPause)
      starttime=starttime+clock()-pausetime;
    else
      pausetime=clock();
    mcpSet(-1, mcpMasterPause, plPause^=1);
    plChanChanged=1;
    break;
  case 0x7700: //ctrl-home
    gmiClearInst();
    midSetPosition(0);

    if (plPause)
      starttime=pausetime;
    else
      starttime=clock();
    break;
  case 0x7300: //ctrl-left
    midSetPosition(midGetPosition()-(mid.ticknum>>5));
    break;
  case 0x7400: //ctrl-right
    midSetPosition(midGetPosition()+(mid.ticknum>>5));
    break;
  case 0x8D00: // ctrl-up
    midSetPosition(midGetPosition()-(mid.ticknum>>8));
    break;
  case 0x9100: //ctrl-down
    midSetPosition(midGetPosition()+(mid.ticknum>>8));
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
    return 0;
  }
  return 1;
}

static void gmiCloseFile()
{
  midStopMidi();
  mid.free();
}

static int gmiLooped()
{
  return !plLoopMods&&midLooped();
}

static void gmiIdle()
{
  midSetLoop(plLoopMods);
  if (mcpIdle)
    mcpIdle();
}

static int gmiOpenFile(const char *path, moduleinfostruct &info, binfile *file)
{
  if (!mcpOpenPlayer)
    return errGen;
  if (!file)
    return errFileOpen;

  const char *gpp=cfGetProfileString("general", "ultradir", "");
  if (!*gpp)
    gpp=getenv("ULTRADIR");

  if (!gpp)
    gpp="";

  int i;

  _splitpath(path, 0, 0, currentmodname, currentmodext);

  printf("loading %s%s (%ik)...\n", currentmodname, currentmodext, file->length()>>10);

  int retval=midLoadMidi(mid, *file, (info.modtype==mtMIDd)?MID_DRUMCH16:0, gpp);

  if (retval==errOk)
  {
    printf("preparing samples (");
    int sampsize=0;
    for (i=0; i<mid.sampnum; i++)
      sampsize+=(mid.samples[i].length)<<(!!(mid.samples[i].type&mcpSamp16Bit));
    printf("%ik)...\n",sampsize>>10);

    if (!mid.loadsamples())
      retval=errAllocSamp;
  }
  else
  {
    mid.free();
    file->close();
    return errGen;
  }

  plNPChan=cfGetProfileInt2(cfSoundSec, "sound", "midichan", 24, 10);
  if (plNPChan<8)
    plNPChan=8;
  if (plNPChan>MAXCHAN)
    plNPChan=MAXCHAN;
  plNLChan=16;
  plPanType=0;
  modname="";
  composer="";

  plIsEnd=gmiLooped;
  plIdle=gmiIdle;
  plProcessKey=gmiProcessKey;
  plDrawGStrings=gmiDrawGStrings;
  plSetMute=midSetMute;
  plGetLChanSample=midGetChanSample;
                        /// todo: enable gmode in win32
#ifdef DOS32
  plUseDots(gmiGetDots);
#endif
  gmiChanSetup(mid);
  gmiInsSetup(mid);

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
  if (!midPlayMidi(mid, plNPChan))
    retval=errPlay;
  plNPChan=mcpNChan;

  plGetRealMasterVolume=mcpGetRealMasterVolume;
  plGetMasterSample=mcpGetMasterSample;
  plGetPChanSample=mcpGetChanSample;

  if (retval)
  {
    mid.free();
    return retval;
  }

  starttime=clock();
  plPause=0;
  mcpSet(-1, mcpMasterPause, 0);

  return errOk;
}

extern "C"
{
  cpifaceplayerstruct gmiPlayer = {gmiOpenFile, gmiCloseFile};
};