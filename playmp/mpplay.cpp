// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// MPPlay - Player for MPEG Audio Layer 1/2/3 files
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717 Tammo Hinrichs <opencp@gmx.net>
//    -kinda enabled Win9x background playing (may cause clicks on
//     very very fast machines tho, i'm not sure about this at all)
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile and new ampeg
//  -ryg981222  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -fixed the amplification/volume stuff AGAIN. hope it keeps here this
//     time...
//  -kb990531 Tammo Hinrichs <opencp@gmx.net>
//    -ryg, FORGET IT, pascal changed the vol/pan defs again, NOW
//     it should work ;)

#include <string.h>
#include <stdlib.h>
#include "binfarc.h"
#include "timer.h"
#include "player.h"
#include "wave.h"
#include "deviplay.h"
#include "imsrtns.h"
#include "ampdec.h"

static unsigned char stereo;
static unsigned char bit16;
static unsigned char signedout;
static unsigned long samprate;
static unsigned char reversestereo;

static unsigned short *buf16;
static unsigned long bufpos;
static int buflen;
static void *plrbuf;

static int amplify;
static int vols[10];

static binfile *wavefile;
static ampegdecoder rawwave;
static int wavestereo;
static int waverate;
static unsigned long wavepos;
static unsigned long wavelen;
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

static int clipbusy=0;

void fsave(void *);
#pragma aux fsave parm [eax] = "fsave [eax]" "finit"
void frstor(void *);
#pragma aux frstor parm [eax] = "frstor [eax]"

