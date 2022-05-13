// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Wavetable Device: FPU HighQuality Software Mixer for Pentium/above CPUs
//
// revision history: (please note changes here)
//  -kb990208   Tammo Hinrichs <kb@vmo.nettrade.de>
//    -first release
//  -ryg990426  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -slave strikes back and applies some of kebbys changes of which i
//     obviously don't exactly know what they are (i'm not interested in it
//     either). blame kb if you wanna have sum description :)
//  -ryg990504  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -added float postprocs. prepare for some really ruling effect filters
//     in opencp soon...

// TODO:
// - some hack to re-enable the display mixer, which is of course
//   NOT capable of handling float values... and I don't want to
//   store both versions of the samples, as this would suck QUITE
//   too much memory


// annotations:
// This mixer is and will always be only half of what I want it to be.
// The OpenCP 2.x.0 framework just does not contain the functionality
// i would need to unleash this mixing routine's full potential, such
// as almost infinite vol/pan resolution and perfect volume ramping.
// so wait for the GSL core which will hopefully be contained in OpenCP 3.x
// and will feature all this mixing routine can do. You've seen nothing yet.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "imsdev.h"
#include "mcp.h"
#include "mix.h"
#include "player.h"
#include "imsrtns.h"
#include "devwmixf.h"


#ifndef CPWIN
#include "poll.h"
#endif

#define MIXBUFLEN 4096
#define MAXCHAN 256


#define MIXF_PLAYSTEREO 1
#define MIXF_INTERPOLATE 2
#define MIXF_INTERPOLATEQ 4
#define MIXF_FILTER 8
#define MIXF_QUIET 16
#define MIXF_LOOPED 32

#define MIXF_PLAYING 256
#define MIXF_MUTE 512

#define MIXF_UNSIGNED 1
#define MIXF_16BITOUT 2

#define MIXF_VOLRAMP  256
#define MIXF_DECLICK  512

extern "C" extern sounddevice mcpFMixer;

struct channel
{
  void *samp;

  unsigned long length;
  unsigned long loopstart;
  unsigned long loopend;

  int   newsamp;

  float dstvols[2];
  int   dontramp;

  float vol[2];
  float orgvol[2];

  float orgvolx;
  float orgpan;
  float orgfrez;

  float *sbpos;
  float sbuf[8];

  long  orgrate;
  long  orgfrq;
  long  orgdiv;
  int   volopt;
  long  samptype;

  long  orgloopstart;
  long  orgloopend;
  long  orgsloopstart;
  long  orgsloopend;

  long  handle;
};


extern "C" {

  void mixer (void);
  #pragma aux mixer "*" parm[] modify [eax ebx ecx edx esi edi ebp];

  void prepare_mixer (void);
  #pragma aux prepare_mixer "*" parm[] modify [];

  void getchanvol (int n, int len);
  #pragma aux getchanvol "*" parm [eax][ecx] modify [];

  extern float * tempbuf;         // ptr to 32 bit temp-buffer
  extern void  * outbuf;          // ptr to 16 bit mono-buffer
  extern long    nsamples;        // # of samples to generate
  extern long    nvoices;         // # of voices

  extern unsigned long freqf[];
  extern unsigned long freqw[];

  extern float * smpposw[];
  extern unsigned long smpposf[];

  extern float * loopend[];
  extern unsigned long looplen[];
  extern float   volleft[];
  extern float   volright[];
  extern float   rampleft[];
  extern float   rampright[];

  extern float   ffreq[];
  extern float   freso[];

  extern float   fl1[];
  extern float   fb1[];

  extern long    voiceflags[];

  extern float   fadeleft,faderight;

  extern int     isstereo;
  extern int     outfmt;
  extern float   voll, volr;

  extern float   ct0[],ct1[],ct2[],ct3[];

  extern mixfpostprocregstruct *postprocs;

  extern unsigned long samprate;
}


static int  pause;
static long playsamps;
static long pausesamps;

