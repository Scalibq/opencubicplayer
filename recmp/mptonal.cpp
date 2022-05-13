// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// MPEG audio psychoacoustic tonal encoding handlers
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

// DAMAGED!!!

#include "common.h"

static int bitrate1[15] = {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448};
static int bitrate2[15] = {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384};

#define CB_FRACTION     0.33
#define MAX_SNR         1000
#define NOISE           10
#define TONE            20
#define DBMIN           -200.0
#define LAST            -1
#define STOP            -100
#define POWERNORM       90.3090 /* = 20 * log10(32768) to normalize */
                                /* max output power to 96 dB per spec */

struct g_thres
{
  int line;
  double bark, hear, x;
};

struct mask
{
  double x;
  int type, next, map;
};

static int cbtablen[2][3]={{25,26,24},{27,27,25}};
static int cbtab[2][3][27]=
{
  {
    {  1,  2,  3,  5,  6,  8,  9, 11, 13, 15, 17, 20, 23, 27, 32, 37, 45, 52, 62, 74, 88,108,132,180,232},
    {  1,  2,  3,  4,  5,  6,  7,  9, 10, 12, 14, 16, 19, 21, 25, 29, 35, 41, 50, 58, 68, 82,100,124,164,216},
    {  1,  3,  5,  7,  9, 11, 13, 15, 18, 21, 24, 27, 32, 37, 44, 52, 62, 74, 88,104,124,148,184,240}
  },
  {
    {  1,  2,  3,  5,  7, 10, 13, 16, 19, 22, 26, 30, 35, 40, 46, 54, 64, 76, 90,104,124,148,176,216,264,360,464},
    {  1,  2,  3,  5,  7,  9, 12, 14, 17, 20, 24, 27, 32, 37, 42, 50, 58, 70, 82,100,116,136,164,200,248,328,432},
    {  1,  3,  6, 10, 13, 17, 21, 25, 30, 35, 41, 47, 54, 64, 74, 88,104,124,148,176,208,248,296,368,480}
  }
};
#include "thtab.h"


#define LONDON                  /* enable "LONDON" modification */
#define MAKE_SENSE              /* enable "MAKE_SENSE" modification */
#define MI_OPTION               /* enable "MI_OPTION" modification */
/**********************************************************************/
/*
/*        This module implements the psychoacoustic model I for the
/* MPEG encoder layer II. It uses simplified tonal and noise masking
/* threshold analysis to generate SMR for the encoder bit allocation
/* routine.
/*
/**********************************************************************/

static int crit_band;
static int *cbound;
static int sub_size;
static g_thres *ltg;

static void make_map(mask power[512])
{
 int i,j;

 for(i=1;i<sub_size;i++)
   for(j=ltg[i-1].line;j<=ltg[i].line;j++)
     power[j].map = i;
}

static double add_db(double a,double b)
{
 a = pow(10.0,a/10.0);
 b = pow(10.0,b/10.0);
 return 10 * log10(a+b);
}

/****************************************************************/
/*
/*        Fast Fourier transform of the input samples.
/*
/****************************************************************/

