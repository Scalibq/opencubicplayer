// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// MPEG audio psychoacoustic encoding handlers
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <math.h>
#include "mppsytab.h"

#define PI 3.14159265358979

static double   crit_band[27] = {0,  100,  200, 300, 400, 510, 630,  770,
                               920, 1080, 1270,1480,1720,2000,2320, 2700,
                              3150, 3700, 4400,5300,6400,7700,9500,12000,
                             15500,25000,30000};
static double   bmax[27] = {14.5, 14.5, 14.5, 14.5, 14.5, 11.5,  9.5,
                             4.5,  1.5, -1.1,   -1,   -1,   -1,   -1,
                              -1,   -1,   -1,   -1,   -1,   -1,   -1,
                              -1,   -1,   -1,   -2,   -2,   -2};
static int s_freq[3] = {44100, 48000, 32000};

static int reorder[513];
static float w_ri[10][2];
static double window[1024];

static double *absthr;
static double fthr[513];
static int numlines[63];
static double cbval[63];
static int partition[513];
static double stab[63][63];
static double rnorm[63];
static double tmn[63];
static double bcmax[63];
static double bcexp[63];
static double bcfac[63];

static double lthr[2][513];
static short savebuf[2][1056];
static float rphi_sav[2][513][2][2];

double fatan2(double,double);
#pragma aux fatan2 parm [8087] [8087] value [8087] modify [8087] = "fpatan"

void psycho_anal(short int *buffer,int chn, int lay, double *snr32)
{
  int i,j,k;
  double temp1, temp2, temp3;

  static double grouped_c[63];
  static double grouped_e[63];
  static double band_e[32];
  static double nb[63];
  static float wsamp[1024][2];
  static double snrtmp[2][32];

  for (i=0; i<lay; i++)
  {
    if (lay==1)
    {
      for (j=0; j<576; j++)
        savebuf[chn][j]=savebuf[chn][j+384];
      for (j=576; j<1024; j++)
        savebuf[chn][j]=*buffer++;
    }
    else
    {
      for (j=0; j<480; j++)
        savebuf[chn][j]=savebuf[chn][j+576];
      for (j=480; j<1056; j++)
        savebuf[chn][j]=*buffer++;
    }
    for (j=0; j<1024; j++)
    {
      wsamp[j][0] = window[j]*savebuf[chn][j];
      wsamp[j][1] = 0;
    }

    for (k=0; k<10; k++)
    {
      int le1=512>>k;
      float w[2];
      w[0]=1;
      w[1]=0;
      for (j=0; j<le1; j++)
      {
        float tmp[2];
        float *cx;
        for (cx=wsamp[j]; cx<wsamp[1024]; cx+=4*le1)
        {
          float *cxp=cx+2*le1;
          tmp[0]=cx[0]-cxp[0];
          tmp[1]=cx[1]-cxp[1];
          cx[0]=cx[0]+cxp[0];
          cx[1]=cx[1]+cxp[1];
          cxp[0]=tmp[0]*w[0]-tmp[1]*w[1];
          cxp[1]=tmp[1]*w[0]+tmp[0]*w[1];
        }
        tmp[0]=w[0];
        w[0]=w[0]*w_ri[k][0]-w[1]*w_ri[k][1];
        w[1]=w[1]*w_ri[k][0]+tmp[0]*w_ri[k][1];
      }
    }

    for (j=0;j<63;j++)
    {
      grouped_e[j] = 0;
      grouped_c[j] = 0;
    }
    for (j=0; j<32; j++)
      band_e[j]=0;
    float (*rphi)[2]=rphi_sav[chn][0];
    for (j=0; j<=512; j++)
    {
      float *vp=wsamp[reorder[j]];
      float eng=vp[0]*vp[0]+vp[1]*vp[1];
      float amp=sqrt(eng);

      float rphi_prime[2];
      rphi_prime[0]=2.0 * rphi[0][0] - rphi[1][0];
      rphi_prime[1]=2.0 * rphi[0][1] - rphi[1][1];
      rphi[0][0] = rphi[1][0];
      rphi[0][1] = rphi[1][1];
      rphi[1][0] = amp;
      rphi[1][1] = fatan2(wsamp[j][1], wsamp[j][0]);
      temp1=vp[0] - rphi_prime[0] * cos(rphi_prime[1]);
      temp2=vp[1] - rphi_prime[0] * sin(rphi_prime[1]);
      temp3=amp + fabs(rphi_prime[0]);

      double c;
      if (temp3 != 0)
        c=sqrt(temp1*temp1+temp2*temp2)/temp3;
      else
        c = 0;
      grouped_e[partition[j]] += eng;
      grouped_c[partition[j]] += eng*c;
      if (j&&!(j&15))
        band_e[(j>>4)-1]+=eng;
      if (j<512)
        band_e[j>>4]+=eng;
      rphi+=2;
    }
    for (j=0;j<63;j++)
    {
      double ecb = 0;
      double cb = 0;
      for (k=0;k<63;k++)
        if (stab[j][k]!=0)
        {
          ecb += stab[j][k]*grouped_e[k];
          cb += stab[j][k]*grouped_c[k];
        }
      if (ecb!=0)
        cb = cb/ecb;
      else
        cb = 0;
      if (cb<0.05)
        cb=0.05;
      else
      if (cb>0.5)
        cb=0.5;
      cb = exp(bcexp[j]*log(cb))*bcfac[j];
      cb = (cb < bcmax[j]) ? cb : bcmax[j];
      nb[j] = cb*ecb;
    }
    if (lay==1)
      for (j=0;j<=512;j++)
      {
        double tmp=nb[partition[j]];
        tmp=(tmp>absthr[j])?tmp:absthr[j];
        fthr[j] = (tmp < lthr[chn][j]) ? tmp : lthr[chn][j];
        double tmp2 = tmp * 0.00316;
        fthr[j] = (tmp2 > fthr[j]) ? tmp2 : fthr[j];
        lthr[chn][j] = 32.0*tmp;
      }
    else
      for (j=0;j<=512;j++)
        fthr[j]=(nb[partition[j]]>absthr[j])?nb[partition[j]]:absthr[j];

    for (j=0;j<13;j++)
    {
      double minthres=60802371420160.0;
      for (k=0;k<=16;k++)
        if (minthres>fthr[j*16+k])
          minthres=fthr[j*16+k];
      snrtmp[i][j]=4.342944819*log(band_e[j]/(minthres*17));
    }
    for (j=13;j<32;j++)
    {
      double minthres=0;
      for (k=0;k<=16;k++)
        minthres+=fthr[j*16+k];
      snrtmp[i][j]=4.342944819*log(band_e[j]/minthres);
    }
  }
  if (lay==2)
    for (i=0; i<32; i++)
      snr32[i]=(snrtmp[0][i]>snrtmp[1][i])?snrtmp[0][i]:snrtmp[1][i];
  else
    for (i=0; i<32; i++)
      snr32[i]=snrtmp[0][i];
}





