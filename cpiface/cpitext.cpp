// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIFace text modes master mode and window handler
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'

#define NO_CPIFACE_IMPORT
#include <string.h>
#include "poutput.h"
#include "pmain.h"
#include "cpiface.h"
#include "psetting.h"
#include "plinkman.h"

static int scrtype;

static cpitextmoderegstruct *cpiTextActModes;
static cpitextmoderegstruct *cpiTextModes;
static cpitextmoderegstruct *cpiTextDefModes;
static cpitextmoderegstruct *cpiFocus;
static char cpiFocusHandle[9];
static int modeactive;

void cpiTextRegisterMode(cpitextmoderegstruct *mode)
{
  if (mode->Event&&!mode->Event(cpievInit))
    return;
  mode->next=cpiTextModes;
  cpiTextModes=mode;
}

static void cpiTextRegisterDefMode(cpitextmoderegstruct *mode)
{
  if (mode->Event&&!mode->Event(cpievInitAll))
    return;
  mode->nextdef=cpiTextDefModes;
  cpiTextDefModes=mode;
}

static void cpiSetFocus(const char *name)
{
  if (cpiFocus&&cpiFocus->Event)
    cpiFocus->Event(cpievLoseFocus);
  cpiFocus=0;
  cpitextmoderegstruct *mode;
  if (!name)
  {
    *cpiFocusHandle=0;
    return;
  }
  for (mode=cpiTextActModes; mode; mode=mode->nextact)
    if (!stricmp(name, mode->handle))
      break;
  *cpiFocusHandle=0;
  if (!mode||(mode->Event&&!mode->Event(cpievGetFocus)))
    return;
  cpiFocus=mode;
  strcpy(cpiFocusHandle, cpiFocus->handle);
}

void cpiTextSetMode(const char *name)
{
  if (!name)
    name=cpiFocusHandle;
  if (!modeactive)
  {
    strcpy(cpiFocusHandle, name);
    cpiSetMode("text");
  }
  else
    cpiSetFocus(name);
}

