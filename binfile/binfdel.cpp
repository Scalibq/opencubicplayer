// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Delete binfile (sbinfile which deletes itself after closing)
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb981207   Tammo Hinrichs <openc@gmx.net>
//    -edited for new binfile version
//  -fd981210   Felix Domke <tmbinc@gmx.net>
//    -finished kb's changes.

#include <io.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include "binfdel.h"

delbinfile::delbinfile()
{
  *delname=0;
}

int delbinfile::open(const char *name, int m)
{
  *delname=0;
  int r=sbinfile::open(name, m);
  if (r==0)
    strcpy(delname, name);
  return r;
}

virtual errstat delbinfile::rawclose()
{
  sbinfile::rawclose();
  if (*delname)
    unlink(delname);
  *delname=0;
  return(0);
}
