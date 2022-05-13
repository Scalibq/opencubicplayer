// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// SIDPlay - SID file player based on Michael Schwendt's SIDPlay routines
//
// revision history: (please note changes here)
//  -kb980717  Tammo Hinrichs <opencp@gmx.net>
//    -first release

#include <stdlib.h>
#include <stdio.h>
#include "binfarc.h"
#include "poll.h"
#include "player.h"
#include "sid.h"
#include "deviplay.h"

#include "emucfg.h"
#include "sidtune.h"
#include "psetting.h"
#include "opstruct.h"

emuEngine *myEmuEngine;
emuConfig *myEmuConfig;

sidTune *mySidTune;
sidTuneInfo *mySidTuneInfo;

extern sidOperator optr1, optr2, optr3;

static unsigned char stereo;
static unsigned char bit16;
static unsigned char signedout;
static unsigned long samprate;
static unsigned char reversestereo;
static unsigned char srnd;

static unsigned short *buf16;
static unsigned long bufpos;
static int buflen;
static void *plrbuf;


static unsigned short *cliptabl;
static unsigned short *cliptabr;
static unsigned long amplify;
static unsigned long voll,volr;

static char active;
static char pause;

static binfile *sidfile;

char sidpmute[4];
short v4outl, v4outr;

extern "C" void mixClipAlt(unsigned short *dst, const unsigned short *src, unsigned long len, const unsigned short *tab);
#pragma aux mixClipAlt parm [edi] [esi] [ecx] [ebx] modify [eax edx]
extern "C" void mixClipAlt2(unsigned short *dst, const unsigned short *src, unsigned long len, const unsigned short *tab);
#pragma aux mixClipAlt2 parm [edi] [esi] [ecx] [ebx] modify [eax edx]


static void mixCalcClipTab(unsigned short *ct, signed long amp)
{
  signed long i,j,a,b;

  amp=(3*amp+1)>>1;

  a=-amp;
  for (i=0; i<256; i++)
    ct[i+768]=(a+=amp)>>16;

  for (i=0; i<256; i++)
    ct[i+1024]=0;

  b=0x800000-(amp<<7);
  for (i=0; i<256; i++)
  {
    if (b<0x000000)
      if ((b+amp)<0x000000)
      {
        ((unsigned short **)ct)[i]=ct+1024;
        ct[i+512]=0x0000;
      }
      else
      {
        a=0;
        for (j=0; j<256; j++)
        {
          ct[j+1280]=(((a>>8)+b)<0x000000)?0x0000:(((a>>8)+b)>>8);
          a+=amp;
        }
        ((unsigned short **)ct)[i]=ct+1280;
        ct[i+512]=0x0000;
      }
    else
    if ((b+amp)>0xFFFFFF)
      if (b>0xFFFFFF)
      {
        ((unsigned short **)ct)[i]=ct+1024;
        ct[i+512]=0xFFFF;
      }
      else
      {
        a=0;
        for (j=0; j<256; j++)
        {
          ct[j+1536]=(((a>>8)+b)>0xFFFFFF)?0x0000:((((a>>8)+b)>>8)+1);
          a+=amp;
        }
        ((unsigned short **)ct)[i]=ct+1536;
        ct[i+512]=0xFFFF;
      }
    else
    {
      ((unsigned short **)ct)[i]=ct+768;
      ct[i+512]=b>>8;
    }
    b+=amp;
  }
}



static int clipbusy=0;

static void calccliptab(signed long ampl, signed long ampr)
{
  clipbusy++;

  if (!stereo)
  {
    ampl=(abs(ampl)+abs(ampr))>>1;
    ampr=0;
  }

  mixCalcClipTab(cliptabl, abs(ampl));
  mixCalcClipTab(cliptabr, abs(ampr));

  int i;
  if (signedout)
    for (i=0; i<256; i++)
    {
      cliptabl[i+512]^=0x8000;
      cliptabr[i+512]^=0x8000;
    }

  clipbusy--;
}




