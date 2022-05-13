// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
#ifdef          WIN32
// PMain - main module (not very much)
#endif
#ifdef          DOS32
// PMain - main module (console output, exception handling and startup code)
#endif
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    - plScreenChanged variable to notify the interfaces when the
//      screen mode has changed
//    - added command line help
//    - replaced INI link symbol reader with _dllinfo reader
//    - added screen mode check for avoiding redundant mode changes
//    - various minor changes
//  -fd981016   Felix Domke    <tmbinc@gmx.net>
//    - Win32-Port
//  -doj981213  Dirk Jagdmann  <doj@cubic.org>
//    - added the nice end ansi
//  -fd981220   Felix Domke    <tmbinc@gmx.net>
//    - added stack dump and fault-in-faultproc-check
//  -kb981224   Tammo Hinrichs <kb@ms.demo.org>
//    - cleaned up dos shell code a bit (but did not help much)
//  -doj990421  Dirk Jagdmann  <doj@cubic.org>
//    - changed conSave(), conRestore, conInit()
//  -fd990518   Felix Domke <tmbinc@gmx.net>
//    - clearscreen now works in higher-modes too. dos shell now switches
//      to mode 3

#define NO_CPDLL_IMPORT

#include <i86.h>
#include <malloc.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "binfpak.h"
#include "dpmi.h"
#include "endansi.h"
#include "err.h"
#include "plinkman.h"
#include "pmain.h"
#include "poutput.h"
#include "psetting.h"
#include "usedll.h"
#include "pindos.h"

int plScreenChanged=1;

static int conactive=0;

static void vgamode(unsigned short n)
{
#if DOS32
  union REGS r;
  r.w.ax=n;
  int386(0x10, &r, &r);
#endif
}

char getvmode();
#pragma aux getvmode modify [ax] value [al] = "mov ah,0fh" "int 10h"

void load816font();
#pragma aux load816font modify [ax bl] = "mov ax,1114h" "mov bl,0" "int 10h"

void setcurpos(unsigned short);
#pragma aux setcurpos parm [dx] modify [ax bx] = "mov ah,2" "mov bh,0" "int 10h"
void setcurshape(unsigned short);
#pragma aux setcurshape parm [cx] modify [ax] = "mov ah,1" "int 10h"
unsigned short getcurpos();
#pragma aux getcurpos value [dx] modify [ax bx] = "mov ah,3" "mov bh,0" "int 10h"
unsigned short getcurshape();
#pragma aux getcurshape value [cx] modify [ax bx] = "mov ah,3" "mov bh,0" "int 10h"

void conSave()
{
#ifdef DOS32
  if (!conactive)
    return;
  conactive=0;
#endif
}

void conRestore()
{
#ifdef DOS32
  if (conactive)
    return;
  unsigned short *screen=(unsigned short*)0xB8000;
  int x=*(short*)0x44A;
  int y=*(char*)0x484;
  for(int i=0; i<(x*(y+1)); i++)
    *screen++=0x0720;
  setcurpos(0);
  plScreenChanged=1;
  conactive=1;
#endif
}

void conInit()
{
#if 0
  if (((*(char*)0x449)!=3)||((*(short*)0x44A)!=80)||((*(char*)0x484)!=24))
    if (getvmode()!=3)
      vgamode(3);
  union REGS r;
  r.w.ax=3;
  int386(0x10, &r, &r);

#else

  vgamode(3);

#endif
  plScreenChanged=1;
  conactive=1;
}

extern char cfProgramDir[];
extern char cfDataDir[];

#ifdef DOS32

extern "C" void breakhnd();
extern "C" void crithnd();
extern "C" void pagefaulthnd();
extern "C" void zerodivhnd();
extern "C" long pagefaultstack[];

static int infault=0;

short GetDS();
#pragma aux GetDS="mov ax, ds" value [ax];

