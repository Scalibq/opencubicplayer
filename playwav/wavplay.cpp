// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// WAVPlay - wave file player
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <kb@nwn.de>
//    -added a few lines in idle routine to make win95 background
//     playing possible

#include <stdlib.h>
#include "binfarc.h"
#include "poll.h"
#include "player.h"
#include "wave.h"
#include "deviplay.h"
#include "imsrtns.h"

static unsigned char stereo;
static unsigned char bit16;
static unsigned char signedout;
static unsigned long samprate;
static unsigned char reversestereo;

static unsigned short *buf16;
static unsigned long bufpos;
static int buflen;
static void *plrbuf;

static unsigned short *cliptabl;
static unsigned short *cliptabr;
static unsigned long amplify;
static unsigned long voll,volr;
static char convtostereo;

static binfile *wavefile;
static abinfile rawwave;
static char wavestereo;
static char wave16bit;
static unsigned long waverate;
static unsigned long wavepos;
static unsigned long wavelen;
static unsigned long waveoffs;
static unsigned char *wavebuf;
static unsigned long wavebuflen;
static unsigned long wavebufpos;
static unsigned long wavebuffpos;
static unsigned long wavebufread;
static unsigned long wavebufrate;
static char active;
static char looped;
static char donotloop;
static unsigned long bufloopat;

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
  if (wavebuflen!=wavelen)
  {
    int towrap=imuldiv((((wavebuflen+wavebufread-wavebufpos-1)%wavebuflen)>>(wavestereo+wave16bit)), 65536, wavebufrate);
    if (bufdelta>towrap)
      quietlen=bufdelta-towrap;
  }

  if (pause)
    quietlen=bufdelta;

  int toloop=imuldiv(((bufloopat-wavebufpos)>>(wave16bit+wavestereo)), 65536, wavebufrate);
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
    if (wave16bit)
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
    else
    {
      if (wavestereo)
        for (i=0; i<bufdelta; i++)
        {
          if (reversestereo)
          {
            buf16[2*i+1]=wavebuf[wavebufpos]<<8;
            buf16[2*i]=wavebuf[wavebufpos+1]<<8;
          }
          else
          {
            buf16[2*i]=wavebuf[wavebufpos]<<8;
            buf16[2*i+1]=wavebuf[wavebufpos+1]<<8;
          }
          wavebuffpos+=wavebufrate;
          wavebufpos+=(wavebuffpos>>16)*2;
          wavebuffpos&=0xFFFF;
          if (wavebufpos>=wavebuflen)
            wavebufpos-=wavebuflen;
        }
      else
        for (i=0; i<bufdelta; i++)
        {
          buf16[2*i+1]=buf16[2*i]=wavebuf[wavebufpos]<<8;
          wavebuffpos+=wavebufrate;
          wavebufpos+=wavebuffpos>>16;
          wavebuffpos&=0xFFFF;
          if (wavebufpos>=wavebuflen)
            wavebufpos-=wavebuflen;
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
  unsigned long bufplayed=plrGetBufPos()>>(stereo+bit16);
  unsigned long bufdelta=(buflen+bufplayed-bufpos)%buflen;
  if (bufdelta>(buflen>>3))
    timerproc();

  if ((wavelen==wavebuflen)||!active)
    return;

  unsigned long clean=(wavebufpos+wavebuflen-wavebufread)%wavebuflen;
  if (clean*8>wavebuflen)
  {
    while (clean)
    {
      rawwave.seek(wavepos);
      int read=clean;
      if ((wavebufread+read)>wavebuflen)
        read=wavebuflen-wavebufread;
      if ((wavepos+read)>=wavelen)
      {
        read=wavelen-wavepos;
        bufloopat=wavebufread+read;
      }
      if (read>0x10000)
        read=0x10000;
      rawwave.read(wavebuf+wavebufread, read);
      wavebufread=(wavebufread+read)%wavebuflen;
      wavepos=(wavepos+read)%wavelen;
      clean-=read;
    }
  }
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

  if (wavefile->getul()!=0x46464952)
    return 0;
  wavefile->getul();
  if (wavefile->getul()!=0x45564157)
    return 0;
  while (1)
  {
    if (wavefile->getul()==0x20746D66)
      break;
    if(wavefile->eof())
      return 0;
    wavefile->seekcur(wavefile->getul());
  }
  int fmtlen=wavefile->getul();
  if (fmtlen<16)
    return 0;
  if (wavefile->getus()!=1)
    return 0;
  wavestereo=wavefile->getus()==2;
  waverate=wavefile->getul();
  wavefile->getul();
  wavefile->getus();
  wave16bit=wavefile->getus()==16;
  wavefile->seekcur(fmtlen-16);

  while (1)
  {
    if (wavefile->getul()==0x61746164)
      break;
    if(wavefile->eof())
      return 0;
    wavefile->seekcur(wavefile->getul());
  }

  wavelen=wavefile->getul();
  waveoffs=wavefile->tell();
  rawwave.open(*wavefile, waveoffs, wavelen);
  if (!wavelen)
    return 0;
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
      return 0;
  }
  wavelen=wavelen&~(1<<(wavestereo+wave16bit)-1);
  wavebufpos=0;
  wavebuffpos=0;
  wavebufread=0;

  rawwave.read(wavebuf, wavebuflen);
  wavepos=wavebuflen;

  plrSetOptions(waverate, (convtostereo||wavestereo)?(PLR_STEREO|PLR_16BIT):PLR_16BIT);

  if (!plrOpenPlayer(plrbuf, buflen, plrBufSize))
    return 0;

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
    return 0;
  }

  bufpos=0;

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

void wpClosePlayer()
{
  active=0;

#ifdef DOS32
  pollClose();
#endif

  plrClosePlayer();

  delete wavebuf;
  delete buf16;

  delete cliptabl;
  delete cliptabr;
  rawwave.close();
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
    return wavebufpos>>(wavestereo+wave16bit);
  else
    return ((wavepos+wavelen-wavebuflen+((wavebufpos-wavebufread+wavebuflen)%wavebuflen))%wavelen)>>(wavestereo+wave16bit);
}

void wpGetInfo(waveinfo &i)
{
  i.pos=wpGetPos();
  i.len=wavelen>>(wavestereo+wave16bit);
  i.rate=waverate;
  i.stereo=wavestereo;
  i.bit16=wave16bit;
}

void wpSetPos(signed long pos)
{
  pos=((pos<<(wave16bit+wavestereo))+wavelen)%wavelen;
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
      wavebufread=1<<(wave16bit+wavestereo);
    }
  }
}