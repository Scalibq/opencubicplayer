// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface link info screen
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#define NO_CPIFACE_IMPORT
#include <string.h>
#include "poutput.h"
#include "cpiface.h"
#include "plinkman.h"

static int plWinFirstLine;
static int plWinHeight;

static int plHelpHeight;
static int plHelpScroll;
static int mode;


static void plDisplayHelp()
{
  plHelpHeight=lnkCountLinks()*(mode?2:1);
  if ((plHelpScroll+plWinHeight)>plHelpHeight)
    plHelpScroll=plHelpHeight-plWinHeight;
  if (plHelpScroll<0)
    plHelpScroll=0;
  displaystr(plWinFirstLine, 0, 0x09, "  Link View", 15);
  displaystr(plWinFirstLine, 15, 0x08, "press tab to toggle copyright                               ", 65);
  int y;
  int x=0;
  linkinfostruct l;
  for (y=0; y<plWinHeight; y++)
  {
    short buf[80];
    writestring(buf, 0, 0, "", 80);
    if (lnkGetLinkInfo(l, (y+plHelpScroll)/(mode?2:1)))
    {
      int dl=strlen(l.desc);
      int i;
      for (i=0; i<dl; i++)
        if (!memicmp(l.desc+i, "(c)", 3))
          break;
      const char *d2=l.desc+i;
      if (i>58)
        i=58;
      if (!((y+plHelpScroll)&1)||!mode)
      {
        writestring(buf, 2, 0x0A, l.name, 8);
        writenum(buf, 12, 0x07, (l.size+1023)>>10, 10, 6, 1);
        writestring(buf, 18, 0x07, "k", 1);
        writestring(buf, 22, 0x0F, l.desc, i);
      }
      else
      {
        char vbuf[30];
        strcpy(vbuf, "version ");
        convnum(l.ver>>16, vbuf+strlen(vbuf), 10, 3, 1);
        strcat(vbuf, ".");
        if ((signed char)(l.ver>>8)>=0)
          convnum((signed char)(l.ver>>8), vbuf+strlen(vbuf), 10, 2, 0);
        else
        {
          strcat(vbuf, "-");
          convnum(-(signed char)(l.ver>>8)/10, vbuf+strlen(vbuf), 10, 1, 0);
        }
        strcat(vbuf, ".");
        convnum((unsigned char)l.ver, vbuf+strlen(vbuf), 10, 2, 0);
        writestring(buf, 2, 0x08, vbuf, 17);
        writestring(buf, 24, 0x08, d2, 56);
      }
    }
    displaystrattr(y+plWinFirstLine+1, 0, buf, 80);
  }
}

static int plHelpKey(unsigned short key)
{
  switch (key)
  {
  case 9:
    if (mode)
      plHelpScroll/=2;
    else
      plHelpScroll*=2;
    mode=!mode;
    break;
  case 0x4900: //pgup
    plHelpScroll--;
    break;
  case 0x5100: //pgdn
    plHelpScroll++;
    break;
  case 0x8400: //ctrl-pgup
    plHelpScroll-=plWinHeight;
    break;
  case 0x7600: //ctrl-pgdn
    plHelpScroll+=plWinHeight;
    break;
  case 0x4700: //home
    plHelpScroll=0;
    break;
  case 0x4F00: //end
    plHelpScroll=plHelpHeight;
    break;
  default:
    return 0;
  }
  if ((plHelpScroll+plWinHeight)>plHelpHeight)
    plHelpScroll=plHelpHeight-plWinHeight;
  if (plHelpScroll<0)
    plHelpScroll=0;
  return 1;
}

static void hlpDraw()
{
  cpiDrawGStrings();
  plDisplayHelp();
}

static void hlpSetMode()
{
  cpiSetTextMode(0);
  plWinFirstLine=5;
  plWinHeight=19;
}

static int hlpIProcessKey(unsigned short key)
{
  switch (key)
  {
  case '\'':
    cpiSetMode("links");
    break;
  default:
    return 0;
  }
  return 1;
}

static int hlpEvent(int)
{
  return 1;
}

extern "C"
{
  cpimoderegstruct cpiModeLinks = {"links", hlpSetMode, hlpDraw, hlpIProcessKey, plHelpKey, hlpEvent};
}