static void II_f_f_t(double sample[1024],mask power[512])      /* this function calculates an */
{
 int i,j,k,L,l=0;
 int ip, le, le1;
 double t_r, t_i, u_r, u_i;
 static int M, MM1, init = 0, N;
 static double x_r[1024], x_i[1024], energy[1024];
 static int rev[1024];
 static double w_r[10], w_i[10];

 for(i=0;i<1024;i++) x_r[i] = x_i[i] = energy[i] = 0;
 if(!init){
    M = 10;
    MM1 = 9;
    N = 1024;
    for(L=0;L<M;L++){
       le = 1 << (M-L);
       le1 = le >> 1;
       w_r[L] = cos(PI/le1);
       w_i[L] = -sin(PI/le1);
    }
    for(i=0;i<1024;rev[i] = l,i++) for(j=0,l=0;j<10;j++){
       k=(i>>j) & 1;
       l |= (k<<9-j);
    }
    init = 1;
 }
 memcpy( (char *) x_r, (char *) sample, sizeof(double) * 1024);
 for(L=0;L<MM1;L++){
    le = 1 << (M-L);
    le1 = le >> 1;
    u_r = 1;
    u_i = 0;
    for(j=0;j<le1;j++){
       for(i=j;i<N;i+=le){
          ip = i + le1;
          t_r = x_r[i] + x_r[ip];
          t_i = x_i[i] + x_i[ip];
          x_r[ip] = x_r[i] - x_r[ip];
          x_i[ip] = x_i[i] - x_i[ip];
          x_r[i] = t_r;
          x_i[i] = t_i;
          t_r = x_r[ip];
          x_r[ip] = x_r[ip] * u_r - x_i[ip] * u_i;
          x_i[ip] = x_i[ip] * u_r + t_r * u_i;
       }
       t_r = u_r;
       u_r = u_r * w_r[L] - u_i * w_i[L];
       u_i = u_i * w_r[L] + t_r * w_i[L];
    }
 }
 for(i=0;i<N;i+=2){
    ip = i + 1;
    t_r = x_r[i] + x_r[ip];
    t_i = x_i[i] + x_i[ip];
    x_r[ip] = x_r[i] - x_r[ip];
    x_i[ip] = x_i[i] - x_i[ip];
    x_r[i] = t_r;
    x_i[i] = t_i;
    energy[i] = x_r[i] * x_r[i] + x_i[i] * x_i[i];
 }
 for(i=0;i<1024;i++) if(i<rev[i]){
    t_r = energy[i];
    energy[i] = energy[rev[i]];
    energy[rev[i]] = t_r;
 }
 for(i=0;i<512;i++){    /* calculate power density spectrum */
    if (energy[i] < 1E-20) energy[i] = 1E-20;
    power[i].x = 10 * log10(energy[i]) + POWERNORM;
    power[i].next = STOP;
    power[i].type = 0;
 }
}

/****************************************************************/
/*
/*         Window the incoming audio signal.
/*
/****************************************************************/

static void II_hann_win(double sample[1024])          /* this function calculates a  */
{                                 /* samples for a 1024-pt. FFT  */
 register int i;
 register double sqrt_8_over_3;
 static int init = 0;
 static double window[1024];

 if(!init)
 {
    sqrt_8_over_3 = pow(8.0/3.0, 0.5);
    for(i=0;i<1024;i++){
       /* Hann window formula */
       window[i]=sqrt_8_over_3*0.5*(1-cos(2.0*PI*i/(1024)))/1024;
    }
    init = 1;
 }
 for(i=0;i<1024;i++) sample[i] *= window[i];
}

/*******************************************************************/
/*
/*        This function finds the maximum spectral component in each
/* subband and return them to the encoder for time-domain threshold
/* determination.
/*
/*******************************************************************/
#ifndef LONDON
static void II_pick_max(mask power[512], double spike[32])
{
 double max;
 int i,j;

 for(i=0;i<512;spike[i>>4] = max, i+=16)      /* calculate the      */
 for(j=0, max = DBMIN;j<16;j++)                    /* maximum spectral   */
    max = (max>power[i+j].x) ? max : power[i+j].x; /* component in each  */
}                                                  /* subband from bound */
                                                   /* 4-16               */
#else
static void II_pick_max(mask power[512], double spike[32])
{
 double sum;
 int i,j;

 for(i=0;i<512;spike[i>>4] = 10.0*log10(sum), i+=16)
                                                   /* calculate the      */
 for(j=0, sum = pow(10.0,0.1*DBMIN);j<16;j++)      /* sum of spectral   */
   sum += pow(10.0,0.1*power[i+j].x);              /* component in each  */
}                                                  /* subband from bound */
                                                   /* 4-16               */
#endif

/****************************************************************/
/*
/*        This function labels the tonal component in the power
/* spectrum.
/*
/****************************************************************/

