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
// -kb990601 Tammo Hinrichs <opencp@gmx.net>


#include <math.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include "devwmixf.h"
#include "imsrtns.h"
#include "mcp.h"
#include "vol.h"


static ocpvolstruct revvol[]={
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


static float *leftl[6], *rightl[6]; // delay lines
static long   llen[6], lpos[6];     // dline length/pos left
static long   rlen[6], rpos[6];     // same right
static float  rlpf[6], llpf[6];     // left/right comb filter LPFs
static float  lpfval=0.5;           // LPF freq value
static float  lpconst,lpl,lpr;      // reverb out hpf (1-lpf)

static float chrminspeed,chrmaxspeed; // chorus speed limits (0.1 - 10Hz)
static float chrspeed;                // chorus speed
static float chrpos;                  // chorus osc pos
static float chrphase;                // chorus l/r phase shift
static float chrdepth;                // chorus depth
static float chrfb;                   // chorus feedback
static float *lcline, *rcline;        // chorus delay lines
static int   cllen,clpos;             // dlines write length/pos

static float *co1dline;               // compr1 analyzer delay line
static int   co1dllen, co1dlpos;      // compr1 dl length/pos
static int   co1amode;                // compr1 analyzer mode (0:rms, 1:peak)
static float co1mean,co1invlen;
static float co1atten, co1attack, co1decay, co1thres, co1cprv;

static float gainsc[6]={0.966384599, 0.958186359, 0.953783929, 0.950933178,
                        0.994260075, 0.998044717};
static float delays[6]={29.68253968, 37.07482993, 41.06575964, 43.67346939,
                         4.98866213,  1.67800453};
static float gainsf[6]={0.966384599, 0.958186359, 0.953783929, 0.950933178,
                        0.994260075, 0.998044717};


static void updatevol(int n)
{
  int i;
  float v;
  switch(n)
  {
    case 0:  // rvb time
      v=pow(25.0/(revvol[0].val+1),2);
      for (i=0; i<6; i++)
        gainsf[i]=pow(gainsc[i],v)*((i&1)?-1:1);
      break;
    case 1:  // rvb high cut
      v=((revvol[1].val+20)/70.0)*(44100.0/srate);
      lpfval=v*v;
      break;
    case 2:  // chr speed
      chrspeed=chrminspeed+pow(revvol[2].val/50.0,3)*(chrmaxspeed-chrminspeed);
      break;
    case 3:  // chr depth
      chrdepth=(float)(cllen-8)*(revvol[3].val/50.0);
      break;
    case 4:  // chr phase shift
      chrphase=revvol[4].val/50.0;
      break;
    case 5:  // chr feedback
      chrfb=revvol[5].val/60.0;
      break;
  }
}


static void init(int rate, int stereo)
{
  initfail=running=0;

  srate=rate;
  st=stereo;

  chrminspeed=0.2/srate;  // 0.1hz
  chrmaxspeed=20/srate;   // 10hz
  cllen=(srate/100)+8;    // 10msec max depth

  lcline=new float[cllen];
  if (!lcline)
  {
    initfail=1;
    return;
  }
  memsetd(lcline, 0, cllen);
  rcline=new float[cllen];
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

    leftl[i]=new float[llen[i]];
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

      rightl[i]=new float[rlen[i]];
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

  lpconst=(150.0/srate)*(150.0/srate);
  lpl=lpr=0;

  co1amode=0;
  co1dllen=srate/20;
  co1dline=new float[co1dllen];
  memsetd(co1dline,0,co1dllen);

  co1invlen=1.0/co1dllen;
  co1dlpos=0;
  co1mean=0;

  co1atten=0;
  co1attack=0.0001;
  co1decay=0.0001;
  co1thres=0;
  co1cprv=1;


  for (int vv=0; vv<(sizeof(revvol)/sizeof(ocpvolstruct)); vv++)
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


inline float doreverb(float inp, long *lpos, float *lines[], float lpf[])
{
  float asum=0;
  inp*=0.25;

  for (int i=0; i<4; i++)
  {
    float a=lpf[i]+=lpfval*(inp+gainsf[i]*lines[i][lpos[i]]-lpf[i]);
    lines[i][lpos[i]]=a;
    asum+=a;
  }

  float          y1=lines[4][lpos[4]];
  float           z=gainsf[4]*y1+asum;
  lines[4][lpos[4]]=z;

  float          y2=lines[5][lpos[5]];
                  z=gainsf[5]*y2+y1-gainsf[4]*z;
  lines[5][lpos[5]]=z;

  asum=y2-gainsf[5]*z;

  return asum;
}

static void process(float *buf, int len, int rate, int stereo)
{
  if (initfail)
    return;

  // THE CHORUS
  float outgainc=mcpGet(0, mcpMasterChorus)/64.0;
  if (outgainc>0)
  {
    if (st)
      for (int i=0; i<len; i++)
      {
        float v1=buf[i*2];
        float v2=buf[i*2+1];

        // update LFO and get l/r delays (0-1)
        chrpos+=chrspeed;
        if (chrpos>=2) chrpos-=2;
        float chrpos1=chrpos;
        if (chrpos1>1) chrpos1=2-chrpos1;
        float chrpos2=chrpos+chrphase;
        if (chrpos2>=2) chrpos2-=2;
        if (chrpos2>1) chrpos2=2-chrpos2;

        // get integer+fractional part of left delay
        chrpos1*=chrdepth;
        int readpos1=chrpos1+clpos;
        if  (readpos1>=cllen) readpos1-=cllen;
        chrpos1-=(int)chrpos1;
        int rpp1=(readpos1<cllen-1)?readpos1+1:0;

        // get integer+fractional part of right delay
        chrpos2*=chrdepth;
        int readpos2=chrpos2+clpos;
        if  (readpos2>=cllen) readpos2-=cllen;
        chrpos2-=(int)chrpos2;
        int rpp2=(readpos2<cllen-1)?readpos2+1:0;

        // now: readposx: integer pos,
        //      rppx:     integer pos+1,
        //      chrposx:  fractional pos

        // determine chorus output
        float lout=lcline[readpos1]+chrpos1*(lcline[rpp1]-lcline[readpos1]);
        float rout=rcline[readpos2]+chrpos2*(rcline[rpp2]-rcline[readpos2]);

        // mix chorus with original buffer
        buf[i*2]=v1+outgainc*(lout-v1);
        buf[i*2+1]=v2+outgainc*(rout-v2);

        // update delay lines and do feedback
        lcline[clpos]=v1-chrfb*lout;
        rcline[clpos]=v2-chrfb*rout;
        clpos=clpos?clpos-1:cllen-1;
      }
    else
      for (int i=0; i<len; i++)
      {
        float v1=buf[i];

        chrpos+=chrspeed;
        if (chrpos>=2) chrpos-=2;
        float chrpos1=chrpos;
        if (chrpos1>1) chrpos1=2-chrpos1;

        chrpos1*=chrdepth;
        int readpos1=chrpos1+clpos;
        if  (readpos1>=cllen) readpos1-=cllen;
        chrpos1-=(int)chrpos1;
        int rpp1=(readpos1<cllen-1)?readpos1+1:0;

        float lout=lcline[readpos1]+chrpos1*(lcline[rpp1]-lcline[readpos1]);

        buf[i]=v1+outgainc*(lout-v1);
        lcline[clpos]=v1-chrfb*lout;

        clpos=clpos?clpos-1:cllen-1;
      }
  }

  float invlog2=6/log(2);

/*
  // THE COMPRESSOR I
  if (co1amode)
  { // peak mode
  }
  else
  { // rms mode
    if (st)
    {
      for (int i=0; i<len; i++)
      {
        co1mean-=co1dline[co1dlpos];
        float v=(buf[2*i]+buf[2*i+1])/65536.0;
        co1dline[co1dlpos]=v*v;
        co1mean+=v*v;
        float co1out=sqrt(co1mean*co1invlen)*2.0;
        float co1db=log(co1out)*invlog2-co1thres;
        if (co1db<-40) co1db=-40;
        if (co1db>40) co1db=40;
        revvol[6].val=co1db+40;

        float dstatten=(co1db>0)?co1db*(1-co1cprv):0;

        co1atten+=((dstatten>co1atten)?co1attack:co1decay)*(dstatten-co1atten);

        //co1atten+=0.0002*(dstatten-co1atten);

        revvol[7].val=co1atten;

        // ok, now apply the gain
        float gain=pow(0.5,co1atten/6.0);
        buf[2*i]*=gain;
        buf[2*i+1]*=gain;
        

        co1dlpos++;
        if (co1dlpos==co1dllen) co1dlpos=0;
      }
    }

  }
*/


  // THE REVERB
  float outgainr=mcpGet(0, mcpMasterReverb)/128.0;
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
  
        float v1=buf[i*2];
        float v2=buf[i*2+1];
        lpl+=lpconst*(v1-lpl);
        lpr+=lpconst*(v2-lpr);
  
        // apply reverb
        buf[i*2]+=doreverb(v2-lpr, rpos, rightl, rlpf)*outgainr;
        buf[i*2+1]+=doreverb(v1-lpl, lpos, leftl, llpf)*outgainr;
      }
    else
      for (int i=0; i<len; i++)
      {
        for (int j=0; j<6; j++)
          if (++lpos[j]>=llen[j])
            lpos[j]=0;
  
        float v1=buf[i];
        lpl+=lpconst*(v1-lpl);
        buf[i]+=doreverb(v1-lpl, lpos, leftl, llpf)*outgainr;
      }
  }
}



static int pkey(unsigned short key)
{
  return 0;
}



static int revGetNumVolume()
{
  return sizeof(revvol)/sizeof(ocpvolstruct);
}

static int revGetVolume(ocpvolstruct &v, int n)
{
  if (n==4 && !st)
    return 0;
  if (running && n<(sizeof(revvol)/sizeof(ocpvolstruct)))
  {
    v=revvol[n];
    return(!0);
  }
  return(0);
}

static int revSetVolume(ocpvolstruct &v, int n)
{
  if(n<(sizeof(revvol)/sizeof(ocpvolstruct)))
  {
    revvol[n]=v;
    updatevol(n);
    return(!0);
  }
  return(0);
}


extern "C"
{
  mixfpostprocregstruct fReverb={process, init, close};
  mixfpostprocaddregstruct fReverbAdd={pkey};
  ocpvolregstruct volrev={revGetNumVolume, revGetVolume, revSetVolume};
  char *dllinfo = "volregs _volrev;";
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