static sampleinfo *samples;
static int samplenum;

static int clipbusy;
static float amplify;
static float transform[2][2];
static int   volopt;
static unsigned long relpitch;
static int interpolation;
static void *plrbuf;

static int volramp;
static int declick;

static unsigned char stereo;
static unsigned char bit16;
static unsigned char signedout;
static unsigned char reversestereo;

static int     channelnum;
static channel *channels;
static float   fadedown[2];

static int     bufpos;
static int     buflen;

static void (*playerproc)();

static unsigned long tickwidth;
static unsigned long tickplayed;
static unsigned long orgspeed;
static unsigned long relspeed;
static unsigned long newtickwidth;
static unsigned long cmdtimerpos;

static float mastervol;
static float masterbal;
static float masterpan;
static int   mastersrnd;
static int   masterrvb;
static int   masterchr;


inline float fabs(float a)
{
  return (a<0)?-a:a;
}


inline float frchk(float a)
{
  unsigned long b=*((unsigned long *)&a);
  b&=0x7f800000;
  b>>=23;
  if (b==0 || b==0xff)
    return 0;
  if (fabs(a)<0.00000001)
    return 0;
  return a;
}

static void calcinterpoltab()
{
  for (int i=0; i<256; i++)
  {
    float x1=i/256.0;
    float x2=x1*x1;
    float x3=x1*x1*x1;
    ct0[i]=-0.5*x3+x2-0.5*x1;
    ct1[i]=1.5*x3-2.5*x2+1;
    ct2[i]=-1.5*x3+2*x2+0.5*x1;
    ct3[i]=0.5*x3-0.5*x2;
  };
}


static void calcstep(channel &c)
{
  int n=c.handle;
  if (!(voiceflags[n]&MIXF_PLAYING))
    return;
  if (!c.orgdiv)
    return;

  unsigned long rstep=imuldiv(imuldiv(c.orgfrq, c.orgrate, c.orgdiv)<<8, relpitch, samprate);
  freqw[n]=rstep>>16;
  freqf[n]=rstep<<16;

  voiceflags[n]&=~(MIXF_INTERPOLATE|MIXF_INTERPOLATEQ);
  voiceflags[n]|=interpolation?((interpolation>1)?MIXF_INTERPOLATEQ:MIXF_INTERPOLATE):0;

}


static void calcsteps()
{
  int i;
  for (i=0; i<channelnum; i++)
    calcstep(channels[i]);
}

static void calcspeed()
{
  if (channelnum)
    newtickwidth=imuldiv(256*256*256, samprate, orgspeed*relspeed);
}


static void transformvol(channel &ch)
{
  int n=ch.handle;

  ch.vol[0]=transform[0][0]*ch.orgvol[0]+transform[0][1]*ch.orgvol[1];
  ch.vol[1]=transform[1][0]*ch.orgvol[0]+transform[1][1]*ch.orgvol[1];
  if (volopt^ch.volopt)
    ch.vol[1]=-ch.vol[1];

  if (voiceflags[n]&MIXF_MUTE)
  {
    ch.dstvols[0]=ch.dstvols[1]=0;
  }
  else if (!stereo)
  {
    ch.dstvols[0]=0.5*(fabs(ch.vol[0])+fabs(ch.vol[1]));
    ch.dstvols[1]=0;
  }
  else if (reversestereo)
  {
    ch.dstvols[0]=ch.vol[1];
    ch.dstvols[1]=ch.vol[0];
  }
  else
  {
    ch.dstvols[0]=ch.vol[0];
    ch.dstvols[1]=ch.vol[1];
  }
}


static void calcvol(channel &chn)
{                                    
  chn.orgvol[0]=chn.orgvolx*(0.5-chn.orgpan);
  chn.orgvol[1]=chn.orgvolx*(0.5+chn.orgpan);
  transformvol(chn);
}


