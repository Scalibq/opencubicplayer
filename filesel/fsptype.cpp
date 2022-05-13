// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Fileselector file type detection routines (covers play lists and internal
// cache files)
//
// revision history: (please note changes here)
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -first release

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pfilesel.h"
#include "binfile.h"

static int fsReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int len)
{
  // check for PLS play list

  char *b=(char *)buf;
  int pos=10;
  int num=0;
  if (!memcmp(buf,"[playlist]", 10))
  {
    char modname[32];
    strcpy(modname,"PLS style play list ");
    while (pos<len)
    {
      if (b[pos]!=0x0a && b[pos]!=0x0d)
        pos++;
      else
      {
        while (isspace(b[pos]) && pos<len)
          pos++;
        if (len-pos>18 && !memcmp(b+pos,"NumberOfEntries=",16))
        {
          pos+=16;
          num=strtol(b+pos,0,10);
          strcat(modname,"(");
          strcat(modname,ltoa(num,0,10));
          strcat(modname," entries)");
          pos=len;
        }
      }
    }
    if (num)
    {
      strcpy(m.modname,modname);
      m.modtype=mtPLS;
      return 1;
    }
    else
    {
      strcat(modname,"?");
      strcpy(m.modname,modname);
      m.modtype=mtPLS;
      return 1;
    }
  }

  // check for M3U-style play list
  pos=0;
  num=0;
  char linebuf[512];
  while (pos<len)
  {
    while (pos<len && isspace(b[pos]))
      pos++;
    if (pos>=len)
      break;
    int pos2=pos;
    while(pos2<len && b[pos2]>=32)
      pos2++;
    if ((pos2<len && b[pos2]!=0x0A && b[pos2]!=0x0D) || (pos2-pos)>=512)
    {
      num=0;
      break;
    }
    if (pos2==len)
    {
      num=-1;
      break;
    }
    memcpy(linebuf,b+pos,pos2-pos);
    linebuf[pos2-pos]=0;
    if (strchr(linebuf,';'))
      *strchr(linebuf,';')=0;
    if (*linebuf && !strchr(linebuf,'.'))
    {
      num=0;
      break;
    }
    num++;
    pos=pos2+1;
  }
  if (num>0)
  {
    strcpy(m.modname,"M3U style play list (");
    strcat(m.modname,ltoa(num,0,10));
    strcat(m.modname," entries)");
    m.modtype=mtM3U;
    return 1;
  }
  else if (num<0)
  {
    strcpy(m.modname,"M3U play list ?");
    m.modtype=mtM3U;
    return 1;
  }

  if (!memcmp(buf, "CPArchiveCache\x1A\x00", 16))
    strcpy(m.modname,"openCP archive data base");
  if (!memcmp(buf, "Cubic Player Module Information Data Base\x1A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 60))
    strcpy(m.modname,"openCP module info data base");
  if (!memcmp(buf, "MDZTagList\x1A\x00", 12))
    strcpy(m.modname,"openCP MDZ file cache");

  m.modtype=0xFF;
  return 0;
}

static int fsReadInfo(moduleinfostruct &, binfile &, const unsigned char *, int)
{
  return 0;
}

extern "C"
{
  mdbreadnforegstruct fsReadInfoReg = {fsReadMemInfo, fsReadInfo};
};