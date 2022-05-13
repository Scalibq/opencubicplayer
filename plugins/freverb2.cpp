// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>

// this shall somewhen become a cool reverb/compressor/EQ audio plug-in
// but i didnt have any time for it yet ;)
// (KB)

#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include "devwmixf.h"
#include "imsrtns.h"
#include "mcp.h"

extern "C"
{
  extern mixfpostprocregstruct fReverb;
  extern mixfpostprocaddregstruct fReverbAdd;
}


static int srate;
static int st;

extern "C"
{
  float *leftl[6];
  float *rightl[6];
  long   llen[6], lpos[6];

  int    initfail=0;
  float  gains[6]={0.966384599, -0.958186359, 0.953783929, -0.950933178,
                   0.994260075, -0.998044717};
  float  outgain=1.0;
  float  vf=0.75;

  void   process_reverb(float *buf, int len, int rate, int stereo);
  #pragma aux process_reverb "_*" parm [eax][ebx][ecx][edx];
};

static float delays[6]={29.68253968, 37.07482993, 41.06575964, 43.67346939,
                         4.98866213,  1.67800453};

static void init(int rate, int stereo)
{
  initfail=0;

  for (int i=0; i<6; i++)
  {
    if (leftl[i])
      delete[] leftl[i];
    if (rightl[i])
      delete[] rightl[i];
  }

  srate=rate;
  st=stereo;

  for (int i=0; i<6; i++)
  {
    llen[i]=(int) (delays[i]*rate/1000.0);

    leftl[i]=new float[llen[i]];
    if (leftl[i])
      memsetd(leftl[i], 0, llen[i]);
    else
      initfail=1;

    if (st)
    {
      rightl[i]=new float[llen[i]];
      if (rightl[i])
        memsetd(rightl[i], 0, llen[i]);
      else
        initfail=1;
    }
    else
    {
      rightl[i]=0;
    }

    lpos[i]=0;
  }
}



static void close()
{
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
}



static int pkey(unsigned short key)
{
  return 0;
}

static void process(float *b, int l, int r, int s)
{
  int outgainf=mcpGet(0, mcpMasterReverb);
  outgainf=(outgainf<0)?0:outgainf;

  if (!outgainf)
    return;

  outgain=(float) outgainf/63.0;
  vf=1.0-0.25*outgain;

  process_reverb(b, l, r, s);
}


extern "C"
{
  mixfpostprocregstruct fReverb2={process, init, close};
  mixfpostprocaddregstruct fReverb2Add={pkey};
}

/*
init         1 rvbctrl time, t1,t2,t3,t4,t5,t6;
rvbctrl
{
  time = 6.;
  t1 = exp (-6.9077553 * .0297 / time);
  t2 = exp (-6.9077553 * .0371 / time);
  t3 = exp (-6.9077553 * .0411 / time);
  t4 = exp (-6.9077553 * .0437 / time);
  t5 = exp (-6.9077553 * .005  / time);
  t6 = exp (-6.9077553 * .0017 / time);
}

table tab1[1309]; table tab2[1636]; table tab3[1812];
table tab4[1927]; table tab5[ 220]; table tab6[  74];

effect   44100 rvb      d1,d2,d3,d4, d5,d6,
                        a1,a2,a3,a4, y1,y2,
                        z, asum,
                        inp, outp;
rvb
{
  inp = tbwav.outp;

  d1 = d1 + 1 % 1308;  d2 = d2 + 1 % 1635;
  d3 = d3 + 1 % 1811;  d4 = d4 + 1 % 1926;

  a1 = inp + rvbctrl.t1 * tab1[d1]; tab1[d1] = a1;
  a2 = inp + rvbctrl.t2 * tab2[d2]; tab2[d2] = a2;
  a3 = inp + rvbctrl.t3 * tab3[d3]; tab3[d3] = a3;
  a4 = inp + rvbctrl.t4 * tab4[d4]; tab4[d4] = a4;

  asum =  a1 + a2 + a3 + a4;

  d5 = d5 + 1 %  220;  d6 = d6 + 1 %   74;

          y1 = tab5[d5];
          z  = rvbctrl.t5 * y1 + asum;
    tab5[d5] = z;

          y2 = tab6[d6];
          z  = rvbctrl.t6 * y2 + y1 - rvbctrl.t5 * z;
    tab6[d6] = z;

        asum = y2 - rvbctrl.t6 * z;

 outp = 800 * asum;
}

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