static void calcvols()
{
  float vols[2][2];

  float realamp=(1.0/65536.0)*amplify;

  vols[0][0]=vols[1][1]=mastervol*(0.5+masterpan);
  vols[0][1]=vols[1][0]=mastervol*(0.5-masterpan);

  if (masterbal>0)
  {
    vols[0][0]=vols[0][0]*(0.5-masterbal);
    vols[0][1]=vols[0][1]*(0.5-masterbal);
  }
  else if (masterbal<0)
  {
    vols[1][0]=vols[1][0]*(0.5+masterbal);
    vols[1][1]=vols[1][1]*(0.5+masterbal);
  }

  volopt=mastersrnd;
  transform[0][0]=realamp*vols[0][0];
  transform[0][1]=realamp*vols[0][1];
  transform[1][0]=realamp*vols[1][0];
  transform[1][1]=realamp*vols[1][1];

  for (int i=0; i<channelnum; i++)
    transformvol(channels[i]);
}


static void stopchan(channel &c)
{
  int n=c.handle;
  if (!(voiceflags[n]&MIXF_PLAYING))
    return;

  // cool end-of-sample-declicking
  if (!(voiceflags[n] & MIXF_QUIET))
  {
    int offs=(voiceflags[n]&MIXF_INTERPOLATEQ)?1:0;
    float ff2=ffreq[n]*ffreq[n];
    fadeleft+=ff2*volleft[n]*smpposw[n][offs];
    faderight+=ff2*volright[n]*smpposw[n][offs];
  }

  voiceflags[n]&=~MIXF_PLAYING;
}


static void rstlbuf(channel &c)
{
  if (c.sbpos)
  {
    for (int i=0; i<8; i++)
      c.sbpos[i]=c.sbuf[i];
    c.sbpos=0;
  }
}


static void setlbuf(channel &c)
{
  int n=c.handle;

  if (c.sbpos)
    rstlbuf(c);

  if (voiceflags[n]&MIXF_LOOPED)
  {
    float *dst=loopend[n];
    float *src=dst-looplen[n];
    for (int i=0; i<8; i++)
    {
      c.sbuf[i]=dst[i];
      dst[i]=src[i];
    }
    c.sbpos=dst;
  }
}



