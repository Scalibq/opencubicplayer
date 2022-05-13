// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// SIDPlay interface routines
//
// revision history: (please note changes here)
//  -kb980717  Tammo Hinrichs <opencp@gmx.net>
//    -first release
//  -ryg981219 Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -made max amplification 793% (as in module players)

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <math.h>
#include "pfilesel.h"
#include "poutput.h"
#include "player.h"
#include "psetting.h"
#include "binfile.h"
#include "sid.h"
#include "sets.h"
#include "deviplay.h"
#include "cpiface.h"
#include "sidtune.h"
#include "timer.h"
#include "emucfg.h"

extern char plPause;

extern int plLoopMods;

static binfile *sidfile;

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

static sidTuneInfo globinfo;
static sidChanInfo ci;
static sidDigiInfo di;

static void sidpDrawGStrings(short (*buf)[132])
{
  long tim;
  if (plPause)
    tim=(pausetime-starttime)/CLK_TCK;
  else
    tim=(clock()-starttime)/CLK_TCK;


  if (plScrWidth==80)
  {
    writestring(buf[0], 0, 0x09, " vol: úúúúúúúú ", 15);
    writestring(buf[0], 15, 0x09, " srnd: ú  pan: lúúúmúúúr  bal: lúúúmúúúr ", 41);
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

    writestring(buf[0], 57, 0x09, "amp: ...% filter: ...  ", 23);
    writenum(buf[0], 62, 0x0F, amp*100/64, 10, 3);
    writestring(buf[0], 75, 0x0F, sidpGetFilter()?"on":"off", 3);


    writestring(buf[1],  0, 0x09," song .. of ..    SID: MOS....    speed: ....    cpu: ...%",80);
    writenum(buf[1],  6, 0x0F, globinfo.currentSong, 16, 2, 0);
    writenum(buf[1], 12, 0x0F, globinfo.songs, 16, 2, 0);
    writestring(buf[1], 23, 0x0F, "MOS", 3);
    writestring(buf[1], 26, 0x0F, sidpGetSIDVersion()?"8580":"6581", 4);
    writestring(buf[1], 41, 0x0F, sidpGetVideo()?"PAL":"NTSC", 4);

#ifdef DOS32
    writenum(buf[1], 54, 0x0F, tmGetCpuUsage(), 10, 3);
#else
    writenum(buf[1], 54, 0x0F, 0, 10, 3);
#endif
    writestring(buf[1], 57, 0x0F, "%", 1);


    writestring(buf[2],  0, 0x09, " file úúúúúúúú.úúú: ...............................                time: ..:.. ", 80);
    writestring(buf[2],  6, 0x0F, currentmodname, 8);
    writestring(buf[2], 14, 0x0F, currentmodext, 4);
    writestring(buf[2], 20, 0x0F, modname, 31);
    if (plPause)
      writestring(buf[2], 58, 0x0C, "paused", 6);
    writenum(buf[2], 73, 0x0F, (tim/60)%60, 10, 2, 1);
    writestring(buf[2], 75, 0x0F, ":", 1);
    writenum(buf[2], 76, 0x0F, tim%60, 10, 2, 0);


  }
  else
  {
    writestring(buf[0], 0, 0x09, "    volume: úúúúúúúúúúúúúúúú  ", 30);
    writestring(buf[0], 30, 0x09, " surround: ú   panning: lúúúúúúúmúúúúúúúr   balance: lúúúúúúúmúúúúúúúr  ", 72);
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

    writestring(buf[0], 105, 0x09, "amp: ...%   filter: ...  ", 23);
    writenum(buf[0], 110, 0x0F, amp*100/64, 10, 3);
    writestring(buf[0], 125, 0x0F, sidpGetFilter()?"on":"off", 3);


    writestring(buf[1],  0, 0x09,"    song .. of ..    SID: MOS....    speed: ....    cpu: ...%",132);
    writenum(buf[1],  9, 0x0F, globinfo.currentSong, 16, 2, 0);
    writenum(buf[1], 15, 0x0F, globinfo.songs, 16, 2, 0);
    writestring(buf[1], 26, 0x0F, "MOS", 3);
    writestring(buf[1], 29, 0x0F, sidpGetSIDVersion()?"8580":"6581", 4);
    writestring(buf[1], 44, 0x0F, sidpGetVideo()?"PAL":"NTSC", 4);


#ifdef DOS32
    writenum(buf[1], 57, 0x0F, tmGetCpuUsage(), 10, 3);
#else
    writenum(buf[1], 57, 0x0F, 0, 10, 3);
#endif
    writestring(buf[1], 60, 0x0F, "%", 1);

    writestring(buf[2],  0, 0x09, "    file úúúúúúúú.úúú: ...............................  composer: ...............................                    time: ..:..   ", 132);
    writestring(buf[2],  9, 0x0F, currentmodname, 8);
    writestring(buf[2], 17, 0x0F, currentmodext, 4);
    writestring(buf[2], 23, 0x0F, modname, 31);
    writestring(buf[2], 66, 0x0F, composer, 31);
    if (plPause)
      writestring(buf[2], 100, 0x0C, "playback paused", 15);
    writenum(buf[2], 123, 0x0F, (tim/60)%60, 10, 2, 1);
    writestring(buf[2], 125, 0x0F, ":", 1);
    writenum(buf[2], 126, 0x0F, tim%60, 10, 2, 0);
  }
}


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


