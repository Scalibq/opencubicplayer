// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMIPlay Initialisiation (reads GUS patches etc)
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kbwhenever Tammo Hinrichs <opencp@gmx.net>
//    -changed path searching for ULTRASND.INI and patch files
//    -corrected some obviously wrong allocations

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "binfstd.h"
#include "binfpak.h"
#include "mcp.h"
#include "psetting.h"

extern char *midInstrumentPaths[4];
extern char *midInstrumentNames[256];

int midUseFFF=0;

unsigned char midInitFFF();

unsigned char midInit(const char *ipath)
{
  midUseFFF=cfGetProfileBool("midi", "usefff", 0, 0);
  if (midUseFFF)
    return midInitFFF();
  pakbinfile ipak;
  sbinfile   istd;
  binfile *  inifile;

  int i;
  for (i=0; i<4; i++)
  {
    midInstrumentPaths[i]=new char[256];
    midInstrumentPaths[i][0]=0;
  }
  for (i=0; i<256; i++)
  {
    midInstrumentNames[i]=new char[256];
    midInstrumentNames[i][0]=0;
  }

  char path0[_MAX_PATH];
  char path1[_MAX_PATH];
  strcpy(path0,ipath);
  if (path0[strlen(path0)-1]!='\\')
    strcat(path0, "\\");
  strcpy(path1,path0);
  strcat(path1,"MIDI\\");

  strcpy(midInstrumentPaths[1],path0);
  strcpy(midInstrumentPaths[0],path1);
  strcpy(midInstrumentPaths[2],cfConfigDir);
  strcat(midInstrumentPaths[2],"MIDI\\");

  strcat(path0, "ULTRASND.INI");
  strcat(path1, "ULTRASND.INI");

  if (!istd.open(path0,sbinfile::openro))
    inifile = &istd;
  else if (!istd.open(path1,sbinfile::openro))
    inifile = &istd;
  else if (!ipak.open("ULTRASND.INI"))
    inifile = &ipak;
  else
    return 0;

  long len = inifile->length();
  char *buf=new char[len+1];
  if (!buf)
    return 0;
  inifile->read(buf, len);
  buf[len]=0;
  inifile->close();

  char path[_MAX_PATH];
  *path=0;

  char *bp=buf;
  char *bp2;
  char type=0;

  while (1)
  {
    while (isspace(*bp))
      bp++;
    if (!*bp)
      break;

    if (*bp=='[')
    {
      if (!memicmp(bp, "[Melodic Bank 0]", 16))
        type=1;
      else
      if (!memicmp(bp, "[Drum Bank 0]", 13))
        type=2;
      else
        type=0;
    }

    if (!memicmp(bp, "PatchDir", 8))
    {
      while ((*bp!='=')&&*bp)
        bp++;
      if (*bp)
        bp++;
      while ((*bp==' ')||(*bp=='\t'))
        bp++;
      bp2=bp;
      while (!isspace(*bp2)&&*bp2)
        bp2++;
      memcpy(path, bp, bp2-bp);
      path[bp2-bp]=0;
      if (path[strlen(path)-1]!='\\')
        strcat(path, "\\");
    }

    if (!isdigit(*bp)||!type)
    {
      while (*bp&&(*bp!='\r')&&(*bp!='\n'))
        bp++;
      continue;
    }

    int insnum=strtoul(bp, 0, 10)+((type==2)?128:0);
    while ((*bp!='=')&&*bp)
      bp++;
    if (*bp)
      bp++;
    while ((*bp==' ')||(*bp=='\t'))
      bp++;
    bp2=bp;
    while (!isspace(*bp2)&&*bp2)
      bp2++;

    if (insnum<256)
    {
      char *x=midInstrumentNames[insnum];
      strcpy(x+(bp2-bp), ".pat");
      memcpy(x, bp, bp2-bp);
    }

    while (*bp&&(*bp!='\r')&&(*bp!='\n'))
      bp++;

  }
  delete buf;
  strcpy(midInstrumentPaths[3],path);

  return 1;
}

extern int loadFFF(binfile &file);
extern void closeFFF();

void midClose()
{
  if (midUseFFF)
    closeFFF();
  else
  {
    for (int i=0; i<4; i++)
      delete midInstrumentPaths[i];
    for (i=0; i<256; i++)
      delete midInstrumentNames[i];
  }
}

unsigned char midInitFFF()
{
  sbinfile fff;
  const char *fn=cfGetProfileString("midi", "fff", "midi.fff");
  if(fff.open(fn, sbinfile::openro))
  {
    printf("[FFF]: %s not found...\n", fn);
    return 0;
  }
  return loadFFF(fff);
}