static int memGetAddress(char *dst, void *ptr)
{
  char buf[100];
  linkaddressinfostruct a;
  lnkGetAddressInfo(a, ptr);
  strcpy(dst, a.module);
  strcat(dst, ".");
  strcat(dst, a.sym);
  strcat(dst, "+0x");
  ultoa(a.symoff, buf, 16);
  strcat(dst, buf);
  strcat(dst, " (");
  strcat(dst, a.source);
  strcat(dst, ".");
  ultoa(a.line, buf, 10);
  strcat(dst, buf);
  strcat(dst, "+0x");
  ultoa(a.lineoff, buf, 16);
  strcat(dst, buf);
  strcat(dst, ")");
  return((a.symoff<0x2000)||(a.lineoff<0x2000));
}

extern "C" void faultproc(void *erraddr, void *codeptr, int type)
{
  if(infault)
  {
    printf("fault in faultproc.\n");
    exit(1);
  }
  infault++;
  conRestore();
  if (!conactive)               // leave this there, PLEASE (fd)
    vgamode(3);
  printf("sorry guys, opencp crashed!\n");

  if (type)
    printf("division overflow ");
  else
    printf("page fault (0x%X) ",erraddr);

  printf("at 0x%X:\n",codeptr);
  linkaddressinfostruct a;
  lnkGetAddressInfo(a, codeptr);
  printf("%s.%s+0x%X\n", a.module, a.sym, a.symoff);
  printf("%s.%i+0x%X\n", a.source, a.line, a.lineoff);

  long far *ptr=(long far*)MK_FP(pagefaultstack[-1], pagefaultstack[-2]);
  if((ptr[7+5]&0xFFFF)==GetDS())        // if SP!=DS, it's too risky.
  {
    unsigned long *stk=(unsigned long*)ptr[6+5];
    printf("stack content: [esp+]\n");
    for(int sp=0; sp<10; sp++)
    {
      {
        char buf[256];
        if((memGetAddress(buf, (long*)stk[sp]))&&(stk[sp]>0x2000))
          printf(" %08x      [%02d] %s\n", stk[sp], sp, buf);
#ifdef DEBUG
        else
          printf(" %08x      [%02d]\n", stk[sp], sp);
#endif
      }
    }
    printf("\n");
  }
  fflush(stdout);

  printf("\nplease restart cp and play another module to restore a clean state\n");
  exit(1);
}

#endif

unsigned char plCmdLineHelp(char *);

#ifdef DOS32

static void __far *oldint1b;
static void __far *oldint23;
static void __far *oldint24;
extern "C"
{
  extern void __far *pagefaultchain;
  extern void __far *zerodivchain;
};


static int intsinit()
{
  oldint1b=getvect(0x1b);
  oldint23=getvect(0x23);
  oldint24=getvect(0x24);
  pagefaultchain=getexvect(0x0e);
  zerodivchain=getexvect(0x00);

  setvect(0x1b, breakhnd);
  setvect(0x23, breakhnd);
  setvect(0x24, (void __interrupt (*)())crithnd);
  setexvect(0x0e, (void __interrupt (*)())pagefaulthnd);
  setexvect(0x00, (void __interrupt (*)())zerodivhnd);

  return errOk;
}

static void intsclose()
{
  setexvect(0x00, zerodivchain);
  setexvect(0x0e, pagefaultchain);
  setvect(0x1b, oldint1b);
  setvect(0x23, oldint23);
  setvect(0x24, oldint24);
}


int plSystem(const char *s)
{
  _heapshrink();
  intsclose();
  int dummy=system(s);
  intsinit();
  return dummy;
}

void plDosShell()
{
  vgamode(3);
  printf("\ntype EXIT to return to opencp\n");
  plSystem(getenv("COMSPEC"));
}

#else

int plSystem(const char *s)
{
  return system(s);
}


#endif


static initcloseregstruct *plInitClose = 0;

static int plRegisterInitClose(initcloseregstruct *r)
{
  if (r->Init)
  {
    int ret=r->Init();
    if (ret<0)
      return ret;
  }
  if (r->Close)
  {
    r->next=plInitClose;
    plInitClose=r;
  }
  return errOk;
}

static void plCloseAll()
{
  while (plInitClose)
  {
    plInitClose->Close();
    plInitClose=plInitClose->next;
  }
}

extern "C"
{
  extern const char **_argv;
};