static char convnote(long freq)
{

  if (freq<256) return 0xff;

  float frfac=(float)freq/(float)0x1167;

  float nte=12*(log(frfac)/log(2))+48;

//  cputs(ltoa(nte,0,10));
//  cputs("\n");

  if (nte<0 || nte>127) nte=0xff;
  return (char)nte;
}



static void drawvolbar(short *buf, int, unsigned char st)
{
  int l,r;
  l=ci.leftvol;
  r=ci.rightvol;
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

static void drawlongvolbar(short *buf, int, unsigned char st)
{
  int l,r;
  l=ci.leftvol;
  r=ci.rightvol;
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


static char *waves4[]={"    ","tri ","saw ","trsw","puls","trpu","swpu","tsp ",
                       "nois","????","????","????","????","????","????","????"};

static char *waves16[]={"                ","triangle        ","sawtooth        ",
                       "tri + saw       ","pulse           ","triangle + pulse",
                       "sawtooth + pulse","tri + saw + puls","noise           ",
                       "invalid         ","invalid         ","invalid         ",
                       "invalid         ","invalid         ","invalid         ",
                       "invalid         "};

static char *filters3[]={"   ","low","bnd","b+l","hgh","h+l","h+b","hbl"};
static char *filters12[]={"","low pass","band pass","low + band","high pass",
                         "band notch","high + band","all pass"};

static char *fx2[]={"  ","sy","ri","rs"};
static char *fx7[]={"","sync","ringmod","snc+rng"};
static char *fx11[]={"","sync","ringmod","sync + ring"};

static void drawchannel(short *buf, int len, int i)
{
  unsigned char st=plMuteCh[i];

  unsigned char tcol=st?0x08:0x0F;
  unsigned char tcold=st?0x08:0x07;
  unsigned char tcolr=st?0x08:0x0B;

  if (i<3)
  {
    switch (len)
    {
    case 36:
      writestring(buf, 0, tcold, " ---- --- -- - -- úúúúúúúú úúúúúúúú ", 36);
      break;
    case 62:
      writestring(buf, 0, tcold, " ---------------- ---- --- --- --- -------  úúúúúúúú úúúúúúúú ", 62);
      break;
    case 128:
      writestring(buf, 0, tcold, "                   ³        ³       ³       ³                ³               ³   úúúúúúúúúúúúúúúú úúúúúúúúúúúúúúúú", 128);
      break;                                                                      
    case 76:
      writestring(buf, 0, tcold, "                  ³      ³     ³     ³     ³             ³ úúúúúúúú úúúúúúúú", 76);
      break;
    case 44:
      writestring(buf, 0, tcold, " ---- ---- --- -- --- --  úúúúúúúú úúúúúúúú ", 44);
      break;
    }
 
    sidpGetChanInfo(i,ci);
    if (!ci.leftvol && !ci.rightvol)
      return;
    char nte=convnote(ci.freq);
    char nchar[4];

    if (nte<0xFF)
    {
      nchar[0]="CCDDEFFGGAAB"[nte%12];
      nchar[1]="-#-#--#-#-#-"[nte%12];
      nchar[2]="0123456789ABCDEFGHIJKLMN"[nte/12];
      nchar[3]=0;
    }
    else
      strcpy(nchar,"   ");

    char ftype=(ci.filttype>>4)&7;
    char efx=(ci.wave>>1)&3;

    switch(len)
    {
      case 36:
        writestring(buf+1, 0, tcol, waves4[ci.wave>>4], 4);
        writestring(buf+6, 0, tcol, nchar, 3);
        writenum(buf+10, 0, tcol, ci.pulse>>4, 16, 2, 0);
        if (ci.filtenabled && ftype)
          writenum(buf+13, 0, tcol, ftype, 16, 1, 0);
        if (efx)
          writestring(buf+15, 0, tcol, fx2[efx], 2);
        drawvolbar(buf+18, i, st);
        break;
      case 44:
        writestring(buf+1, 0, tcol, waves4[ci.wave>>4], 4);
        writenum(buf+6, 0, tcol, ci.ad, 16, 2, 0);
        writenum(buf+8, 0, tcol, ci.sr, 16, 2, 0);
        writestring(buf+11, 0, tcol, nchar, 3);
        writenum(buf+15, 0, tcol, ci.pulse>>4, 16, 2, 0);
        if (ci.filtenabled && ftype)
          writestring(buf+18, 0, tcol, filters3[ftype], 3);
        if (efx)
          writestring(buf+22, 0, tcol, fx2[efx], 2);
        drawvolbar(buf+26, i, st);
        break;
      case 62:
        writestring(buf+1, 0, tcol, waves16[ci.wave>>4], 16);
        writenum(buf+18, 0, tcol, ci.ad, 16, 2, 0);
        writenum(buf+20, 0, tcol, ci.sr, 16, 2, 0);
        writestring(buf+23, 0, tcol, nchar, 3);
        writenum(buf+27, 0, tcol, ci.pulse, 16, 3, 0);
        if (ci.filtenabled && ftype)
          writestring(buf+31, 0, tcol, filters3[ftype], 3);
        if (efx)
          writestring(buf+35, 0, tcol, fx7[efx], 7);
        drawvolbar(buf+44, i, st);
        break;
      case 76:
        writestring(buf+1, 0, tcol, waves16[ci.wave>>4], 16);
        writenum(buf+20, 0, tcol, ci.ad, 16, 2, 0);
        writenum(buf+22, 0, tcol, ci.sr, 16, 2, 0);
        writestring(buf+27, 0, tcol, nchar, 3);
        writenum(buf+33, 0, tcol, ci.pulse, 16, 3, 0);
        if (ci.filtenabled && ftype)
          writestring(buf+39, 0, tcol, filters3[ftype], 3);
        writestring(buf+45, 0, tcol, fx11[efx], 11);
        drawvolbar(buf+59, i, st);
        break;
      case 128:
        writestring(buf+1, 0, tcol, waves16[ci.wave>>4], 16);
        writenum(buf+22, 0, tcol, ci.ad, 16, 2, 0);
        writenum(buf+24, 0, tcol, ci.sr, 16, 2, 0);
        writestring(buf+31, 0, tcol, nchar, 3);
        writenum(buf+39, 0, tcol, ci.pulse, 16, 3, 0);
        if (ci.filtenabled && ftype)
          writestring(buf+47, 0, tcol, filters12[ftype], 12);
        writestring(buf+64, 0, tcol, fx11[efx], 11);
        drawlongvolbar(buf+80, i, st);
        break;
    }
  }
  else
  {
    switch (len)
    {
    case 36:
      writestring(buf, 0, tcold, " samples/galway   úúúúúúúú úúúúúúúú ", 36);
      break;
    case 62:
      writestring(buf, 0, tcold, " pcm samples / galway noises                úúúúúúúú úúúúúúúú ", 62);
      break;
    case 128:
      writestring(buf, 0, tcold, " pcm samples or galway noises                                                ³   úúúúúúúúúúúúúúúú úúúúúúúúúúúúúúúú", 128);
      break;                                                                      
    case 76:
      writestring(buf, 0, tcold, " pcm samples or galway noises                            ³ úúúúúúúú úúúúúúúú", 76);
      break;
    case 44:
      writestring(buf, 0, tcold, " samples or galway        úúúúúúúú úúúúúúúú ", 44);
      break;
    }

    sidpGetDigiInfo(di);

    ci.leftvol=di.l;
    ci.rightvol=di.r;
    switch(len)
    {
      case 36:
        drawvolbar(buf+18, i, st);
        break;
      case 44:
        drawvolbar(buf+26, i, st);
        break;
      case 62:
        drawvolbar(buf+44, i, st);
        break;
      case 76:
        drawvolbar(buf+59, i, st);
        break;
      case 128:
        drawlongvolbar(buf+80, i, st);
        break;
    }

  }


}


static void normalize()
{
  pan=set.pan;
  bal=set.bal;
  vol=set.vol;
  amp=set.amp;
  srnd=set.srnd;
  sidpSetAmplify(1024*amp);
  sidpSetVolume(vol, bal, pan, srnd);
}

static void sidpCloseFile()
{
  sidpClosePlayer();
  sidfile->close();
}

static int sidpProcessKey(unsigned short key)
{
  char csg;
  switch (key)
  {
  case 'p': case 'P': case 0x10:
    if (plPause)
      starttime=starttime+clock()-pausetime;
    else
      pausetime=clock();
    plPause=!plPause;
    sidpPause(plPause);
    break;
  case 0x8D00: //ctrl-up
    break;
  case 0x9100: //ctrl-down
    break;
  case 0x7300: //ctrl-left
    csg=globinfo.currentSong-1;
    if (csg)
    {
      sidpStartSong(csg);
      starttime=clock();
    }
    sidpGetGlobInfo(globinfo);
    break;
  case 0x7400: //ctrl-right
    csg=globinfo.currentSong+1;
    if (csg<=globinfo.songs)
    {
      sidpStartSong(csg);
      starttime=clock();
    }
    sidpGetGlobInfo(globinfo);
    break;
  case 0x7700: //ctrl-home
    sidpStartSong(globinfo.currentSong);
    starttime=clock();
    sidpGetGlobInfo(globinfo);
    break;
  case 8: //backspace
    sidpToggleFilter();
    break;
  case '-':
    if (vol>=2)
      vol-=2;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case '+':
    if (vol<=62)
      vol+=2;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case '/':
    if ((bal-=4)<-64)
      bal=-64;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case '*':
    if ((bal+=4)>64)
      bal=64;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case ',':
    if ((pan-=4)<-64)
      pan=-64;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case '.':
    if ((pan+=4)>64)
      pan=64;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x3c00: //f2
    if ((vol-=8)<0)
      vol=0;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x3d00: //f3
    if ((vol+=8)>64)
      vol=64;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x3e00: //f4
    sidpSetVolume(vol, bal, pan, srnd=srnd?0:2);
    break;
  case 0x3f00: //f5
    if ((pan-=16)<-64)
      pan=-64;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x4000: //f6
    if ((pan+=16)>64)
      pan=64;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x4100: //f7
    if ((bal-=16)<-64)
      bal=-64;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x4200: //f8
    if ((bal+=16)>64)
      bal=64;
    sidpSetVolume(vol, bal, pan, srnd);
    break;
  case 0x4300: //f9
    break;
  case 0x4400: //f10
    break;
  case 0x8500: //f11
    sidpToggleSIDVersion();
    sidpGetGlobInfo(globinfo);
    break;
  case 0x8600: //f12
    sidpToggleVideo();
    sidpGetGlobInfo(globinfo);
    break;
  case 0x5f00: // ctrl f2
    if ((amp-=4)<4)
      amp=4;
    sidpSetAmplify(1024*amp);
    break;
  case 0x6000: // ctrl f3
    if ((amp+=4)>508)
      amp=508;
    sidpSetAmplify(1024*amp);
    break;
  case 0x8900: // ctrl f11
    break;
  case 0x6a00:
    normalize();
    break;
  case 0x6900:
    set.pan=pan;
    set.bal=bal;
    set.vol=vol;
    set.amp=amp;
    set.srnd=srnd;
    break;
  case 0x6b00:
    pan=64;
    bal=0;
    vol=64;
    amp=64;
    sidpSetVolume(vol, bal, pan, srnd);
    sidpSetAmplify(1024*amp);
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

static int sidLooped()
{
  sidpIdle();
  if (plrIdle)
    plrIdle();
  return 0;
}

static int sidpOpenFile(const char *path, moduleinfostruct &info, binfile *sidf)
{
  if (!sidf)
    return -1;

  _splitpath(path, 0, 0, currentmodname, currentmodext);
  modname=info.modname;
  composer=info.composer;

  cputs("loading ");
  cputs(currentmodname);
  cputs(currentmodext);
  cputs("...\r\n");

  sidfile=sidf;


  if (!sidpOpenPlayer(*sidfile))
    return -1;

  plNLChan=4;
  plNPChan=4;
  plUseChannels(drawchannel);
  plSetMute=sidpMute;

  plIsEnd=sidLooped;
  plProcessKey=sidpProcessKey;
  plDrawGStrings=sidpDrawGStrings;
  plGetMasterSample=plrGetMasterSample;
  plGetRealMasterVolume=plrGetRealMasterVolume;
  sidpGetGlobInfo(globinfo);

  starttime=clock();
  normalize();

  return 0;
}

extern "C"
{
  cpifaceplayerstruct sidPlayer = {sidpOpenFile, sidpCloseFile};
};
