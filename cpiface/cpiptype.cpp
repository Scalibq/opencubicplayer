// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIFace file type dectection routines for the file selector
//
// revision history: (please note changes here)
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -first release
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'

#define NO_CPIFACE_IMPORT
#include <string.h>
#include <stdlib.h>
#include "pfilesel.h"
#include "binfile.h"

static int cpiReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int)
{
  if (!memcmp(buf, "CPANI\x1A\x00\x00", 8))
  {
     strncpy(m.modname,(char *)buf+8,31);
     if (!m.modname[0])
       strcpy(m.modname,"wÅrfel mode animation");
  }

  m.modtype=0xFF;
  return 0;
}

static int cpiReadInfo(moduleinfostruct &, binfile &, const unsigned char *, int)
{
  return 0;
}

extern "C"
{
  mdbreadnforegstruct cpiReadInfoReg = {cpiReadMemInfo, cpiReadInfo};
};