static void timerproc()
{
  if (clipbusy)
    return;
  clipbusy++;

  unsigned long bufplayed=plrGetBufPos()>>(stereo+bit16);
  unsigned long bufdelta;
  unsigned long pass2;

  if (bufplayed==bufpos)
  {
    clipbusy--;
    return;
  }
  int quietlen=0;
  bufdelta=(buflen+bufplayed-bufpos)%buflen;

  if (pause)
    quietlen=bufdelta;

  bufdelta-=quietlen;

  if (bufdelta)
  {
    if ((bufpos+bufdelta)>buflen)
      pass2=bufpos+bufdelta-buflen;
    else
      pass2=0;

    plrClearBuf(buf16, bufdelta*2, 1);

    //sidplay nach buf16

    sidEmuFillBuffer(*myEmuEngine,*mySidTune,buf16,bufdelta<<(stereo+1));

    if (stereo && srnd)
      for (int i=0; i<bufdelta; i++)
        buf16[2*i]^=0xFFFF;

    if (bit16)
    {
      if (stereo)
      {
        mixClipAlt2((unsigned short*)plrbuf+bufpos*2, buf16, bufdelta-pass2, cliptabl);
        mixClipAlt2((unsigned short*)plrbuf+bufpos*2+1, buf16+1, bufdelta-pass2, cliptabr);
        if (pass2)
        {
          mixClipAlt2((unsigned short*)plrbuf, buf16+2*(bufdelta-pass2), pass2, cliptabl);
          mixClipAlt2((unsigned short*)plrbuf+1, buf16+2*(bufdelta-pass2)+1, pass2, cliptabr);
        }
      }
      else
      {
        mixClipAlt((unsigned short*)plrbuf+bufpos, buf16, bufdelta-pass2, cliptabl);
        if (pass2)
          mixClipAlt((unsigned short*)plrbuf, buf16+bufdelta-pass2, pass2, cliptabl);
      }
    }
    else
    {
      if (stereo)
      {
        mixClipAlt2(buf16, buf16, bufdelta, cliptabl);
        mixClipAlt2(buf16+1, buf16+1, bufdelta, cliptabr);
      }
      else
        mixClipAlt(buf16, buf16, bufdelta, cliptabl);
      plr16to8((unsigned char*)plrbuf+(bufpos<<stereo), buf16, (bufdelta-pass2)<<stereo);
      if (pass2)
        plr16to8((unsigned char*)plrbuf, buf16+((bufdelta-pass2)<<stereo), pass2<<stereo);
    }
    bufpos+=bufdelta;
    if (bufpos>=buflen)
      bufpos-=buflen;
  }

  bufdelta=quietlen;
  if (bufdelta)
  {
    if ((bufpos+bufdelta)>buflen)
      pass2=bufpos+bufdelta-buflen;
    else
      pass2=0;
    if (bit16)
    {
      plrClearBuf((unsigned short*)plrbuf+(bufpos<<stereo), (bufdelta-pass2)<<stereo, !signedout);
      if (pass2)
        plrClearBuf((unsigned short*)plrbuf, pass2<<stereo, !signedout);
    }
    else
    {
      plrClearBuf(buf16, bufdelta<<stereo, !signedout);
      plr16to8((unsigned char*)plrbuf+(bufpos<<stereo), buf16, (bufdelta-pass2)<<stereo);
      if (pass2)
        plr16to8((unsigned char*)plrbuf, buf16+((bufdelta-pass2)<<stereo), pass2<<stereo);
    }
    bufpos+=bufdelta;
    if (bufpos>=buflen)
      bufpos-=buflen;
  }

  plrAdvanceTo(bufpos<<(stereo+bit16));
  clipbusy--;

}


static void updateconf()
{
  clipbusy++;
    myEmuEngine->setConfig(*myEmuConfig);
  clipbusy--;
}


void sidpIdle()
{
  timerproc();
}

