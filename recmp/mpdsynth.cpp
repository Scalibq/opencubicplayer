// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// MPEG audio encoder multiband synthesizer
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <math.h>

#define PI 3.14159265358979

static double cint[8]={63.99798583,-127.9999654,127.5192785,-120.2337705,90.5125788,-43.89676134,11.08139218,-0.989464311};

void ifdct(float *ou, float *in);
void initifdct();

static float xbuf[2][512];
static float ctab[1024];
static xbufoff[2];

void filter_subband(short *buffer, float *s, int k)
{
  int i,j;
  float y[32];

  int ok=xbufoff[k];
  int o0=((ok&32)<<3)+(ok>>6);
  float *xp=xbuf[k]+o0+32*8;
  for (i=0;i<32;i++)
    *(xp-=8)=*buffer++;
  for (i=0;i<32;i++)
  {
    int v;
    float *cp,*xp;
    register double t=0;
    v=16+i;
    cp=ctab+v*16+8-(((v+ok)>>6)&7);
    xp=xbuf[k]+(((v+ok)&63)<<3);
    for (j=0;j<8;j++)
      t+=*xp++**cp++;
    v=(16-i)&63;
    cp=ctab+v*16+8-(((v+ok)>>6)&7);
    xp=xbuf[k]+(((v+ok)&63)<<3);
    for (j=0;j<8;j++)
      t+=*xp++**cp++;
    y[i]=t;
  }
  ifdct(s,y);

  xbufoff[k] = (xbufoff[k]-32+512)&511;
}

void initfiltersubband()
{
  int i,j;
  initifdct();
  for (i=0; i<1024; i++)
  {
    double v=0;
    for (j=0; j<8; j++)
      v+=cos(2*PI/512*(i&511)*j)*cint[j]/(32768*32768.0);
    ctab[((i&63)<<4)|(i>>6)]=v*((i&64)?-1:1)*(((i&48)==48)?-1:1);
  }
  for (i=0;i<2;i++)
    for (j=0;j<512;j++)
      xbuf[i][j] = 0;
  xbufoff[0]=xbufoff[1]=0;
}