static int confinit()
{
  char cmdlinea[300];
  char *cmdline=cmdlinea+5;
  *cmdline=0;
  getcmd(cmdline);
  if (strlen(cmdline)>120)
    if (getenv("CMDLINE"))
    {
      char *envp=getenv("CMDLINE");
      while ((*envp==' ')||(*envp=='\t'))
        envp++;
      while (*envp&&(*envp!=' ')&&(*envp!='\t'))
        envp++;
      while ((*envp==' ')||(*envp=='\t'))
        envp++;
      strcpy(cmdline, envp);
    }
  putenv("CMDLINE=");

  cfGetConfig(_argv[0], cmdline);

  cfConfigSec=cfGetProfileString("commandline", "c", "defaultconfig");

  cfSoundSec=cfGetProfileString(cfConfigSec, "soundsec", "sound");
  cfScreenSec=cfGetProfileString(cfConfigSec, "screensec", "screen");

  if (cfGetProfileString("general", "datapath", 0))
  {
    strcpy(cfDataDir, cfGetProfileString("general", "datapath", 0));
    if (cfDataDir[strlen(cfDataDir)-1]!='\\')
      strcat(cfDataDir+strlen(cfDataDir)-1, "\\");
  }
  return errOk;
}

static void confclose()
{
  cfCloseConfig();
}

static int lnkinit()
{
  if (!lnkInit())
  {
    printf("could not init link manager!\n");
    return errGen;
  }
  return errOk;
}

static void lnkclose()
{
  lnkClose();
}


static void endscr()
{
  conInit();
#if defined(DOS32) && (!defined(DEBUG))
  // place cursor at the bottom of the picure
  for (int i=0; i<endansi_DEPTH; i++)
    fprintf(stderr, "\n");
  // and copy it into screen memory
  memcpy((unsigned char*)0xB8000, endansi, endansi_LENGTH);
#endif
  printf("have a nice day...\n");
}


static int cmdhlp()
{
  if (cfGetProfileString("commandline", "h", 0) || cfGetProfileString("commandline", "?", 0))
  {
    printf("\nopencp command line help\n");
    printf("Usage: cp [<options>]* [@<playlist>]* [<modulename>]* \n");
    printf("\nOptions:\n");
    printf("-h                : show this help\n");
    printf("-c<name>          : use specific configuration\n");
    printf("-m                : use hercules card for display (if present)\n");
    printf("-f : fileselector settings\n");
    printf("     r[0|1]       : remove played files from module list\n");
    printf("     o[0|1]       : don't scramble module list order\n");
    printf("     l[0|1]       : loop modules\n");
    printf("-v : sound settings\n");
    printf("     a{0..800}    : set amplification\n");
    printf("     v{0..100}    : set volume\n");
    printf("     b{-100..100} : set balance\n");
    printf("     p{-100..100} : set panning\n");
    printf("     r{-100..100} : set reverb\n");
    printf("     c{-100..100} : set chorus\n");
    printf("     s{0|1}       : set surround on/off\n");
    printf("     f{0..2}      : set filter (0=off, 1=AOI, 2=FOI)\n");
    printf("-s : device settings\n");
    printf("     p<name>      : use specific player device\n");
    printf("     s<name>      : use specific sampler device\n");
    printf("     w<name>      : use specific wavetable device\n");
    printf("     r{0..64000}  : sample at specific rate\n");
    printf("     8            : play/sample/mix as 8bit\n");
    printf("     m            : play/sample/mix mono\n");
    printf("\nExample : cp -fl0,r1 -vp75,f2 -spdevpdisk -sr48000 ftstar.xm\n");
    printf("          (for nice HD rendering of modules)\n");
    return errGen;
  }
  return errOk;
}