unsigned char sidpOpenPlayer(binfile &f)
{
  if (!plrPlay)
    return 0;

  myEmuEngine = new emuEngine;
  if (!myEmuEngine)
  {
    printf("SID Emulator Engine is out of memory.\n");
    return 0;
  }

  myEmuConfig = new emuConfig;
  if (!myEmuConfig)
  {
    delete myEmuEngine;
    return 0;
  }

  cliptabl=new unsigned short[1793];
  cliptabr=new unsigned short[1793];

  if (!cliptabl||!cliptabr)
  {
    delete cliptabl;
    delete cliptabr;
    delete myEmuEngine;
    delete myEmuConfig;
    return 0;
  }

  sidfile=&f;
  mySidTune = new sidTune;
  mySidTuneInfo = new sidTuneInfo;
  if (!mySidTune || !mySidTuneInfo)
  {
    delete mySidTune;
    delete mySidTuneInfo;
    delete cliptabl;
    delete cliptabr;
    delete myEmuEngine;
    delete myEmuConfig;
    return 0;
  }


  if (!mySidTune->open(f))
  {
    mySidTune->returnInfo(*mySidTuneInfo);
    printf("%s\n",mySidTuneInfo->statusString);
    return 0;
  }

  int playrate=cfGetProfileInt("commandline_s", "r", cfGetProfileInt2(cfSoundSec, "sound", "mixrate", 44100, 10), 10);
  if (playrate<66)
    if (playrate%11)
      playrate*=1000;
    else
      playrate=playrate*11025/11;

  plrSetOptions(playrate, PLR_STEREO|PLR_16BIT);
  if (!plrOpenPlayer(plrbuf, buflen, plrBufSize))
    return 0;

  stereo=!!(plrOpt&PLR_STEREO);
  bit16=!!(plrOpt&PLR_16BIT);
  signedout=!!(plrOpt&PLR_SIGNEDOUT);
  reversestereo=!!(plrOpt&PLR_REVERSESTEREO);
  samprate=plrRate;
  srnd=0;

  myEmuEngine->getConfig(*myEmuConfig);

  myEmuConfig->frequency=samprate;
  myEmuConfig->bitsPerSample=SIDEMU_16BIT;
  myEmuConfig->sampleFormat=SIDEMU_UNSIGNED_PCM;
  myEmuConfig->channels=stereo?SIDEMU_STEREO:SIDEMU_MONO;
  myEmuConfig->sidChips=1;

  myEmuConfig->volumeControl=SIDEMU_FULLPANNING;
  myEmuConfig->autoPanning=SIDEMU_CENTEREDAUTOPANNING;

  myEmuConfig->mos8580=0;
  myEmuConfig->measuredVolume=0;
  myEmuConfig->emulateFilter=1;
  myEmuConfig->filterFs=SIDEMU_DEFAULTFILTERFS;
  myEmuConfig->filterFm=SIDEMU_DEFAULTFILTERFM;
  myEmuConfig->filterFt=SIDEMU_DEFAULTFILTERFT;
  myEmuConfig->memoryMode=MPU_BANK_SWITCHING;
  myEmuConfig->clockSpeed=SIDTUNE_CLOCK_PAL;
  myEmuConfig->forceSongSpeed=0;
  myEmuConfig->digiPlayerScans=10;

  myEmuEngine->setConfig(*myEmuConfig);

  memset(sidpmute,0,4);

  pause=0;
  amplify=65536;
  voll=256;
  volr=256;
  calccliptab((amplify*voll)>>8, (amplify*volr)>>8);

  buf16=new unsigned short [buflen*2];

  if (!buf16)
  {
    plrClosePlayer();
    delete buf16;
    delete mySidTune;
    delete mySidTuneInfo;
    delete cliptabl;
    delete cliptabr;
    delete myEmuEngine;
    delete myEmuConfig;
    return 0;
  }

  bufpos=0;

  mySidTune->returnInfo(*mySidTuneInfo);
  sidEmuInitializeSong(*myEmuEngine,*mySidTune,mySidTuneInfo->startSong);
  extern char sidEmuFastForwardReplay(int percent);
  sidEmuFastForwardReplay(100);
  mySidTune->returnInfo(*mySidTuneInfo);

#ifdef DOS32
  if (!pollInit(timerproc))
  {
    plrClosePlayer();
    return 0;
  }
#endif

  active=1;

  return 1;
}


void sidpClosePlayer()
{
  active=0;

#ifdef DOS32
  pollClose();
#endif

  plrClosePlayer();

  delete myEmuEngine;
  delete myEmuConfig;
  delete mySidTune;
  delete mySidTuneInfo;

  delete buf16;
  delete cliptabl;
  delete cliptabr;
}


void sidpPause(unsigned char p)
{
  pause=p;
}

void sidpSetAmplify(unsigned long amp)
{
  amplify=amp;
  calccliptab((amplify*voll)>>8, (amplify*volr)>>8);
}

