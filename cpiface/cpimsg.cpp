// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIFace song message mode
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'

#define NO_CPIFACE_IMPORT
#include <string.h>
#include "poutput.h"
#include "cpiface.h"

static short plWinFirstLine;
static short plWinHeight;

static char *const *plSongMessage;

static short plMsgScroll;
static short plMsgHeight;

static void plDisplayMessage()
{
  if ((plMsgScroll+plWinHeight)>plMsgHeight)
    plMsgScroll=plMsgHeight-plWinHeight;
  if (plMsgScroll<0)
    plMsgScroll=0;
  displaystr(plWinFirstLine-1, 0, 0x09, "   and that's what the composer really wants to tell you:", 80);
  int y;
  for (y=0; y<plWinHeight; y++)
    if ((y+plMsgScroll)<plMsgHeight)
      displaystr(y+plWinFirstLine, 0, 0x07, plSongMessage[y+plMsgScroll], 80);
    else
      displayvoid(y+plWinFirstLine, 0, 80);
}

static int plMsgKey(unsigned short key)
{
  switch (key)
  {
  case 0x4900: //pgup
    plMsgScroll--;
    break;
  case 0x5100: //pgdn
    plMsgScroll++;
    break;
  case 0x8400: //ctrl-pgup
    plMsgScroll-=plWinHeight;
    break;
  case 0x7600: //ctrl-pgdn
    plMsgScroll+=plWinHeight;
    break;
  case 0x4700: //home
    plMsgScroll=0;
    break;
  case 0x4F00: //end
    plMsgScroll=plMsgHeight;
    break;
  case 'm': case 'M':
    break;
  default:
    return 0;
  }
  if ((plMsgScroll+plWinHeight)>plMsgHeight)
    plMsgScroll=plMsgHeight-plWinHeight;
  if (plMsgScroll<0)
    plMsgScroll=0;
  return 1;
}

static void msgDraw()
{
  cpiDrawGStrings();
  plDisplayMessage();
}

static void msgSetMode()
{
  cpiSetTextMode(0);
  plWinFirstLine=6;
  plWinHeight=19;
}

static int msgIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 'm': case 'M':
    cpiSetMode("msg");
    break;
  default:
    return 0;
  }
  return 1;
}

static int msgEvent(int)
{
  return 1;
}

static cpimoderegstruct plMessageMode = {"msg", msgSetMode, msgDraw, msgIProcessKey, plMsgKey, msgEvent};

void plUseMessage(char **msg)
{
  plSongMessage=msg;
  for (plMsgHeight=0; plSongMessage[plMsgHeight]; plMsgHeight++);
  plMsgScroll=0;
  cpiRegisterMode(&plMessageMode);
}
