// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// MPPlay interface routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981210   Felix Domke <tmbinc@gmx.net>
//    -edited for new binfile, made and fixed a memory leak :)
//  -ryg981218  Fabian Giesen <fabian@jdcs.su.nw.schule.de>
//    -made max. amplification 793%, as in module players

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include "pmain.h"
#include "pfilesel.h"
#include "poutput.h"
#include "player.h"
#include "psetting.h"
#include "binfile.h"
#include "binfmem.h"
#include "mpplay.h"
#include "sets.h"
#include "deviplay.h"
#include "cpiface.h"

extern char plPause;

extern int plLoopMods;

extern int ampeg_BoeseBitrate; // schlimm aber funzt.

static binfile *wavefile;
static mbinfile memfile;
static unsigned long wavelen;
static unsigned long waverate;

static long starttime;
static long pausetime;
static char currentmodname[_MAX_FNAME];
static char currentmodext[_MAX_EXT];
static char *modname;
static char *composer;
static short vol;
static short bal;
static short pan;
static char srnd;
static long amp;
static short speed;
static short reverb;
static short chorus;
static char finespeed=8;

static char *membuf;

static void wavDrawGStrings(short (*buf)[132])
{
  if (plScrWidth==80)
  {
    writestring(buf[0], 0, 0x09, " vol: úúúúúúúú ", 15);
    writestring(buf[0], 15, 0x09, " srnd: ú  pan: lúúúmúúúr  bal: lúúúmúúúr ", 41);
    writestring(buf[0], 56, 0x09, " spd: ---% \x1D ptch: ---% ", 24);
    writestring(buf[0], 6, 0x0F, "þþþþþþþþ", (vol+4)>>3);
    writestring(buf[0], 22, 0x0F, srnd?"x":"o", 1);
    if (((pan+70)>>4)==4)
      writestring(buf[0], 34, 0x0F, "m", 1);
    else
    {
      writestring(buf[0], 30+((pan+70)>>4), 0x0F, "r", 1);
      writestring(buf[0], 38-((pan+70)>>4), 0x0F, "l", 1);
    }
    writestring(buf[0], 46+((bal+70)>>4), 0x0F, "I", 1);
    writenum(buf[0], 62, 0x0F, speed*100/256, 10, 3);
    writenum(buf[0], 75, 0x0F, speed*100/256, 10, 3);

    writestring(buf[1], 57, 0x09, "amp: ...% filter: ...  ", 23);
    writenum(buf[1], 62, 0x0F, amp*100/64, 10, 3);
    writestring(buf[1], 75, 0x0F, "off", 3);

    waveinfo inf;
    wpGetInfo(inf);
    long tim;

    tim=inf.len/inf.rate;
    writestring(buf[1], 0, 0x09, "  pos: ...% / ......k  size: ......k  len: ..:..", 57);
    writenum(buf[1], 7, 0x0F, (inf.pos*100+(inf.len>>1))/inf.len, 10, 3);
    writenum(buf[1], 43, 0x0F, (tim/60)%60, 10, 2, 1);
    writestring(buf[1], 45, 0x0F, ":", 1);
    writenum(buf[1], 46, 0x0F, tim%60, 10, 2, 0);
    writenum(buf[1], 29, 0x0F, (inf.len>>(10-inf.stereo-inf.bit16)), 10, 6, 1);
    writenum(buf[1], 14, 0x0F, (inf.pos>>(10-inf.stereo-inf.bit16)), 10, 6, 1);

    if (plPause)
      tim=(pausetime-starttime)/CLK_TCK;
    else
      tim=(clock()-starttime)/CLK_TCK;

    writestring(buf[2],  0, 0x09, "   wave úúúúúúúú.úúú: ...............................               time: ..:.. ", 80);
    writestring(buf[2],  8, 0x0F, currentmodname, 8);
    writestring(buf[2], 16, 0x0F, currentmodext, 4);
    writestring(buf[2], 22, 0x0F, modname, 31);
    if (plPause)
      writestring(buf[2], 57, 0x0C, " paused ", 8);
    else
    {
      writestring(buf[2], 57, 0x09, "kbps: ", 6);
      writenum(buf[2], 63, 0x0F, ampeg_BoeseBitrate, 10, 3, 1);
    }
    writenum(buf[2], 74, 0x0F, (tim/60)%60, 10, 2, 1);
    writestring(buf[2], 76, 0x0F, ":", 1);
    writenum(buf[2], 77, 0x0F, tim%60, 10, 2, 0);
  }
  else
  {
    writestring(buf[0], 0, 0x09, "    volume: úúúúúúúúúúúúúúúú  ", 30);
    writestring(buf[0], 30, 0x09, " surround: ú   panning: lúúúúúúúmúúúúúúúr   balance: lúúúúúúúmúúúúúúúr  ", 72);
    writestring(buf[0], 102, 0x09,  " speed: ---% \x1D pitch: ---%    ", 30);
    writestring(buf[0], 12, 0x0F, "þþþþþþþþþþþþþþþþ", (vol+2)>>2);
    writestring(buf[0], 41, 0x0F, srnd?"x":"o", 1);
    if (((pan+68)>>3)==8)
      writestring(buf[0], 62, 0x0F, "m", 1);
    else
    {
      writestring(buf[0], 54+((pan+68)>>3), 0x0F, "r", 1);
      writestring(buf[0], 70-((pan+68)>>3), 0x0F, "l", 1);
    }
    writestring(buf[0], 83+((bal+68)>>3), 0x0F, "I", 1);
    writenum(buf[0], 110, 0x0F, speed*100/256, 10, 3);
    writenum(buf[0], 124, 0x0F, speed*100/256, 10, 3);

    waveinfo inf;
    wpGetInfo(inf);
    long tim;
    tim=inf.len/inf.rate;
    writestring(buf[1], 0, 0x09, "    position: ...% / ......k  size: ......k  length: ..:..  opt: .....Hz, .. bit, ......", 92);
    writenum(buf[1], 14, 0x0F, (inf.pos*100+(inf.len>>1))/inf.len, 10, 3);
    writenum(buf[1], 53, 0x0F, (tim/60)%60, 10, 2, 1);
    writestring(buf[1], 55, 0x0F, ":", 1);
    writenum(buf[1], 56, 0x0F, tim%60, 10, 2, 0);
    writenum(buf[1], 36, 0x0F, (inf.len>>(10-inf.stereo-inf.bit16)), 10, 6, 1);
    writenum(buf[1], 21, 0x0F, (inf.pos>>(10-inf.stereo-inf.bit16)), 10, 6, 1);
    writenum(buf[1], 65, 0x0F, inf.rate, 10, 5, 1);
    writenum(buf[1], 74, 0x0F, 8<<inf.bit16, 10, 2, 1);
    writestring(buf[1], 82, 0x0F, inf.stereo?"stereo":"mono", 6);

    writestring(buf[1], 92, 0x09, "   amplification: ...%  filter: ...     ", 40);
    writenum(buf[1], 110, 0x0F, amp*100/64, 10, 3);
    writestring(buf[1], 124, 0x0F, "off", 3);

    if (plPause)
      tim=(pausetime-starttime)/CLK_TCK;
    else
      tim=(clock()-starttime)/CLK_TCK;

    writestring(buf[2],  0, 0x09, "      wave úúúúúúúú.úúú: ...............................  composer: ...............................                  time: ..:..    ", 132);
    writestring(buf[2], 11, 0x0F, currentmodname, 8);
    writestring(buf[2], 19, 0x0F, currentmodext, 4);
    writestring(buf[2], 25, 0x0F, modname, 31);
    writestring(buf[2], 68, 0x0F, composer, 31);
    if (plPause)
      writestring(buf[2], 100, 0x0C, "playback paused", 15);
    else
    {
      writestring(buf[2], 100, 0x09, "kbps: ", 6);
      writenum(buf[2], 106, 0x0F, ampeg_BoeseBitrate, 10, 3, 1);
    }
    writenum(buf[2], 123, 0x0F, (tim/60)%60, 10, 2, 1);
    writestring(buf[2], 125, 0x0F, ":", 1);
    writenum(buf[2], 126, 0x0F, tim%60, 10, 2, 0);
  }
}

