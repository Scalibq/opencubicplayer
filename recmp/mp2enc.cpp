// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// MPEG audio layer 2 encoder
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <math.h>
#include "mpenccom.h"

struct sb_alloc
{
  unsigned int steps;
  unsigned int bits;
  unsigned int tbits;
  double aval;
  double snrval;
};

static sb_alloc _alloc[18];

static sb_alloc *atab0[]=
{
  &_alloc[17],&_alloc[0], &_alloc[3], &_alloc[4], &_alloc[5], &_alloc[6], &_alloc[7], &_alloc[8],
  &_alloc[9], &_alloc[10],&_alloc[11],&_alloc[12],&_alloc[13],&_alloc[14],&_alloc[15],&_alloc[16]
};

static sb_alloc *atab1[]=
{
  &_alloc[17],&_alloc[0], &_alloc[1], &_alloc[3], &_alloc[2], &_alloc[4], &_alloc[5], &_alloc[6],
  &_alloc[7], &_alloc[8], &_alloc[9], &_alloc[10],&_alloc[11],&_alloc[12],&_alloc[13],&_alloc[16]
};

static sb_alloc *atab2[]=
{
  &_alloc[17],&_alloc[0], &_alloc[1], &_alloc[3], &_alloc[2], &_alloc[4], &_alloc[5], &_alloc[16]
};

static sb_alloc *atab3[]=
{
  &_alloc[17],&_alloc[0], &_alloc[1], &_alloc[16]
};

static sb_alloc *atab4[]=
{
  &_alloc[17],&_alloc[0], &_alloc[1], &_alloc[2], &_alloc[4], &_alloc[5], &_alloc[6], &_alloc[7],
  &_alloc[8], &_alloc[9], &_alloc[10],&_alloc[11],&_alloc[12],&_alloc[13],&_alloc[14],&_alloc[15]
};

static int alloctablens[4]={27,30,8,12};
static sb_alloc **alloctabs[2][32]=
{
  {
    atab0, atab0, atab0, atab1, atab1, atab1, atab1, atab1, atab1, atab1,
    atab1, atab2, atab2, atab2, atab2, atab2, atab2, atab2, atab2, atab2,
    atab2, atab2, atab2, atab3, atab3, atab3, atab3, atab3, atab3, atab3
  },
  {
    atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4,
    atab4, atab4
  }
};
static int alloctabbits[2][32]=
{
  {4,4,4,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2},
  {4,4,3,3,3,3,3,3,3,3,3,3}
};

static int s_freq2[4] = {44100, 48000, 32000, 0};
static int bitrate2[15] = {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384};

static double snr[17] = {7.00, 11.00, 20.84, 16.00,
                         25.28, 31.59, 37.75, 43.84,
                         49.89, 55.93, 61.96, 67.98, 74.01,
                         80.03, 86.05, 92.01, 98.01};
static int sfsPerScfsi[] = { 20,14,8,14 };
static int pattern[5][5] = {0x123, 0x122, 0x122, 0x133, 0x123,
                            0x113, 0x111, 0x111, 0x444, 0x113,
                            0x111, 0x111, 0x111, 0x333, 0x113,
                            0x222, 0x222, 0x222, 0x333, 0x123,
                            0x123, 0x122, 0x122, 0x133, 0x123};

static double multiple2[64];
static double imultiple2[64];

static int carry;


//#define PSYCHO1

#ifdef PSYCHO1
void II_Psycho_One(short[2][1152], double[2][32], double[2][32], frame_params*);
#endif

