// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CP hypertext help viewer (CPIFACE wrapper)
//
// revision history: (please note changes here)
//  -fg980924  Fabian Giesen <gfabian@jdcs.su.nw.schule.de>
//    -first release

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include "pfilesel.h"
#include "psetting.h"
#include "poutput.h"
#include "plinkman.h"
#include "err.h"
#include "pmain.h"
#include "cpiface.h"
#include "binfile.h"
#include "binfstd.h"
#include "binfpak.h"
#include "inflate.h"
#include "cphelper.h"

static char beforehelp[9];

extern "C" cpimoderegstruct hlpHelpBrowser;

static int plHelpInit()
{
  *beforehelp=0;
  return 1;
}

static void hlpDraw()
{
#ifdef DOS32
  cpiDrawGStrings();
#endif
  brDisplayHelp();
}

static void hlpSetMode()
{
  cpiSetTextMode(0);
  brSetWinStart(6);
  brSetWinHeight(19);
}

static int hlpOpen()
{
  return 1;
}

static int hlpIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 0x6800: case 0x3b00: case 'h': case 'H': case '?': case '!': //f1
    cpiGetMode(beforehelp);
    cpiSetMode("coolhelp");
    break;
  default:
    return 0;
  }
  return 1;
}

static int plHelpKey(unsigned short key)
{
  switch(key)
  {
  case 0x6800: case 0x3b00: case 'h': case 'H': case '?': case '!': case 27: //f1
    cpiSetMode(beforehelp);
    break;
  default:
    return brHelpKey(key);
  }

  return 1;
};

static int hlpEvent(int ev)
{
  switch (ev)
  {
  case cpievOpen: return hlpOpen();
  case cpievInitAll: return plHelpInit();
  }
  return 1;
}

static int hlpGlobalInit()
{
  cpiRegisterMode(&hlpHelpBrowser);

  return errOk;
};

static void hlpGlobalClose()
{
  hlpEvent(cpievDoneAll);
};

extern "C"
{
  initcloseregstruct hlpWrIReg = { hlpGlobalInit, hlpGlobalClose };
  cpimoderegstruct hlpHelpBrowser = {"coolhelp", hlpSetMode, hlpDraw, hlpIProcessKey, plHelpKey, hlpEvent};

  char *dllinfo = "initcloseafter _hlpWrIReg; defmodes _hlpHelpBrowser";
}
