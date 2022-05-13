// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Display mixer routines/vars/defs
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#define NO_MCPBASE_IMPORT
#include <stdio.h>
#include <string.h>
#include "mcp.h"
#include "imsrtns.h"
#include "mix.h"

#define MIXBUFLEN 2048

extern "C"
{
  extern signed char (*mixIntrpolTab)[256][2];
  extern short (*mixIntrpolTab2)[256][2];
}

static void (*mixGetMixChannel)(int ch, mixchannel &chn, int rate);

static mixchannel *channels;

static int channum;
static long (*voltabs)[2][256];
static short (*amptab)[256];
static unsigned long clipmax;
static long *mixbuf;
static unsigned long amplify;


extern "C" unsigned long mixAddAbs(const mixchannel &ch, unsigned long len);
#pragma aux mixAddAbs parm [eax] [edi] value [ecx] modify [ebx edx esi]
extern "C" void mixPlayChannel(long *buf, unsigned long len, mixchannel &ch, char st);
#pragma aux mixPlayChannel parm [] caller modify [eax ebx ecx edx esi edi]
extern "C" void mixClip(void *dst, const long *src, unsigned long len, short (*tab)[256], long max);
#pragma aux mixClip parm [edi] [esi] [ecx] [ebx] [edx] modify [eax]


static void mixCalcIntrpolTab()
{
  int i,j;
  for (i=0; i<16; i++)
    for (j=0; j<256; j++)
    {
      mixIntrpolTab[i][j][1]=(i*(signed char)j)>>4;
      mixIntrpolTab[i][j][0]=j-mixIntrpolTab[i][j][1];
    }
  for (i=0; i<32; i++)
    for (j=0; j<256; j++)
    {
      mixIntrpolTab2[i][j][1]=(i*(signed char)j)<<3;
      mixIntrpolTab2[i][j][0]=((signed char)j<<8)-mixIntrpolTab2[i][j][1];
    }
}

static void calcamptab(signed long amp)
{
  if (!amptab)
    return;
  amp>>=4;

  int i;
  for (i=0; i<256; i++)
  {
    amptab[0][i]=(amp*i)>>12;
    amptab[1][i]=(amp*i)>>4;
    amptab[2][i]=(amp*(signed char)i)<<4;
  }

  clipmax=amp?(0x07FFF000/amp):0x7FFFFFFF;
}

static void mixgetmixch(int ch, mixchannel &chn, int rate)
{
  mixGetMixChannel(ch, chn, rate);

  if (!(chn.status&MIX_PLAYING))
    return;
  if (chn.pos>=chn.length)
  {
    chn.status&=~MIX_PLAYING;
    return;
  }
  if (chn.status&MIX_PLAY16BIT)
    chn.samp=(void*)((unsigned long)chn.samp>>1);
  if (chn.status&MIX_PLAY32BIT)
    chn.samp=(void*)((unsigned long)chn.samp>>2);
  chn.replen=(chn.status&MIX_LOOPED)?(chn.loopend-chn.loopstart):0;
}

static void putchn(mixchannel &chn, int len, int opt)
{
  if (!(chn.status&MIX_PLAYING)||(chn.status&MIX_MUTE))
    return;
  if (opt&mcpGetSampleHQ)
    chn.status|=MIX_MAX|MIX_INTERPOLATE;
  if (!(chn.status&MIX_PLAY32BIT))
  {
    int voll=chn.vols[0];
    int volr=chn.vols[1];
    if (!(opt&mcpGetSampleStereo))
    {
      voll=(voll+volr)>>1;
      volr=0;
    }
    if (voll<0)
      voll=0;
    if (voll>64)
      voll=64;
    if (volr<0)
      volr=0;
    if (volr>64)
      volr=64;
    if (!voll&&!volr)
      return;
    chn.voltabs[0]=voltabs[voll];
    chn.voltabs[1]=voltabs[volr];
  }
  mixPlayChannel(mixbuf, len, chn, opt&mcpGetSampleStereo);
}

void mixGetMasterSample(short *s, int len, int rate, int opt)
{
  int stereo=(opt&mcpGetSampleStereo)?1:0;
  int i;
  for (i=0; i<channum; i++)
    mixgetmixch(i, channels[i], rate);

  if (len>(MIXBUFLEN>>stereo))
  {
    memset((short *)s+MIXBUFLEN, 0, ((len<<stereo)-MIXBUFLEN)<<1);
    len=MIXBUFLEN>>stereo;
  }

  memsetd(mixbuf, 0, len<<stereo);
  for (i=0; i<channum; i++)
    putchn(channels[i], len, opt);
  mixClip(s, mixbuf, len<<stereo, amptab, clipmax);
}