static int doinits()
{
  printf("linking default objects...\n");
  if ((lnkLink(cfGetProfileString("general", "link", ""))<0)||(lnkLink(cfGetProfileString2(cfConfigSec, "defaultconfig", "link", ""))<0))
  {
    printf("could not link default objects!\n");
    return errGen;
  }

  lnkLink(cfGetProfileString2(cfConfigSec, "defaultconfig", "prelink", ""));

  printf("running initializers...\n");
  char regname[50];
  char regsline[1024];
  char const *regs;
  void *reg;
  int ret;

  strcpy(regsline,lnkReadInfoReg("preinitclose"));
  regs=regsline;
  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    reg=lnkGetSymbol(regname);
    ret=errSymSym;
    if (reg)
      ret=plRegisterInitClose((initcloseregstruct*)reg);
    if (ret<0)
      return errGen;
  }

  strcpy(regsline,lnkReadInfoReg("initclose"));
  regs=regsline;
  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    reg=lnkGetSymbol(regname);
    ret=errSymSym;
    if (reg)
      ret=plRegisterInitClose((initcloseregstruct*)reg);
    if (ret<0)
      return errGen;
  }

  strcpy(regsline,lnkReadInfoReg("initcloseafter"));
  regs=regsline;
  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    reg=lnkGetSymbol(regname);
    ret=errSymSym;
    if (reg)
      ret=plRegisterInitClose((initcloseregstruct*)reg);
    if (ret<0)
      return errGen;
  }

  strcpy(regsline,lnkReadInfoReg("main"));
  regs=regsline;
  if (strchr(regs,' '))
  {
    printf("WARNING - multiple main functions found, using first one\n");
    *strchr(regs,' ')=0;
  }

  reg=lnkGetSymbol(regs);
  if (!reg)
    return errSymSym;

  printf("running main (%s)...\n", regs);

  if (((int (*)())reg)()<0)
    return errGen;

  return errOk;
}

#ifdef DOS32
static initcloseregstruct intsreg={intsinit, intsclose};
#endif
static initcloseregstruct confreg={confinit, confclose};
static initcloseregstruct lnkreg={lnkinit, lnkclose};
static initcloseregstruct cmdhlpreg={cmdhlp, 0};
static initcloseregstruct initrunreg={doinits, 0};
static initcloseregstruct endscrreg={0, endscr};
static initcloseregstruct pakfreg={pakfInit, pakfClose};
#ifdef DOS32
static initcloseregstruct indos={indosInit, indosClose};
#endif
static initcloseregstruct *(initlist[])={
#ifdef DOS32
                                         &intsreg,
                                         &indos,
#endif
                                         &confreg,
                                         &lnkreg,
                                         &cmdhlpreg,
                                         &pakfreg,
                                         &initrunreg,
                                         &endscrreg,
                                         0};
#ifdef DOS32

unsigned char whisper();
#pragma aux whisper value [al] modify [ax] = "mov ax,0x7bce" "int 0x21"

#endif


extern char compiledate[], compiletime[], compiledby[];

#ifdef WIN32
static HINSTANCE hInst;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
  hInst=hInstance;

#else
int main(const int argc, char **argv)
{
#endif

#ifdef DOS32
  if (whisper()==0xce)
  {
    printf("whisper.tai-pan VIRUS found in memory... please remove it!\n");
    return 1;
  }
#endif

  printf(VERSION " compiled on %s, %s by %s\n", compiledate, compiletime, compiledby);

  srand(time(0));
  rand();
  rand();
  rand();

  initcloseregstruct **l;
  for (l=initlist; *l; l++)
    if (plRegisterInitClose(*l))
      break;

  plCloseAll();

#if defined(DEBUG) && defined(DOS32)
  memShowResults();
#endif

  return 0;
}

#ifdef WIN32
HINSTANCE win32GetHInstance()
{
 return(hInst);
}
#endif

const char *GetcfSoundSec()      // obsolete, don't use.
{
 return(cfSoundSec);
}

const char *GetcfConfigSec()
{
 return(cfConfigSec);
}

const char *GetcfScreenSec()
{
 return(cfScreenSec);
}

const char *GetcfCPPath()
{
 return(cfCPPath);
}

const char *GetcfConfigDir()
{
 return(cfConfigDir);
}

const char *GetcfDataDir()
{
 return(cfDataDir);
}

const char *GetcfProgramDir()
{
 return(cfProgramDir);
}

const char *GetcfTempDir()
{
 return(cfTempDir);
}

const char *GetcfCommandLine()
{
 return(cfCommandLine);
}