int encode2(short *insamp,frame_params &fr_ps)
{
#ifdef PSYCHO1
  static double max_sc[2][32];
#endif
  static short buffer[2][1152];
  static double ltmin[2][32];
  static unsigned int bit_alloc[2][32];
  static unsigned int scalar[2][3][32];
  static float j_sample[3][12][32];
  static unsigned int j_scale[3][32];
  static unsigned int scfsi[2][32];
  static float sb_sample[3][12][2][32];
  static double mnr[2][32];
  static char used[2][32];
  int i,j,k,s;

  layer &info=fr_ps.header;

  int table;

  fr_ps.stereo = (fr_ps.header.mode == 3) ? 1 : 2;
  int br_per_ch = bitrate2[fr_ps.header.bitrate_index] / fr_ps.stereo;
  int ws = fr_ps.header.sampling_frequency;
  if ((ws == 1 && br_per_ch >= 56) ||
     (br_per_ch >= 56 && br_per_ch <= 80)) table = 0;
  else if (ws != 1 && br_per_ch >= 96) table = 1;
  else if (ws != 2 && br_per_ch <= 48) table = 2;
  else table = 3;

  sb_alloc ***alloc=alloctabs[table>>1];
  fr_ps.sblimit=alloctablens[table];
  int *allocbits=alloctabbits[table>>1];
  int jsbound = fr_ps.sblimit;

  int stereo=fr_ps.stereo;
  int sblimit = fr_ps.sblimit;

  for(j=0;j<1152;j++)
    for (i=0; i<stereo; i++)
      buffer[i][j] = insamp[stereo*j+i];

  int sfr=s_freq2[info.sampling_frequency];
  int whole_SpF=(144000*bitrate2[info.bitrate_index])/sfr;
  carry+=(144000*bitrate2[info.bitrate_index])%sfr;
  if (carry>=sfr)
  {
    carry-=sfr;
    info.padding=1;
  }
  else
    info.padding=0;
  int adb=(whole_SpF+info.padding) * 8;
  adb-=32+((info.error_protection)?16:0);

  for (i=0;i<3;i++)
    for (j=0;j<12;j++)
      for (k=0;k<stereo;k++)
        filter_subband(buffer[k]+j*32+i*32*12, sb_sample[i][j][k], k);

  for (k=0;k<stereo;k++)
    for (s=0;s<3;s++)
    {
      for (i=0;i<sblimit;i++)
      {
        double max = fabs(sb_sample[s][0][k][i]);
        for (j=1;j<12;j++)
          if (fabs(sb_sample[s][j][k][i]) > max)
            max = fabs(sb_sample[s][j][k][i]);
        for (j=64-2;j>0;j--)
          if (max <= multiple2[j])
            break;
        scalar[k][s][i]=j;
      }
    }

  if (fr_ps.actual_mode == 1)
  {
    for (k = 0; k<3; k++)
      for (j = 0; j<12; j++)
        for (i = 0; i<sblimit; i++)
          j_sample[k][j][i] = (sb_sample[k][j][0][i] + sb_sample[k][j][1][i])*0.5;

    for (s=0;s<3;s++)
    {
      for (i=0;i<sblimit;i++)
      {
        double max = fabs(j_sample[s][0][i]);
        for (j=1;j<12;j++)
          if (fabs(j_sample[s][j][i]) > max)
            max = fabs(j_sample[s][j][i]);
        for (j=64-2;j>0;j--)
          if (max <= multiple2[j])
            break;
        j_scale[s][i]=j;
      }
    }
  }

#ifdef PSYCHO1
  if (fr_ps.model == 1)
  {
    for (k=0;k<stereo;k++)
      for (i=0;i<sblimit;i++)
      {
        int max=0;
        for (j=0;j<3;j++)
          if (max > scalar[k][j][i])
             max = scalar[k][j][i];
        max_sc[k][i] = multiple2[max];
      }
    for (i=sblimit;i<32;i++)
      max_sc[0][i] = max_sc[1][i] = 1E-20;
    II_Psycho_One(buffer, max_sc, ltmin, &fr_ps);
  }
  else
#endif
    for (k=0;k<stereo;k++)
      psycho_anal(buffer[k], k, 2, ltmin[k]);

  for (k=0;k<stereo;k++)
    for (i=0;i<sblimit;i++)
    {
      int dclass[2];
      for (j=0;j<2;j++)
      {
        int dscf=(scalar[k][j][i]-scalar[k][j+1][i]);
        if (dscf<=-3)
          dclass[j]=0;
        else
        if (dscf>-3&&dscf<0)
          dclass[j]=1;
        else
        if (dscf==0)
          dclass[j]=2;
        else
        if (dscf>0 && dscf<3)
          dclass[j]=3;
        else
          dclass[j]=4;
      }
      switch (pattern[dclass[0]][dclass[1]])
      {
      case 0x123:
        scfsi[k][i] = 0;
        break;
      case 0x122:
        scfsi[k][i] = 3;
        scalar[k][2][i] = scalar[k][1][i];
        break;
      case 0x133:
        scfsi[k][i] = 3;
        scalar[k][1][i] = scalar[k][2][i];
        break;
      case 0x113:
        scfsi[k][i] = 1;
        scalar[k][1][i] = scalar[k][0][i];
        break;
      case 0x111:
        scfsi[k][i] = 2;
        scalar[k][1][i] = scalar[k][2][i] = scalar[k][0][i];
        break;
      case 0x222:
        scfsi[k][i] = 2;
        scalar[k][0][i] = scalar[k][2][i] = scalar[k][1][i];
        break;
      case 0x333:
        scfsi[k][i] = 2;
        scalar[k][0][i] = scalar[k][1][i] = scalar[k][2][i];
        break;
      case 0x444:
        scfsi[k][i] = 2;
        if (scalar[k][0][i] > scalar[k][2][i])
          scalar[k][0][i] = scalar[k][2][i];
        scalar[k][1][i] = scalar[k][2][i] = scalar[k][0][i];
        break;
      }
    }

  if (fr_ps.actual_mode==1)
  {
    int mode_ext=4;
    while (1)
    {
      if (mode_ext==4)
      {
        fr_ps.header.mode = 0;
        fr_ps.header.mode_ext = 0;
        jsbound=fr_ps.sblimit;
      }
      else
      {
        fr_ps.header.mode = 1;
        fr_ps.header.mode_ext = mode_ext;
        jsbound=4*(mode_ext+1);
      }
      if (mode_ext==0)
        break;

      int req_bits = 0;
      for (j=0; j<jsbound; j++)
        req_bits += ((j<jsbound)?stereo:1) * allocbits[j];

      for (j=0; j<sblimit; j++)
        for (k=0; k<((j<jsbound)?stereo:1); k++)
        {
          int maxAlloc = (1<<allocbits[j])-1;
          int ba;
          for (ba=0;ba<maxAlloc-1; ++ba)
            if (alloc[j][ba]->snrval>=ltmin[k][j])
              break;
          if (stereo == 2 && j >= jsbound)
            for (;ba<maxAlloc-1; ++ba)
              if (alloc[j][ba]->snrval>=ltmin[1][j])
                break;
          if (ba)
          {
            req_bits += alloc[j][ba]->tbits + sfsPerScfsi[scfsi[k][j]];
            if (stereo == 2 && j >= jsbound)
              req_bits += sfsPerScfsi[scfsi[1][j]];
          }
        }

      if (req_bits<=adb)
        break;
      mode_ext--;
    }
  }

  for (i=0; i<sblimit; i++)
    adb -= ((i<jsbound)?stereo:1) * allocbits[i];

  for (i=0;i<sblimit;i++)
    for (k=0;k<stereo;k++)
    {
      mnr[k][i]=-ltmin[k][i];
      bit_alloc[k][i] = 0;
      used[k][i] = 0;
    }

  while (1)
  {
    int small = 999999.0;
    int min_sb = -1;
    int min_ch = -1;
    for (i=0;i<sblimit;i++)
      for(k=0;k<stereo;++k)
        if (used[k][i]  != 2 && small > mnr[k][i])
        {
          small = mnr[k][i];
          min_sb = i;
          min_ch = k;
        }
    if (min_sb==-1)
      break;

    int increment = alloc[min_sb][bit_alloc[min_ch][min_sb]+1]->tbits;
    if (used[min_ch][min_sb])
      increment -= alloc[min_sb][bit_alloc[min_ch][min_sb]]->tbits;
    else
    {
      increment += sfsPerScfsi[scfsi[min_ch][min_sb]];
      if (stereo == 2 && min_sb >= jsbound)
        increment += sfsPerScfsi[scfsi[1-min_ch][min_sb]];
    }
    if (adb >= increment)
    {
      int ba = ++bit_alloc[min_ch][min_sb];
      adb-= increment;
      used[min_ch][min_sb] = 1;
      mnr[min_ch][min_sb] = alloc[min_sb][ba]->snrval-ltmin[min_ch][min_sb];
      if (ba >= (1<<allocbits[min_sb])-1)
        used[min_ch][min_sb] = 2;
      if (min_sb >= jsbound && stereo == 2)
      {
        bit_alloc[1-min_ch][min_sb] = bit_alloc[min_ch][min_sb];
        used[1-min_ch][min_sb] = used[min_ch][min_sb];
        mnr[1-min_ch][min_sb] = alloc[min_sb][ba]->snrval-ltmin[1-min_ch][min_sb];
      }
    }
    else
    {
      used[min_ch][min_sb] = 2;
      if(fr_ps.stereo == 2 && min_sb >= fr_ps.jsbound)
        used[1-min_ch][min_sb] = 2;
    }
  }

  unsigned int crc = 0xffff;
  update_CRC(info.bitrate_index, 4, &crc);
  update_CRC(info.sampling_frequency, 2, &crc);
  update_CRC(info.padding, 1, &crc);
  update_CRC(info.extension, 1, &crc);
  update_CRC(info.mode, 2, &crc);
  update_CRC(info.mode_ext, 2, &crc);
  update_CRC(info.copyright, 1, &crc);
  update_CRC(info.original, 1, &crc);
  update_CRC(info.emphasis, 2, &crc);

  for (i=0;i<sblimit;i++)
    for (k=0;k<((i<jsbound)?stereo:1);k++)
      update_CRC(bit_alloc[k][i], allocbits[i], &crc);

  for (i=0;i<sblimit;i++)
    for (k=0;k<stereo;k++)
      if (bit_alloc[k][i])
        update_CRC(scfsi[k][i], 2, &crc);

// output...

  putbits(0xfff,12);
  put1bit(1);
  putbits(4-info.lay,2);
  put1bit(!info.error_protection);
  putbits(info.bitrate_index,4);
  putbits(info.sampling_frequency,2);
  put1bit(info.padding);
  put1bit(info.extension);
  putbits(info.mode,2);
  putbits(info.mode_ext,2);
  put1bit(info.copyright);
  put1bit(info.original);
  putbits(info.emphasis,2);

  if (info.error_protection)
    putbits(crc, 16);

  for (i=0;i<sblimit;i++)
    for (k=0;k<((i<jsbound)?stereo:1);k++)
      putbits(bit_alloc[k][i],allocbits[i]);

  for (i=0;i<sblimit;i++)
    for (k=0;k<stereo;k++)
      if (bit_alloc[k][i])
        putbits(scfsi[k][i],2);

  for (i=0;i<sblimit;i++)
    for (k=0;k<stereo;k++)
      if (bit_alloc[k][i])
        switch (scfsi[k][i])
        {
        case 0:
          putbits(scalar[k][0][i],6);
          putbits(scalar[k][1][i],6);
          putbits(scalar[k][2][i],6);
          break;
        case 1:
        case 3:
          putbits(scalar[k][0][i],6);
          putbits(scalar[k][2][i],6);
          break;
        case 2:
          putbits(scalar[k][0][i],6);
          break;
        }

  for (s=0;s<3;s++)
    for (j=0;j<12;j+=3)
      for (i=0;i<sblimit;i++)
        for (k=0;k<((i<jsbound)?stereo:1);k++)
          if (bit_alloc[k][i])
          {
            sb_alloc *a=alloc[i][bit_alloc[k][i]];
            int q;
            int b[3];
            for (q=0; q<3; q++)
            {
              double d;
              if(stereo == 2 && i>=jsbound)
                d = j_sample[s][j+q][i] * imultiple2[j_scale[s][i]];
              else
                d = sb_sample[s][j+q][k][i] * imultiple2[scalar[k][s][i]];
              b[q] = (d+1) * a->aval;
            }
            if (!a->steps)
            {
              putbits(b[0], a->bits);
              putbits(b[1], a->bits);
              putbits(b[2], a->bits);
            }
            else
              putbits(b[0]+a->steps*(b[1]+a->steps*b[2]),a->bits);
          }

  for (i=0;i<adb;i++)
    put1bit(0);
  return 1;
}

void initencode2()
{
  int i;
  for (i=0; i<3; i++)
  {
    _alloc[i].steps=(2<<i)+1;
    _alloc[i].bits="\x05\x07\x0A"[i];
    _alloc[i].tbits=12*_alloc[i].bits;
    _alloc[i].aval=((2<<i)+1)/2.0;
    _alloc[i].snrval=snr[i];
  }
  for (i=3; i<17; i++)
  {
    _alloc[i].steps=0;
    _alloc[i].bits=i;
    _alloc[i].tbits=36*i;
    _alloc[i].aval=((1<<i)-1)/2.0;
    _alloc[i].snrval=snr[i];
  }
  for (i=0; i<64; i++)
    multiple2[i]=exp(log(2)*(3-i)/3.0);
  for (i=0; i<64; i++)
    imultiple2[i]=exp(log(2)*(i-3)/3.0);
  carry=0;
}
