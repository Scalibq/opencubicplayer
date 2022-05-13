// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Link manager - contains high-level routines to load and handle
//                external DLLs
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -added lnkReadInfoReg() to read out _dllinfo entries
//  -fd981014   Felix Domke    <tmbinc@gmx.net>
//    -increased the dllinfo-buffersize from 256 to 1024 chars in parseinfo
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile
//  -ryg981206  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -added DLL autoloader (DOS only)

#define NO_CPDLL_IMPORT

#ifdef DOS32
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include "binfstd.h"
#include "binfpak.h"
#include "usedll.h"

#include "psetting.h"

extern char cfProgramDir[];
extern char cfCPPath[];

#define MAXDLLLIST 150



static int dlllist[MAXDLLLIST][2];

static char loadlist[MAXDLLLIST][128];
static int  loadlist_p;

static int lnkDoLoad(char *fn)
{
  int i, j;
  int h=0;
  char path[_MAX_NAME];

  while (1)
  {
    for (i=2; i<MAXDLLLIST; i++)
      if (dlllist[i][0]<0)
        break;
    if (i==MAXDLLLIST)
      return -1;

    dlllist[i][1]=h;
    h=i;

    strcpy(path, fn);
    strcat(path, ".dll");

    pakbinfile fil;
    if(fil.open(path))
    {
      printf("DLL error - could not load %s\n", path);
      return -1;
    }
    int r=dllLoad(fil);
    fil.close();
    if ((r<0) && (r!=-42))
    {
      printf("DLL loading of %s failed, return code is %i\n", path, r);
      return -1;
    }
    else if (r==-42)
    {
      for (j=0; j<loadlist_p; j++)
        if (!stricmp(loadlist[j], modlinkname))
        {
          printf("recursive dependency found loading %s\n", modlinkname);
          return -1;
        };

      strcpy(loadlist[loadlist_p++], fn);

      if (lnkDoLoad(modlinkname)<0)
      {
        printf("failed to resolve dependencies for %s: module %s missing\n\nload failed because of unresolved dependencies.\n", fn, modlinkname);
        return -1;
      }
      else
        continue;
    }

    if (loadlist_p>0) loadlist_p--;

    dlllist[h][0]=r;

    return h;
  };
};

int lnkLink(const char *files)
{
  int h;

  while (1)
  {
    char fn[9];
    if (!cfGetSpaceListEntry(fn, files, 8))
      break;

    loadlist_p=0;

    if ((h=lnkDoLoad(fn))<0) return h;
  }

  return h;
}

void lnkFree(int h)
{
  while (h>=0)
  {
    if (dlllist[h][0]>=0)
      dllFree(dlllist[h][0]);
    h=dlllist[h][1];
  }
}

void *lnkGetSymbol(const char *name)
{
  return dllGetSymbol(name);
}


extern "C"
{
  void CODEREFPOINT() {}
  int DATAREFPOINT;
#pragma aux CODEREFPOINT "*"
#pragma aux DATAREFPOINT "*"
};


static dllnameentry refs[] =
{
  {"CODEREFPOINT",CODEREFPOINT},
  {"DATAREFPOINT",&DATAREFPOINT},
};

int lnkInit()
{
  memset(dlllist, -1, sizeof(dlllist));
  char drive[_MAX_DRIVE];
  _splitpath(cfCPPath, drive, 0, 0, 0);
  dllInit();
  sbinfile fil;
  fil.open(cfCPPath, sbinfile::openro);
  dlllist[1][0]=dllLoadLocals(fil, refs, sizeof(refs)/sizeof(*refs), 0, 0);
  fil.close();
  if (dlllist[1][0]<0)
    return 0;

  return 1;
}


void lnkClose()
{
  dllClose();
}


int lnkCountLinks()
{
  return dllCountLinks();
}


int lnkGetLinkInfo(linkinfostruct &i, int first)
{
  return dllGetLinkInfo(i, first);
}


void lnkGetAddressInfo(linkaddressinfostruct &a, void *ptr)
{
  dllGetAddressInfo(a, ptr);
}


static char reglist[1024];
static void parseinfo (char *pi, const char *key)
{
  char s[1024];
  strcpy (s,pi);
  s[strlen(s)+1]=0;

  char *dip=s;
  char keyok=0;
  char kstate=0;
  while (*dip)
  {
    char *d2p=dip;
    while (*dip)
    {
      d2p++;
      char t=*d2p;
      if (t==';' || t==' ' || !t)
      {
        *d2p=0;
        if (!kstate)
        {
          keyok = !strcmp(dip,key);
          kstate= 1;
        }
        else if (keyok)
        {
          strcat(reglist,dip);
          strcat(reglist," ");
        }

        if (t==';')
          kstate=keyok=0;

        do
          dip=++d2p;
        while (*dip && (*dip==' ' || *dip==';'));
      }
    }
  }
}

char *lnkReadInfoReg(const char *key)
{
  *reglist=0;
  for (int d=1; d<MAXDLLLIST; d++)
  {
    linkinfostruct li;
    if (!dllGetLinkInfo(li,d))
      break;
    char **pi=(char **)dllGetSymbol(li.handle,"_dllinfo");
    if (pi)
      parseinfo(*pi,key);

  }

  if (*reglist)
    reglist[strlen(reglist)-1]=0;

  return reglist;
}


