// OpenCP Module Player
// copyright (c) '94-'99 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// WMAPlay - wma file player
//
// revision history: (please note changes here)
//  -fd990510   Felix Domke <tmbinc@gmx.net>
//    -first release

                        // does not yet work. reason: unknown.
// #define TIMERDECODE

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"
#include "player.h"
#include "pmain.h"
#include "deviplay.h"
#include "imsrtns.h"
#include "binfile.h"
#include "binfstd.h"
#include "binfmem.h"
#include "asfread.h"
#include "codec.h"
#include "wave.h"
#include "pe.h"
#include "mtw.h"
#include "psetting.h"

static DRIVERPROC DriverProc;
static pe_c *pe=0;
static unsigned char stereo;
static unsigned char bit16;
static unsigned char signedout;
static unsigned long samprate;
static unsigned char reversestereo;

static unsigned short *buf16;
static unsigned long bufpos;
static int buflen;
static void *plrbuf;
static int loaded=0;

static unsigned short *cliptabl;
static unsigned short *cliptabr;
static unsigned long amplify;
static unsigned long voll,volr;
static char convtostereo;

static binfile *wavefile;
static asfreader rawwma;
static acmcodec rawwave;
static char wavestereo;
static unsigned long wavelen;
static unsigned long waverate;
static unsigned long wavepos;
static unsigned char *wavebuf;
static unsigned long wavebuflen;
static unsigned long wavebufpos;
static unsigned long wavebuffpos;
static unsigned long wavebufread;
static unsigned long wavebufrate;
static int maxread;
static char active;
static char looped;
static char donotloop;
static unsigned long bufloopat;

int wmaplaybufperc;

static char pause;

extern "C" void mixClipAlt(unsigned short *dst, const unsigned short *src, unsigned long len, const unsigned short *tab);
#pragma aux mixClipAlt parm [edi] [esi] [ecx] [ebx] modify [eax edx]
extern "C" void mixClipAlt2(unsigned short *dst, const unsigned short *src, unsigned long len, const unsigned short *tab);
#pragma aux mixClipAlt2 parm [edi] [esi] [ecx] [ebx] modify [eax edx]

