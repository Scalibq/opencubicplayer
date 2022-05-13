// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface main interface code
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980517   Tammo Hinrichs <kb@nwn.de>
//    -fixed one small bug in Ctrl-Q/Ctrl-S key handler
//    -various minor changes
//  -doj980928  Dirk Jagdmann <doj@cubic.org>
//    -added cpipic.h to the #include list
//  -kb981118   Tammo Hinrichs <opencp@gmx.net>
//    -restructured key handler to let actual modes override otherwise
//     important keys
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT', along with some
//     other portability-related changes
//  -doj990328  Dirk Jagdmann <doj@cubic.org>
//    -fixed bug in delete plOpenCPPict
//    -changed note strings
//    -made title string Y2K compliant

#define NO_CPIFACE_IMPORT
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <i86.h>    // just for 'delay'
#include <time.h>
#include "pfilesel.h"
#include "psetting.h"
#include "poutput.h"
#include "plinkman.h"
#include "err.h"
#include "pmain.h"
#include "cpiface.h"
#include "cpipic.h"

#define MAXLCHAN 64


void (*plGetRealMasterVolume)(int &l, int &r);


extern cpimoderegstruct cpiModeText;


static cpifaceplayerstruct *curplayer;
void (*plSetMute)(int i, int m);
void (*plDrawGStrings)(short (*)[132]);
int (*plProcessKey)(unsigned short key);
int (*plIsEnd)();
void (*plIdle)();

char plMuteCh[MAXLCHAN];

void (*plGetMasterSample)(short *, int len, int rate, int mode);
int (*plGetLChanSample)(int ch, short *, int len, int rate, int opt);
int (*plGetPChanSample)(int ch, short *, int len, int rate, int opt);

unsigned short plNLChan;
unsigned short plNPChan;
unsigned char plSelCh;
unsigned char plChanChanged;
static signed char soloch=-1;

char plPause;

char plCompoMode;
char plPanType;


static cpimoderegstruct *cpiModes;
static cpimoderegstruct *cpiDefModes;

extern char fsLoopMods;
int plLoopMods;

short plTitleBuf[5][132];
static short plTitleBufOld[4][132];

unsigned long plEscTick;


static cpimoderegstruct *curmode;
static char curmodehandle[9];

void cpiSetGraphMode(int big)
{
  plSetGraphMode(big);
  memset(plTitleBufOld, 0xFF, sizeof(plTitleBufOld));
  plChanChanged=1;
}

void cpiSetTextMode(int size)
{
  plSetTextMode(size);
  plChanChanged=1;
}

