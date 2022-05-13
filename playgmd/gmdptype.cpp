// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMDPlay file type detection routines for the fileselector
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <string.h>
#include <stdlib.h>
#include "pfilesel.h"
#include "binfile.h"

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

static unsigned char gmdGetModuleType(const unsigned char *buf, const char *)
{

  if (*(unsigned long*)(buf+44)==0x4D524353)
    return mtS3M;

  if (*(unsigned long*)(buf+44)==0x464D5450)
    return mtPTM;

  if (!memcmp(buf, "AMShdr\x1A", 7))
    return mtAMS;

  if (!memcmp(buf, "MAS_UTrack_V00", 14))
    return mtULT;

  if (!memcmp(buf, "OKTASONG", 8))
    return mtOKT;

  if (*(unsigned long*)buf==0x4C444D44)
    return mtMDL;

  if (*(unsigned long*)buf==0x104D544D)
    return mtMTM;

  if (*(unsigned long*)buf==0x464D4444)
    return mtDMF;

  if ((*(unsigned short*)buf==0x6669)||(*(unsigned short*)buf==0x4E4A))
    return mt669;


  return 0xFF;
}


static int gmdReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int)
{
  if (!memcmp(buf, "ziRCONia", 8))
  {
    strcpy(m.modname, "MMCMPed module");
    return 0;
  }

  char ext[_MAX_EXT];
  getext(ext, m.name);

  int type=gmdGetModuleType(buf, ext);
  m.modtype=type;
  int i;


  switch (type)
  {
  case mtS3M:
    memcpy(m.modname, buf, 28);
    m.modname[28]=0;
    m.channels=0;
    for (i=0; i<32; i++)
      if (buf[64+i]!=0xFF)
        m.channels++;
    return 1;

  case mtMDL:
    if (buf[4]<0x10)
    {
      m.modtype=0xFF;
      strcpy(m.modname,"too old version");
      return 0;
    }
    memcpy(m.modname, buf+11, 32);
    for (i=32; i>0; i--)
      if (m.modname[i-1]!=' ')
        break;
    if (i!=32)
      m.modname[i]=0;
    memcpy(m.composer, buf+43, 20);
    for (i=20; i>0; i--)
      if (m.composer[i-1]!=' ')
        break;
    if (i!=20)
      m.composer[i]=0;
    m.channels=0;
    for (i=0; i<32; i++)
      if (!(buf[i+70]&0x80))
        m.channels++;
    return 1;

  case mtPTM:
    memcpy(m.modname, buf, 28);
    m.modname[28]=0;
    m.channels=buf[38];
    return 1;


  case mtAMS:
    memcpy(m.modname, buf+8, buf[7]);
    m.modname[buf[7]]=0;
    return 1;

  case mtMTM:
    memcpy(m.modname, buf+4, 20);
    m.modname[20]=0;
    m.channels=buf[33];
    return 1;

  case mt669:
    memcpy(m.modname, buf+2, 32);
    m.channels=8;
    return 1;

  case mtOKT:
    m.channels=4+(buf[17]&1)+(buf[19]&1)+(buf[21]&1)+(buf[23]&1);
    return 1;

  case mtULT:
    m.modtype=0xFF;
    memcpy(m.modname, buf+15, 32);
    return 0;

  case mtDMF:
    m.modtype=0xFF;
    memcpy(m.modname, buf+13, 30);
    m.modname[30]=0;
    memcpy(m.composer, buf+43, 20);
    m.composer[20]=0;
    m.date=(*(unsigned long*)(buf+63))&0xFFFFFF;
    return 0;
  }
  m.modtype=0xFF;
  return 0;
}

static int gmdReadInfo(moduleinfostruct &m, binfile &f, const unsigned char *buf, int)
{
  char ext[_MAX_EXT];
  getext(ext, m.name);

  int type=gmdGetModuleType(buf, ext);
  m.modtype=type;

  switch (type)
  {
  case mtULT:
    f.seek(48+buf[47]*32);
    f.seekcur(256+f.getc()*((buf[14]>='4')?66:64));
    m.channels=f.getc()+1;
    return 1;

  case mtDMF:
    f.seek(66);
    m.channels=32;
    while (1)
    {
      unsigned long sig;
      unsigned long len;
      if (!f.eread(&sig, 4))
        break;
      if (!f.eread(&len, 4))
        break;
      if (sig==0x54544150)
      {
        f.gets();
        m.channels=f.getc();
        break;
      }
      f.seekcur(len);
    }
    return 1;
  }
  m.modtype=0xFF;
  return 0;
}

extern "C"
{
  mdbreadnforegstruct gmdReadInfoReg = {gmdReadMemInfo, gmdReadInfo};
};