char *lnkReadInfoReg(const char *name, const char *key)
{
  *reglist=0;

  char **pi=(char **)dllGetSymbol(name,"_dllinfo");
  if (pi)
    parseinfo(*pi,key);

  if (*reglist)
    reglist[strlen(reglist)-1]=0;

  return reglist;
}

#endif
#ifdef WIN32

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "usedll.h"
#include "binfstd.h"
#include "binfpak.h"

#include "psetting.h"

extern char cfProgramDir[];
extern char cfCPPath[];

#define MAXDLLLIST 150

static HINSTANCE dllhandle[MAXDLLLIST];
static int dlllist[MAXDLLLIST];
static char *dllname[MAXDLLLIST];

int lnkLink(const char *files)
{
  int i;
  int h=0;
  while (1)
  {
    char fn[9];
    if (!cfGetSpaceListEntry(fn, files, 8))
      break;

    for(i=2; i<MAXDLLLIST; i++)
     if(dlllist[i]<0) break;

    if(i==MAXDLLLIST) return(-1);

    dlllist[i]=h;
    h=i;

    char path[_MAX_FNAME];
    strcpy(path, fn);
    strcat(path, ".dll");

    HINSTANCE r=LoadLibrary(path);
    if(!r) { printf("loadlibrary \'%s\' failed (%d).\n", path, GetLastError()); return -1; }
    dllhandle[h]=r;
    dllname[h]=new char[strlen(path)+1];
    if(!dllname[h]) continue;
    strcpy(dllname[h], fn);
  }
  return h;
}

void lnkFree(int h)
{
  while (h>0)
  {
    if(dllhandle[h])
      FreeLibrary(dllhandle[h]);
    if(dllname[h])
      delete[] dllname[h];
    dllname[h]=0;
    int i=h;
    h=dlllist[h];
    dlllist[i]=-1;
  }
}

void *lnkGetSymbol(const char *name)
{
 for(int i=0; i<MAXDLLLIST; i++)
   if(dllhandle[i]&&GetProcAddress(dllhandle[i], name))
     return(GetProcAddress(dllhandle[i], name));
 return(0);
}

int lnkInit()
{
  memset(dlllist, -1, sizeof(dlllist));
  memset(dllhandle, 0, sizeof(dllhandle));
  memset(dllname, 0, sizeof(dllname));
  char drive[_MAX_DRIVE];
  _splitpath(cfCPPath, drive, 0, 0, 0);
  sbinfile fil;
  return 1;
}

void lnkClose()
{
 for(int i=0; i<MAXDLLLIST; i++)
   if(dllhandle[i])
     FreeLibrary(dllhandle[i]);
 for(i=0; i<MAXDLLLIST; i++)
   if(dllname[i])
     delete[] dllname[i];

}

int lnkCountLinks()
{
 int a=0;
 for(int i=0; i<MAXDLLLIST; i++)
   if(dllhandle[i])
     a++;
 return a;
}

int lnkGetLinkInfo(linkinfostruct &i, int first)
{
 printf("lnkGetLinkInfo ignored.\n");
//  return dllGetLinkInfo(i, first);
  return 0;
}


void lnkGetAddressInfo(linkaddressinfostruct &a, void *ptr)
{
 printf("lnkGetAddressInfo ignored.\n");
//  dllGetAddressInfo(a, ptr);
}


static char reglist[1024];
static void parseinfo (char *pi, const char *key)
{
  char s[1024];
  strcpy (s,pi);
  s[strlen(s)+1]=0;

  char *dip=s;
  char keyok=0;
  char kstate=0;
  while (*dip)
  {
    char *d2p=dip;
    while (*dip)
    {
      d2p++;
      char t=*d2p;
      if (t==';' || t==' ' || !t)
      {
        *d2p=0;
        if (!kstate)
        {
          keyok = !strcmp(dip,key);
          kstate= 1;
        }
        else if (keyok)
        {
          strcat(reglist,dip);
          strcat(reglist," ");
        }

        if (t==';')
          kstate=keyok=0;

        do
          dip=++d2p;
        while (*dip && (*dip==' ' || *dip==';'));
      }
    }
  }
}

char *lnkReadInfoReg(const char *key)
{
  *reglist=0;
  for (int d=0; d<MAXDLLLIST; d++)
  {
    if(dllhandle[d])
    {
      char **pi=(char **)GetProcAddress(dllhandle[d], "_dllinfo");
      if (pi)
        parseinfo(*pi,key);
    }
  }

  if (*reglist)
    reglist[strlen(reglist)-1]=0;

  return reglist;
}


char *lnkReadInfoReg(const char *name, const char *key)
{
  *reglist=0;

  for (int d=0; d<MAXDLLLIST; d++)
  {
    if(dllname[d]&&(!stricmp(name, dllname[d])))
    {
      char **pi=(char **)GetProcAddress(dllhandle[d], "_dllinfo");
      if (pi)
        parseinfo(*pi,key);

      if (*reglist)
        reglist[strlen(reglist)-1]=0;
    }
  }
  return reglist;
}
#endif
