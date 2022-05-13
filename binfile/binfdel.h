// For a desciption of the binfile class, please have a look at binfile.h

#ifndef __BINFDEL_H
#define __BINFDEL_H

#include <stdlib.h>
#include "binfstd.h"

class delbinfile : public sbinfile
{
protected:
  char delname[_MAX_PATH];

public:
  delbinfile();

  int open(const char *name, int m);

  virtual errstat rawclose();
};

#endif
