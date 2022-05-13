// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CP.INI file and environment reading functions
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981014   Felix Domke <tmbinc@gmx.net>
//    -Bugfix at cfReadINIFile (first if-block, skips the filename in
//     the commandline, without these, funny errors occured.)
//  -fd981106   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile

#define NO_CPDLL_IMPORT

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "binfstd.h"
#include "psetting.h"
#include "dpmi.h"

char cfCPPath[_MAX_PATH];
char cfConfigDir[_MAX_PATH];
char cfDataDir[_MAX_PATH];
char cfProgramDir[_MAX_PATH];
char cfTempDir[_MAX_PATH];

extern char fsListScramble;
extern char fsListRemove;
extern char fsScanNames;
extern char fsScanArcs;
extern char fsScanInArc;
extern char fsScanMIF;
extern char fsWriteModInfo;
extern char fsScrType;
extern char fsEditWin;
extern char fsColorTypes;

const char *cfConfigSec;
const char *cfSoundSec;
const char *cfScreenSec;
const char *cfCommandLine;


struct profilekey
{
  char *key;
  char *str;
  int linenum;
};

struct profileapp
{
  char *app;
  profilekey *keys;
  int nkeys;
  int linenum;
};

static char *cfINIStrBuf;
static profilekey *cfINIKeys;
static profileapp *cfINIApps;
static int cfINInApps;

#ifdef DOS32

static unsigned char iscdrom(unsigned char drv)
{
  callrmstruct regs;
  regs.w.ax=0x150B;
  regs.w.bx=0;
  regs.w.cx=drv;
  intrrm(0x2F, regs);
  if (regs.w.bx!=0xADAD)
    return 0;
  return !!regs.w.ax;
}

#endif

static int readiniline(char *key, char *str, char *&inibuf, int &linenum)
{
  while (1)
  {
    while (isspace(*inibuf))
      inibuf++;
    if (*inibuf==';')
    {
      while ((*inibuf!='\r')&&(*inibuf!='\n')&&(*inibuf!=26))
        inibuf++;
    }
    if ((*inibuf=='\r')||(*inibuf=='\n'))
    {
      while ((*inibuf=='\r')||(*inibuf=='\n'))
        inibuf++;
      linenum++;
      continue;
    }
    if (*inibuf==26)
      return 0;

    const char *sol=inibuf;

    while ((*inibuf!='\r')&&(*inibuf!='\n')&&(*inibuf!=26))
      inibuf++;
    while ((*inibuf=='\r')||(*inibuf=='\n'))
      inibuf++;
    linenum++;


    const char *eol=sol;
    while ((*eol!='\r')&&(*eol!='\n')&&(*eol!=26)&&(*eol!=';'))
      eol++;
    while (isspace(eol[-1]))
      eol--;

    if ((*sol=='[')&&(eol[-1]==']'))
    {
      strcpy(key, "[]");
      if ((eol-sol)>400)
        continue;
      memcpy(str, sol+1, eol-sol-2);
      str[eol-sol-2]=0;
      return 1;
    }
    if (*sol=='=')
      continue;

    const char *chk=sol;
    while ((chk<eol)&&(*chk!='='))
      chk++;
    if (chk==eol)
      continue;
    while (isspace(chk[-1]))
      chk--;

    if ((chk-sol)>100)
      continue;
    memcpy(key, sol, chk-sol);
    key[chk-sol]=0;

    while (chk[-1]!='=')
      chk++;

    while ((chk<eol)&&isspace(*chk))
      chk++;

    if ((eol-chk)>400)
      continue;
    memcpy(str, chk, eol-chk);
    str[eol-chk]=0;

    return 2;
  }
}


static int readcmdline(char &key, char *str, const char *&cmd)
{
  while (1)
  {
    while (isspace(*cmd))
      cmd++;
    if (!*cmd)
      return 0;
    const char *cs=cmd;
    while (*cmd&&!isspace(*cmd))
      cmd++;
    if ((*cs!='/')&&(*cs!='-'))
      continue;
    cs++;
    if (!isalnum(*cs))
      continue;
    key=*cs++;
    memcpy(str, cs, cmd-cs);
    str[cmd-cs]=0;
    return 1;
  }
}