void cpiDrawGStrings()
{
  char *verstr="  opencp " VERSION;
  char *author="(c) 1994-1999 Niklas Beisert et al.  ";
  if (plScrWidth==80)
  {
    char tstr80[81];
    strcpy(tstr80,verstr);
    while (strlen(tstr80)+strlen(author)<80)
      strcat(tstr80," ");
    strcat(tstr80,author);

    writestring(plTitleBuf[0], 0, plEscTick?0xC0:0x30, tstr80 , 80);
    if (plDrawGStrings)
      plDrawGStrings(plTitleBuf+1);
    else
    {
      writestring(plTitleBuf[1], 0, 0x07, "", 80);
      writestring(plTitleBuf[2], 0, 0x07, "", 80);
      writestring(plTitleBuf[3], 0, 0x07, "", 80);
    }
    if (!plScrMode)
    {
      writestring(plTitleBuf[4], 0, 0x08, " Ä ÄÄ ÄÄÄ ÄÄÄÄÄÄÄ80x  ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ ÄÄÄ ÄÄ Ä ", 80);
      writenum(plTitleBuf[4], 20, 0x08, plScrHeight, 10, 2, 0);

      int chann=plNLChan;
      if (chann>32)
        chann=32;
      int chan0=plSelCh-(chann/2);
      if ((chan0+chann)>=plNLChan)
        chan0=plNLChan-chann;
      if (chan0<0)
        chan0=0;

      int i;
      for (i=0; i<chann; i++)
      {
        unsigned short x;
        x='0'+(i+chan0+1)%10;
        if (plMuteCh[i+chan0]&&((i+chan0)!=plSelCh))
          x='Ä'|0x0800;
        else
        if (plMuteCh[i+chan0])
          x|=0x8000;
        else
        if ((i+chan0)!=plSelCh)
          x|=0x0800;
        else
          x|=0x0700;
        plTitleBuf[4][35+i+((i+chan0)>=plSelCh)]=x;
        if ((i+chan0)==plSelCh)
          plTitleBuf[4][35+i]=(x&~0xFF)|('0'+(i+chan0+1)/10);
      }
      if (chann)
      {
        plTitleBuf[4][34]=chan0?0x081B:0x0804;
        plTitleBuf[4][36+chann]=((chan0+chann)!=plNLChan)?0x081A:0x0804;
      }

      displaystrattr(0, 0, plTitleBuf[0], 80);
      displaystrattr(1, 0, plTitleBuf[1], 80);
      displaystrattr(2, 0, plTitleBuf[2], 80);
      displaystrattr(3, 0, plTitleBuf[3], 80);
      displaystrattr(4, 0, plTitleBuf[4], 80);
    }
    else
    {
      gupdatestr(0, 0, plTitleBuf[0], 80, plTitleBufOld[0]);
      gupdatestr(1, 0, plTitleBuf[1], 80, plTitleBufOld[1]);
      gupdatestr(2, 0, plTitleBuf[2], 80, plTitleBufOld[2]);
      gupdatestr(3, 0, plTitleBuf[3], 80, plTitleBufOld[3]);

      if (plChanChanged)
      {
        int chann=plNLChan;
        if (chann>32)
          chann=32;
        int chan0=plSelCh-(chann/2);
        if ((chan0+chann)>=plNLChan)
          chan0=plNLChan-chann;
        if (chan0<0)
          chan0=0;

        int i;
        for (i=0; i<chann; i++)
        {
          gdrawchar8(384+i*8, 64, '0'+(i+chan0+1)/10, plMuteCh[i+chan0]?8:7, 0);
          gdrawchar8(384+i*8, 72, '0'+(i+chan0+1)%10, plMuteCh[i+chan0]?8:7, 0);
          gdrawchar8(384+i*8, 80, ((i+chan0)==plSelCh)?0x18:((i==0)&&chan0)?0x1B:((i==(chann-1))&&((chan0+chann)!=plNLChan))?0x1A:' ', 15, 0);
        }
      }
    }
  }
  else
  {
    char tstr132[133];
    strcpy(tstr132,verstr);
    while (strlen(tstr132)+strlen(author)<132)
      strcat(tstr132," ");
    strcat(tstr132,author);

    writestring(plTitleBuf[0], 0, plEscTick?0xC0:0x30, tstr132, 132);
    if (plDrawGStrings)
      plDrawGStrings(plTitleBuf+1);
    else
    {
      writestring(plTitleBuf[1], 0, 0x07, "", 132);
      writestring(plTitleBuf[2], 0, 0x07, "", 132);
      writestring(plTitleBuf[3], 0, 0x07, "", 132);
    }
    if (!plScrMode)
    {
      writestring(plTitleBuf[4], 0, 0x08, " Ä ÄÄ ÄÄÄ ÄÄÄÄÄÄ132x  ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ ÄÄÄ ÄÄ Ä ", 132);
      writenum(plTitleBuf[4], 20, 0x08, plScrHeight, 10, 2, 0);

      int i;
      for (i=0; i<plNLChan; i++)
      {
        unsigned short x;
        x='0'+(i+1)%10;
        if (plMuteCh[i]&&(i!=plSelCh))
          x='Ä'|0x0800;
        else
        if (plMuteCh[i])
          x|=0x8000;
        else
        if (i!=plSelCh)
          x|=0x0800;
        else
          x|=0x0700;
        plTitleBuf[4][55+i+(i>=plSelCh)]=x;
        if (i==plSelCh)
          plTitleBuf[4][55+i]=(x&~0xFF)|('0'+(i+1)/10);
      }
      if (plNLChan)
      {
        plTitleBuf[4][54]=0x0804;
        plTitleBuf[4][56+plNLChan]=0x0804;
      }

      displaystrattr(0, 0, plTitleBuf[0], 132);
      displaystrattr(1, 0, plTitleBuf[1], 132);
      displaystrattr(2, 0, plTitleBuf[2], 132);
      displaystrattr(3, 0, plTitleBuf[3], 132);
      displaystrattr(4, 0, plTitleBuf[4], 132);
    }
    else
    {
      gupdatestr(0, 0, plTitleBuf[0]+2, 128, plTitleBufOld[0]+2);
      gupdatestr(1, 0, plTitleBuf[1]+2, 128, plTitleBufOld[1]+2);
      gupdatestr(2, 0, plTitleBuf[2]+2, 128, plTitleBufOld[2]+2);
      gupdatestr(3, 0, plTitleBuf[3]+2, 128, plTitleBufOld[3]+2);

      if (plChanChanged)
      {
        int i;
        for (i=0; i<plNLChan; i++)
        {
          char c;
          c=plMuteCh[i]?8:7;
          gdrawchar8(512+i*8, 64, '0'+(i+1)/10, c, 0);
          gdrawchar8(512+i*8, 72, '0'+(i+1)%10, c, 0);
          gdrawchar8(512+i*8, 80, (i==plSelCh)?0x18:' ', 15, 0);
        }
      }
    }
  }
}