void cpiTextRecalc()
{
  plChanChanged=1;
  int i;
  int winfirst=5;
  int winheight=plScrHeight-winfirst;
  int sidefirst=5;
  int sideheight=plScrHeight-sidefirst;

  cpitextmodequerystruct win[10];
  int nwin=0;

  cpitextmoderegstruct *mode;
  for (mode=cpiTextActModes; mode; mode=mode->nextact)
  {
    mode->active=0;
    if (mode->GetWin(win[nwin]))
      win[nwin++].owner=mode;
  }
  if (plScrWidth==80)
    for (i=0; i<nwin; i++)
      win[i].xmode&=1;

  int sidemin,sidemax,sidesize;
  int winmin,winmax,winsize;
  while (1)
  {
    sidemin=sidemax=sidesize=0;
    winmin=winmax=winsize=0;
    for (i=0; i<nwin; i++)
    {
      if (win[i].xmode&1)
      {
        winmin+=win[i].hgtmin;
        winmax+=win[i].hgtmax;
        winsize+=win[i].size;
      }
      if (win[i].xmode&2)
      {
        sidemin+=win[i].hgtmin;
        sidemax+=win[i].hgtmax;
        sidesize+=win[i].size;
      }
    }
    if ((winmin<=winheight)&&(sidemin<=sideheight))
      break;
    if (sidemin>sideheight)
    {
      int worst=0;
      for (i=0; i<nwin; i++)
        if (win[i].xmode&2)
          if (win[i].killprio>win[worst].killprio)
            worst=i;
      win[i].xmode=0;
      continue;
    }
    if (winmin>winheight)
    {
      int worst=0;
      for (i=0; i<nwin; i++)
        if (win[i].xmode&1)
          if (win[i].killprio>win[worst].killprio)
            worst=i;
      win[i].xmode=0;
      continue;
    }
  }
  {
    for (i=0; i<nwin; i++)
      win[i].owner->active=0;
    while (1)
    {
      int best=-1;
      for (i=0; i<nwin; i++)
        if ((win[i].xmode==3)&&!win[i].owner->active)
          if ((best==-1)||(win[i].viewprio>win[best].viewprio))
            best=i;
      if (best==-1)
        break;
      int whgt,shgt,hgt;
      if (!win[best].size)
        hgt=win[best].hgtmin;
      else
      {
        whgt=win[best].hgtmin+(winheight-winmin)*win[best].size/winsize;
        shgt=win[best].hgtmin+(sideheight-sidemin)*win[best].size/sidesize;
        hgt=(whgt<shgt)?whgt:shgt;
      }
      if (hgt>win[best].hgtmax)
        hgt=win[best].hgtmax;
      if (win[best].top)
      {
        win[best].owner->SetWin(0, plScrWidth, winfirst, hgt);
        winfirst+=hgt;
        sidefirst+=hgt;
      }
      else
        win[best].owner->SetWin(0, plScrWidth, winfirst+winheight-hgt, hgt);
      win[best].owner->active=1;
      winheight-=hgt;
      sideheight-=hgt;
      winmin-=win[best].hgtmin;
      winsize-=win[best].size;
      sidemin-=win[best].hgtmin;
      sidesize-=win[best].size;
    }
    while (1)
    {
      int best=-1;
      for (i=0; i<nwin; i++)
        if ((win[i].xmode==2)&&!win[i].owner->active)
          if ((best==-1)||(win[i].viewprio>win[best].viewprio))
            best=i;
      if (best==-1)
        break;
      int hgt=win[best].hgtmin;
      if (win[best].size)
        hgt+=(sideheight-sidemin)*win[best].size/sidesize;
      if (hgt>win[best].hgtmax)
        hgt=win[best].hgtmax;
      if (win[best].top)
      {
        win[best].owner->SetWin(80, 52, sidefirst, hgt);
        sidefirst+=hgt;
      }
      else
        win[best].owner->SetWin(80, 52, sidefirst+sideheight-hgt, hgt);
      win[best].owner->active=1;
      sideheight-=hgt;
      sidemin-=win[best].hgtmin;
      sidesize-=win[best].size;
    }
    while (1)
    {
      int best=-1;
      for (i=0; i<nwin; i++)
        if ((win[i].xmode==1)&&!win[i].owner->active)
          if ((best==-1)||(win[i].viewprio>win[best].viewprio))
            best=i;
      if (best==-1)
        break;
      int hgt=win[best].hgtmin;
      if (win[best].size)
        hgt+=(winheight-winmin)*win[best].size/winsize;
      if (hgt>win[best].hgtmax)
        hgt=win[best].hgtmax;
      int wid;
      if (win[best].top)
      {
        wid=((winfirst>=sidefirst)&&((winfirst+hgt)<=(sidefirst+sideheight))&&(plScrWidth==132))?132:80;
        win[best].owner->SetWin(0, wid, winfirst, hgt);
        winfirst+=hgt;
      }
      else
      {
        wid=(((winfirst+winheight)<=(sidefirst+sideheight))&&((winfirst+winheight-hgt)>=sidefirst)&&(plScrWidth==132))?132:80;
        win[best].owner->SetWin(0, wid, winfirst+winheight-hgt, hgt);
      }
      win[best].owner->active=1;
      winheight-=hgt;
      winmin-=win[best].hgtmin;
      winsize-=win[best].size;
      if (wid==132)
        if (win[best].top)
        {
          for (i=sidefirst; i<winfirst; i++)
            displayvoid(i, 80, 52);
          sideheight=sidefirst+sideheight-winfirst;
          sidefirst=winfirst;
        }
        else
        {
          for (i=winfirst+hgt; i<(sidefirst+sideheight); i++)
            displayvoid(i, 80, 52);
          sideheight=winfirst+winheight-sidefirst;
        }
    }
  }
  for (i=0; i<winheight; i++)
    displayvoid(winfirst+i, 0, 80);
  for (i=0; i<sideheight; i++)
    displayvoid(sidefirst+i, 80, 52);
}

static void txtSetMode()
{
  plSetTextMode(scrtype);
  scrtype=plScrType;
  cpitextmoderegstruct *mode;
  for (mode=cpiTextActModes; mode; mode=mode->nextact)
    if (mode->Event)
      mode->Event(cpievSetMode);
  cpiTextRecalc();
}