static int readcmdline2(char &key, char *str, const char *&cmd, int &state)
{
  while (1)
  {
    if (state)
    {
      while (*cmd==',')
        cmd++;
      if (isspace(*cmd))
        state=0;
    }
    while (isspace(*cmd))
      cmd++;
    if (!*cmd)
      return 0;
    if (state)
    {
      const char *cs=cmd;
      while (*cmd&&!isspace(*cmd)&&(*cmd!=','))
        cmd++;
      if (!isalnum(*cs))
        continue;
      key=*cs++;
      memcpy(str, cs, cmd-cs);
      str[cmd-cs]=0;
      return 2;
    }
    else
    {
      const char *cs=cmd;
      while (*cmd&&!isspace(*cmd))
        cmd++;
      if ((*cs!='/')&&(*cs!='-'))
        continue;
      cs++;
      if (!isalnum(*cs))
        continue;
      key=*cs++;
      cmd=cs;
      state=1;
      return 1;
    }
  }
}


static int cfReadINIFile(const char *cmd)
{

  if(cmd)
  {
    while(isspace(*cmd)) cmd++;
    while(*cmd) if(!isspace(*cmd)) cmd++; else break;
    while(isspace(*cmd)) cmd++;
  }

  char path[_MAX_PATH];

  strcpy(path, cfConfigDir);
#ifdef DOS32
  strcat(path, "cp.ini");
#else
  strcat(path, "cpwin.ini");
#endif

  cfINIApps=0;
  cfINIKeys=0;
  cfINInApps=0;
  cfINIStrBuf=0;

  sbinfile f;
  if(f.open(path, sbinfile::openro))
  {
    sbinfile f2;
    strcpy(path, cfProgramDir);
    strcat(path, "cp.ini");
    if (f2.open(path, sbinfile::openro))
      return 1;

    int f2len=f2.length();
    char *copybuf=new char [f2len];
    if (!copybuf)
      return 1;
    f2.read(copybuf, f2len);
    f2.close();

    strcpy(path, cfConfigDir);
    strcat(path, "cp.ini");
    if(f.open(path, sbinfile::opencr))
      return 1;
    f.write(copybuf, f2len);
    f.close();

    delete copybuf;

    if(f.open(path, sbinfile::openro))
      return 1;
  }

  int flen=f.length();
  char *inibuf=new char [flen+1];
  if(!inibuf)
    return 0;
  f.read(inibuf, flen);
  inibuf[flen]=26;
  f.close();

  int appnum=0;
  int keynum=0;
  int strlens=0;

  char keybuf[105];
  char strbuf[405];

  char *iniptr=inibuf;
  int linenum=0;
  while (1)
  {
    switch (readiniline(keybuf, strbuf, iniptr, linenum))
    {
    case 0:
      break;
    case 1:
      appnum++;
      strlens+=strlen(strbuf)+1;
      continue;
    case 2:
      keynum++;
      strlens+=strlen(keybuf)+strlen(strbuf)+2;
      continue;
    }
    break;
  }
  const char *cmdptr=cmd;
  int cmdstat=0;
  while (1)
  {
    switch (readcmdline2(*keybuf, strbuf, cmdptr, cmdstat))
    {
    case 0:
      break;
    case 1:
      appnum++;
      strlens+=14;
      continue;
    case 2:
      keynum++;
      strlens+=strlen(strbuf)+3;
      continue;
    }
    break;
  }
  cmdptr=cmd;
  appnum++;
  strlens+=12;
  while (readcmdline(*keybuf, strbuf, cmdptr))
  {
    keynum++;
    strlens+=strlen(strbuf)+3;
  }
  strlens+=strlen(cmd)+1;

  cfINIStrBuf=new char [strlens];
  cfINIApps=new profileapp [appnum];
  cfINIKeys=new profilekey [keynum];
  if (!cfINIStrBuf||!cfINIApps||!cfINIKeys)
    return 0;

  int i;
  char *strptr=cfINIStrBuf;

  memset(cfINIApps, 0, sizeof(*cfINIApps)*appnum);
  cfINInApps=0;

  int curapp=-1;

  iniptr=inibuf;
  linenum=0;
  while (1)
  {
    switch (readiniline(keybuf, strbuf, iniptr, linenum))
    {
    case 0:
      break;
    case 1:
      cfINIApps[cfINInApps].linenum=linenum-1;
      cfINIApps[cfINInApps++].app=strptr;
      strcpy(strptr, strbuf);
      strptr+=strlen(strptr)+1;
      continue;
    case 2:
      if (cfINInApps)
        cfINIApps[cfINInApps-1].nkeys++;
      continue;
    }
    break;
  }

  cmdptr=cmd;
  cmdstat=0;
  while (1)
  {
    switch (readcmdline2(*keybuf, strbuf, cmdptr, cmdstat))
    {
    case 0:
      break;
    case 1:
      cfINIApps[cfINInApps].linenum=-1;
      cfINIApps[cfINInApps++].app=strptr;
      memcpy(strptr, "commandline_", 12);
      strptr[12]=*keybuf;
      strptr[13]=0;
      strptr+=14;
      continue;
    case 2:
      cfINIApps[cfINInApps-1].nkeys++;
      continue;
    }
    break;
  }
  cfINIApps[cfINInApps].linenum=-1;
  cfINIApps[cfINInApps].app=strptr;
  strcpy(strptr, "CommandLine");
  strptr+=strlen(strptr)+1;
  cmdptr=cmd;
  while (readcmdline(*keybuf, strbuf, cmdptr))
    cfINIApps[cfINInApps].nkeys++;
  cfINInApps++;


  keynum=0;
  for (i=0; i<cfINInApps; i++)
  {
    cfINIApps[i].keys=cfINIKeys+keynum;
    keynum+=cfINIApps[i].nkeys;
    cfINIApps[i].nkeys=0;
  }

  cfINInApps=0;
  iniptr=inibuf;
  linenum=0;
  while (1)
  {
    switch (readiniline(keybuf, strbuf, iniptr, linenum))
    {
    case 0:
      break;
    case 1:
      cfINInApps++;
      continue;
    case 2:
      if (!cfINInApps)
        continue;
      cfINIApps[cfINInApps-1].keys[cfINIApps[cfINInApps-1].nkeys].linenum=linenum-1;
      cfINIApps[cfINInApps-1].keys[cfINIApps[cfINInApps-1].nkeys].key=strptr;
      strcpy(strptr, keybuf);
      strptr+=strlen(strptr)+1;
      cfINIApps[cfINInApps-1].keys[cfINIApps[cfINInApps-1].nkeys++].str=strptr;
      strcpy(strptr, strbuf);
      strptr+=strlen(strptr)+1;
      continue;
    }
    break;
  }

  cmdptr=cmd;
  cmdstat=0;
  while (1)
  {
    switch (readcmdline2(*keybuf, strbuf, cmdptr, cmdstat))
    {
    case 0:
      break;
    case 1:
      cfINInApps++;
      continue;
    case 2:
      cfINIApps[cfINInApps-1].keys[cfINIApps[cfINInApps-1].nkeys].linenum=-1;
      cfINIApps[cfINInApps-1].keys[cfINIApps[cfINInApps-1].nkeys].key=strptr;
      *strptr++=*keybuf;
      *strptr++=0;
      cfINIApps[cfINInApps-1].keys[cfINIApps[cfINInApps-1].nkeys++].str=strptr;
      strcpy(strptr, strbuf);
      strptr+=strlen(strptr)+1;
      continue;
    }
    break;
  }

  cmdptr=cmd;
  while (readcmdline(*keybuf, strbuf, cmdptr))
  {
    cfINIApps[cfINInApps].keys[cfINIApps[cfINInApps].nkeys].linenum=-1;
    cfINIApps[cfINInApps].keys[cfINIApps[cfINInApps].nkeys].key=strptr;
    *strptr++=*keybuf;
    *strptr++=0;
    cfINIApps[cfINInApps].keys[cfINIApps[cfINInApps].nkeys++].str=strptr;
    strcpy(strptr, strbuf);
    strptr+=strlen(strptr)+1;
  }
  cfINInApps++;
  strcpy(strptr, cmd);
  cfCommandLine=strptr;
  strptr+=strlen(strptr);

  delete inibuf;
  return 1;
}