static void II_tonal_label(mask power[512], int *tone)  /* this function extracts (tonal) */
{
 int i,j, last = LAST, first, run, last_but_one = LAST; /* dpwe */
 double max;

 *tone = LAST;
 for(i=2;i<512-12;i++){
    if(power[i].x>power[i-1].x && power[i].x>=power[i+1].x){
       power[i].type = TONE;
       power[i].next = LAST;
       if(last != LAST) power[last].next = i;
       else first = *tone = i;
       last = i;
    }
 }
 last = LAST;
 first = *tone;
 *tone = LAST;
 while(first != LAST){               /* the conditions for the tonal          */
    if(first<3 || first>500) run = 0;/* otherwise k+/-j will be out of bounds */
    else if(first<63) run = 2;       /* components in layer II, which         */
    else if(first<127) run = 3;      /* are the boundaries for calc.          */
    else if(first<255) run = 6;      /* the tonal components                  */
    else run = 12;
    max = power[first].x - 7;        /* after calculation of tonal   */
    for(j=2;j<=run;j++)              /* components, set to local max */
       if(max < power[first-j].x || max < power[first+j].x){
          power[first].type = 0;
          break;
       }
    if(power[first].type == TONE){   /* extract tonal components */
       int help=first;
       if(*tone==LAST) *tone = first;
       while((power[help].next!=LAST)&&(power[help].next-first)<=run)
          help=power[help].next;
       help=power[help].next;
       power[first].next=help;
       if((first-last)<=run){
          if(last_but_one != LAST) power[last_but_one].next=first;
       }
       if(first>1 && first<500){     /* calculate the sum of the */
          double tmp;                /* powers of the components */
          tmp = add_db(power[first-1].x, power[first+1].x);
          power[first].x = add_db(power[first].x, tmp);
       }
       for(j=1;j<=run;j++){
          power[first-j].x = power[first+j].x = DBMIN;
          power[first-j].next = power[first+j].next = STOP;
          power[first-j].type = power[first+j].type = 0;
       }
       last_but_one=last;
       last = first;
       first = power[first].next;
    }
    else {
       int ll;
       if(last == LAST); /* *tone = power[first].next; dpwe */
       else power[last].next = power[first].next;
       ll = first;
       first = power[first].next;
       power[ll].next = STOP;
    }
 }
}

/****************************************************************/
/*
/*        This function groups all the remaining non-tonal
/* spectral lines into critical band where they are replaced by
/* one single line.
/*
/****************************************************************/
        
static void noise_label(mask *power, int *noise)
{
 int i,j, centre, last = LAST;
 double index, weight, sum;
                              /* calculate the remaining spectral */
 for(i=0;i<crit_band-1;i++){  /* lines for non-tonal components   */
     for(j=cbound[i],weight = 0.0,sum = DBMIN;j<cbound[i+1];j++){
        if(power[j].type != TONE){
           if(power[j].x != DBMIN){
              sum = add_db(power[j].x,sum);
/* the line below and others under the "MAKE_SENSE" condition are an alternate
   interpretation of "geometric mean". This approach may make more sense but
   it has not been tested with hardware. */
#ifdef MAKE_SENSE
              weight += pow(10.0, power[j].x/10.0) * (ltg[power[j].map].bark-i);
#endif
              power[j].x = DBMIN;
           }
        }   /*  check to see if the spectral line is low dB, and if  */
     }      /* so replace the center of the critical band, which is */
            /* the center freq. of the noise component              */

#ifdef MAKE_SENSE
     if(sum <= DBMIN)  centre = (cbound[i+1]+cbound[i]) /2;
     else {
        index = weight/pow(10.0,sum/10.0);
        centre = cbound[i] + (int) (index * (double) (cbound[i+1]-cbound[i]) );
     }
#else
     index = (double)( ((double)cbound[i]) * ((double)(cbound[i+1]-1)) );
     centre = (int)(pow(index,0.5)+0.5);
#endif

    /* locate next non-tonal component until finished; */
    /* add to list of non-tonal components             */
#ifdef MI_OPTION
     /* Masahiro Iwadare's fix for infinite looping problem? */
     if(power[centre].type == TONE) 
       if (power[centre+1].type == TONE) centre++; else centre--;
#else
     /* Mike Li's fix for infinite looping problem */
     if(power[centre].type == 0) centre++;

     if(power[centre].type == NOISE){
       if(power[centre].x >= ltg[power[i].map].hear){
         if(sum >= ltg[power[i].map].hear) sum = add_db(power[j].x,sum);
         else
         sum = power[centre].x;
       }
     }
#endif
     if(last == LAST) *noise = centre;
     else {
        power[centre].next = LAST;
        power[last].next = centre;
     }
     power[centre].x = sum;
     power[centre].type = NOISE;        
     last = centre;
 }        
}

