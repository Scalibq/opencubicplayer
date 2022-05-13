// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMIPlay note dots routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include "binfile.h"
#include "mcp.h"
#include "gmiplay.h"
#include "cpiface.h"

extern unsigned short plNLChan;

int gmiGetDots(notedotsdata *d, int max)
{
  int i,j;
  int pos=0;
  for (i=0; i<plNLChan; i++)
  {
    if (pos>=max)
      break;
    mchaninfo2 ci;
    midGetRealNoteVol(i, ci);
    for (j=0; j<ci.notenum; j++)
    {
      if (pos>=max)
        break;
      unsigned short vl=ci.voll[j];
      unsigned short vr=ci.volr[j];

      if (!vl&&!vr&&!ci.opt[j])
        continue;

      d[pos].voll=vl<<1;
      d[pos].volr=vr<<1;
      d[pos].chan=i;
      d[pos].note=ci.note[j]+12*256;
      d[pos].col=(ci.ins[j]&15)+(ci.opt[j]?32:16);
      pos++;
    }
  }
  return pos;
}