static void mixmain(int min)
{
  #ifdef RASTER
  outp(0x3c8,0);
  outp(0x3c9,63);
  outp(0x3c9,0);
  outp(0x3c9,0);
  #endif

  int i;

  if (!channelnum)
    return;
  if (clipbusy)
    return;

  clipbusy++;

  int bufmax=imulshr16(mcpMixMax, samprate);
  if (bufmax>buflen)
    bufmax=buflen;
  int bufmin=bufmax-imulshr16(min, samprate);
  if (bufmin<0)
    bufmin=0;

  int bufdeltatot=((buflen+(plrGetBufPos()>>(stereo+bit16))-bufpos)%buflen)-buflen+bufmax;
  if (bufdeltatot<bufmin)
    bufdeltatot=0;

  if (pause)
  {
    int pass2=((bufpos+bufdeltatot)>buflen)?(bufpos+bufdeltatot-buflen):0;

    if (bit16)
    {
      memsetw((short*)plrbuf+(bufpos<<stereo), signedout?0:0x8000, (bufdeltatot-pass2)<<stereo);
      if (pass2)
        memsetw((short*)plrbuf, signedout?0:0x8000, pass2<<stereo);
    }
    else
    {
      memsetb((char*)plrbuf+(bufpos<<stereo), signedout?0:0x80, (bufdeltatot-pass2)<<stereo);
      if (pass2)
        memsetb((char*)plrbuf, signedout?0:0x80, pass2<<stereo);
    }

    bufpos+=bufdeltatot;
    if (bufpos>=buflen)
      bufpos-=buflen;

    plrAdvanceTo(bufpos<<(stereo+bit16));
    pausesamps+=bufdeltatot;
  }
  else
    while (bufdeltatot>0)
    {
      int bufdelta=(bufdeltatot>MIXBUFLEN)?MIXBUFLEN:bufdeltatot;
      if (bufdelta>(buflen-bufpos))
        bufdelta=buflen-bufpos;

      long ticks2go=(tickwidth-tickplayed)>>8;

      if (bufdelta>ticks2go)
        bufdelta=ticks2go;

      float invt2g=1.0/(float)ticks2go;

      // set-up mixing core variables
      for (i=0; i<channelnum; i++) if (voiceflags[i]&MIXF_PLAYING)
      {
        channel &ch=channels[i];

        volleft[i]=frchk(volleft[i]);
        volright[i]=frchk(volright[i]);

        if (!volleft[i] && !volright[i] && !rampleft[i] && !rampright[i])
          voiceflags[i]|=MIXF_QUIET;
        else
          voiceflags[i]&=~MIXF_QUIET;


        if (ffreq[i]!=1 || freso[i]!=0)
          voiceflags[i]|=MIXF_FILTER;
        else
          voiceflags[i]&=~MIXF_FILTER;

        // declick start of sample
        if (ch.newsamp)
        {
          if (!(voiceflags[i] & MIXF_QUIET))
          {
            int offs=(voiceflags[i]&MIXF_INTERPOLATEQ)?1:0;
            float ff2=ffreq[i]*ffreq[i];
            fadeleft-=ff2*volleft[i]*smpposw[i][offs];
            faderight-=ff2*volright[i]*smpposw[i][offs];
          }
          ch.newsamp=0;
        }

        voiceflags[i]|=isstereo;
      }
      nsamples=bufdelta;
      outbuf=((char*)(plrbuf))+(bufpos<<(stereo+bit16));

      // optionally turn off the declicking
      if (!declick)
        fadeleft=faderight=0;

      mixer();

      tickplayed+=bufdelta<<8;
      if (!((tickwidth-tickplayed)>>8))
      {
        tickplayed-=tickwidth;
        playerproc();
        cmdtimerpos+=tickwidth;
        tickwidth=newtickwidth;
        float invt2g=256.0/tickwidth;

        // set up volume ramping
        for (i=0; i<channelnum; i++) if (voiceflags[i]&MIXF_PLAYING)
        {

          channel &ch=channels[i];
          if (ch.dontramp)
          {
            volleft[i]=frchk(ch.dstvols[0]);
            volright[i]=frchk(ch.dstvols[1]);
            rampleft[i]=rampright[i]=0;
            if (volramp)
              ch.dontramp=0;
          }
          else
          {
            rampleft[i]=frchk(invt2g*(ch.dstvols[0]-volleft[i]));
            if (rampleft[i]==0)
              volleft[i]=ch.dstvols[0];
            rampright[i]=frchk(invt2g*(ch.dstvols[1]-volright[i]));
            if (rampright[i]==0)
              volright[i]=ch.dstvols[1];
          }

          // filter resonance
          freso[i]=pow(ch.orgfrez,ffreq[i]);


        }
      }
      bufpos+=bufdelta;
      if (bufpos>=buflen)
        bufpos-=buflen;

      plrAdvanceTo(bufpos<<(stereo+bit16));
      bufdeltatot-=bufdelta;
      playsamps+=bufdelta;
    }

  clipbusy--;

  #ifdef RASTER
  outp(0x3c8,0);
  outp(0x3c9,0);
  outp(0x3c9,0);
  outp(0x3c9,0);
  #endif
}

#ifndef CPWIN
static void timerproc()
{
  mixmain(mcpMixPoll);
}
#endif