static void normalize()
{
  speed=set.speed;
  pan=set.pan;
  bal=set.bal;
  vol=set.vol;
  amp=set.amp;
  srnd=set.srnd;
  reverb=set.reverb;
  chorus=set.chorus;
  wpSetAmplify(1024*amp);
  wpSetVolume(vol, bal, pan, srnd);
  wpSetSpeed(speed);
//  wpSetMasterReverbChorus(reverb, chorus);
}

static int wavProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'p': case 'P': case 0x10:
    if (plPause)
      starttime=starttime+clock()-pausetime;
    else
      pausetime=clock();
    plPause=!plPause;
    wpPause(plPause);
    break;
  case 0x8D00: //ctrl-up
    wpSetPos(wpGetPos()-waverate);
    break;
  case 0x9100: //ctrl-down
    wpSetPos(wpGetPos()+waverate);
    break;
  case 0x7300: //ctrl-left
    wpSetPos(wpGetPos()-(wavelen>>5));
    break;
  case 0x7400: //ctrl-right
    wpSetPos(wpGetPos()+(wavelen>>5));
    break;
  case 0x7700: //ctrl-home
    wpSetPos(0);
    break;
  case '-':
    if (vol>=2)
      vol-=2;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case '+':
    if (vol<=62)
      vol+=2;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case '/':
    if ((bal-=4)<-64)
      bal=-64;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case '*':
    if ((bal+=4)>64)
      bal=64;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case ',':
    if ((pan-=4)<-64)
      pan=-64;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case '.':
    if ((pan+=4)>64)
      pan=64;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x3c00: //f2
    if ((vol-=8)<0)
      vol=0;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x3d00: //f3
    if ((vol+=8)>64)
      vol=64;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x3e00: //f4
    wpSetVolume(vol, bal, pan, srnd=srnd?0:2);
    break;
  case 0x3f00: //f5
    if ((pan-=16)<-64)
      pan=-64;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x4000: //f6
    if ((pan+=16)>64)
      pan=64;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x4100: //f7
    if ((bal-=16)<-64)
      bal=-64;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x4200: //f8
    if ((bal+=16)>64)
      bal=64;
    wpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x4300: //f9
  case 0x8500: //f11
    if ((speed-=finespeed)<16)
      speed=16;
    wpSetSpeed(speed);
    break;
  case 0x4400: //f10
  case 0x8600: //f12
    if ((speed+=finespeed)>2048)
      speed=2048;
    wpSetSpeed(speed);
    break;
  case 0x5f00: // ctrl f2
    if ((amp-=4)<4)
      amp=4;
    wpSetAmplify(1024*amp);
    break;
  case 0x6000: // ctrl f3
    if ((amp+=4)>508)
      amp=508;
    wpSetAmplify(1024*amp);
    break;
  case 0x8900: // ctrl f11
    finespeed=(finespeed==8)?1:8;
    break;
  case 0x6a00:
    normalize();
    break;
  case 0x6900:
    set.pan=pan;
    set.bal=bal;
    set.vol=vol;
    set.speed=speed;
    set.amp=amp;
    set.srnd=srnd;
    break;
  case 0x6b00:
    pan=64;
    bal=0;
    vol=64;
    speed=256;
    amp=64;
    wpSetVolume(vol, bal, pan, srnd);
    wpSetSpeed(speed);
    wpSetAmplify(1024*amp);
    break;
  default:
    if (plrProcessKey)
    {
      int ret=plrProcessKey(key);
      if (ret==2)
        cpiResetScreen();
      if (ret)
        return 1;
    }
    return 0;
  }
  return 1;
}