void cpiResetScreen()
{
  curmode->SetMode();
}

static void cpiChangeMode(cpimoderegstruct *m)
{
  if (curmode)
    if (curmode->Event)
      curmode->Event(cpievClose);
  if (!m)
    m=&cpiModeText;
  curmode=m;
  if (m->Event&&!m->Event(cpievOpen))
    curmode=&cpiModeText;
  curmode->SetMode();
}

void cpiGetMode(char *hand)
{
  strcpy(hand, curmode->handle);
}

void cpiSetMode(const char *hand)
{
  cpimoderegstruct *mod;
  for (mod=cpiModes; mod; mod=mod->next)
    if (!stricmp(mod->handle, hand))
      break;
  cpiChangeMode(mod);
}

void cpiRegisterMode(cpimoderegstruct *m)
{
  if (m->Event)
    if (!m->Event(cpievInit))
      return;
  m->next=cpiModes;
  cpiModes=m;
}

static void cpiRegisterDefMode(cpimoderegstruct *m)
{
  if (m->Event)
    if (!m->Event(cpievInitAll))
      return;
  m->nextdef=cpiDefModes;
  cpiDefModes=m;
}

static int plmpInit()
{
  plCompoMode=cfGetProfileBool2(cfScreenSec, "screen", "compomode", 0, 0);
  strncpy(curmodehandle, cfGetProfileString2(cfScreenSec, "screen", "startupmode", "text"), 8);
  curmodehandle[8]=0;

  char regname[50];
  const char *regs;
  regs=lnkReadInfoReg("defmodes");
  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
      cpiRegisterDefMode((cpimoderegstruct*)reg);
  }
  cpiRegisterDefMode(&cpiModeText);

  return errOk;
}

static void plmpClose()
{
  while (cpiDefModes)
  {
    if (cpiDefModes->Event)
      cpiDefModes->Event(cpievDoneAll);
    cpiDefModes=cpiDefModes->nextdef;
  }
#ifdef DOS32
  delete [] plOpenCPPict;
#endif
}

static int linkhandle;

static int plmpOpenFile(const char *path, moduleinfostruct &info, binfile *fi)
{
  cpiModes=0;

  plLoopMods=fsLoopMods;
  plEscTick=0;
  plPause=0;

  plNLChan=0;
  plNPChan=0;
  plSetMute=0;
  plIsEnd=0;
  plIdle=0;
  plGetMasterSample=0;
  plGetRealMasterVolume=0;
  plGetLChanSample=0;
  plGetPChanSample=0;

  char secname[20];
  strcpy(secname, "filetype ");
  ultoa(info.modtype&0xFF, secname+strlen(secname), 10);

  const char *link=cfGetProfileString(secname, "pllink", "");
  const char *name=cfGetProfileString(secname, "player", "");

  linkhandle=lnkLink(link);
  if (linkhandle<0)
    return 0;

  void *fp=lnkGetSymbol(name);
  if (!fp)
  {
    lnkFree(linkhandle);
    return 0;
  }

  if (!fp)
  {
    printf("link error\r\n");
    return 0;
  }

  curplayer=(cpifaceplayerstruct*)fp;

  int retval=curplayer->OpenFile(path, info, fi);

  if (retval<0)
  {
    lnkFree(linkhandle);
#ifndef WIN32
    printf("error: %s\r\n", errGetShortString(retval));
#endif
    delay(1000);
    return 0;
  }

  cpimoderegstruct *mod;
  for (mod=cpiDefModes; mod; mod=mod->nextdef)
    cpiRegisterMode(mod);
  for (mod=cpiModes; mod; mod=mod->next)
    if (!stricmp(mod->handle, curmodehandle))
      break;
  curmode=mod;

  soloch=-1;
  memset(plMuteCh, 0, sizeof(plMuteCh));
  plSelCh=0;

  return 1;
}