static void SET(int ch, int opt, int val)
{
  if (ch>=channelnum)
    ch=channelnum-1;
  if (ch<0)
    ch=0;
  channel &chn=channels[ch];
  switch (opt)
  {
  case mcpCReset:
    rstlbuf(chn);
    stopchan(chn);
    int reswasmute;
    reswasmute=voiceflags[ch]&MIXF_MUTE;
    memset(&chn, 0, sizeof(channel));
    chn.handle=ch;
    voiceflags[ch]=reswasmute;
    break;
  case mcpCInstrument:
    rstlbuf(chn);
    stopchan(chn);
    if ((val<0)||(val>=samplenum))
      break;
    sampleinfo *samp;
    samp=&samples[val];
    chn.samptype=samp->type;
    chn.length=samp->length;
    chn.orgrate=samp->samprate;
    chn.samp=samp->ptr;
    chn.orgloopstart=samp->loopstart;
    chn.orgloopend=samp->loopend;
    chn.orgsloopstart=samp->sloopstart;
    chn.orgsloopend=samp->sloopend;
    chn.dontramp=1;
    chn.newsamp=1;

    voiceflags[ch]&=~(MIXF_PLAYING|MIXF_LOOPED);

    freqw[ch]=0;
    freqf[ch]=0;
    fl1[ch]=0;
    fb1[ch]=0;
    ffreq[ch]=1;
    freso[ch]=0;
    smpposf[ch]=0;
    smpposw[ch]=(float *)chn.samp;


    if (chn.samptype&mcpSampSLoop)
    {
      voiceflags[ch]|=MIXF_LOOPED;
      chn.loopstart=chn.orgsloopstart;
      chn.loopend=chn.orgsloopend;
    }
    else
    if (chn.samptype&mcpSampLoop)
    {
      voiceflags[ch]|=MIXF_LOOPED;
      chn.loopstart=chn.orgloopstart;
      chn.loopend=chn.orgloopend;
    }

    if (voiceflags[ch]&MIXF_LOOPED)
    {
      looplen[ch]=chn.loopend-chn.loopstart;
      loopend[ch]=(float *)chn.samp+chn.loopend;
    }
    else
    {
      looplen[ch]=chn.length;
      loopend[ch]=(float *)chn.samp+chn.length-1;
    }
    setlbuf(chn);


    break;
  case mcpCStatus:
    if (!val)
      stopchan(chn);
    else
    {
      if (smpposw[ch] >= (float *)(chn.samp)+chn.length)
        break;
      voiceflags[ch]|=MIXF_PLAYING;
      calcstep(chn);
    }
    break;
  case mcpCMute:
    if (val)
      voiceflags[ch]|=MIXF_MUTE;
    else
      voiceflags[ch]&=~MIXF_MUTE;
    calcvol(chn);
    break;
  case mcpCVolume:
    val=(val>0x200)?0x200:(val<0)?0:val;
    chn.orgvolx=(float)(val)/256.0;
    calcvol(chn);
    break;
  case mcpCPanning:
    val=(val>0x80)?0x80:(val<-0x80)?-0x80:val;
    chn.orgpan=(float)(val)/256.0;
    calcvol(chn);
    break;
  case mcpCSurround:
    chn.volopt=val?1:0;
    transformvol(chn);
    break;
  case mcpCLoop:
    rstlbuf(chn);
    voiceflags[ch]&=~MIXF_LOOPED;

    if ((val==1)&&!(chn.samptype&mcpSampSLoop))
      val=2;
    if ((val==2)&&!(chn.samptype&mcpSampLoop))
      val=0;

    if (val==1)
    {
      voiceflags[ch]|=MIXF_LOOPED;
      chn.loopstart=chn.orgsloopstart;
      chn.loopend=chn.orgsloopend;
    }
    if (val==2)
    {
      voiceflags[ch]|=MIXF_LOOPED;
      chn.loopstart=chn.orgloopstart;
      chn.loopend=chn.orgloopend;
    }

    if (voiceflags[ch]&MIXF_LOOPED)
    {
      looplen[ch]=chn.loopend-chn.loopstart;
      loopend[ch]=(float *)chn.samp+chn.loopend;
    }
    else
    {
      looplen[ch]=chn.length;
      loopend[ch]=(float *)chn.samp+chn.length-1;
    }
    setlbuf(chn);

    break;
  case mcpCPosition:
    int poswasplaying;
    poswasplaying=voiceflags[ch]&MIXF_PLAYING;
    stopchan(chn);
    chn.newsamp=1;
    if (val<0)
      val=0;
    if (val>=chn.length)
      val=chn.length-1;
    smpposw[ch]=(float *)(chn.samp)+val;
    smpposf[ch]=0;
    voiceflags[ch]|=poswasplaying;
    break;
  case mcpCPitch:
    chn.orgfrq=8363;
    chn.orgdiv=mcpGetFreq8363(-val);
    calcstep(chn);
    break;
  case mcpCPitchFix:
    chn.orgfrq=val;
    chn.orgdiv=0x10000;
    calcstep(chn);
    break;
  case mcpCPitch6848:
    chn.orgfrq=6848;
    chn.orgdiv=val;
    calcstep(chn);
    break;
  case mcpCFilterFreq:
/*
    cputs("ch");
    cputs(utoa(ch,0,10));
    cputs(": freq ");
    cputs(utoa(val,0,16));
    cputs("\r\n");
*/
    if (!(val&128))
    {
      ffreq[ch]=1;
      freso[ch]=0;
      break;
    }
    ffreq[ch]=33075.0*pow(2,(val-255)/24.0)/samprate;
    if (ffreq[ch]<0)
      ffreq[ch]=0;
    if (ffreq[ch]>1)
      ffreq[ch]=1;
    break;
  case mcpCFilterRez:
    chn.orgfrez=val/300.0;
    if (chn.orgfrez>1)
      chn.orgfrez=1;
    if (chn.orgfrez==0 && ffreq[ch]==0)
      ffreq[ch]=1;
    break;

  case mcpGSpeed:
    orgspeed=val;
    calcspeed();
    break;

  case mcpMasterVolume:
    if (val>=0 && val<=64)
      mastervol=(float)(val)/64.0;
    calcvols();
    break;
  case mcpMasterPanning:
    if (val>=-64 && val<=64)
      masterpan=(float)(val)/128.0;
    calcvols();
    break;
  case mcpMasterBalance:
    if (val>=-64 && val<=64)
      masterbal=(float)(val)/128.0;
    calcvols();
    break;
  case mcpMasterSurround:
    mastersrnd=val?1:0;
    calcvols();
    break;
  case mcpMasterReverb:
    masterrvb=(val>=64)?63:(val<-64)?-64:val;
    break;
  case mcpMasterChorus:
    masterchr=(val>=64)?63:(val<-64)?-64:val;
    break;
  case mcpMasterSpeed:
    relspeed=(val<16)?16:val;
    calcspeed();
    break;
  case mcpMasterPitch:
    relpitch=val;
    calcsteps();
    break;
  case mcpMasterFilter:
    interpolation=val;
    break;
  case mcpMasterAmplify:
    amplify=val;
    if (channelnum)
      mixSetAmplify(amplify);
    calcvols();
    break;
  case mcpMasterPause:
    pause=val;
    break;
  }
}

