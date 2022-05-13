// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Player system variables / auxiliary routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#define NO_PLRBASE_IMPORT

#include <string.h>
#include "player.h"
#include "imsrtns.h"

void (*plrIdle)();

int plrRate;
int plrOpt;
int (*plrPlay)(void *&buf, int &len);
void (*plrStop)();
void (*plrSetOptions)(int rate, int opt);
int (*plrGetBufPos)();
int (*plrGetPlayPos)();
void (*plrAdvanceTo)(int pos);
long (*plrGetTimer)();

static unsigned char stereo;
static unsigned char bit16;
static unsigned char reversestereo;
static unsigned char signedout;
static unsigned long samprate;

static unsigned char *plrbuf;
static unsigned long buflen;


#pragma aux mixMasterAddAbs parm [esi] [edi] value [ecx] modify [eax]
extern "C" unsigned long __pragma("mixMasterAddAbs") mixAddAbs16M(const void *ch, unsigned long len);
extern "C" unsigned long __pragma("mixMasterAddAbs") mixAddAbs16MS(const void *ch, unsigned long len);
extern "C" unsigned long __pragma("mixMasterAddAbs") mixAddAbs16S(const void *ch, unsigned long len);
extern "C" unsigned long __pragma("mixMasterAddAbs") mixAddAbs16SS(const void *ch, unsigned long len);
extern "C" unsigned long __pragma("mixMasterAddAbs") mixAddAbs8M(const void *ch, unsigned long len);
extern "C" unsigned long __pragma("mixMasterAddAbs") mixAddAbs8MS(const void *ch, unsigned long len);
extern "C" unsigned long __pragma("mixMasterAddAbs") mixAddAbs8S(const void *ch, unsigned long len);
extern "C" unsigned long __pragma("mixMasterAddAbs") mixAddAbs8SS(const void *ch, unsigned long len);

#pragma aux mixGetMasterSample parm [edi] [esi] [ecx] [edx] modify [eax ebx]
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleMS8M(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleMU8M(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleMS8S(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleMU8S(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSS8M(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSU8M(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSS8S(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSU8S(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSS8SR(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSU8SR(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleMS16M(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleMU16M(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleMS16S(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleMU16S(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSS16M(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSU16M(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSS16S(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSU16S(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSS16SR(short *buf2, const void *buf, int len, long step);
extern "C" void __pragma("mixGetMasterSample") mixGetMasterSampleSU16SR(short *buf2, const void *buf, int len, long step);


void plrGetRealMasterVolume(int &l, int &r)
{
  int len=samprate/20;
  if (len>buflen)
    len=buflen;
  int p=plrGetPlayPos()>>(stereo+bit16);

  long pass2=len-buflen+p;

  unsigned long __pragma("mixMasterAddAbs") (*fn)(const void *ch, unsigned long len);
  unsigned long v;
  if (stereo)
  {
    fn=bit16?(signedout?mixAddAbs16SS:mixAddAbs16S):(signedout?mixAddAbs8SS:mixAddAbs8S);

    if (pass2>0)
      v=fn(plrbuf+(p<<(1+bit16)), len-pass2)+fn(plrbuf, pass2);
    else
      v=fn(plrbuf+(p<<(1+bit16)), len);
    v=v*128/(len*16384);
    l=(v>255)?255:v;

    if (pass2>0)
      v=fn(plrbuf+(p<<(1+bit16))+(1<<bit16), len-pass2)+fn(plrbuf+(1<<bit16), pass2);
    else
      v=fn(plrbuf+(p<<(1+bit16))+(1<<bit16), len);
    v=v*128/(len*16384);
    r=(v>255)?255:v;
  }
  else
  {
    fn=bit16?(signedout?mixAddAbs16MS:mixAddAbs16M):(signedout?mixAddAbs8MS:mixAddAbs8M);

    if (pass2>0)
      v=fn(plrbuf+(p<<bit16), len-pass2)+fn(plrbuf, pass2);
    else
      v=fn(plrbuf+(p<<bit16), len);
    v=v*128/(len*16384);
    r=l=(v>255)?255:v;
  }
  if (reversestereo)
  {
    int t=r;
    r=l;
    l=t;
  }
}

void plrGetMasterSample(short *buf, int len, int rate, int opt)
{
  int step=imuldiv(samprate, 0x10000, rate);
  if (step<0x1000)
    step=0x1000;
  if (step>0x800000)
    step=0x800000;

  int maxlen=imuldiv(buflen, 0x10000, step);
  int stereoout=(opt&plrGetSampleStereo)?1:0;
  if (len>maxlen)
  {
    memset((short*)buf+(maxlen<<stereoout), 0, (len-maxlen)<<(1+stereoout));
    len=maxlen;
  }

  unsigned long bp=plrGetPlayPos()>>(stereo+bit16);
  signed long pass2=len-imuldiv(buflen-bp,0x10000,step);

  void __pragma("mixGetMasterSample") (*fn)(short *buf2, const void *buf, int len, long step);

  if (bit16)
    if (stereo)
      if (!stereoout)
        fn=signedout?mixGetMasterSampleSS16M:mixGetMasterSampleSU16M;
      else
        if (reversestereo)
          fn=signedout?mixGetMasterSampleSS16SR:mixGetMasterSampleSU16SR;
        else
          fn=signedout?mixGetMasterSampleSS16S:mixGetMasterSampleSU16S;
    else
      if (!stereoout)
        fn=signedout?mixGetMasterSampleMS16M:mixGetMasterSampleMU16M;
      else
        fn=signedout?mixGetMasterSampleMS16S:mixGetMasterSampleMU16S;
  else
    if (stereo)
      if (!stereoout)
        fn=signedout?mixGetMasterSampleSS8M:mixGetMasterSampleSU8M;
      else
        if (reversestereo)
          fn=signedout?mixGetMasterSampleSS8SR:mixGetMasterSampleSU8SR;
        else
          fn=signedout?mixGetMasterSampleSS8S:mixGetMasterSampleSU8S;
    else
      if (!stereoout)
        fn=signedout?mixGetMasterSampleMS8M:mixGetMasterSampleMU8M;
      else
        fn=signedout?mixGetMasterSampleMS8S:mixGetMasterSampleMU8S;

  if (pass2>0)
  {
    fn(buf, plrbuf+(bp<<(stereo+bit16)), len-pass2, step);
    fn(buf+((len-pass2)<<stereoout), plrbuf, pass2, step);
  }
  else
    fn(buf, plrbuf+(bp<<(stereo+bit16)), len, step);
}


int plrOpenPlayer(void *&buf, int &len, int bufl)
{
  if (!plrPlay)
    return 0;

  stereo=!!(plrOpt&PLR_STEREO);
  bit16=!!(plrOpt&PLR_16BIT);
  reversestereo=!!(plrOpt&PLR_REVERSESTEREO);
  signedout=!!(plrOpt&PLR_SIGNEDOUT);
  samprate=plrRate;

  int dmalen=umuldiv(samprate<<(stereo+bit16), bufl, 65000)&~15;

  plrbuf=0;
  if (!plrPlay(*(void**)&plrbuf, dmalen))
    return 0;

  buflen=dmalen>>(stereo+bit16);
  buf=plrbuf;
  len=buflen;


  return 1;
}

void plrClosePlayer()
{
  plrStop();
}
