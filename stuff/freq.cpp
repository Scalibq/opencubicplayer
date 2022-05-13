// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Freqency calculation routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include "imsrtns.h"

static unsigned long hnotetab6848[16]={11131415,4417505,1753088,695713,276094,109568,43482,17256,6848,2718,1078,428,170,67,27,11};
static unsigned long hnotetab8363[16]={13594045,5394801,2140928,849628,337175,133808,53102,21073,8363,3319,1317,523,207,82,33,13};
static unsigned short notetab[16]={32768,30929,29193,27554,26008,24548,23170,21870,20643,19484,18390,17358,16384,15464,14596,13777};
static unsigned short finetab[16]={32768,32650,32532,32415,32298,32182,32066,31950,31835,31720,31606,31492,31379,31266,31153,31041};
static unsigned short xfinetab[16]={32768,32761,32753,32746,32738,32731,32724,32716,32709,32702,32694,32687,32679,32672,32665,32657};

int mcpGetFreq8363(int note)
{
  note=-note;
  return umulshr16(umulshr16(umulshr16(hnotetab8363[((note+0x8000)>>12)&0xF],notetab[(note>>8)&0xF]*2),finetab[(note>>4)&0xF]*2),xfinetab[note&0xF]*2);
}

int mcpGetFreq6848(int note)
{
  note=-note;
  return umulshr16(umulshr16(umulshr16(hnotetab6848[((note+0x8000)>>12)&0xF],notetab[(note>>8)&0xF]*2),finetab[(note>>4)&0xF]*2),xfinetab[note&0xF]*2);
}

int mcpGetNote8363(int frq)
{
  signed short x;
  int i;
  for (i=0; i<15; i++)
    if (hnotetab8363[i+1]<frq)
      break;
  x=(i-8)*16*256;
  frq=umuldiv(frq, 32768, hnotetab8363[i]);
  for (i=0; i<15; i++)
    if (notetab[i+1]<frq)
      break;
  x+=i*256;
  frq=umuldiv(frq, 32768, notetab[i]);
  for (i=0; i<15; i++)
    if (finetab[i+1]<frq)
      break;
  x+=i*16;
  frq=umuldiv(frq, 32768, finetab[i]);
  for (i=0; i<15; i++)
    if (xfinetab[i+1]<frq)
      break;
  return -x-i;
}

int mcpGetNote6848(int frq)
{
  signed short x;
  int i;
  for (i=0; i<15; i++)
    if (hnotetab6848[i+1]<frq)
      break;
  x=(i-8)*16*256;
  frq=umuldiv(frq, 32768, hnotetab6848[i]);
  for (i=0; i<15; i++)
    if (notetab[i+1]<frq)
      break;
  x+=i*256;
  frq=umuldiv(frq, 32768, notetab[i]);
  for (i=0; i<15; i++)
    if (finetab[i+1]<frq)
      break;
  x+=i*16;
  frq=umuldiv(frq, 32768, finetab[i]);
  for (i=0; i<15; i++)
    if (xfinetab[i+1]<frq)
      break;
  return -x-i;
}