static int GET(int ch, int opt)
{
  if (ch>=channelnum)
    ch=channelnum-1;
  if (ch<0)
    ch=0;
  channel &chn=channels[ch];
  switch (opt)
  {
  case mcpCStatus:
    return !!(voiceflags[ch]&MIXF_PLAYING);
  case mcpCMute:
    return !!(voiceflags[ch]&MIXF_MUTE);
  case mcpGTimer:
    if (pause)
      return imuldiv(playsamps, 65536, samprate);
    else
      return plrGetTimer()-imuldiv(pausesamps, 65536, samprate);
  case mcpGCmdTimer:
    return umuldiv(cmdtimerpos, 256, samprate);
  case mcpMasterReverb:
    return masterrvb;
  case mcpMasterChorus:
    return masterchr;
  }
  return 0;
}

static void Idle()
{
  mixmain(mcpMixMax);
  if (plrIdle)
    plrIdle();
}




// Houston, we've got a problem there... the display mixer isn't
// able to handle floating point values at all. shit.
// (kebby, irgendwann mal: "ich will das nicht")

// bla, toller hack: es funktioniert. (fd)

static void GetMixChannel(int ch, mixchannel &chn, int rate)
{
  channel &c=channels[ch];

  chn.samp=c.samp;
  chn.length=c.length;
  chn.loopstart=c.loopstart;
  chn.loopend=c.loopend;
  chn.fpos=smpposf[ch];
  chn.pos=smpposw[ch]-(float*)c.samp;
  chn.volfs[0]=fabs(c.vol[0]);
  chn.volfs[1]=fabs(c.vol[1]);
  chn.step=imuldiv((freqw[ch]<<16)|(freqf[ch]>>16), samprate, rate); 
  chn.status=MIX_PLAY32BIT;
  if (voiceflags[ch]&MIXF_MUTE)
    chn.status|=MIX_MUTE;
  if (voiceflags[ch]&MIXF_LOOPED)
    chn.status|=MIX_LOOPED;
  if (voiceflags[ch]&MIXF_PLAYING)
    chn.status|=MIX_PLAYING;
  if (voiceflags[ch]&MIXF_INTERPOLATE)
    chn.status|=MIX_INTERPOLATE;
}


