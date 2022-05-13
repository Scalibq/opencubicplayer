// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// auxiliary timer setup routines (quite obsolete, but still used)
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include "timer.h"

int pollInit(void (*proc)())
{
  tmInit(proc, 17100, 32768);
  return 1;
}

void pollClose()
{
  tmClose();
}