void wpIdler()
{
  if ((wavelen==wavebuflen)||!active)
    return;

  unsigned long clean=(wavebufpos+wavebuflen-wavebufread)%wavebuflen;
  if (clean<8)
    return;

  clean-=8;
  char fstat[108];
  fsave(fstat);
  while (clean)
  {
    if (rawwave.tell()!=wavepos)
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
  frstor(fstat);
}


static void timerproc()
{
  if (clipbusy)
    return;
  clipbusy=1;

  unsigned long bufplayed=plrGetBufPos()>>(stereo+bit16);

  unsigned long bufdelta;
  unsigned long pass2;
  int quietlen=0;
  bufdelta=(buflen+bufplayed-bufpos)%buflen;

  if (!bufdelta)
  {
    clipbusy=0;
    return;
  }
  wpIdler();
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
    int i;
    if (wavebufrate==0x10000)
    {
      if (wavestereo)
      {
        int o=0;
        while (o<bufdelta)
        {
          int w=(bufdelta-o)*4;
          if ((wavebuflen-wavebufpos)<w)
            w=wavebuflen-wavebufpos;
          memcpy(buf16+2*o, wavebuf+wavebufpos, w);
          o+=w>>2;
          wavebufpos+=w;
          if (wavebufpos>=wavebuflen)
            wavebufpos-=wavebuflen;
        }
      }
      else
      {
        int o=0;
        while (o<bufdelta)
        {
          int w=(bufdelta-o)*2;
          if ((wavebuflen-wavebufpos)<w)
            w=wavebuflen-wavebufpos;
          memcpy(buf16+o, wavebuf+wavebufpos, w);
          o+=w>>1;
          wavebufpos+=w;
          if (wavebufpos>=wavebuflen)
            wavebufpos-=wavebuflen;
        }
      }
    }
    else
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

          buf16[2*i]=(unsigned short)ls^0x8000;
          buf16[2*i+1]=(unsigned short)rs^0x8000;

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

          buf16[i]=(unsigned short)c3^0x8000;

          wavebuffpos+=wavebufrate;
          wavebufpos+=(wavebuffpos>>16)*2;
          wavebuffpos&=0xFFFF;
          if (wavebufpos>=wavebuflen)
            wavebufpos-=wavebuflen;
        }
      }

    if ((bufpos+bufdelta)>buflen)
      pass2=bufpos+bufdelta-buflen;
    else
      pass2=0;
    bufdelta-=pass2;
    if (bit16)
    {
      if (stereo)
      {
        if (reversestereo)
        {
          short *p=(short*)plrbuf+2*bufpos;
          short *b=(short*)buf16;
          if (signedout)
          {
            for (i=0; i<bufdelta; i++)
            {
              p[0]=b[1];
              p[1]=b[0];
              p+=2;
              b+=2;
            }
            p=(short*)plrbuf;
            for (i=0; i<pass2; i++)
            {
              p[0]=b[1];
              p[1]=b[0];
              p+=2;
              b+=2;
            }
          }
          else
          {
            for (i=0; i<bufdelta; i++)
            {
              p[0]=b[1]^0x8000;
              p[1]=b[0]^0x8000;
              p+=2;
              b+=2;
            }
            p=(short*)plrbuf;
            for (i=0; i<pass2; i++)
            {
              p[0]=b[1]^0x8000;
              p[1]=b[0]^0x8000;
              p+=2;
              b+=2;
            }
          }
        }
        else
        {
          short *p=(short*)plrbuf+2*bufpos;
          short *b=(short*)buf16;
          if (signedout)
          {
            for (i=0; i<bufdelta; i++)
            {
              p[0]=b[0];
              p[1]=b[1];
              p+=2;
              b+=2;
            }
            p=(short*)plrbuf;
            for (i=0; i<pass2; i++)
            {
              p[0]=b[0];
              p[1]=b[1];
              p+=2;
              b+=2;
            }
          }
          else
          {
            for (i=0; i<bufdelta; i++)
            {
              p[0]=b[0]^0x8000;
              p[1]=b[1]^0x8000;
              p+=2;
              b+=2;
            }
            p=(short*)plrbuf;
            for (i=0; i<pass2; i++)
            {
              p[0]=b[0]^0x8000;
              p[1]=b[1]^0x8000;
              p+=2;
              b+=2;
            }
          }
        }
      }
      else
      {
        short *p=(short*)plrbuf+bufpos;
        short *b=(short*)buf16;
        if (signedout)
        {
          for (i=0; i<bufdelta; i++)
          {
            p[0]=b[0];
            p++;
            b++;
          }
          p=(short*)plrbuf;
          for (i=0; i<pass2; i++)
          {
            p[0]=b[0];
            p++;
            b++;
          }
        }
        else
        {
          for (i=0; i<bufdelta; i++)
          {
            p[0]=b[0]^0x8000;
            p++;
            b++;
          }
          p=(short*)plrbuf;
          for (i=0; i<pass2; i++)
          {
            p[0]=b[0]^0x8000;
            p++;
            b++;
          }
        }
      }
    }
    else
    {
      if (stereo)
      {
        if (reversestereo)
        {
          char *p=(char*)plrbuf+2*bufpos;
          char *b=(char*)buf16;
          if (signedout)
          {
            for (i=0; i<bufdelta; i++)
            {
              p[0]=b[3];
              p[1]=b[1];
              p+=2;
              b+=4;
            }
            p=(char*)plrbuf;
            for (i=0; i<pass2; i++)
            {
              p[0]=b[3];
              p[1]=b[1];
              p+=2;
              b+=4;
            }
          }
          else
          {
            for (i=0; i<bufdelta; i++)
            {
              p[0]=b[3]^0x80;
              p[1]=b[1]^0x80;
              p+=2;
              b+=4;
            }
            p=(char*)plrbuf;
            for (i=0; i<pass2; i++)
            {
              p[0]=b[3]^0x80;
              p[1]=b[1]^0x80;
              p+=2;
              b+=4;
            }
          }
        }
        else
        {
          char *p=(char*)plrbuf+2*bufpos;
          char *b=(char*)buf16;
          if (signedout)
          {
            for (i=0; i<bufdelta; i++)
            {
              p[0]=b[1];
              p[1]=b[3];
              p+=2;
              b+=4;
            }
            p=(char*)plrbuf;
            for (i=0; i<pass2; i++)
            {
              p[0]=b[1];
              p[1]=b[3];
              p+=2;
              b+=4;
            }
          }
          else
          {
            for (i=0; i<bufdelta; i++)
            {
              p[0]=b[1]^0x80;
              p[1]=b[3]^0x80;
              p+=2;
              b+=4;
            }
            p=(char*)plrbuf;
            for (i=0; i<pass2; i++)
            {
              p[0]=b[1]^0x80;
              p[1]=b[3]^0x80;
              p+=2;
              b+=4;
            }
          }
        }
      }
      else
      {
        char *p=(char*)plrbuf+bufpos;
        char *b=(char*)buf16;
        if (signedout)
        {
          for (i=0; i<bufdelta; i++)
          {
            p[0]=b[1];
            p++;
            b+=2;
          }
          p=(char*)plrbuf;
          for (i=0; i<pass2; i++)
          {
            p[0]=b[1];
            p++;
            b+=2;
          }
        }
        else
        {
          for (i=0; i<bufdelta; i++)
          {
            p[0]=b[1]^0x80;
            p++;
            b+=2;
          }
          p=(char*)plrbuf;
          for (i=0; i<pass2; i++)
          {
            p[0]=b[1]^0x80;
            p++;
            b+=2;
          }
        }
      }
    }
    bufpos+=bufdelta+pass2;
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

  clipbusy=0;
}