void getrealvol(int ch, int &l, int &r)
{
  getchanvol(ch,256);
  if (voll<0) voll=-voll;
  l=(voll>16319)?255:(voll/64.0);
  if (volr<0) volr=-volr;
  r=(volr>16319)?255:(volr/64.0);
}

static int LoadSamples(sampleinfo *sil, int n)
{
  if (!mcpReduceSamples(sil, n, 0x40000000, mcpRedToMono|mcpRedToFloat|mcpRedNoPingPong))
    return 0;

  samples=sil;
  samplenum=n;

  return 1;
}

static int OpenPlayer(int chan, void (*proc)())
{
  fadedown[0]=fadedown[1]=0;
  playsamps=pausesamps=0;
  if (chan>MAXCHAN)
    chan=MAXCHAN;

  if (!plrPlay)
    return 0;

  unsigned long currentrate=mcpMixProcRate/chan;
  unsigned short mixfate=(currentrate>mcpMixMaxRate)?mcpMixMaxRate:currentrate;
  plrSetOptions(mixfate, mcpMixOpt);

  playerproc=proc;

  tempbuf=new float [MIXBUFLEN<<1];
  channels=new channel[chan];
  if (!tempbuf||!channels)
  {
    delete tempbuf;
    delete channels;
    return 0;
  }

  mcpGetMasterSample=plrGetMasterSample;
  mcpGetRealMasterVolume=plrGetRealMasterVolume;

  if (!mixInit(GetMixChannel, 0, chan, amplify))
    return 0; 
  mcpGetRealVolume=getrealvol;


  memset(channels, 0, sizeof(channel)*chan);
  calcvols();

  for (int i=0; i<chan; i++)
  {
    channels[i].handle=i;
    voiceflags[i]=0;
  }


  if (!plrOpenPlayer(plrbuf, buflen, mcpMixBufSize))
  {
    mixClose();
    return 0;
  }


  stereo=(plrOpt&PLR_STEREO)?1:0;
  bit16=(plrOpt&PLR_16BIT)?1:0;
  signedout=(plrOpt&PLR_SIGNEDOUT)?1:0;
  reversestereo=!!(plrOpt&PLR_REVERSESTEREO);
  samprate=plrRate;
  bufpos=0;
  pause=0;
  orgspeed=12800;


  channelnum=chan;
  mcpNChan=chan;
  mcpIdle=Idle;

  isstereo=stereo;
  outfmt=(bit16<<1)|(!signedout);
  nvoices=channelnum;
  prepare_mixer();

  calcspeed();
//  playerproc();  // some timing is wrong here!
  tickwidth=newtickwidth;
  tickplayed=0;
  cmdtimerpos=0;

  #ifndef CPWIN
  if (!pollInit(timerproc))
  {
    mcpNChan=0;
    mcpIdle=0;
    plrClosePlayer();
    mixClose();
    return 0;
  }
  #endif

  mixfpostprocregstruct *mode;
  for (mode=postprocs; mode; mode=mode->next)
    if (mode->Init) mode->Init(samprate, stereo);

  return 1;
}

