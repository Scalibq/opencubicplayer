// For a desciption of the binfile class, please have a look at binfile.h

#ifndef __BINFPAK_H
#define __BINFPAK_H

#include "binfile.h"

class pakbinfile : public binfile
{
protected:
  binfile *file;

public:
  pakbinfile();

  errstat open(const char *name);

  virtual errstat rawclose();
};

int pakfInit();
void pakfClose();

#endif
