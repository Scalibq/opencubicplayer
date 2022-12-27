// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>

// this shall somewhen become a cool reverb/compressor/EQ audio plug-in
// but i didnt have any time for it yet ;)
// (KB)

// ok, time for the changelog:
// -rygwhenever Fabian Giesen <ripped@purge.com>
//   -first release
// -kb990531 Tammo Hinrichs <opencp@gmx.net>
//   -added high pass filter to reverb output to remove bass and
//    dc offsets
//   -added low pass filters to the comb filter delays (treble cut, simulates
//    echo damping at the walls)
//   -implemented a simple two-delay stereo chorus
//   -implemented volreg structure to make reverb and chorus parameters
//    user adjustable
//   -fixed some minor things here and there
// -ryg990610 Fabian Giesen <fabian@jdcs.su.nw.schule.de>
//   -converted this back to int for normal/quality mixer

#include <math.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include "devwmix.h"
#include "imsrtns.h"
#include "mcp.h"
#include "vol.h"


static ocpvolstruct irevvol[]={
                             {16, 0, 50, 1, 0, "reverb time"},
                             {37, 0, 50, 1, 0, "reverb high cut"},
                             {12, 0, 50, 1, 0, "chorus speed"},
                             {25, 0, 50, 1, 0, "chorus depth"},
                             {25, 0, 50, 1, 0, "chorus phase"},
                             {05, 0, 50, 1, 0, "chorus feedback"},
                             {00, 0, -2, 1, 0, "chorus mode select\tchorus\tflanger"},
                             {30, 0, 80, 1, 0, "level detector"},
                             {00, 0, 40, 1, 0, "db reduction"},
                            };

static float srate;
static int st;

static int initfail=0;
static int running=0;


static long  *leftl[6], *rightl[6]; // delay lines
static long   llen[6], lpos[6];     // dline length/pos left
static long   rlen[6], rpos[6];     // same right
static long   rlpf[6], llpf[6];     // left/right comb filter LPFs
static long   lpfval=8388608;       // LPF freq value
static long   lpconst,lpl,lpr;      // reverb out hpf (1-lpf)

static long  chrminspeed,chrmaxspeed; // chorus speed limits (0.1-10Hz, 24:8)
static long  chrspeed;                // chorus speed (24:8fp)
static long  chrpos;                  // chorus osc pos
static long  chrphase;                // chorus l/r phase shift
static long  chrdepth;                // chorus depth
static long  chrfb;                   // chorus feedback
static long  *lcline, *rcline;        // chorus delay lines
static int   cllen,clpos;             // dlines write length/pos

static long  *co1dline;               // compr1 analyzer delay line
static int   co1dllen, co1dlpos;      // compr1 dl length/pos
static int   co1amode;                // compr1 analyzer mode (0:rms, 1:peak)
static long  co1mean,co1invlen;
static long  co1atten, co1attack, co1decay, co1thres, co1cprv;

static long  gainsf[6]={63333, 62796, 62507, 62320, 65160, 65408};
static float delays[6]={29.68253968, 37.07482993, 41.06575964, 43.67346939,
                         4.98866213,  1.67800453};
static float gainsc[6]={0.966384599, 0.958186359, 0.953783929, 0.950933178,
                        0.994260075, 0.998044717};

long imulshr24(long a, long b);
#pragma aux imulshr24 parm [eax][edx] value [eax]="imul edx" "shrd eax, edx, 24";
long imulshr32(long a, long b);
#pragma aux imulshr32 parm [eax][edx] value [edx]="imul edx";