static void mixCalcClipTab(unsigned short *ct, signed long amp)
{
  signed long i,j,a,b;

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

void wpIdler();

#ifdef TIMERDECODE
static int oldtimer, cycmiss;
#endif

static void timerproc()
{
  if (clipbusy)
    return;
  clipbusy++;

#ifdef TIMERDECODE
  wpIdler();
#endif

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
  if (wavebuflen!=wavelen)
  {
    int towrap=imuldiv((((wavebuflen+wavebufread-wavebufpos-1)%wavebuflen)>>(wavestereo+1)), 65536, wavebufrate);
    if (bufdelta>towrap)
      quietlen=bufdelta-towrap;
  }

  if (pause)
    quietlen=bufdelta;

  int toloop=imuldiv(((bufloopat-wavebufpos)>>(1+wavestereo)), 65536, wavebufrate);
  if (looped)
    toloop=0;

  bufdelta-=quietlen;

  if (bufdelta>=toloop)
  {
    looped=1;
    if (donotloop)
    {
      quietlen+=bufdelta-toloop;
      bufdelta=toloop;
    }
  }

  if (bufdelta)
  {
    if ((bufpos+bufdelta)>buflen)
      pass2=bufpos+bufdelta-buflen;
    else
      pass2=0;
    plrClearBuf(buf16, bufdelta*2, 1);

    int i;
    
    {
      if (wavestereo)
      {
        signed long wpm1, wp1, wp2, c0, c1, c2, c3, ls, rs, vm1,v1,v2;
        for (i=0; i<bufdelta; i++)
        {

          wpm1=wavebufpos-4; if (wpm1<0) wpm1+=wavebuflen;
          wp1=wavebufpos+4; if (wp1>=wavebuflen) wp1-=wavebuflen;
          wp2=wavebufpos+8; if (wp2>=wavebuflen) wp2-=wavebuflen;


          c0 = *(unsigned short*)(wavebuf+wavebufpos)^0x8000;
          vm1= *(unsigned short*)(wavebuf+wpm1)^0x8000;
          v1 = *(unsigned short*)(wavebuf+wp1)^0x8000;
          v2 = *(unsigned short*)(wavebuf+wp2)^0x8000;
          c1 = v1-vm1;
          c2 = 2*vm1-2*c0+v1-v2;
          c3 = c0-vm1-v1+v2;
          c3 =  imulshr16(c3,wavebuffpos);
          c3 += c2;
          c3 =  imulshr16(c3,wavebuffpos);
          c3 += c1;
          c3 =  imulshr16(c3,wavebuffpos);
          ls = c3+c0;

          c0 = *(unsigned short*)(wavebuf+wavebufpos+2)^0x8000;
          vm1= *(unsigned short*)(wavebuf+wpm1+2)^0x8000;
          v1 = *(unsigned short*)(wavebuf+wp1+2)^0x8000;
          v2 = *(unsigned short*)(wavebuf+wp2+2)^0x8000;
          c1 = v1-vm1;
          c2 = 2*vm1-2*c0+v1-v2;
          c3 = c0-vm1-v1+v2;
          c3 =  imulshr16(c3,wavebuffpos);
          c3 += c2;
          c3 =  imulshr16(c3,wavebuffpos);
          c3 += c1;
          c3 =  imulshr16(c3,wavebuffpos);
          rs = c3+c0;

          buf16[2*i]=(unsigned short)ls;
          buf16[2*i+1]=(unsigned short)rs;

          wavebuffpos+=wavebufrate;
          wavebufpos+=(wavebuffpos>>16)*4;
          wavebuffpos&=0xFFFF;
          if (wavebufpos>=wavebuflen)
            wavebufpos-=wavebuflen;
        }
      }
      else
      {
        signed long wpm1, wp1, wp2, c0, c1, c2, c3, vm1,v1,v2;
        for (i=0; i<bufdelta; i++)
        {

          wpm1=wavebufpos-2; if (wpm1<0) wpm1+=wavebuflen;
          wp1=wavebufpos+2; if (wp1>=wavebuflen) wp1-=wavebuflen;
          wp2=wavebufpos+4; if (wp2>=wavebuflen) wp2-=wavebuflen;

          c0 = *(unsigned short*)(wavebuf+wavebufpos)^0x8000;
          vm1= *(unsigned short*)(wavebuf+wpm1)^0x8000;
          v1 = *(unsigned short*)(wavebuf+wp1)^0x8000;
          v2 = *(unsigned short*)(wavebuf+wp2)^0x8000;
          c1 = v1-vm1;
          c2 = 2*vm1-2*c0+v1-v2;
          c3 = c0-vm1-v1+v2;
          c3 =  imulshr16(c3,wavebuffpos);
          c3 += c2;
          c3 =  imulshr16(c3,wavebuffpos);
          c3 += c1;
          c3 =  imulshr16(c3,wavebuffpos);
          c3 += c0;

          buf16[2*i]=buf16[2*i+1]=(unsigned short)c3;

          wavebuffpos+=wavebufrate;
          wavebufpos+=(wavebuffpos>>16)*2;
          wavebuffpos&=0xFFFF;
          if (wavebufpos>=wavebuflen)
            wavebufpos-=wavebuflen;
        }
      }
    }

    if (!stereo)
    {
      for (i=0; i<bufdelta; i++)
        buf16[i]=(buf16[2*i]+buf16[2*i+1])>>1;
    }

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

void wpIdle()
{
  // this is a bloody hack. i know ;)
#ifndef TIMERDECODE
  wpIdler();
#else
#ifdef DOS32
  if (oldtimer==tmGetTicker())
  {
    cycmiss++;
    if (cycmiss>=50)
    {
      timerproc();
    }
  }
  else
  {
    cycmiss=0;
    oldtimer=tmGetTicker();
  }
#else
  timerproc();
#endif
#endif
}


void wpIdler()
{
  unsigned long bufplayed=plrGetBufPos()>>(stereo+bit16);
  unsigned long bufdelta=(buflen+bufplayed-bufpos)%buflen;
#ifndef TIMERDECODE
  if (bufdelta>(buflen>>3))
    timerproc();
#endif

  if ((wavelen==wavebuflen)||!active)
    return;

  unsigned long clean=(wavebufpos+wavebuflen-wavebufread)%wavebuflen;
  wmaplaybufperc=(wavebuflen-clean)*100/wavebuflen;
  int cnt=0;
  if (clean>=16384)      // any value, no "alignment" required.
  {
    while (clean && (cnt<maxread))
    {
      wavepos=rawwave.seek(wavepos);
      int read=clean;
      if ((wavebufread+read)>wavebuflen)
        read=wavebuflen-wavebufread;
      if ((wavepos+read)>=wavelen)
      {
        read=wavelen-wavepos;
        bufloopat=wavebufread+read;
      }
      if (read>0x1000)
        read=0x1000;
      if (!read) break;
//      printf("reading %d bytes\n", read);

      cnt++;
      int r=rawwave.read(wavebuf+wavebufread, read);
      
      wavebufread=(wavebufread+r)%wavebuflen;
      wavepos=(wavepos+r)%wavelen;

      if (r<read)
        wavepos=0;

      clean-=read;
    }
  }
}

extern symbol_c symbol;
mt_c *mt;
#undef Yield
void InitFnc();

int InitPE()
{
  mt=new mtw_c;
  if (!mt)
  {
    printf("Unable to initialize multithreading. sorry.\n");
    return -1;
  }

  InitFnc();
  return 0;
}

static int fileexist(const char *name)
{
  find_t f;
  return !_dos_findfirst(name, _A_NORMAL, &f);
}

static void searchlist(char *dest, const char *name, const char *path)
{
  if (!path)
    path="";
  while (*path)
  {
    if ((*path==' ')||(*path==';')||(*path=='\t'))
    {
      path++;
      continue;
    }
    const char *pe=path;
    while (*pe&&(*pe!=';'))
      pe++;
    memcpy(dest, path, pe-path);
    dest[pe-path]=0;
    path=pe;
    if (dest[strlen(dest)]!='\\')
      strcat(dest, "\\");
    strcat(dest, name);
    if (fileexist(dest))
      return;
  }
  *dest=0;
}

unsigned char wpOpenPlayer(binfile &wav, int tostereo, int tolerance)
{
  if (!plrPlay)
    return 0;

  convtostereo=tostereo;

  cliptabl=new unsigned short[1793];
  cliptabr=new unsigned short[1793];

  if (!cliptabl||!cliptabr)
  {
    delete cliptabl;
    delete cliptabr;
    return 0;
  }

  wavefile=&wav;

  maxread=cfGetProfileInt("wma", "maxread", 3, 10);
  if (maxread<1) maxread=1;

  if (!loaded)
  {
    sbinfile codecimage;
    char filename[_MAX_PATH];
    const char *iniacm=cfGetProfileString("wma", "msaud32", 0);
    if (!iniacm)
      searchlist(filename, "msaud32.acm", getenv("PATH"));
    else
      strcpy(filename, iniacm);

    if ((!*filename) || codecimage.open(filename, sbinfile::openro))
    {
      printf("open of %s failed. make shure that msaud32.acm is in your path\nor specifiy the location in the cp.ini.\n", filename);
      delete cliptabl;
      delete cliptabr;
      return 0;
    }
    if (InitPE())
    {
      codecimage.close();
      delete cliptabl;
      delete cliptabr;
      return 0;
    }
    pe=new pe_c(codecimage);
    codecimage.close();
  
    if (!pe->IsLoaded())
    {
      printf("error while loading the codecimage (msaud32.acm)...\n");
      delete mt;
      delete pe;
      delete cliptabl;
      delete cliptabr;
      return 0;
    }

    DriverProc=(DRIVERPROC)symbol.Get(0, "DriverProc");
    if (!DriverProc)
    {
      printf("symbol \'DriverProc\' not found in codec.\n");
      delete mt;
      delete pe;
      delete cliptabl;
      delete cliptabr;
      return 0;
    }
    loaded=!0; 
  }

  if (rawwma.open(wav))
  {
    delete cliptabl;
    delete cliptabr;
    return 0;
  }

  WAVEFORMATEX wmx, *dstfmt=(WAVEFORMATEX*)rawwma.ioctl(asfreader::ioctlgetformat);
  if (!dstfmt)
  {
    printf("asf couldn't set format. (should not happen)\n");
    rawwma.close();
    delete cliptabl;
    delete cliptabr;
    return 0;
  }
  memcpy(&wmx, dstfmt, sizeof(WAVEFORMATEX));

  wmx.wFormatTag=WAVE_FORMAT_PCM;
  wmx.nBlockAlign=(wmx.wBitsPerSample*wmx.nChannels)/8;
  wmx.nAvgBytesPerSec=wmx.nSamplesPerSec*wmx.nBlockAlign;

  wmx.cbSize=0; 

  wavestereo=wmx.nChannels==2;
  waverate=wmx.nSamplesPerSec;
 
//  wavestereo=1;
//  waverate=44100;

//  printf("decoding %dhz %s, 16bit\n", wmx.nSamplesPerSec, (wmx.nChannels==1)?"mono":"stereo");

  if (rawwave.open(rawwma, &wmx, DriverProc, pe, !0))
  {
    rawwma.close();
    delete cliptabl;
    delete cliptabr;
    return 0;
  }

  wavelen=rawwave.length();
//  wavelen=31337*1024;

  if (!wavelen)
  {
    rawwma.close();
    rawwave.close();
    delete cliptabl;
    delete cliptabr;
    return 0;
  }
  wavebuflen=1024*1024;
  if (wavebuflen>wavelen)
  {
    wavebuflen=wavelen;
    bufloopat=wavebuflen;
  }
  else
    bufloopat=0x40000000;
  wavebuf=new unsigned char [wavebuflen];
  if (!wavebuf)
  {
    wavebuflen=256*1024;
    wavebuf=new unsigned char [wavebuflen];
    if (!wavebuf)
    {
      rawwma.close();
      rawwave.close();
      delete cliptabl;
      delete cliptabr;
      return 0;
    }
  }
  wavelen=wavelen&~(1<<(wavestereo+1)-1);
  wavebufpos=0;
  wavebuffpos=0;
  wavebufread=0;

  rawwave.read(wavebuf, wavebuflen);
  wavepos=wavebuflen;

  plrSetOptions(waverate, (convtostereo||wavestereo)?(PLR_STEREO|PLR_16BIT):PLR_16BIT);

  if (!plrOpenPlayer(plrbuf, buflen, plrBufSize))
  {
    rawwave.close();
    rawwma.close();
    delete cliptabl;
    delete cliptabr;
    return 0;
  }

  stereo=!!(plrOpt&PLR_STEREO);
  bit16=!!(plrOpt&PLR_16BIT);
  signedout=!!(plrOpt&PLR_SIGNEDOUT);
  reversestereo=!!(plrOpt&PLR_REVERSESTEREO);
  samprate=plrRate;
  if (abs(samprate-waverate)<((waverate*tolerance)>>16))
    waverate=samprate;

  wavebufrate=imuldiv(65536, waverate, samprate);

  pause=0;
  looped=0;
  amplify=65536;
  voll=256;
  volr=256;
  calccliptab((amplify*voll)>>8, (amplify*volr)>>8);

  buf16=new unsigned short [buflen*2];

  if (!buf16)
  {
    plrClosePlayer();
    delete buf16;
    rawwave.close();
    rawwma.close();
    delete cliptabl;
    delete cliptabr;
    return 0;
  }

  bufpos=0;

#ifdef DOS32
  tmInit(timerproc, 17100, 65536);
#endif

  active=1;

  return 1;
}

void wpClosePlayer()
{
  active=0;

#ifdef DOS32
  tmClose();
#endif

  plrClosePlayer();

  delete wavebuf;
  delete buf16;

  delete cliptabl;
  delete cliptabr;
  rawwave.close();
  rawwma.close();
}

char wpLooped()
{
  return looped;
}

void wpSetLoop(unsigned char s)
{
  donotloop=!s;
}

void wpPause(unsigned char p)
{
  pause=p;
}

void wpSetAmplify(unsigned long amp)
{
  amplify=amp;
  calccliptab((amplify*voll)>>8, (amplify*volr)>>8);
}

void wpSetSpeed(unsigned short sp)
{
  if (sp<32)
    sp=32;
  wavebufrate=imuldiv(256*sp, waverate, samprate);
}

void wpSetVolume(unsigned char vol, signed char bal, signed char pan, unsigned char opt)
{
  pan=pan;
  opt=opt;
  voll=vol*4;
  volr=vol*4;
  if (bal<0)
    volr=(volr*(64+bal))>>6;
  else
    voll=(voll*(64-bal))>>6;
  wpSetAmplify(amplify);
}

unsigned long wpGetPos()
{
  if (wavelen==wavebuflen)
    return wavebufpos>>(wavestereo+1);
  else
    return ((wavepos+wavelen-wavebuflen+((wavebufpos-wavebufread+wavebuflen)%wavebuflen))%wavelen)>>(wavestereo+1);
}

void wpGetInfo(waveinfo &i)
{
  i.pos=wpGetPos();
  i.len=wavelen>>(wavestereo+1);
  i.rate=waverate;
  i.stereo=wavestereo;
  i.bit16=1;
}

void wpSetPos(signed long pos)
{
  pos=((pos<<(1+wavestereo))+wavelen)%wavelen;
  if (wavelen==wavebuflen)
    wavebufpos=pos;
  else
  {
    if (((pos+wavebuflen)>wavepos)&&(pos<wavepos))
      wavebufpos=(wavebufread-(wavepos-pos)+wavebuflen)%wavebuflen;
    else
    {
      wavepos=pos;
      wavebufpos=0;
      wavebufread=1<<(1+wavestereo);
    }
  }
}

static void freepe()
{
  if (loaded)
  {
    delete pe;
    delete mt;
  }
}

extern "C"
{
  initcloseregstruct wmareg={0, freepe};
}
