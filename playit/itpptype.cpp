// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// ITPlay file type detection routines for the file selector
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <string.h>
#include "pfilesel.h"


static unsigned char itpGetModuleType(const unsigned char *buf)
{
  if (*(unsigned long*)buf==0x4D504D49)
    return mtIT;
  return 0xFF;
}


static int itpReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int)
{
  if (!memcmp(buf, "ziRCONia", 8))
  {
    strcpy(m.modname, "MMCMPed module");
    return 0;
  }

  int type=itpGetModuleType(buf);
  m.modtype=type;
  int i;

  switch (type)
  {
  case mtIT:
    if (buf[0x2C]&4)
      if (buf[0x2B]<2)
        return 0;
    memcpy(m.modname, buf+4, 26);
    m.modname[26]=0;
    m.channels=0;
    for (i=0; i<64; i++)
      if (!(buf[64+i]&0x80))
        m.channels++;
    return 1;
  }
  return 0;
}

static int itpReadInfo(moduleinfostruct &, binfile &, const unsigned char *, int)
{
  return 0;
}

extern "C"
{
  mdbreadnforegstruct itpReadInfoReg = {itpReadMemInfo, itpReadInfo};
};