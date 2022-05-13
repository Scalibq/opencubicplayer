// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// WAVPlay file type detection routines for the fileselector
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <string.h>
#include <stdlib.h>
#include "pfilesel.h"

static unsigned char wavGetModuleType(const unsigned char *buf)
{
  if ((*(unsigned long*)buf==0x46464952)&&(*(unsigned long*)(buf+8)==0x45564157)&&(*(unsigned long*)(buf+12)==0x20746D66)&&(*(unsigned short*)(buf+20)==1))
    return mtWAV;

  return 0xFF;
}


static int wavReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int)
{
  int type=wavGetModuleType(buf);
  m.modtype=type;
  int i,j;

  switch (type)
  {
  case mtWAV:
  {
    i=20;

    char rate[10];
    *m.modname=0;
    ultoa(*(unsigned long*)(buf+i+4), rate, 10);
    for (j=strlen(rate); j<5; j++)
      strcat(m.modname, " ");
    strcat(m.modname, rate);
    if (*(unsigned short*)(buf+i+14)==8)
      strcat(m.modname, "Hz,  8 bit, ");
    else
      strcat(m.modname, "Hz, 16 bit, ");
    if (*(unsigned short*)(buf+i+2)==1)
      strcat(m.modname, "mono");
    else
      strcat(m.modname, "stereo");
    m.channels=*(unsigned short*)(buf+i+2);
    if (*(unsigned long*)(buf+i+16)==61746164)
      m.playtime=*(unsigned long*)(buf+i+20)/ *(unsigned long*)(buf+i+8);
    return 1;
  }
  }
  m.modtype=0xFF;
  return 0;
}

static int wavReadInfo(moduleinfostruct &, binfile &, const unsigned char *, int)
{
  return 0;
}

extern "C"
{
  mdbreadnforegstruct wavReadInfoReg = {wavReadMemInfo, wavReadInfo};
};