void initpsychoanal(int sfreq_idx)
{
  int i,j,k;
// consts
  j=0;
  for (i=0; i<=512; i++)
  {
    reorder[i]=j;
    k=512;
    while(k<=j)
    {
      j = j-k;
      k = k >> 1;
    }
    j = j+k;
  }
  for (i=0;i<1024;i++)
    window[i]=0.5*(1-cos(2.0*PI*(i-0.5)/1024));
  for (i=0; i<10; i++)
  {
    w_ri[i][0] = cos(PI/(1<<(9-i)));
    w_ri[i][1] = -sin(PI/(1<<(9-i)));
  }

// const per stream
  absthr=absthrx[sfreq_idx];
  double freq_mult=s_freq[sfreq_idx]/1024.0;
  for (i=0;i<=512;i++)
  {
    double temp1 = i*freq_mult;
    j = 1;
    while(temp1>crit_band[j])
      j++;
    fthr[i]=j-1+(temp1-crit_band[j-1])/(crit_band[j]-crit_band[j-1]);
  }
  partition[0] = 0;
  int itemp2 = 1;
  cbval[0]=fthr[0];
  double bval_lo=fthr[0];
  for (i=1;i<=512;i++)
  {
    if ((fthr[i]-bval_lo)>0.33)
    {
      partition[i]=partition[i-1]+1;
      cbval[partition[i-1]] = cbval[partition[i-1]]/itemp2;
      cbval[partition[i]] = fthr[i];
      bval_lo = fthr[i];
      numlines[partition[i-1]] = itemp2;
      itemp2 = 1;
    }
    else
    {
      partition[i]=partition[i-1];
      cbval[partition[i]] += fthr[i];
      itemp2++;
    }
  }
  numlines[partition[i-1]] = itemp2;
  cbval[partition[i-1]] = cbval[partition[i-1]]/itemp2;
  for (j=0;j<63;j++)
  {
    for (i=0;i<63;i++)
    {
      double temp1 = (cbval[i] - cbval[j])*1.05;
      double temp2;
      if (temp1>=0.5 && temp1<=2.5)
      {
        temp2 = temp1 - 0.5;
        temp2 = 8.0 * (temp2*temp2 - 2.0 * temp2);
      }
      else
        temp2 = 0;
      temp1 += 0.474;
      double temp3 = 15.811389+7.5*temp1-17.5*sqrt(1.0+temp1*temp1);
      if (temp3 <= -100)
        stab[i][j] = 0;
      else
        stab[i][j] = exp((temp2 + temp3)*0.2302585093);
    }
  }
  for (j=0;j<63;j++)
  {
    double temp1 = 15.5 + cbval[j];
    tmn[j] = ((temp1>24.5) ? temp1 : 24.5)-5.5;
    rnorm[j] = 0;
    for (i=0;i<63;i++)
      rnorm[j] += stab[j][i];
    rnorm[j]*=numlines[j];
    if (rnorm[j])
      rnorm[j]=1/rnorm[j];
    else
      rnorm[j]=0;
    rnorm[j]*=0.281838293;
  }
  for (j=0; j<63; j++)
    bcmax[j]=exp(-bmax[(int)floor(cbval[j]+0.5)]*0.2302585093)*rnorm[j];
  for (j=0; j<63; j++)
    bcexp[j]=tmn[j]*0.434294482*0.2302585093;
  for (j=0; j<63; j++)
    bcfac[j]=exp(tmn[j]*0.301029996*0.2302585093)*rnorm[j];

// buffers
  for (i=0; i<1056; i++)
    savebuf[0][i]=savebuf[1][i];
  for (i=0;i<=512;i++)
  {
    rphi_sav[0][i][0][0]=rphi_sav[1][i][0][0]=rphi_sav[0][i][1][0]=rphi_sav[1][i][1][0]=0;
    rphi_sav[0][i][0][1]=rphi_sav[1][i][0][1]=rphi_sav[0][i][1][1]=rphi_sav[1][i][1][1]=0;
    lthr[0][i] = lthr[1][i] = 60802371420160.0;
  }
}
