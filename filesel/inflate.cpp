// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// PKZIP inflation routine for decompressing help texts / ZIP archive lookup
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb981201   Tammo Hinrichs <opencp@gmx.net>
//    -Included a few checks to prevent crashing with some data this
//     source can handle (more a workaround than a fix, as pkunzip
//     is able to uncompress every file this one crashes with)

#include <string.h>

static const unsigned char *ibuf;

static char *obuf;
static char *obuf0;

static unsigned long bitbuf;
static char bitnum;
static short huff1[0x120][2];
static short huff2[0x20][2];
static char lu[0x240];

static inline unsigned short readbits(char n)
{
  unsigned short v=bitbuf&((1L<<n)-1);
  bitbuf>>=n;
  bitnum-=n;
  while (bitnum<=24)
  {
    bitbuf|=(long)*ibuf++<<bitnum;
    bitnum+=8;
  }
  return v;
}

static short distributehuff(short (*huff)[2], short &huffpos, char *codelen, short codenum, char curlen)
{
  short curcode;
  if (!curlen)
    curcode=codenum;
  else
    for (curcode=0; curcode<codenum; curcode++)
      if (codelen[curcode]==curlen)
        break;
  if (curcode==codenum)
  {
    short hp=huffpos++;
    huff[hp][0]=distributehuff(huff, huffpos, codelen, codenum, curlen+1);
    huff[hp][1]=distributehuff(huff, huffpos, codelen, codenum, curlen+1);
    return hp;
  }
  else
  {
    codelen[curcode]=0;
    return curcode|0x8000;
  }
}

static int makehuff(short (*huff)[2], char *codelen, short codenum)
{
  short i, lastnode;
  short nodes=-1;
  unsigned short chksum=0;
  for (i=0; i<codenum; i++)
    if (codelen[i])
    {
      nodes++;
      lastnode=i;
      chksum+=32768>>codelen[i];
    }
  if (chksum==32768)
  {
    short hp=0;
    distributehuff(huff, hp, codelen, codenum, 0);
  }
  else
    if (!nodes&&(chksum==16384))
      huff[0][0]=huff[0][1]=lastnode|0x8000;
    else
      return 0;
  return 1;
}

static inline unsigned short readhuff(short (*huff)[2])
{
  signed short pos=0;
  while (pos>=0)
    pos=huff[pos][readbits(1)];
  return pos&0x7FFF;
}

static int readtype1()
{
  memset(lu, 8, 0x90);
  memset(lu+0x90, 9, 0x70);
  memset(lu+0x100, 7, 0x18);
  memset(lu+0x118, 8, 0x8);
  memset(lu+0x120, 5, 0x20);

  return (makehuff(huff1, lu, 0x120) && makehuff(huff2, lu+0x120, 0x20));
}

static int readtype2()
{
  short a=readbits(5)+0x101;
  short b=readbits(5)+1;

  memset(lu, 0, 19);
  short n=readbits(4)+4;
  short i;
  for (i=0; i<n; i++)
    lu["\x10\x11\x12\x00\x08\x07\x09\x06\x0A\x05\x0B\x04\x0C\x03\x0D\x02\x0E\x01\x0F"[i]]=readbits(3);

  if (!makehuff(huff1, lu, 19))
    return 0;

  char *lup=lu, *lue=lu+a+b;
  while (lup<lue)
  {
    unsigned short d=readhuff(huff1);
    if (d<0x10)
      *lup++=d;
    else
    {
      switch (d)
      {
      case 0x10:
        d=lup[-1];
        n=readbits(2)+3;
        break;
      case 0x11:
        d=0;
        n=readbits(3)+3;
        break;
      case 0x12:
        d=0;
        n=readbits(7)+11;
        break;
      }
      if (n>(lue-lup))
        n=lue-lup;
      memset(lup, d, n);
      lup+=n;
    }
  }
  return (makehuff(huff1, lu, a) && makehuff(huff2, lu+a, b));
}

static inline unsigned short getcode1()
{
  unsigned short a=readhuff(huff1);
  if (a<0x109)
    return a;
  if (a==0x11D)
    return 0x200;
  short n=(a-0x105)>>2;
  return 0x101+((((a-1)&3)+4)<<n)+readbits(n);
}

static inline unsigned short getcode2()
{
  unsigned short a=readhuff(huff2);
  if (a<4)
    return a;
  short n=(a>>1)-1;
  return (((a&1)+2)<<n)+readbits(n);
}

void mymemmove(void *, void *, unsigned long);
#pragma aux mymemmove parm [edi] [esi] [ecx] = "rep movsb"

void inflate(void *ob, const void *ib)
{
  ibuf=(const unsigned char*)ib;
  obuf0=(char *)ob;

  obuf=obuf0;
  bitbuf=*(long*)ibuf;
  ibuf+=4;
  bitnum=32;
  char brk=0;
  while (!brk)
  {
    brk=readbits(1);
    char type=readbits(2);
    if (type==0)
    {
      if (bitnum!=32)
        readbits(bitnum-24);
      unsigned short len=readbits(16);
      if (readbits(16)!=~len)
        return;
      do
        *obuf++=readbits(8);
      while (--len);
      continue;
    }
    else
    if (type==1)
    {
      if (!readtype1()) return;
    }
    else
    if (type==2)
    {
      if (!readtype2()) return;
    }
    else
      return;
    while (1)
    {
      unsigned short a=getcode1();
      if (a<0x100)
      {
        *obuf++=a;
        continue;
      }
      if (a==0x100)
        break;
      a-=0xFE;
      char *b=obuf-getcode2()-1;
      if (b<obuf0)
      {
        unsigned short lz=obuf0-b;
        if (a<lz)
          lz=a;
        memset(obuf, 0, lz);
        obuf+=lz;
        a-=lz;
        b+=lz;
      }
      mymemmove(obuf, b, a);
      obuf+=a;
    }
  }
}

void inflatemax(void *ob, const void *ib, unsigned long max)
{
  memset(ob,0,max);
  ibuf=(const unsigned char*)ib;
  obuf0=(char *)ob;

  obuf=obuf0;
  bitbuf=*(long*)ibuf;
  ibuf+=4;
  bitnum=32;
  char brk=0;
  while (!brk&&max)
  {
    brk=readbits(1);
    char type=readbits(2);
    if (type==0)
    {
      if (bitnum!=32)
        readbits(bitnum-24);
      unsigned short len=readbits(16);
      if (readbits(16)!=~len)
        return;
      if (len>max)
        len=max;
      max-=len;
      do
        *obuf++=readbits(8);
      while (--len);
      continue;
    }
    else
    if (type==1)
    {
      if (!readtype1()) return;
    }
    else
    if (type==2)
    {
      if (!readtype2()) return;
    }
    else
      return;
    while (max)
    {
      unsigned short a=getcode1();
      if (a<0x100)
      {
        max--;
        *obuf++=a;
        continue;
      }
      if (a==0x100)
        break;
      a-=0xFE;
      if (a>max)
        a=max;
      char *b=obuf-getcode2()-1;
      if (b<obuf0)
      {
        unsigned long lz=obuf0-b;
        if (a<lz)
          lz=a;
        memset(obuf, 0, lz);
        obuf+=lz;
        max-=lz;
        a-=lz;
        b=obuf0;
      }
      mymemmove(obuf, b, a);
      obuf+=a;
      max-=a;
    }
  }
}