static void cfFreeINI()
{
  delete cfINIStrBuf;
  delete cfINIApps;
  delete cfINIKeys;
}


void cfGetConfig(const char *cppath, const char *cmdline)
{
  strcpy(cfCPPath, cppath);
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  _splitpath(cppath, drive, dir, 0, 0);
  _makepath(cfProgramDir, drive, dir, 0, 0);
  strcpy(cfDataDir, cfProgramDir);

  if (getenv("CPDIR"))
  {
    strcpy(cfConfigDir, getenv("CPDIR"));
    if (cfConfigDir[strlen(cfConfigDir)-1]!='\\')
      strcat(cfConfigDir, "\\");
  }
  else
#ifdef DOS32
    if (iscdrom(*drive-'A'))
      strcpy(cfConfigDir, "C:\\");
    else
      strcpy(cfConfigDir, cfDataDir);
#endif
#ifdef WIN32
      strcpy(cfConfigDir, cfDataDir);
#endif

  if (cfDataDir[strlen(cfDataDir)-1]!='\\')
    strcat(cfDataDir, "\\");

  cfReadINIFile(cmdline);

  const char *t=getenv("TEMP");
  if (!t)
    t=getenv("TMP");
  if (!t)
    t="C:\\";
  t=cfGetProfileString("general", "tempdir", t);
  strcpy(cfTempDir, t);
  if (cfTempDir[strlen(cfTempDir)-1]!='\\')
    strcat(cfTempDir, "\\");
}