/****************************************************************/
/*
/*        This function reduces the number of noise and tonal
/* component for further threshold analysis.
/*
/****************************************************************/

static void subsampling(mask power[512], int *tone, int *noise)
{
 int i, old;

 i = *tone; old = STOP;    /* calculate tonal components for */
 while(i!=LAST){           /* reduction of spectral lines    */
    if(power[i].x < ltg[power[i].map].hear){
       power[i].type = 0;
       power[i].x = DBMIN;
       if(old == STOP) *tone = power[i].next;
       else power[old].next = power[i].next;
    }
    else old = i;
    i = power[i].next;
 }
 i = *noise; old = STOP;    /* calculate non-tonal components for */
 while(i!=LAST){            /* reduction of spectral lines        */
    if(power[i].x < ltg[power[i].map].hear){
       power[i].type = 0;
       power[i].x = DBMIN;
       if(old == STOP) *noise = power[i].next;
       else power[old].next = power[i].next;
    }
    else old = i;
    i = power[i].next;
 }
 i = *tone; old = STOP;
 while(i != LAST){                              /* if more than one */
    if(power[i].next == LAST)break;             /* tonal component  */
    if(ltg[power[power[i].next].map].bark -     /* is less than .5  */
       ltg[power[i].map].bark < 0.5) {          /* bark, take the   */
       if(power[power[i].next].x > power[i].x ){/* maximum          */
          if(old == STOP) *tone = power[i].next;
          else power[old].next = power[i].next;
          power[i].type = 0;
          power[i].x = DBMIN;
          i = power[i].next;
       }
       else {
          power[power[i].next].type = 0;
          power[power[i].next].x = DBMIN;
          power[i].next = power[power[i].next].next;
          old = i;
       }
    }
    else {
      old = i;
      i = power[i].next;
    }
 }
}

/****************************************************************/
/*
/*        This function calculates the individual threshold and
/* sum with the quiet threshold to find the global threshold.
/*
/****************************************************************/