static void txtDraw()
{
  cpiDrawGStrings();
  cpitextmoderegstruct *mode;
  for (mode=cpiTextActModes; mode; mode=mode->nextact)
    if (mode->active)
      mode->Draw(mode==cpiFocus);
  for (mode=cpiTextModes; mode; mode=mode->next)
    mode->Event(42);
}

static int txtIProcessKey(unsigned short key)
{
  cpitextmoderegstruct *mode;
  for (mode=cpiTextModes; mode; mode=mode->next)
    if (mode->IProcessKey(key))
      return 1;
  switch (key)
  {
  case 'x': case 'X':
    scrtype=7;
    cpiTextSetMode(cpiFocusHandle);
    return 1;
  case 0x2d00: //alt-x
    scrtype=0;
    cpiTextSetMode(cpiFocusHandle);
    return 1;
  case 'z': case 'Z':
    cpiTextSetMode(cpiFocusHandle);
    break;
  default:
    return 0;
  }
  return 1;
}

static int txtAProcessKey(unsigned short key)
{
  if (cpiFocus)
    cpiFocus->AProcessKey(key);
  cpitextmoderegstruct *mode;
  for (mode=cpiTextModes; mode; mode=mode->next)
    if (mode->IProcessKey(key))
      return 1;
  switch (key)
  {
  case 'x': case 'X':
    scrtype=7;
    cpiResetScreen();
    return 1;
  case 0x2d00: //alt-x
    scrtype=0;
    cpiResetScreen();
    return 1;
  case 'z': case 'Z':
    scrtype^=2;
    cpiResetScreen();
    break;
  case 0x2c00:
    scrtype^=4;
    cpiResetScreen();
    break;
  case 26:
    scrtype^=1;
    cpiResetScreen();
    break;
  default:
    return 0;
  }
  return 1;
}

static int txtInit()
{
  cpitextmoderegstruct *mode;
  for (mode=cpiTextDefModes; mode; mode=mode->nextdef)
    cpiTextRegisterMode(mode);
  cpiSetFocus(cpiFocusHandle);
  return 1;
}

static void txtClose()
{
  cpitextmoderegstruct *mode;
  for (mode=cpiTextModes; mode; mode=mode->next)
    if (mode->Event)
      mode->Event(cpievDone);
  cpiTextModes=0;
}

static int txtInitAll()
{
  cpiTextModes=0;
  cpiTextDefModes=0;

  char regname[50];
  const char *regs;
  regs=lnkReadInfoReg("deftmodes");
  //cfGetProfileString2(cfScreenSec, "screen", "deftmodes", "");
  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
      cpiTextRegisterDefMode((cpitextmoderegstruct*)reg);
  }

  scrtype=cfGetProfileInt2(cfScreenSec, "screen", "screentype", 7, 10);

  return 1;
}

static void txtCloseAll()
{
  cpitextmoderegstruct *mode;
  for (mode=cpiTextDefModes; mode; mode=mode->nextdef)
    if (mode->Event)
      mode->Event(cpievDoneAll);
  cpiTextDefModes=0;
}

static int txtOpenMode()
{
  modeactive=1;
  cpiTextActModes=0;
  cpitextmoderegstruct *mode;
  for (mode=cpiTextModes; mode; mode=mode->next)
  {
    if (mode->Event&&!mode->Event(cpievOpen))
      continue;
    mode->nextact=cpiTextActModes;
    cpiTextActModes=mode;
  }
  cpiSetFocus(cpiFocusHandle);

  return 1;
}

static void txtCloseMode()
{
  cpiSetFocus(0);
  cpitextmoderegstruct *mode;
  for (mode=cpiTextActModes; mode; mode=mode->nextact)
    if (mode->Event)
      mode->Event(cpievClose);
  cpiTextActModes=0;
  modeactive=0;
}

static int txtEvent(int ev)
{
  switch (ev)
  {
  case cpievOpen: return txtOpenMode();
  case cpievClose: txtCloseMode(); return 1;
  case cpievInit: return txtInit();
  case cpievDone: txtClose(); return 1;
  case cpievInitAll: return txtInitAll();
  case cpievDoneAll: txtCloseAll(); return 1;
  }
  return 1;
}

cpimoderegstruct cpiModeText = {"text", txtSetMode, txtDraw, txtIProcessKey, txtAProcessKey, txtEvent};