#ifdef DOS32
static int oldtimer=0;
static volatile int cycmiss=0;
#endif

void wpIdle()
{
  // this is a bloody hack. i know ;)
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
}

void wpSetAmplify(unsigned long amp)
{
  amplify=amp;
  float v[9];
  float ampf=(float)vols[9]*(amplify/65536.0)/65536.0;
  int i;
  for (i=0; i<9; i++)
    v[i]=ampf*vols[i];
  rawwave.ioctl(ampegdecoder::ioctlsetstereo, v, 4*9);
}


unsigned char wpOpenPlayer(binfile &wav, int tostereo, int tolerance, int maxrate, int tomono)
{
  if (!plrPlay)
    return 0;

  wavefile=&wav;

  int mplay, mpver, mpfreq, mpstereo, mprate;
  if (!ampegdecoder::getheader(*wavefile, mplay, mpver, mpfreq, mpstereo, mprate))
    return 0;
  int redfac=(mpfreq<=maxrate)?0:(mpfreq<=(maxrate*2))?1:2;

  plrSetOptions(mpfreq>>redfac, (PLR_SIGNEDOUT|PLR_16BIT)|((!tomono&&(tostereo||mpstereo))?PLR_STEREO:0));
  stereo=!!(plrOpt&PLR_STEREO);
  bit16=!!(plrOpt&PLR_16BIT);
  signedout=!!(plrOpt&PLR_SIGNEDOUT);
  reversestereo=!!(plrOpt&PLR_REVERSESTEREO);
  samprate=plrRate;

  if (rawwave.open(*wavefile, waverate, wavestereo, !0, redfac, stereo?2:1))
    return 0;
  if ((abs(samprate-waverate)<((waverate*tolerance)>>16)))
    waverate=samprate;
  wavebufrate=imuldiv(65536, waverate, samprate);

  wavelen=rawwave.length();
  if (!wavelen)
    return 0;

  wavebuflen=16384;
  if (wavebuflen>wavelen)
  {
    wavebuflen=wavelen;
    bufloopat=wavebuflen;
  }
  else
    bufloopat=0x40000000;
  wavebuf=new unsigned char [wavebuflen];
  if (!wavebuf)
    return 0;
  wavelen=wavelen&~(1<<(wavestereo+1)-1);
  wavebufpos=0;
  wavebuffpos=0;
  wavebufread=0;

  rawwave.read(wavebuf, wavebuflen);
  wavepos=wavebuflen;

  if (!plrOpenPlayer(plrbuf, buflen, plrBufSize))
    return 0;

  pause=0;
  looped=0;
  amplify=65536;
  wpSetVolume(64, 0, 64, 0);
  wpSetAmplify(amplify);

  buf16=new unsigned short [buflen*2];

  if (!buf16)
  {
    plrClosePlayer();
    delete wavebuf;
    return 0;
  }

  bufpos=0;

#ifdef DOS32
  if (!tmInit(timerproc, 15582, 8192))
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
  tmClose();
#endif

  plrClosePlayer();

  delete wavebuf;
  delete buf16;

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

void wpSetSpeed(unsigned short sp)
{
  if (sp<32)
    sp=32;
  wavebufrate=imuldiv(256*sp, waverate, samprate);
}

void wpSetVolume(unsigned char vol, signed char bal, signed char pan, unsigned char opt)
{
  vols[0]=((64-bal)*(64+pan))>>5;
  vols[1]=((64-bal)*(64-pan))>>5;
  vols[2]=(64-bal)*4;
  vols[3]=((64+bal)*(64-pan)*(opt?-1:1))>>5;
  vols[4]=((64+bal)*(64+pan)*(opt?-1:1))>>5;
  vols[5]=(64+bal)*4*(opt?-1:1);
  vols[6]=128;
  vols[7]=128;
  vols[8]=256;
  vols[9]=vol*4;
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