static void threshold(mask power[512], int *tone, int *noise, int bit_rate)
{
 int k, t;
 double dz, tmps, vf;

 for(k=1;k<sub_size;k++){
    ltg[k].x = DBMIN;
    t = *tone;          /* calculate individual masking threshold for */
    while(t != LAST){   /* components in order to find the global     */
       if(ltg[k].bark-ltg[power[t].map].bark >= -3.0 && /*threshold (LTG)*/
          ltg[k].bark-ltg[power[t].map].bark <8.0){
          dz = ltg[k].bark-ltg[power[t].map].bark; /* distance of bark value*/
          tmps = -1.525-0.275*ltg[power[t].map].bark - 4.5 + power[t].x;
             /* masking function for lower & upper slopes */
          if(-3<=dz && dz<-1) vf = 17*(dz+1)-(0.4*power[t].x +6);
          else if(-1<=dz && dz<0) vf = (0.4 *power[t].x + 6) * dz;
          else if(0<=dz && dz<1) vf = (-17*dz);
          else if(1<=dz && dz<8) vf = -(dz-1) * (17-0.15 *power[t].x) - 17;
          tmps += vf;        
          ltg[k].x = add_db(ltg[k].x, tmps);
       }
       t = power[t].next;
    }

    t = *noise;        /* calculate individual masking threshold  */
    while(t != LAST){  /* for non-tonal components to find LTG    */
       if(ltg[k].bark-ltg[power[t].map].bark >= -3.0 &&
          ltg[k].bark-ltg[power[t].map].bark <8.0){
          dz = ltg[k].bark-ltg[power[t].map].bark; /* distance of bark value */
          tmps = -1.525-0.175*ltg[power[t].map].bark -0.5 + power[t].x;
             /* masking function for lower & upper slopes */
          if(-3<=dz && dz<-1) vf = 17*(dz+1)-(0.4*power[t].x +6);
          else if(-1<=dz && dz<0) vf = (0.4 *power[t].x + 6) * dz;
          else if(0<=dz && dz<1) vf = (-17*dz);
          else if(1<=dz && dz<8) vf = -(dz-1) * (17-0.15 *power[t].x) - 17;
          tmps += vf;
          ltg[k].x = add_db(ltg[k].x, tmps);
       }
       t = power[t].next;
    }
    if(bit_rate<96)ltg[k].x = add_db(ltg[k].hear, ltg[k].x);
    else ltg[k].x = add_db(ltg[k].hear-12.0, ltg[k].x);
 }
}

/****************************************************************/
/*
/*        This function finds the minimum masking threshold and
/* return the value to the encoder.
/*
/****************************************************************/

static void II_minimum_mask(double ltmin[32],int sblimit)
{
 double min;
 int i,j;

 j=1;
 for(i=0;i<sblimit;i++)
    if(j>=sub_size-1)                   /* check subband limit, and       */
       ltmin[i] = ltg[sub_size-1].hear; /* calculate the minimum masking  */
    else {                              /* level of LTMIN for each subband*/
       min = ltg[j].x;
       while(ltg[j].line>>4 == i && j < sub_size){
       if(min>ltg[j].x)  min = ltg[j].x;
       j++;
    }
    ltmin[i] = min;
 }
}

/*****************************************************************/
/*
/*        This procedure is called in musicin to pick out the
/* smaller of the scalefactor or threshold.
/*
/*****************************************************************/

static void II_smr(double ltmin[32], double spike[32], double scale[32], int sblimit)
{
 int i;
 double max;
                
 for(i=0;i<sblimit;i++){                     /* determine the signal   */
    max = 20 * log10(scale[i] * 32768) - 10; /* level for each subband */
    if(spike[i]>max) max = spike[i];         /* for the maximum scale  */
    max -= ltmin[i];                         /* factors                */
    ltmin[i] = max;
 }
}
        
/****************************************************************/
/*
/*        This procedure calls all the necessary functions to
/* complete the psychoacoustic analysis.
/*
/****************************************************************/

void II_Psycho_One(short buffer[2][1152], double scale[2][32], double ltmin[2][32], frame_params *fr_ps)
{
 layer *info = &fr_ps->header;
 int   stereo = fr_ps->stereo;
 int   sblimit = fr_ps->sblimit;
 int k,i, tone=0, noise=0;
 static char init = 0;
 static int off[2] = {256,256};
 static double sample[1024];
 double spike[2][32];
 static double fft_buf[2][1408];
 static mask power[512];

     /* call functions for critical boundaries, freq. */
 if(!init){  /* bands, bark values, and mapping */
    crit_band=cbtablen[1][info->sampling_frequency];
    cbound=cbtab[1][info->sampling_frequency];
    sub_size=thtablen[1][info->sampling_frequency];
    ltg=thtab[1][info->sampling_frequency];
    make_map(power);
    for (i=0;i<1408;i++) fft_buf[0][i] = fft_buf[1][i] = 0;
    init = 1;
 }
 for(k=0;k<stereo;k++){  /* check pcm input for 3 blocks of 384 samples */
    for(i=0;i<1152;i++) fft_buf[k][(i+off[k])%1408]= (double)buffer[k][i]/32768.0;
    for(i=0;i<1024;i++) sample[i] = fft_buf[k][(i+1216+off[k])%1408];
    off[k] += 1152;
    off[k] %= 1408;
                            /* call functions for windowing PCM samples,*/
    II_hann_win(sample);    /* location of spectral components in each  */
    for(i=0;i<512;i++) power[i].x = DBMIN;  /*subband with labeling*/
    II_f_f_t(sample, power);                     /*locate remaining non-*/
    II_pick_max(power, &spike[k][0]);            /*tonal sinusoidals,   */
    II_tonal_label(power, &tone);                /*reduce noise & tonal */
    noise_label(power, &noise);             /*components, find     */
    subsampling(power, &tone, &noise);      /*global & minimal     */
    threshold(power, &tone, &noise,         /*threshold, and sgnl- */
      bitrate2[info->bitrate_index]/stereo); /*to-mask ratio*/
    II_minimum_mask(&ltmin[k][0], sblimit);
    II_smr(&ltmin[k][0], &spike[k][0], &scale[k][0], sblimit);
 }
}