static void ClosePlayer()
{
  mcpNChan=0;
  mcpIdle=0;

#ifndef CPWIN
  pollClose();
#endif

  plrClosePlayer();

  channelnum=0;

  mixClose();

  mixfpostprocregstruct *mode;
  for (mode=postprocs; mode; mode=mode->next)
    if (mode->Close) mode->Close();

  delete channels;
  delete tempbuf;
}

static int Init(const deviceinfo &dev)
{
  volramp=!!(dev.opt&MIXF_VOLRAMP);
  declick=!!(dev.opt&MIXF_DECLICK);

  calcinterpoltab();

  amplify=65535;
  relspeed=256;
  relpitch=256;
  interpolation=0;
  mastervol=64;
  masterbal=0;
  masterpan=0;
  mastersrnd=0;
  channelnum=0;

  mcpLoadSamples=LoadSamples;
  mcpOpenPlayer=OpenPlayer;
  mcpClosePlayer=ClosePlayer;
  mcpGet=GET;
  mcpSet=SET;

  return 1;
}

static void Close()
{
  mcpOpenPlayer=0;
}


static int Detect(deviceinfo &c)
{
  c.dev=&mcpFMixer;
  c.port=-1;
  c.port2=-1;
  c.irq=-1;
  c.irq2=-1;
  c.dma=-1;
  c.dma2=-1;
  if (c.subtype==-1)
    c.subtype=0;
  c.chan=(MAXCHAN>99)?99:MAXCHAN;
  c.mem=0;
  return 1;
}



#include "devigen.h"
#include "psetting.h"
#include "deviplay.h"
#include "pmain.h"
#include "plinkman.h"

static mixfpostprocaddregstruct *postprocadds;

static unsigned long mixfGetOpt(const char *sec)
{
  unsigned long opt=0;
  if (cfGetProfileBool(sec, "volramp", 1, 1))
    opt|=MIXF_VOLRAMP;
  if (cfGetProfileBool(sec, "declick", 1, 1))
    opt|=MIXF_DECLICK;
  return opt;
}


void mixfRegisterPostProc(mixfpostprocregstruct *mode)
{
  mode->next=postprocs;
  postprocs=mode;
}


static void mixfInit(const char *sec)
{
  postprocs=0;
  char regname[50];
  const char *regs;
  regs=cfGetProfileString(sec, "postprocs", "");

  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
      mixfRegisterPostProc((mixfpostprocregstruct*)reg);
  }

  postprocadds=0;
  regs=cfGetProfileString(sec, "postprocadds", "");
  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
    {
      ((mixfpostprocaddregstruct*)reg)->next=postprocadds;
      postprocadds=(mixfpostprocaddregstruct*)reg;
    }
  }
}

static int mixfProcKey(unsigned short key)
{
  mixfpostprocaddregstruct *mode;
  for (mode=postprocadds; mode; mode=mode->next)
  {
    int r=mode->ProcessKey(key);
    if (r)
      return r;
  }

  if (plrProcessKey)
    return plrProcessKey(key);
  return 0;
}

void dummy()
{
  postprocs->Process(tempbuf, 1, 2, 3);
}

extern "C" {
  sounddevice mcpFMixer={SS_WAVETABLE|SS_NEEDPLAYER, "FPU Mixer", Detect, Init, Close};
  devaddstruct mcpFMixAdd = {mixfGetOpt, mixfInit, 0, mixfProcKey};
  char *dllinfo="driver _mcpFMixer; addprocs _mcpFMixAdd";
}