void sidpSetVolume(unsigned char vol, signed char bal, signed char pan, unsigned char opt)
{
  pan=pan;
  voll=vol*4;
  volr=vol*4;
  if (bal<0)
    volr=(volr*(64+bal))>>6;
  else
    voll=(voll*(64-bal))>>6;
  sidpSetAmplify(amplify);
  srnd=opt;
}

void sidpGetGlobInfo(sidTuneInfo &si)
{
  mySidTune->returnInfo(si);
}

void sidpStartSong(char sng)
{
  if (sng<1)
    sng=1;
  if (sng>mySidTuneInfo->songs)
    sng=mySidTuneInfo->songs;
  while (clipbusy);
  clipbusy++;
    sidEmuInitializeSong(*myEmuEngine,*mySidTune,sng);
    mySidTune->returnInfo(*mySidTuneInfo);
  clipbusy--;
}

void sidpToggleVideo()
{
  int &cs=myEmuConfig->clockSpeed;
  cs=(cs==SIDTUNE_CLOCK_PAL)?SIDTUNE_CLOCK_NTSC:SIDTUNE_CLOCK_PAL;
  updateconf();
}

char sidpGetVideo()
{
  int &cs=myEmuConfig->clockSpeed;
  return (cs==SIDTUNE_CLOCK_PAL);
}

char sidpGetFilter()
{
  return myEmuConfig->emulateFilter;
}


void sidpToggleFilter()
{
  myEmuConfig->emulateFilter^=1;
  updateconf();
}


char sidpGetSIDVersion()
{
  return myEmuConfig->mos8580;
}


void sidpToggleSIDVersion()
{
  myEmuConfig->mos8580^=1;
  updateconf();
}

void sidpMute(int i, int m)
{
  sidpmute[i]=m;
}


extern ubyte filterType;


void sidpGetChanInfo(int i, sidChanInfo &ci)
{
  sidOperator ch;
  switch (i)
  {
    case 0: ch=optr1; break;
    case 1: ch=optr2; break;
    case 2: ch=optr3; break;
  };
  ci.freq=ch.SIDfreq;
  ci.ad=ch.SIDAD;
  ci.sr=ch.SIDSR;
  ci.pulse=ch.SIDpulseWidth&0xfff;
  ci.wave=ch.SIDctrl;
  ci.filtenabled=ch.filtEnabled;
  ci.filttype=filterType;
  ci.leftvol =ch.enveVol*ch.gainLeft>>16;
  ci.rightvol=ch.enveVol*ch.gainRight>>16;
  long pulsemul;
  switch (ch.SIDctrl & 0xf0)
  {
    case 0x10:
      ci.leftvol*=192;
      ci.rightvol*=192;
      break;
    case 0x20:
      ci.leftvol*=224;
      ci.rightvol*=224;
      break;
    case 0x30:
      ci.leftvol*=208;
      ci.rightvol*=208;
      break;
    case 0x40:
      pulsemul=2*(ci.pulse>>4);
      if (ci.pulse & 0x800)
        pulsemul=511-pulsemul;
      ci.leftvol*=pulsemul;
      ci.rightvol*=pulsemul;
      break;
    case 0x50:
      pulsemul=255-(ci.pulse>>4);
      ci.leftvol*=pulsemul;
      ci.rightvol*=pulsemul;
      break;
    case 0x60:
      pulsemul=255-(ci.pulse>>4);
      ci.leftvol*=pulsemul;
      ci.rightvol*=pulsemul;
      break;
    case 0x70:
      ci.leftvol*=224;
      ci.rightvol*=224;
      break;
    case 0x80:
      ci.leftvol*=240;
      ci.rightvol*=240;
      break;
    default:
      ci.leftvol=ci.rightvol=0;
  }
  ci.leftvol=ci.leftvol>>8;
  ci.rightvol=ci.rightvol>>8;
}


void sidpGetDigiInfo(sidDigiInfo &di)
{
  short vv=abs(v4outl)>>7;
  if (vv>di.l)
    di.l=vv;
  else
    if (di.l>4)
      di.l-=4;
    else
      di.l=0;
  short v=abs(v4outr)>>7;
  if (vv>di.r)
    di.r=vv;
  else
    if (di.r>4)
      di.r-=4;
    else
      di.r=0;
}