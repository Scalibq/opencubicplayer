// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMDPlay note dots routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include "binfile.h"
#include "mcp.h"
#include "gmdplay.h"
#include "cpiface.h"

extern unsigned short plNLChan;

int gmdGetDots(notedotsdata *d, int max)
{
  int pos=0;
  int i;
  for (i=0; i<plNLChan; i++)
  {
    if (!mpGetChanStatus(i))
      continue;

    chaninfo ci;
    mpGetChanInfo(i, ci);

    int vl,vr;
    mpGetRealVolume(i, vl, vr);
    if (!vl&&!vr&&!ci.vol)
      continue;

    if (pos>=max)
      break;
    d[pos].voll=vl;
    d[pos].volr=vr;
    d[pos].chan=i;
    d[pos].note=mpGetRealNote(i);
    d[pos].col=32+(ci.ins&15);//sustain
    pos++;
  }
  return pos;
}
