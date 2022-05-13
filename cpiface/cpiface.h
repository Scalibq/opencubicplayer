// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface note dots mode
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -doj980928  Dirk Jagdmann <doj@cubic.org>
//    -deleted plReadOpenCPPic() which is now found in cpipic.h
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added some really important declspec-stuff

#ifndef __CPIFACE_H
#define __CPIFACE_H

#ifdef WIN32
#include "w32idata.h"
#endif

class binfile;
struct moduleinfostruct;
struct cpifaceplayerstruct
{
  int (*OpenFile)(const char *path, moduleinfostruct &info, binfile *f);
  void (*CloseFile)();
};

enum
{
  cpiGetSampleStereo=1, cpiGetSampleHQ=2,
};

#if defined(DOS32) || (defined(WIN32)&&defined(NO_CPIFACE_IMPORT))
extern unsigned short plNLChan;
extern unsigned short plNPChan;
extern unsigned char plSelCh;
extern unsigned char plChanChanged;
extern char plPause;
extern char plMuteCh[];
extern char plPanType;
extern int (*plProcessKey)(unsigned short key);
extern void (*plDrawGStrings)(short (*plTitleBuf)[132]);
extern void (*plGetRealMasterVolume)(int &l, int &r);
extern void (*plGetMasterSample)(short *, int len, int rate, int mode);
extern int (*plIsEnd)();
extern void (*plIdle)();
extern void (*plSetMute)(int i, int m);
extern int (*plGetLChanSample)(int ch, short *, int len, int rate, int opt);
extern int (*plGetPChanSample)(int ch, short *, int len, int rate, int opt);
#else
extern_data unsigned short plNLChan;
extern_data unsigned short plNPChan;
extern_data unsigned char plSelCh;
extern_data unsigned char plChanChanged;
extern_data char plPause;
extern_data char plMuteCh[];
extern_data char plPanType;
extern_data int (*plProcessKey)(unsigned short key);
extern_data void (*plDrawGStrings)(short (*plTitleBuf)[132]);
extern_data void (*plGetRealMasterVolume)(int &l, int &r);
extern_data void (*plGetMasterSample)(short *, int len, int rate, int mode);
extern_data int (*plIsEnd)();
extern_data void (*plIdle)();
extern_data void (*plSetMute)(int i, int m);
extern_data int (*plGetLChanSample)(int ch, short *, int len, int rate, int opt);
extern_data int (*plGetPChanSample)(int ch, short *, int len, int rate, int opt);
#endif

struct cpimoderegstruct
{
  char handle[9];
  void (*SetMode)();
  void (*Draw)();
  int (*IProcessKey)(unsigned short);
  int (*AProcessKey)(unsigned short);
  int (*Event)(int ev);
  cpimoderegstruct *next;
  cpimoderegstruct *nextdef;
};

struct cpitextmoderegstruct;

struct cpitextmodequerystruct
{
  unsigned char top;
  unsigned char xmode;
  unsigned char killprio;
  unsigned char viewprio;
  unsigned char size;
  int hgtmin;
  int hgtmax;
  cpitextmoderegstruct *owner;
};

struct cpitextmoderegstruct
{
  char handle[9];
  int (*GetWin)(cpitextmodequerystruct &q);
  void (*SetWin)(int xmin, int xwid, int ymin, int ywid);
  void (*Draw)(int focus);
  int (*IProcessKey)(unsigned short);
  int (*AProcessKey)(unsigned short);
  int (*Event)(int ev);
  int active;
  cpitextmoderegstruct *nextact;
  cpitextmoderegstruct *next;
  cpitextmoderegstruct *nextdef;
};

enum
{
  cpievOpen, cpievClose, cpievInit, cpievDone, cpievInitAll, cpievDoneAll,
  cpievGetFocus, cpievLoseFocus, cpievSetMode,
};

void cpiDrawGStrings();
void cpiSetGraphMode(int big);
void cpiSetTextMode(int size);
void cpiResetScreen();
void cpiRegisterMode(cpimoderegstruct *m);
void cpiSetMode(const char *hand);
void cpiGetMode(char *hand);
void cpiTextRegisterMode(cpitextmoderegstruct *mode);
void cpiTextSetMode(const char *name);
void cpiTextRecalc();

void plUseMessage(char **);

struct insdisplaystruct
{
  int n40,n52,n80;
  char *title80;
  char *title132;
  void (*Mark)();
  void (*Clear)();
  void (*Display)(short *buf, int len, int n, int mode);
  void (*Done)();
};

void plUseInstruments(insdisplaystruct &x);

void plUseChannels(void (*Display)(short *buf, int len, int i));

struct notedotsdata
{
  unsigned char chan;
  unsigned short note;
  unsigned short voll,volr;
  unsigned char col;
};

void plUseDots(int (*get)(notedotsdata *, int));

struct cpitrakdisplaystruct
{
  int (*getcurpos)();
  int (*getpatlen)(int n);
  const char *(*getpatname)(int n);
  void (*seektrack)(int n, int c);
  int (*startrow)();
  int (*getnote)(short *bp, int small);
  int (*getins)(short *bp);
  int (*getvol)(short *bp);
  int (*getpan)(short *bp);
  void (*getfx)(short *bp, int n);
  void (*getgcmd)(short *bp, int n);
};

void cpiTrkSetup(const cpitrakdisplaystruct &c, int npat);
#endif