static void updatevol(int n)
{
  int i;
  float v;
  switch(n)
  {
    case 0:  // rvb time
      v=pow(25.0/(irevvol[0].val+1),2);
      for (i=0; i<6; i++)
        gainsf[i]=pow(gainsc[i],v)*65536.0;
      break;
    case 1:  // rvb high cut
      v=((irevvol[1].val+20)/70.0)*(44100.0/srate);
      lpfval=v*v*16777216.0;
      break;
    case 2:  // chr speed
      chrspeed=chrminspeed+pow(irevvol[2].val/50.0,3)*(chrmaxspeed-chrminspeed);
      break;
    case 3:  // chr depth
      chrdepth=(cllen-8)*(irevvol[3].val*1310.72);
      break;
    case 4:  // chr phase shift
      chrphase=irevvol[4].val*1310.72;
      break;
    case 5:  // chr feedback
      chrfb=irevvol[5].val*1092.2666;
      break;
  }
}


static void init(int rate, int stereo)
{
  initfail=running=0;

  srate=rate;
  st=stereo;

  chrminspeed=3355443/srate;   // 0.1hz
  chrmaxspeed=335544320/srate; // 10hz
  cllen=(srate/100)+8;       // 10msec max depth

  lcline=new long[cllen];
  if (!lcline)
  {
    initfail=1;
    return;
  }
  memsetd(lcline, 0, cllen);
  rcline=new long[cllen];
  if (!rcline)
  {
    delete lcline;
    initfail=1;
    return;
  }
  memsetd(rcline, 0, cllen);

  chrpos=0;
  clpos=0;

  // init reverb
  for (int i=0; i<6; i++)
  {
    llen[i]=(int) (delays[i]*rate/1000.0);
    lpos[i]=0;
    llpf[i]=rlpf[i]=0;

    leftl[i]=new long[llen[i]];
    if (leftl[i])
      memsetd(leftl[i], 0, llen[i]);
    else
    {
      for (int j=0; j<i; j++)
      {
        delete[] leftl[j];
        leftl[j]=0;

        if (st)
        {
          delete[] rightl[j];
          rightl[j]=0;
        }
      }

      delete lcline;
      delete rcline;
      initfail=1;
      return;
    }

    if (st)
    {
      rlen[i]=((delays[i]+(((float) rand()/16384.0)-1.0))*rate/1000.0);
      rpos[i]=0;

      rightl[i]=new long[rlen[i]];
      if (rightl[i])
        memsetd(rightl[i], 0, rlen[i]);
      else
      {
        for (int j=0; j<i; j++)
        {
          delete[] leftl[j];
          leftl[j]=0;

          delete[] rightl[j];
          rightl[j]=0;
        }

        delete[] leftl[i];
        leftl[i]=0;

        delete lcline;
        delete rcline;
        initfail=1;
        return;
      }
    }
    else
    {
      rightl[i]=0;
    }
  }

  // most of the compressor values aren't right, i'll update them when
  // kebby gets them running in the FLOAT version *mg*

  lpconst=(150.0/(float) srate)*(150.0/(float) srate)*4294967296.0;
  lpl=lpr=0;

  co1amode=0;
  co1dllen=srate/20;
  co1dline=new long[co1dllen];
  memsetd(co1dline,0,co1dllen);

  co1invlen=65536/co1dllen;
  co1dlpos=0;
  co1mean=0;

  co1atten=0;
  co1attack=0.0001;
  co1decay=0.0001;
  co1thres=0;
  co1cprv=1;


  for (int vv=0; vv<(sizeof(irevvol)/sizeof(ocpvolstruct)); vv++)
    updatevol(vv);

  running=1;
}



static void close()
{
  running=0;
  for (int i=0; i<6; i++)
  {
    if (leftl[i])
    {
      delete leftl[i];
      leftl[i]=0;
    }

    if (rightl[i])
    {
      delete rightl[i];
      rightl[i]=0;
    }
  }
  if (lcline) delete lcline;
  if (rcline) delete rcline;
  if (co1dline) delete co1dline;
}


int doreverb(int inp, long *lpos, long *lines[], long lpf[])
{
  long asum=0;

  inp>>=2;

  for (int i=0; i<4; i++)
  {
    long a=lpf[i]+=imulshr24(lpfval, (inp+imulshr16(gainsf[i], lines[i][lpos[i]])-lpf[i]));
    lines[i][lpos[i]]=a;
    asum+=a;
  }

  long           y1=lines[4][lpos[4]];
  long            z=imulshr16(gainsf[4], y1)+asum;
  lines[4][lpos[4]]=z;

  long           y2=lines[5][lpos[5]];
                  z=imulshr16(gainsf[5], y2)+y1-imulshr16(gainsf[4], z);
  lines[5][lpos[5]]=z;

  asum=y2-imulshr16(gainsf[5], z);

  return asum;
}