int mixMixChanSamples(int *ch, int n, short *s, int len, int rate, int opt)
{
  int stereo=(opt&mcpGetSampleStereo)?1:0;

  if (!n)
  {
    memset(s, 0, len<<(1+stereo));
    return 0;
  }

  if (len>MIXBUFLEN)
  {
    memset((short *)s+(MIXBUFLEN<<stereo), 0, ((len<<stereo)-MIXBUFLEN)<<1);
    len=MIXBUFLEN>>stereo;
  }
  int ret=3;

  int i;
  for (i=0; i<n; i++)
    mixgetmixch(ch[i], channels[i], rate);
  memsetd(mixbuf, 0, len<<stereo);
  for (i=0; i<n; i++)
  {
    if (!(channels[i].status&MIX_PLAYING))
      continue;
    ret&=~2;
    if (!(channels[i].status&MIX_MUTE))
      ret=0;
    channels[i].status&=~MIX_MUTE;
    putchn(channels[i], len, opt);
  }
  len<<=stereo;
  for (i=0; i<len; i++)
    s[i]=mixbuf[i]>>8;
  return ret;
}

int mixGetChanSample(int ch, short *s, int len, int rate, int opt)
{
  return mixMixChanSamples(&ch, 1, s, len, rate, opt);
}

void mixGetRealVolume(int ch, int &l, int &r)
{
  mixchannel chn;
  mixgetmixch(ch, chn, 44100);
  chn.status&=~MIX_MUTE;
  if (chn.status&MIX_PLAYING)
  {
    unsigned long v=mixAddAbs(chn, 256);
    unsigned long i;
    if (chn.status&MIX_PLAY32BIT)
    {
      i=((int)(v*(chn.volfs[0]*64.0)))>>16;
      l=(i>255)?255:i;
      i=((int)(v*(chn.volfs[1]*64.0)))>>16;
      r=(i>255)?255:i;
    } else
    {
      i=(v*chn.vols[0])>>16;
      l=(i>255)?255:i;
      i=(v*chn.vols[1])>>16;
      r=(i>255)?255:i;
    }
  }
  else
    l=r=0;
}

void mixGetRealMasterVolume(int &l, int &r)
{
  int i;
  for (i=0; i<channum; i++)
    mixgetmixch(i, channels[i], 44100);

  l=r=0;
  for (i=0; i<channum; i++)
  {
    if ((channels[i].status&MIX_MUTE)||!(channels[i].status&MIX_PLAYING))
      continue;
    unsigned long v=mixAddAbs(channels[i], 256);
    l+=(((v*channels[i].vols[0])>>16)*amplify)>>18;
    r+=(((v*channels[i].vols[1])>>16)*amplify)>>18;
  }
  l=(l>255)?255:l;
  r=(r>255)?255:r;
}

void mixSetAmplify(int amp)
{
  amplify=amp*8;
  calcamptab((amplify*channum)>>11);
}

int mixInit(void (*getchan)(int ch, mixchannel &chn, int rate), int masterchan, int chn, int amp)
{
  mixGetMixChannel=getchan;
  mixbuf=new long [MIXBUFLEN];
  mixIntrpolTab=new signed char [16][256][2];
  mixIntrpolTab2=new short [32][256][2];
  voltabs=new long [65][2][256];
  channels=new mixchannel[chn+16];
  if (!mixbuf||!voltabs||!mixIntrpolTab2||!mixIntrpolTab||!channels)
    return 0;
  amptab=0;

  if (masterchan)
  {
    amptab=new short [3][256];
    if (!amptab)
      return 0;
  }

  mixCalcIntrpolTab();

  amplify=amp*8;

  mcpGetRealVolume=mixGetRealVolume;
  mcpGetChanSample=mixGetChanSample;
  mcpMixChanSamples=mixMixChanSamples;
  if (masterchan)
  {
    mcpGetRealMasterVolume=mixGetRealMasterVolume;
    mcpGetMasterSample=mixGetMasterSample;
  }

  channum=chn;
  int i,j;
  for (i=0; i<=64; i++)
    for (j=0; j<256; j++)
    {
      voltabs[i][0][j]=(((i*0xFFFFFF/channum)>>6)*(signed char)j)>>8;
      voltabs[i][1][j]=(((i*0xFFFFFF/channum)>>14)*j)>>8;
    }
  calcamptab((amplify*channum)>>11);

  return 1;
}

void mixClose()
{
  delete channels;
  delete mixbuf;
  delete voltabs;
  delete amptab;
  delete mixIntrpolTab;
  delete mixIntrpolTab2;
}