/**********************************************************************/
/*
/*        This module implements the psychoacoustic model I for the
/* MPEG encoder layer I. It uses simplified tonal and noise masking
/* threshold analysis to generate SMR for the encoder bit allocation
/* routine.
/*
/**********************************************************************/

/****************************************************************/
/*
/*        Fast Fourier transform of the input samples.
/*
/****************************************************************/

static void I_f_f_t(double sample[512],mask power[256])         /* this function calculates */
{
 int i,j,k,L,l=0;
 int ip, le, le1;
 double t_r, t_i, u_r, u_i;
 static int M, MM1, init = 0, N;
 static double x_r[512], x_i[512], energy[512];
 static int rev[512];
 static double w_r[9], w_i[9];

 for(i=0;i<512;i++) x_r[i] = x_i[i] = energy[i] = 0;
 if(!init){
    M = 9;
    MM1 = 8;
    N = 512;
    for(L=0;L<M;L++){
       le = 1 << (M-L);
       le1 = le >> 1;
       w_r[L] = cos(PI/le1);
       w_i[L] = -sin(PI/le1);
    }
    for(i=0;i<512;rev[i] = l,i++) for(j=0,l=0;j<9;j++){
       k=(i>>j) & 1;
       l |= (k<<8-j);                
    }
    init = 1;
 }
 memcpy( (char *) x_r, (char *) sample, sizeof(double) * 512);
 for(L=0;L<MM1;L++){
    le = 1 << (M-L);
    le1 = le >> 1;
    u_r = 1;
    u_i = 0;
    for(j=0;j<le1;j++){
       for(i=j;i<N;i+=le){
          ip = i + le1;
          t_r = x_r[i] + x_r[ip];
          t_i = x_i[i] + x_i[ip];
          x_r[ip] = x_r[i] - x_r[ip];
          x_i[ip] = x_i[i] - x_i[ip];
          x_r[i] = t_r;
          x_i[i] = t_i;
          t_r = x_r[ip];
          x_r[ip] = x_r[ip] * u_r - x_i[ip] * u_i;
          x_i[ip] = x_i[ip] * u_r + t_r * u_i;
       }
       t_r = u_r;
       u_r = u_r * w_r[L] - u_i * w_i[L];
       u_i = u_i * w_r[L] + t_r * w_i[L];
    }
 }
 for(i=0;i<N;i+=2){
    ip = i + 1;
    t_r = x_r[i] + x_r[ip];
    t_i = x_i[i] + x_i[ip];
    x_r[ip] = x_r[i] - x_r[ip];
    x_i[ip] = x_i[i] - x_i[ip];
    x_r[i] = t_r;
    x_i[i] = t_i;
    energy[i] = x_r[i] * x_r[i] + x_i[i] * x_i[i];
 }
 for(i=0;i<512;i++) if(i<rev[i]){
    t_r = energy[i];
    energy[i] = energy[rev[i]];
    energy[rev[i]] = t_r;
 }
 for(i=0;i<256;i++){                     /* calculate power  */
    if(energy[i] < 1E-20) energy[i] = 1E-20;    /* density spectrum */
       power[i].x = 10 * log10(energy[i]) + POWERNORM;
       power[i].next = STOP;
       power[i].type = 0;
 }
}