static void plmpCloseFile()
{
  cpiGetMode(curmodehandle);
  curplayer->CloseFile();
  while (cpiModes)
  {
    if (cpiModes->Event)
      cpiModes->Event(cpievDone);
    cpiModes=cpiModes->next;
  }
  lnkFree(linkhandle);
}

static void plmpOpenScreen()
{
  if (!curmode)
    curmode=&cpiModeText;
  if (curmode->Event&&!curmode->Event(cpievOpen))
    curmode=&cpiModeText;
  curmode->SetMode();
}

static void plmpCloseScreen()
{
  if (curmode->Event)
    curmode->Event(cpievClose);
/*
  cpimoderegstruct *mod;
  for (mod=cpiModes; mod; mod=mod->next)
    if (mod->Event)
      mod->Event(cpievClose);
*/
}

static int cpiChanProcessKey(unsigned short key)
{
  int i;
  switch (key)
  {
  case 0x4b00: //left
    if (plSelCh)
    {
      plSelCh--;
      plChanChanged=1;
    }
    break;
  case 0x4800: //up
    plSelCh=(plSelCh-1+plNLChan)%plNLChan;
    plChanChanged=1;
    break;
  case 0x4d00: //right
    if ((plSelCh+1)<plNLChan)
    {
      plSelCh++;
      plChanChanged=1;
    }
    break;
  case 0x5000: //down
    plSelCh=(plSelCh+1)%plNLChan;
    plChanChanged=1;
    break;


  case '1': case '2': case '3': case '4': case '5':
  case '6': case '7': case '8': case '9': case '0':
  case 0x7800: case 0x7900: case 0x7A00: case 0x7B00: case 0x7C00:
  case 0x7D00: case 0x7E00: case 0x7F00: case 0x8000: case 0x8100:
    if (key=='0')
      key=9;
    else
    if (key<='9')
      key-='1';
    else
      key=(key>>8)-0x78+10;
    if (key>=plNLChan)
      break;
    plSelCh=key;

  case 'q': case 'Q':
    plMuteCh[plSelCh]=!plMuteCh[plSelCh];
    plSetMute(plSelCh, plMuteCh[plSelCh]);
    plChanChanged=1;
    break;

  case 's': case 'S':
    if (plSelCh==soloch)
    {
      for (i=0; i<plNLChan; i++)
      {
        plMuteCh[i]=0;
        plSetMute(i, plMuteCh[i]);
      }
      soloch=-1;
    }
    else
    {
      for (i=0; i<plNLChan; i++)
      {
        plMuteCh[i]=i!=plSelCh;
        plSetMute(i, plMuteCh[i]);
      }
      soloch=plSelCh;
    }
    plChanChanged=1;
    break;

  case 17: case 19:
    for (i=0; i<plNLChan; i++)
    {
      plMuteCh[i]=0;
      plSetMute(i, plMuteCh[i]);
    }
    soloch=-1;
    plChanChanged=1;
    break;
  default:
    return 0;
  }
  return 1;
}

int plmpProcessKey(unsigned short key)
{
  if (curmode->AProcessKey(key))
    return 1;
  cpimoderegstruct *mod;
  for (mod=cpiModes; mod; mod=mod->next)
    if (mod->IProcessKey(key))
      return 1;
  if (plNLChan)
    if (cpiChanProcessKey(key))
      return 1;
  if (plProcessKey)
    if (plProcessKey(key))
      return 1;
  return 0;
}


