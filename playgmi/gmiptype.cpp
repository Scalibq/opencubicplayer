// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMIPlay file type detection routines for the fileselector
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <string.h>
#include <stdlib.h>
#include "pfilesel.h"
#include "binfile.h"

static void getext(char *ext, char *name)
{
  name+=8;
  int i;
  for (i=0; i<4; i++)
    if (*name==' ')
      break;
    else
      *ext++=*name++;
  *ext=0;
}

static unsigned char gmiGetModuleType(const unsigned char *buf, const char *ext)
{
  if (!strcmp(ext, ".MID"))
    return mtMID;

  if (*(unsigned long*)buf==0x6468544D)
    return mtMID;

  if ((*(unsigned long*)buf==0x46464952)&&(*(unsigned long*)(buf+8)==0x44494D52))
    return mtMID;

  return 0xFF;
}

static int gmiReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int)
{
  char ext[_MAX_EXT];
  getext(ext, m.name);

  int type=gmiGetModuleType(buf, ext);
  m.modtype=type;
  int i;

  switch (type)
  {
  case mtMID:
    m.channels=16;

    unsigned long len;
    i=0;
    if (*(unsigned long*)buf==0x46464952)
    {
      i=12;
      while (i<800)
      {
        i+=8;
        if (*(unsigned long*)(buf+i-8)==0x61746164)
          break;
        i+=*(unsigned long*)(buf+i-4);
      }
    }
    while (i<800)
    {
      i+=8;
      len=(buf[i-4]<<24)|(buf[i-3]<<16)|(buf[i-2]<<8)|(buf[i-1]);
      if (!memcmp(buf+i-8, "MTrk", 4))
        break;
      i+=len;
    }
    len+=i;
    if (len>800)
      len=800;
    while (i<len)
    {
      if (*(unsigned short*)(buf+i)!=0xFF00)
        break;
      if (buf[i+2]!=0x03)
      {
        i+=4+buf[i+3];
        continue;
      }
      len=buf[i+3];
      if (len>31)
        len=31;
      memcpy(m.modname, buf+i+4, len);
      m.modname[len]=0;
      break;
    }
    return 1;
  }
  m.modtype=0xFF;
  return 0;
}

static int gmiReadInfo(moduleinfostruct &, binfile &, const unsigned char *, int)
{
  return 0;
}

extern "C"
{
  mdbreadnforegstruct gmiReadInfoReg = {gmiReadMemInfo, gmiReadInfo};
};