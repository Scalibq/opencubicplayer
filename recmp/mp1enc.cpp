// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// MPEG audio layer 1 encoder
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <math.h>
#include "mpenccom.h"

//#define PSYCHO1

#ifdef PSYCHO1
extern void I_Psycho_One(short[2][448], double[2][32], double[2][32], frame_params*);
#endif

static double multiple1[64];
static double imultiple1[64];
static int s_freq1[4] = {44100, 48000, 32000, 0};
static int bitrate1[15] = {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448};
static double snr[16] = {0.00, 7.00, 16.00, 25.28, 31.59, 37.75, 43.84, 49.89, 55.93, 61.96, 67.98, 74.01, 80.03, 86.05, 92.01, 98.01};
static double atab[14];

static short buffer[2][448];
static int carry;

int encode1(short *insamp,frame_params &fr_ps)
{
  static double ltmin[2][32];
  static unsigned int bit_alloc[2][32];
  static unsigned int scalar[2][32];
  static unsigned int j_scale[32];
  static float sb_sample[12][2][32];
  static double j_sample[12][32];
  static double mnr[2][32];
  static char used[2][32];
  layer &info=fr_ps.header;

  fr_ps.stereo = (fr_ps.header.mode == 3) ? 1 : 2;
  int stereo=fr_ps.stereo;

  int i,j,k;

  for(j=0;j<64;j++)
    for (i=0; i<stereo; i++)
      buffer[i][j] = buffer[i][j+384];
  for(j=0;j<384;j++)
    for (i=0; i<stereo; i++)
      buffer[i][j+64] = insamp[stereo*j+i];

  int sfr=s_freq1[info.sampling_frequency];
  int whole_SpF=(12000*bitrate1[info.bitrate_index])/sfr;
  carry+=(12000*bitrate1[info.bitrate_index])%sfr;
  if (carry>=sfr)
  {
    carry-=sfr;
    info.padding=1;
  }
  else
    info.padding=0;
  int adb=(whole_SpF+info.padding)*32;
  adb-=32+((info.error_protection)?16:0);

  for (j=0;j<12;j++)
    for (k=0;k<stereo;k++)
      filter_subband(buffer[k]+32*j, sb_sample[j][k], k);

  for (k=0;k<stereo;k++)
    for (i=0;i<32;i++)
    {
      double s=fabs(sb_sample[0][k][i]);
      for (j=1;j<12;j++)
        if (fabs(sb_sample[j][k][i]) > s)
          s = fabs(sb_sample[j][k][i]);
      for (j=64-2,scalar[k][i]=0;j>=0;j--)
        if (s <= multiple1[j])
        {
          scalar[k][i] = j;
          break;
        }
    }
  if (fr_ps.actual_mode == 1)
  {
    for (i=0; i<32; i++)
      for (j=0; j<12; j++)
        j_sample[j][i] = 0.5 * (sb_sample[j][0][i] + sb_sample[j][1][i]);
    for (i=0;i<32;i++)
    {
      double s=fabs(j_sample[0][i]);
      for (j=1;j<12;j++)
        if (fabs(j_sample[j][i]) > s)
          s = fabs(j_sample[j][i]);
      for (j=64-2,j_scale[i]=0;j>=0;j--)
        if (s <= multiple1[j])
        {
          j_scale[i] = j;
          break;
        }
    }
  }

  for (k=0;k<stereo;k++)
    psycho_anal(buffer[k], k, 1, ltmin[k]);

  if (fr_ps.actual_mode==1)
  {
    int mode_ext=4;
    while (1)
    {
      if (mode_ext==4)
      {
        fr_ps.header.mode = 0;
        fr_ps.header.mode_ext = 0;
        fr_ps.jsbound=32;
      }
      else
      {
        fr_ps.header.mode = 1;
        fr_ps.header.mode_ext = mode_ext;
        fr_ps.jsbound=4*(mode_ext+1);
      }
      if (mode_ext==0)
        break;

      int req_bits = 0;
      req_bits=4*((fr_ps.jsbound*fr_ps.stereo)+(32-fr_ps.jsbound));

      for(i=0; i<32; i++)
        for(j=0; j<((i<fr_ps.jsbound)?fr_ps.stereo:1); j++)
        {
          for (k=0; k<14; k++)
            if (snr[k]>=ltmin[j][i])
              break;
          if(stereo == 2 && i >= fr_ps.jsbound)
            for(;k<14; ++k)
              if (snr[k]>=ltmin[1-j][i])
                break;
          if (k>0)
            req_bits += (k+1)*12 + 6*((i>=fr_ps.jsbound)?fr_ps.stereo:1);
        }
        if (req_bits <= adb)
          break;
        mode_ext--;
    }
  }
  else
    fr_ps.jsbound = 32;

  adb -= 4 * ( (fr_ps.jsbound * fr_ps.stereo) + (32-fr_ps.jsbound) );

  for (i=0;i<32;i++)
    for (k=0;k<fr_ps.stereo;k++)
    {
      mnr[k][i]=-ltmin[k][i];
      bit_alloc[k][i] = 0;
      used[k][i] = 0;
    }

  while (1)
  {
    double small = mnr[0][0]+1;
    int min_sb = -1;
    int min_ch = -1;
    for (i=0;i<32;i++)
      for (k=0;k<fr_ps.stereo;k++)
        if (used[k][i] != 2 && small > mnr[k][i])
        {
          small = mnr[k][i];
          min_sb = i;
          min_ch = k;
        }
    if (min_sb==-1)
      break;
    int smpl_bits=12;
    if (!used[min_ch][min_sb])
      smpl_bits+=12+(((min_sb >= fr_ps.jsbound)&&(fr_ps.stereo==2))?12:6);

    if (adb >= smpl_bits)
    {
      adb -= smpl_bits;
      bit_alloc[min_ch][min_sb]++;
      used[min_ch][min_sb] = 1;
      mnr[min_ch][min_sb] = snr[bit_alloc[min_ch][min_sb]]-ltmin[min_ch][min_sb];
      if (bit_alloc[min_ch][min_sb] ==  14 )
        used[min_ch][min_sb] = 2;
      if(fr_ps.stereo == 2 && min_sb >= fr_ps.jsbound)
      {
        bit_alloc[1-min_ch][min_sb] = bit_alloc[min_ch][min_sb];
        used[1-min_ch][min_sb] = used[min_ch][min_sb];
        mnr[1-min_ch][min_sb] = snr[bit_alloc[1-min_ch][min_sb]]-ltmin[1-min_ch][min_sb];
      }
    }
    else
    {
      used[min_ch][min_sb] = 2;
      if(fr_ps.stereo == 2 && min_sb >= fr_ps.jsbound)
        used[1-min_ch][min_sb] = 2;
    }
  }

  unsigned int crc=0xFFFF;
  update_CRC(info.bitrate_index, 4, &crc);
  update_CRC(info.sampling_frequency, 2, &crc);
  update_CRC(info.padding, 1, &crc);
  update_CRC(info.extension, 1, &crc);
  update_CRC(info.mode, 2, &crc);
  update_CRC(info.mode_ext, 2, &crc);
  update_CRC(info.copyright, 1, &crc);
  update_CRC(info.original, 1, &crc);
  update_CRC(info.emphasis, 2, &crc);
  for (i=0;i<32;i++)
    for (k=0;k<((i<fr_ps.jsbound)?fr_ps.stereo:1);k++)
      update_CRC(bit_alloc[k][i], 4, &crc);

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

  for (i=0;i<32;i++)
    for (k=0;k<((i<fr_ps.jsbound)?fr_ps.stereo:1);k++)
      putbits(bit_alloc[k][i],4);
  for (i=0;i<32;i++)
    for (j=0;j<fr_ps.stereo;j++)
      if (bit_alloc[j][i])
        putbits(scalar[j][i],6);
   for (j=0;j<12;j++)
     for (i=0;i<32;i++)
       for (k=0;k<((i<fr_ps.jsbound)?fr_ps.stereo:1);k++)
         if (bit_alloc[k][i])
         {
           double d;
           if(fr_ps.stereo == 2 && i>=fr_ps.jsbound)
             d = j_sample[j][i] * imultiple1[j_scale[i]];
           else
             d = sb_sample[j][k][i] * imultiple1[scalar[k][i]];
           unsigned int smpl = (d+1) * atab[bit_alloc[k][i]-1];
           putbits(smpl,bit_alloc[k][i]+1);
         }
  for (i=0;i<adb;i++)
    put1bit(0);
  return 1;
}

void initencode1()
{
  int i;
  for (i=0; i<14; i++)
    atab[i]=(2<<i)-0.5;
  for (i=0; i<64; i++)
    buffer[0][i+384]=buffer[1][i+384]=0;
  for (i=0; i<64; i++)
    multiple1[i]=exp(log(2)*(3-i)/3.0);
  for (i=0; i<64; i++)
    imultiple1[i]=exp(log(2)*(i-3)/3.0);
  carry=0;
}
