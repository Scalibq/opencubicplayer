// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// MPEG audio generic encoding routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <string.h>
#include "binfile.h"
#include "mpenccom.h"
#include "mpencode.h"

void initfiltersubband();

static int bitratetab[2][15] =
{
  {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
  {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
};



static char smpbuf[4608];
static int smpbuflen;
static binfile *musicout;
static unsigned char buf[16384];
static int bufsize;
static int bufmax;
static int bufpos;

static frame_params fr_ps;

void initencode1();
void initencode2();

int initencoder(binfile &out, const ampegencoderparams &par)
{
  if (par.model==1)
    return 0;
  fr_ps.header.version=1;
  fr_ps.header.lay=par.lay;
  fr_ps.header.error_protection=par.crc;

  int i;
  for (i=1; i<15; i++)
    if (bitratetab[par.lay-1][i]*1000 == par.bitrate)
      break;
  if (i==15)
    return 0;
  fr_ps.header.bitrate_index=i;
  switch (par.sampfreq)
  {
  case 32000: fr_ps.header.sampling_frequency=2; break;
  case 44100: fr_ps.header.sampling_frequency=0; break;
  case 48000: fr_ps.header.sampling_frequency=1; break;
  default: return 0;
  }
  fr_ps.header.extension=0;
  fr_ps.header.mode=par.mode;
  fr_ps.header.mode_ext=0;
  fr_ps.header.copyright=par.copyright;
  fr_ps.header.original=par.original;
  fr_ps.header.emphasis=par.emphasis;
  fr_ps.model=par.model;
  fr_ps.actual_mode = par.mode;
  fr_ps.frmlen=((fr_ps.header.lay==1)?768:2304)*((fr_ps.header.mode==3)?1:2);
  musicout=&out;
  bufsize=16384;
  bufpos=0;
  bufmax=8192;
  initfiltersubband();
  if (par.lay==1)
    initencode1();
  else
    initencode2();
  if (par.model==2)
    initpsychoanal(fr_ps.header.sampling_frequency);
  return 1;
}

int encode1(short *smp,frame_params &fr_ps);
int encode2(short *smp,frame_params &fr_ps);

static void flush_buffer()
{
  int p=bufpos>>3;
  if (p<bufmax)
    return;
  musicout->write(buf, p);
  bufpos&=7;
  *buf=buf[p];
  memset(buf+1, 0, bufsize-1);
}

int encodeframe(void *smp, int len)
{
  int w=0;
  while (len)
  {
    int l=len;
    if (l>(fr_ps.frmlen-smpbuflen))
      l=fr_ps.frmlen-smpbuflen;
    memcpy(smpbuf+smpbuflen, smp, l);
    *(char**)&smp+=l;
    len-=l;
    smpbuflen+=l;
    w+=l;
    if (smpbuflen==fr_ps.frmlen)
    {
      if (fr_ps.header.lay==1)
        encode1((short*)smpbuf,fr_ps);
      else
        encode2((short*)smpbuf,fr_ps);
      flush_buffer();
      smpbuflen=0;
    }
  }
  return w;
}

void doneencoder()
{
  if (smpbuflen)
  {
    memset(smpbuf+smpbuflen, 0, fr_ps.frmlen-smpbuflen);
    if (fr_ps.header.lay==1)
      encode1((short*)smpbuf,fr_ps);
    else
      encode2((short*)smpbuf,fr_ps);
  }
  musicout->write(buf, (bufpos+7)>>3);
}


void put1bit(int bit)
{
  if (bit)
    buf[bufpos>>3]|=128>>(bufpos&7);
  bufpos++;
}

unsigned long swap(unsigned long);
#pragma aux swap parm [eax] value [eax] = "bswap eax"

void putbits(unsigned int val, int n)
{
  *(unsigned long*)&buf[bufpos>>3]|=swap((val&((1<<n)-1))<<(32-n-(bufpos&7)));
  bufpos+=n;
}

void update_CRC(unsigned int data, unsigned int length, unsigned int *crc)
{
  unsigned int masking, carry;

  masking = 1 << length;

  while ((masking>>=1))
  {
    carry = *crc & 0x8000;
    *crc <<= 1;
    if (!carry ^ !(data & masking))
      *crc ^= 0x8005;
  }
  *crc &= 0xffff;
}