void cfCloseConfig()
{
  cfFreeINI();
}

const char *cfGetProfileString(const char *app, const char *key, const char *def)
{
  int i,j;
  for (i=0; i<cfINInApps; i++)
    if (!stricmp(cfINIApps[i].app, app))
      for (j=0; j<cfINIApps[i].nkeys; j++)
        if (!stricmp(cfINIApps[i].keys[j].key, key))
          return cfINIApps[i].keys[j].str;
  return def;
}

const char *cfGetProfileString2(const char *app, const char *app2, const char *key, const char *def)
{
  return cfGetProfileString(app, key, cfGetProfileString(app2, key, def));
}

int cfGetProfileInt(const char *app, const char *key, int def, int radix)
{
  const char *s=cfGetProfileString(app, key, "");
  if (!*s)
    return def;
  return strtol(s, 0, radix);
}

int cfGetProfileInt2(const char *app, const char *app2, const char *key, int def, int radix)
{
  return cfGetProfileInt(app, key, cfGetProfileInt(app2, key, def, radix), radix);
}

int cfGetProfileBool(const char *app, const char *key, int def, int err)
{
  const char *s=cfGetProfileString(app, key, 0);
  if (!s)
    return def;
  if (!*s)
    return err;
  if (!stricmp(s, "on")||!stricmp(s, "yes")||!stricmp(s, "+")||!stricmp(s, "true")||!stricmp(s, "1"))
    return 1;
  if (!stricmp(s, "off")||!stricmp(s, "no")||!stricmp(s, "-")||!stricmp(s, "false")||!stricmp(s, "0"))
    return 0;
  return err;
}

int cfGetProfileBool2(const char *app, const char *app2, const char *key, int def, int err)
{
  return cfGetProfileBool(app, key, cfGetProfileBool(app2, key, def, err), err);
}


int cfCountSpaceList(const char *str, int maxlen)
{
  int i=0;
  while (1)
  {
    while (isspace(*str))
      str++;
    if (!*str)
      return i;
    const char *fb=str;
    while (!isspace(*str)&&*str)
      str++;
    if ((str-fb)<=maxlen)
      i++;
  }
}

int cfGetSpaceListEntry(char *buf, const char *&str, int maxlen)
{
  while (1)
  {
    while (isspace(*str))
      str++;
    if (!*str)
      return 0;
    const char *fb=str;
    while (!isspace(*str)&&*str)
      str++;
    if ((str-fb)>maxlen)
      continue;
    memcpy(buf, fb, str-fb);
    buf[str-fb]=0;
    return 1;
  }
}