/****************************************************************/
/*
/*         Window the incoming audio signal.
/*
/****************************************************************/

static void I_hann_win(double sample[512])             /* this function calculates a  */
{                                   /* samples for a 512-pt. FFT   */
 register int i;
 register double sqrt_8_over_3;
 static int init = 0;
 static double window[512];

 if(!init){  /* calculate window function for the Fourier transform */
    sqrt_8_over_3 = pow(8.0/3.0, 0.5);
    for(i=0;i<512;i++){
      /* Hann window formula */
      window[i]=sqrt_8_over_3*0.5*(1-cos(2.0*PI*i/(512)))/(512);
    }
    init = 1;
 }
 for(i=0;i<512;i++) sample[i] *= window[i];
}

/*******************************************************************/
/*
/*        This function finds the maximum spectral component in each
/* subband and return them to the encoder for time-domain threshold
/* determination.
/*
/*******************************************************************/
#ifndef LONDON
static void I_pick_max(mask power[256], double spike[32])
{
 double max;
 int i,j;

 /* calculate the spectral component in each subband */
 for(i=0;i<256;spike[i>>3] = max, i+=8)
    for(j=0, max = DBMIN;j<8;j++) max = (max>power[i+j].x) ? max : power[i+j].x;
}
#else
static void I_pick_max(mask power[512], double spike[32])
{
 double sum;
 int i,j;

 for(i=0;i<256;spike[i>>3] = 10.0*log10(sum), i+=8)
                                                   /* calculate the      */
 for(j=0, sum = pow(10.0,0.1*DBMIN);j<8;j++)       /* sum of spectral   */
   sum += pow(10.0,0.1*power[i+j].x);              /* component in each  */
}                                                  /* subband from bound */
#endif
/****************************************************************/
/*
/*        This function labels the tonal component in the power
/* spectrum.
/*
/****************************************************************/

static void I_tonal_label(mask power[256], int *tone)     /* this function extracts   */
{
 int i,j, last = LAST, first, run;
 double max;
 int last_but_one= LAST;

 *tone = LAST;
 for(i=2;i<256-6;i++){
    if(power[i].x>power[i-1].x && power[i].x>=power[i+1].x){
       power[i].type = TONE;
       power[i].next = LAST;
       if(last != LAST) power[last].next = i;
       else first = *tone = i;
       last = i;
    }
 }
 last = LAST;
 first = *tone;
 *tone = LAST;
 while(first != LAST){                /* conditions for the tonal     */
    if(first<3 || first>250) run = 0; /* otherwise k+/-j will be out of bounds*/
    else if(first<63) run = 2;        /* components in layer I, which */
    else if(first<127) run = 3;       /* are the boundaries for calc.   */
    else run = 6;                     /* the tonal components          */
    max = power[first].x - 7;
    for(j=2;j<=run;j++)  /* after calc. of tonal components, set to loc.*/
       if(max < power[first-j].x || max < power[first+j].x){   /* max   */
          power[first].type = 0;
          break;
       }
    if(power[first].type == TONE){    /* extract tonal components */
       int help=first;
       if(*tone == LAST) *tone = first;
       while((power[help].next!=LAST)&&(power[help].next-first)<=run)
          help=power[help].next;
       help=power[help].next;
       power[first].next=help;
       if((first-last)<=run){
          if(last_but_one != LAST) power[last_but_one].next=first;
       }
       if(first>1 && first<255){     /* calculate the sum of the */
          double tmp;                /* powers of the components */
          tmp = add_db(power[first-1].x, power[first+1].x);
          power[first].x = add_db(power[first].x, tmp);
       }
       for(j=1;j<=run;j++){
          power[first-j].x = power[first+j].x = DBMIN;
          power[first-j].next = power[first+j].next = STOP; /*dpwe: 2nd was .x*/
          power[first-j].type = power[first+j].type = 0;
       }
       last_but_one=last;
       last = first;
       first = power[first].next;
    }
    else {
       int ll;
       if(last == LAST) ; /* *tone = power[first].next; dpwe */
       else power[last].next = power[first].next;
       ll = first;
       first = power[first].next;
       power[ll].next = STOP;
    }
 }
}