#ifdef DOS32
void releasets();
#pragma aux releasets modify [eax edx] = "mov eax, 1680h" "int 2fh"
#endif


static int plmpDrawScreen()
{
  curmode->Draw();
  plChanChanged=0;

  if (plIsEnd)
    if (plIsEnd())
      return 1;

  if (plIdle)
  {
    plIdle();
  }

  cpimoderegstruct *mod;
  for (mod=cpiModes; mod; mod=mod->next)
    mod->Event(42);

  if (plEscTick&&(clock()>(plEscTick+2*CLK_TCK)))
    plEscTick=0;

  while (ekbhit())
  {
    unsigned short key=egetch();
    if ((key&0xFF)==0xE0)
      key&=0xFF00;
    if (key&0xFF)
      key&=0x00FF;
    if (plEscTick)
    {
      plEscTick=0;
      if (key==27)
        return 2;
    }

    if (curmode->AProcessKey(key))
      continue;

    switch (key)
    {
    case 27:
      plEscTick=clock();
      break;
    case 13:
      return 3;
    case 'f': case 'F': case 0x5200:
      return 4;
    case 'd': case 'D': case 4: case 0x6f00:
      return 5;
    case 10:
      return 1;
    case 12:
      plLoopMods=!plLoopMods;
      break;
    default:
      cpimoderegstruct *mod;
      for (mod=cpiModes; mod; mod=mod->next)
        if (mod->IProcessKey(key))
          continue;
      if (plNLChan)
        if (cpiChanProcessKey(key))
          continue;
      if (plProcessKey)
        if (plProcessKey(key))
          continue;
    }
  }

#ifdef DOS32
  //IF-Abfrage hier rein
  //releasets();
#endif
  return 0;

}

static int plmpCallBack()
{
  plLoopMods=fsLoopMods;
  plmpOpenScreen();
  int stop=0;
  while (!stop)
    stop=plmpDrawScreen();
  plmpCloseScreen();
  fsLoopMods=plLoopMods;
  return stop;
}



char plNoteStr[132][4]=
{
  "c-1","c#1","d-1","d#1","e-1","f-1","f#1","g-1","g#1","a-1","a#1","b-1",
  "C-0","C#0","D-0","D#0","E-0","F-0","F#0","G-0","G#0","A-0","A#0","B-0",
  "C-1","C#1","D-1","D#1","E-1","F-1","F#1","G-1","G#1","A-1","A#1","B-1",
  "C-2","C#2","D-2","D#2","E-2","F-2","F#2","G-2","G#2","A-2","A#2","B-2",
  "C-3","C#3","D-3","D#3","E-3","F-3","F#3","G-3","G#3","A-3","A#3","B-3",
  "C-4","C#4","D-4","D#4","E-4","F-4","F#4","G-4","G#4","A-4","A#4","B-4",
  "C-5","C#5","D-5","D#5","E-5","F-5","F#5","G-5","G#5","A-5","A#5","B-5",
  "C-6","C#6","D-6","D#6","E-6","F-6","F#6","G-6","G#6","A-6","A#6","B-6",
  "C-7","C#7","D-7","D#7","E-7","F-7","F#7","G-7","G#7","A-7","A#7","B-7",
  "C-8","C#8","D-8","D#8","E-8","F-8","F#8","G-8","G#8","A-8","A#8","B-8",
  "C-9","C#9","D-9","D#9","E-9","F-9","F#9","G-9","G#9","A-9","A#9","B-9"
};


extern "C"
{
  initcloseregstruct plCPReg = {plmpInit, plmpClose};
  interfacestruct plOpenCP = {plmpOpenFile, plmpCallBack, plmpCloseFile};
  char *dllinfo = "initcloseafter _plCPReg; interface _plOpenCP; defmodes "
#ifdef DOS32
  "_cpiModeScope _cpiModeGraph _cpiModePhase _cpiModeWuerfel "
#endif
  "_cpiModeLinks; deftmodes _cpiVolCtrl _cpiTModeAnal _cpiTModeMVol _cpiTModeChan _cpiTModeInst _cpiTModeTrack; readinfos _cpiReadInfoReg";
};
