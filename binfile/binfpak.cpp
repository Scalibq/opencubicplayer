// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// pak binfile (handles files in opencp's CP.PAK file)
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981207   Felix Domke <tmbinc@gmx.net>
//    -edited for new binfile

#define NO_CPDLL_IMPORT
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include "binfpak.h"
#include "binfarc.h"
#include "binfstd.h"
#include "err.h"
#include "psetting.h"

struct packdirentry
{
  char name[0x38];
  long off;
  long len;
};

static int nfiles;
static packdirentry *dir;
static sbinfile packfile;

pakbinfile::pakbinfile()
{
  file=0;
}

errstat pakbinfile::rawclose()
{
  if(file)
    file->close();
  closemode();
  return 0;
}

errstat pakbinfile::open(const char *name)
{
  close();

  sbinfile *sf=new sbinfile;
  if (!sf)
    return -1;
  char path[_MAX_PATH];
  strcpy(path, cfDataDir);
  strcat(path, name);
  if (sf->open(path, sbinfile::openro))
  {
    delete sf;
    int i;
    for (i=0; i<nfiles; i++)
      if (!stricmp(name, dir[i].name))
        break;
    if (i==nfiles)
      return 0;
    abinfile *f=new abinfile;
    if (!f)
      return -1;
    if (f->open(packfile, dir[i].off, dir[i].len))
    {
      delete f;
      return -1;
    }
    file=f;
  }
  else
  {
    file=sf;
  }

  openpipe(*file, 0, 0, -1, -1, -1);
  return 0;
}

int pakfInit()
{
  char path[_MAX_PATH];
  strcpy(path, cfDataDir);
  strcat(path, "cp.pak");
  nfiles=0;
  dir=0;
  if (packfile.open(path, sbinfile::openro))
    return errOk;
  if (packfile.getl()!=0x4B434150)
    return errOk;
  int o=packfile.getl();
  nfiles=packfile.getl()/0x40;
  dir=new packdirentry[nfiles];
  if (!dir)
  {
    nfiles=0;
    return errGen;
  }
  packfile[o].read(dir, nfiles*0x40);
  int i,j;
  for (i=0; i<nfiles; i++)
    for (j=0; j<0x38; j++)
      if (dir[i].name[j]=='/')
        dir[i].name[j]='\\';

  return errOk;
}

void pakfClose()
{
  delete dir;
  packfile.close();
}