static int wavLooped()
{
  wpSetLoop(plLoopMods);
  wpIdle();
  if (plrIdle)
    plrIdle();
  return !plLoopMods&&wpLooped();
}


static void wavCloseFile()
{
  wpClosePlayer();
  wavefile->close();
  delete membuf;
}

static int wavOpenFile(const char *path, moduleinfostruct &info, binfile *wavf)
{
  if (!wavf)
    return -1;

  _splitpath(path, 0, 0, currentmodname, currentmodext);
  modname=info.modname;
  composer=info.composer;

  cputs("loading ");
  cputs(currentmodname);
  cputs(currentmodext);
  cputs("...\r\n");

  wavefile=wavf;
  unsigned char sig[4];
  wavefile->read(sig, 4);
  wavefile->seek(0);
  int fl;
  if (!memcmp(sig, "RIFF", 4))
  {
    wavefile->seek(12);
    fl=0;
    while (1)
    {
      if (wavefile->read(sig, 4)!=4)
        break;
      fl=wavefile->getl();
      if (!memcmp(sig, "data", 4))
        break;
      wavefile->seekcur(fl);
    }
  }
  else
  {
    fl=wavefile->length();

    wavefile->seekend(-128);
    char tag[3];
    wavefile->read(tag, 3);
    if (!memcmp(tag, "TAG", 3))
      fl-=128;
    wavefile->seek(0);
  }

  membuf=new char [fl];
  if (membuf)
  {
    fl=wavefile->read(membuf, fl);
    wavefile->close();
    memfile.open(membuf, fl, memfile.openfree);
    wavefile=&memfile;
  }
  else
    return -1;

  plIsEnd=wavLooped;
  plProcessKey=wavProcessKey;
  plDrawGStrings=wavDrawGStrings;
  plGetMasterSample=plrGetMasterSample;
  plGetRealMasterVolume=plrGetRealMasterVolume;

  if (!wpOpenPlayer(*wavefile, cfGetProfileBool2(cfSoundSec, "sound", "wavetostereo", 1, 1), cfGetProfileInt2(cfSoundSec, "sound", "waveratetolerance", 50, 10)*65, cfGetProfileInt2(cfSoundSec, "sound", "ampegmaxrate", 48000, 10), cfGetProfileBool2(cfSoundSec, "sound", "ampegtomono", 0, 0)))
    return -1;

  starttime=clock();
  normalize();

  waveinfo inf;
  wpGetInfo(inf);
  wavelen=inf.len;
  waverate=inf.rate;

  return 0;
}

extern "C"
{
  cpifaceplayerstruct ampegpPlayer = {wavOpenFile, wavCloseFile};
};
