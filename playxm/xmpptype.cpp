// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// XMPlay file type detection routines for the fileselector
//
// revision history: (please note changes here)
//  -kb980717   Tammo Hinrichs <opencp@groove.org>
//    -first release
//    -separated this code from gmdptype.cpp
//    -added 0x1a and version number checking
//    -added MXM file type

#include <string.h>
#include <stdlib.h>
#include "pfilesel.h"

static void getext(char *ext, const char *name)
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


static unsigned char xmpGetModuleType(const unsigned char *buf, const char *ext)
{
  if (!strcmp(ext, ".WOW")&&(*(unsigned long*)(buf+1080)==0x2E4B2E4D))
    return mtWOW;

  switch (*(unsigned long*)(buf+1080))
  {
  case 0x2E4B2E4D: case 0x214B214D: case 0x2E542E4E: case 0x34544C46:
  case 0x4E484331: case 0x4E484332: case 0x4E484333: case 0x4E484334:
  case 0x4E484335: case 0x4E484336: case 0x4E484337: case 0x4E484338:
  case 0x4E484339: case 0x48433031: case 0x48433131: case 0x48433231:
  case 0x48433331: case 0x48433431: case 0x48433531: case 0x48433631:
  case 0x48433731: case 0x48433831: case 0x48433931: case 0x48433032:
  case 0x48433132: case 0x48433232: case 0x48433332: case 0x48433432:
  case 0x48433532: case 0x48433632: case 0x48433732: case 0x48433832:
  case 0x48433932: case 0x48433033: case 0x48433133: case 0x48433233:
    return mtMOD;
  }

  if (!memcmp(buf, "Extended Module: ", 17) && buf[37]==0x1a)
    return mtXM;

  if (!memcmp(buf, "MXM\0", 4))
    return mtMXM;

  if (!strcmp(ext, ".MOD"))
  {
    int i,j;
    int mod=3;
    for (i=0; i<20; i++)
      if (buf[i])
        break;
      else
        if (buf[i]<0x20)
          mod=0;
    for (i=0; i<31; i++)
      for (j=0; j<21; j++)
        if (!buf[20+i*30+j])
          break;
        else
          if (buf[20+i*30+j]<0x20)
            mod&=(i<15)?0:1;
    if (mod==3)
      return mtM31;
    if (mod==1)
      return mtM15;
  }

  return 0xFF;
}


static int xmpReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int)
{
  if (!memcmp(buf, "ziRCONia", 8))
  {
    strcpy(m.modname, "MMCMPed module");
    return 0;
  }

  char ext[_MAX_EXT];
  getext(ext, m.name);

  int type=xmpGetModuleType(buf, ext);
  m.modtype=type;

  typedef struct
  {
    char sig[17];
    char name[20];
    char eof;
    char tracker[20];
    unsigned short ver;
    unsigned long hdrsize;
  } head1;

  head1 *xmhdr;

  switch (type)
  {
  case mtM15: case mtM31:
    m.channels=4;
    memcpy(m.modname, buf+0, 20);
    m.modname[20]=0;
    return 1;

  case mtMOD:
    switch (*(unsigned long*)(buf+1080))
    {
    case 0x2E4B2E4D: // M.K.
    case 0x214B214D: // M!K!
    case 0x2E542E4E: // N.T.
    case 0x34544C46: m.channels=4; break; // FLT4
    case 0x4E484331: m.channels=1; break; // 1CHN...
    case 0x4E484332: m.channels=2; break;
    case 0x4E484333: m.channels=3; break;
    case 0x4E484334: m.channels=4; break;
    case 0x4E484335: m.channels=5; break;
    case 0x4E484336: m.channels=6; break;
    case 0x4E484337: m.channels=7; break;
    case 0x4E484338: m.channels=8; break;
    case 0x4E484339: m.channels=9; break;
    case 0x48433031: m.channels=10; break; // 10CH...
    case 0x48433131: m.channels=11; break;
    case 0x48433231: m.channels=12; break;
    case 0x48433331: m.channels=13; break;
    case 0x48433431: m.channels=14; break;
    case 0x48433531: m.channels=15; break;
    case 0x48433631: m.channels=16; break;
    case 0x48433731: m.channels=17; break;
    case 0x48433831: m.channels=18; break;
    case 0x48433931: m.channels=19; break;
    case 0x48433032: m.channels=20; break;
    case 0x48433132: m.channels=21; break;
    case 0x48433232: m.channels=22; break;
    case 0x48433332: m.channels=23; break;
    case 0x48433432: m.channels=24; break;
    case 0x48433532: m.channels=25; break;
    case 0x48433632: m.channels=26; break;
    case 0x48433732: m.channels=27; break;
    case 0x48433832: m.channels=28; break;
    case 0x48433932: m.channels=29; break;
    case 0x48433033: m.channels=30; break;
    case 0x48433133: m.channels=31; break;
    case 0x48433233: m.channels=32; break;
    }
    memcpy(m.modname, buf+0, 20);
    m.modname[20]=0;
    return 1;

  case mtXM:
    xmhdr =  (head1 *)buf;
    if (xmhdr->ver<0x104)
    {
      m.modtype=0xFF;
      strcpy(m.modname,"too old version");
      return 0;
    }
    else
    {
      memcpy(m.modname, xmhdr->name, 20);
      m.modname[20]=0;
      m.channels=buf[68];
    }
    return 1;

  case mtMXM:
    strcpy(m.modname,"MXMPlay module");
    m.channels=buf[12];
    return 1;

  }
  m.modtype=0xFF;
  return 0;
}

static int xmpReadInfo(moduleinfostruct &, binfile &, const unsigned char *, int)
{
  return 0;
}

extern "C"
{
  mdbreadnforegstruct xmpReadInfoReg = {xmpReadMemInfo, xmpReadInfo};
};