static void process(long *buf, int len, int rate, int stereo)
{
  if (initfail)
    return;

  long outgainr=mcpGet(0, mcpMasterReverb)<<9;
  if (outgainr>0)
  {
    if (stereo)
      for (int i=0; i<len; i++)
      {
        for (int j=0; j<6; j++)
        {
          if (++lpos[j]>=llen[j]) lpos[j]=0;
          if (++rpos[j]>=rlen[j]) rpos[j]=0;
        }
  
        long v1=buf[i*2];
        long v2=buf[i*2+1];
        lpl+=imulshr24(lpconst, (v1-(lpl>>8)));
        lpr+=imulshr24(lpconst, (v2-(lpr>>8)));

        // apply reverb
        buf[i*2]+=imulshr16(doreverb(v2-(lpr>>8), rpos, rightl, rlpf), outgainr);
        buf[i*2+1]+=imulshr16(doreverb(v1-(lpl>>8), lpos, leftl, llpf), outgainr);
      }
    else
      for (int i=0; i<len; i++)
      {
        for (int j=0; j<6; j++)
          if (++lpos[j]>=llen[j])
            lpos[j]=0;
  
        long v1=buf[i];
        lpl+=imulshr24(lpconst, (v1-(lpl>>8)));
        buf[i]+=imulshr16(doreverb(v1-(lpl>>8), lpos, leftl, llpf), outgainr);
      }
  }
}



static int pkey(unsigned short key)
{
  return 0;
}



static int revGetNumVolume()
{
  return sizeof(irevvol)/sizeof(ocpvolstruct);
}

static int revGetVolume(ocpvolstruct &v, int n)
{
  if (n==4 && !st)
    return 0;
  if (running && n<(sizeof(irevvol)/sizeof(ocpvolstruct)))
  {
    v=irevvol[n];
    return(!0);
  }
  return(0);
}

static int revSetVolume(ocpvolstruct &v, int n)
{
  if(n<(sizeof(irevvol)/sizeof(ocpvolstruct)))
  {
    irevvol[n]=v;
    updatevol(n);
    return(!0);
  }
  return(0);
}


extern "C"
{
  mixqpostprocregstruct iReverb={process, init, close};
  mixqpostprocaddregstruct iReverbAdd={pkey};
  ocpvolregstruct ivolrev={revGetNumVolume, revGetVolume, revSetVolume};
  char *dllinfo = "volregs _ivolrev;";
}

/*

  notizen dazu (von ryg):
    1. der reverbeffekt besteht aus 4 comb- und 2 allpassfiltern
       mit folgenden parametern:

       1. comb   gain: 0.966384599   delay: 29.68253968 ms
       2. comb   gain: 0.958186359   delay: 37.07482993 ms
       3. comb   gain: 0.953783929   delay: 41.06575964 ms
       4. comb   gain: 0.950933178   delay: 43.67346939 ms

       1. apass  gain: 0.994260075   delay:  4.98866213 ms
       2. apass  gain: 0.998044717   delay:  1.67800453 ms

    2. gains in fixedpoint sind dabei:

       1. comb   63333
       2. comb   62796
       3. comb   62507
       4. comb   62320

       1. apass  65160
       2. apass  65473

    3. originalmodul von totraum kriegt inputwerte vom tb 303-synthesizer,
       sprich im bereich -8000..8000 (d.h. inputsignal durch 4 teilen fr
       richtigen sound)

    4. in totraum sind weiterhin tb303 und reverberator getrennt, d.h. im
       reverbmodul wird tb-output nicht nochmal geaddet. das sollte man hier
       natuerlich tun :)
*/
