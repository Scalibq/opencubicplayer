// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CP hypertext help viewer (Fileselector wrapper)
//
// revision history: (please note changes here)
//  -fg980924  Fabian Giesen <gfabian@jdcs.su.nw.schule.de>
//    -first release

#include "poutput.h"
#include "cphelper.h"
#include "cphlpfs.h"

int fsmode;

// ripped from fileselector


// the wrapper

static int plHelpKey(unsigned short key)
{
  switch(key)
  {
  case 0x6800: case 0x3b00: case 'h': case 'H': case '?': case '!':
  case 0x001b:
    fsmode=0;
    break;
  default:
    return brHelpKey(key);
  }

  return 1;
};

unsigned char fsHelp2()
{
  helppage *cont;

  plSetTextMode(0);
  displaystr(0, 0, 0x30, "  opencp help                                (c) '94-'98 Niklas Beisert et al.  ", 80);

  cont=brDecodeRef("Contents");

  if (!cont) displaystr(1, 0, 0x04, "shit!", 5);

  brSetPage(cont);

  brSetWinStart(2);
  brSetWinHeight(23);

  fsmode=1;

  while (fsmode)
  {
    brDisplayHelp();

    while (!ekbhit());

    unsigned short key=egetch();

    if ((key&0xFF)==0xE0) key&=0xFF00;
    if (key&0xFF) key&=0x00FF;

    plHelpKey(key);
  };

  return 1;
};