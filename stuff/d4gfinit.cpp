// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// DOS4GFIX initialisation handlers
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include "pmain.h"
#include "err.h"
#include "psetting.h"

extern "C" int InitDOS4GFix();
extern "C" void CloseDOS4GFix();

static int inited=0;

static int initd4gfix()
{
  if (!cfGetProfileBool("general", "dos4gfix", 0, 0))
    return errOk;
  if (!InitDOS4GFix())
    return errAllocMem;
  inited=1;
  return errOk;
}

static void closed4gfix()
{
  if (inited)
    CloseDOS4GFix();
  inited=0;
}

extern "C"
{
  initcloseregstruct Dos4GFixReg={initd4gfix, closed4gfix};
  char *dllinfo = "preinitclose _Dos4GFixReg";
}