/****************************************************************/
/*
/*        This function finds the minimum masking threshold and
/* return the value to the encoder.
/*
/****************************************************************/

static void I_minimum_mask(double ltmin[32])
{
 double min;
 int i,j;

 j=1;
 for(i=0;i<32;i++)
    if(j>=sub_size-1)                   /* check subband limit, and       */
       ltmin[i] = ltg[sub_size-1].hear; /* calculate the minimum masking  */
    else {                              /* level of LTMIN for each subband*/
       min = ltg[j].x;
       while(ltg[j].line>>3 == i && j < sub_size){
          if (min>ltg[j].x)  min = ltg[j].x;
          j++;
       }
       ltmin[i] = min;
    }
}

/*****************************************************************/
/*
/*        This procedure is called in musicin to pick out the
/* smaller of the scalefactor or threshold.
/*
/*****************************************************************/

static void I_smr(double ltmin[32], double spike[32], double scale[32])
{
 int i;
 double max;

 for(i=0;i<32;i++){                      /* determine the signal   */
    max = 20 * log10(scale[i] * 32768) - 10;  /* level for each subband */
    if(spike[i]>max) max = spike[i];          /* for the scalefactor    */
    max -= ltmin[i];
    ltmin[i] = max;
 }
}

/****************************************************************/
/*
/*        This procedure calls all the necessary functions to
/* complete the psychoacoustic analysis.
/*
/****************************************************************/

void I_Psycho_One(short buffer[2][448], double scale[2][32], double ltmin[2][32], frame_params *fr_ps)
{
 int stereo = fr_ps->stereo;
 layer *info = &fr_ps->header;
 int k,i, tone=0, noise=0;
 static char init = 0;
 static int off[2] = {256,256};
 static double sample[512];
 double spike[2][32];
 static double fft_buf[2][640];
 static mask power[256];

 if(!init)
 {
    crit_band=cbtablen[0][info->sampling_frequency];
    cbound=cbtab[0][info->sampling_frequency];
    sub_size=thtablen[0][info->sampling_frequency];
    ltg=thtab[0][info->sampling_frequency];
    make_map(power);
    for(i=0;i<640;i++) fft_buf[0][i] = fft_buf[1][i] = 0;
    init = 1;
 }
 for(k=0;k<stereo;k++){    /* check PCM input for a block of */
    for(i=0;i<384;i++)     /* 384 samples for a 512-pt. FFT  */
       fft_buf[k][(i+off[k])%640]= (double) buffer[k][i]/32768.0;
    for(i=0;i<512;i++)
       sample[i] = fft_buf[k][(i+448+off[k])%640];
    off[k] += 384;
    off[k] %= 640;
                        /* call functions for windowing PCM samples,   */
    I_hann_win(sample); /* location of spectral components in each     */
    for(i=0;i<256;i++) power[i].x = DBMIN;   /* subband with    */
    I_f_f_t(sample, power);              /* labeling, locate remaining */
    I_pick_max(power, &spike[k][0]);     /* non-tonal sinusoidals,     */
    I_tonal_label(power, &tone);         /* reduce noise & tonal com., */
    noise_label(power, &noise);     /* find global & minimal      */
    subsampling(power, &tone, &noise);  /* threshold, and sgnl-   */
    threshold(power, &tone, &noise,     /* to-mask ratio          */
      bitrate1[info->bitrate_index]/stereo);
    I_minimum_mask(&ltmin[k][0]);
    I_smr(&ltmin[k][0], &spike[k][0], &scale[k][0]);
 }
}
