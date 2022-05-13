// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Interface routines for Audio CD player
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pfilesel.h"
#include "cdaudio.h"
#include "poutput.h"
#include "sampler.h"
#include "psetting.h"
#include "pmain.h"
#include "err.h"
#include "devisamp.h"
#include "cpiface.h"

extern int plLoopMods;

static int nodevice;
static char cfCDAtLineIn;
static unsigned long cdpTrackStarts[33];
static unsigned char cdpFirstTrack;
static unsigned char cdpTrackNum;
static char cdpDriveName[_MAX_DRIVE];
static unsigned char cdpDrive;
static unsigned char cdpPlayMode;
static unsigned char cdpViewSectors;
static unsigned long basesec;
static unsigned long length;
static signed long newpos;
static unsigned char setnewpos;
static unsigned long curpos;
static unsigned short status;


static char *gettimestr(unsigned long s, char *time)
{
  unsigned char csec=(s%75)*4/3;
  time[8]=0;
  time[7]='0'+csec%10;
  time[6]='0'+csec/10;
  unsigned char sec=(s/75)%60;
  time[5]=':';
  time[4]='0'+sec%10;
  time[3]='0'+sec/10;
  unsigned char min=s/(75*60);
  time[2]=':';
  time[1]='0'+min%10;
  time[0]='0'+min/10;
  return time;
}

static void cdaDrawGStrings(short (*buf)[132])
{
  writestring(buf[0], 0, 0x09, "  mode: ..........  ", 20);
  writestring(buf[0], 8, 0x0F, "cd-audio", 10);

  int i;
  for (i=1; i<=cdpTrackNum; i++)
    if (curpos<cdpTrackStarts[i])
      break;

  writestring(buf[0], 20, 0x09, "playmode: .....  status: .......", plScrWidth-20);
  writestring(buf[0], 30, 0x0F, cdpPlayMode?"disk ":"track", 5);
  writestring(buf[0], 45, 0x0F, (status&0x8000)?"  error":(status&0x200)?"playing":" paused", 7);

  char timestr[9];
  writestring(buf[1], 0, 0x09, "  drive: .:  start:   :..:..  pos:   :..:..  length:   :..:..  size: ...... kb  ", plScrWidth);
  writestring(buf[1], 9, 0x0F, cdpDriveName, 2);
  if (cdpViewSectors)
  {
    writenum(buf[1], 20, 0x0F, cdpTrackStarts[0], 10, 8, 0);
    writenum(buf[1], 35, 0x0F, curpos-cdpTrackStarts[0], 10, 8, 0);
    writenum(buf[1], 53, 0x0F, cdpTrackStarts[cdpTrackNum]-cdpTrackStarts[0], 10, 8, 0);
  }
  else
  {
    writestring(buf[1], 20, 0x0F, gettimestr(cdpTrackStarts[0]+150, timestr), 8);
    writestring(buf[1], 35, 0x0F, gettimestr(curpos-cdpTrackStarts[0], timestr), 8);
    writestring(buf[1], 53, 0x0F, gettimestr(cdpTrackStarts[cdpTrackNum]-cdpTrackStarts[0], timestr), 8);
  }
  writenum(buf[1], 69, 0x0F, (cdpTrackStarts[cdpTrackNum]-cdpTrackStarts[0])*147/64, 10, 6);

  writestring(buf[2], 0, 0x09, "  track: ..  start:   :..:..  pos:   :..:..  length:   :..:..  size: ...... kb  ", plScrWidth);
  writenum(buf[2], 9, 0x0F, i-1+cdpFirstTrack, 10, 2);
  if (cdpViewSectors)
  {
    writenum(buf[2], 20, 0x0F, cdpTrackStarts[i-1]+150, 10, 8, 0);
    writenum(buf[2], 35, 0x0F, curpos-cdpTrackStarts[i-1], 10, 8, 0);
    writenum(buf[2], 53, 0x0F, cdpTrackStarts[i]-cdpTrackStarts[i-1], 10, 8, 0);
  }
  else
  {
    writestring(buf[2], 20, 0x0F, gettimestr(cdpTrackStarts[i-1]+150, timestr), 8);
    writestring(buf[2], 35, 0x0F, gettimestr(curpos-cdpTrackStarts[i-1], timestr), 8);
    writestring(buf[2], 53, 0x0F, gettimestr(cdpTrackStarts[i]-cdpTrackStarts[i-1], timestr), 8);
  }
  writenum(buf[2], 69, 0x0F, (cdpTrackStarts[i]-cdpTrackStarts[i-1])*147/64, 10, 6);
}


static int cdaProcessKey(unsigned short key)
{
  int i;
  switch (key)
  {
  case 'p': case 'P': case 0x10:
    plPause=!plPause;
    if (plPause)
      cdStop(cdpDrive);
    else
      cdRestart(cdpDrive);
    break;
  case 0x2000:
    cdpPlayMode=1;
    setnewpos=0;
    basesec=cdpTrackStarts[0];
    length=cdpTrackStarts[cdpTrackNum]-basesec;
//    strcpy(name, "DISK");
    break;
  case 0x4800: //up
    newpos-=75;
    setnewpos=1;
    break;
  case 0x5000: //down
    newpos+=75;
    setnewpos=1;
    break;
  case 0x4b00: //left
    newpos-=75*10;
    setnewpos=1;
    break;
  case 0x4d00: //right
    newpos+=75*10;
    setnewpos=1;
    break;
  case 0x9700: //home
    if (!cdpPlayMode)
    {
      newpos=0;
      setnewpos=1;
      break;
    }
    for (i=1; i<=cdpTrackNum; i++)
      if (newpos<(cdpTrackStarts[i]-basesec))
        break;
    newpos=cdpTrackStarts[i-1]-basesec;
    setnewpos=1;
    break;
  case 0x8D00: //ctrl-up
    newpos-=60*75;
    setnewpos=1;
    break;
  case 0x9100: //ctrl-down
    newpos+=60*75;
    setnewpos=1;
    break;
  case 0x7300: //ctrl-left
    if (!cdpPlayMode)
      break;
    for (i=2; i<=cdpTrackNum; i++)
      if (newpos<(cdpTrackStarts[i]-basesec))
        break;
    newpos=cdpTrackStarts[i-2]-basesec;
    setnewpos=1;
    break;
  case 0x7400: //ctrl-right
    if (!cdpPlayMode)
      break;
    for (i=1; i<=cdpTrackNum; i++)
      if (newpos<(cdpTrackStarts[i]-basesec))
        break;
    newpos=cdpTrackStarts[i]-basesec;
    setnewpos=1;
    break;
  case 0x7700: //ctrl-home
    newpos=0;
    setnewpos=1;
    break;
  default:
    if (smpProcessKey)
    {
      int ret=smpProcessKey(key);
      if (ret==2)
        cpiResetScreen();
      if (ret)
        return 1;
    }
    return 0;
  }
  return 1;
}

static unsigned char cdCheck()
{
  curpos=cdGetHeadLocation(cdpDrive, status);
  if (status&0x8000)
    return 1;
  if (!(status&0x200)&&!plPause&&!setnewpos)
  {
    setnewpos=1;
    newpos=length;
  }

  if (setnewpos)
  {
    if (newpos<0)
      newpos=0;
    if (newpos>=length)
      if (plLoopMods)
        newpos=0;
      else
        return 1;
    cdStop(cdpDrive);
    cdPlay(cdpDrive, basesec+newpos, length-newpos);
    if (plPause)
      cdStop(cdpDrive);
    setnewpos=0;
  }
  else
    newpos=curpos-basesec;
  return 0;
}

static int cdaLooped()
{
  static char counter=0;
  if (!counter||setnewpos||plPause)
  {
    if (cdCheck())
      return 1;
    counter=8;
  }
  else
    counter--;

  return 0;
}



static void cdaCloseFile()
{
  if (!nodevice)
    smpCloseSampler();
  cdStop(cdpDrive);
//  cdLockTray(cdpDrive, 0);
}

static int cdaOpenFile(const char *path, moduleinfostruct &, binfile *)
{
  nodevice=!smpSample;

  char name[_MAX_FNAME];
  char ext[_MAX_FNAME];

  _splitpath(path, cdpDriveName, 0, name, ext);

  unsigned char tnum;

  if (!strcmp(name, "DISK"))
    tnum=0xFF;
  else
  if (!memcmp(name, "TRACK", 5)&&isdigit(name[5])&&isdigit(name[6])&&(strlen(name)==7))
    tnum=(name[5]-'0')*10+(name[6]-'0');
  else
    return -1;

  cdpDrive=*cdpDriveName-'A';

  if (!cdIsCDDrive(cdpDrive))
    return -1;

  cdpTrackNum=cdGetTracks(cdpDrive, cdpTrackStarts, cdpFirstTrack, 32);

  if (tnum!=0xFF)
  {
    if ((tnum<cdpFirstTrack)||(tnum>=(cdpFirstTrack+cdpTrackNum)))
      return -1;
    cdpPlayMode=0;
    basesec=cdpTrackStarts[tnum-cdpFirstTrack];
    length=cdpTrackStarts[tnum-cdpFirstTrack+1]-basesec;
  }
  else
  {
    if (!cdpTrackNum)
      return -1;
    cdpPlayMode=1;
    basesec=cdpTrackStarts[0];
    length=cdpTrackStarts[cdpTrackNum]-basesec;
  }

  newpos=0;
  setnewpos=1;
  plPause=0;

  plIsEnd=cdaLooped;
  plProcessKey=cdaProcessKey;
  plDrawGStrings=cdaDrawGStrings;
  if (!nodevice)
  {
    plGetMasterSample=smpGetMasterSample;
    plGetRealMasterVolume=smpGetRealMasterVolume;
    smpSetSource(cfCDAtLineIn?SMP_LINEIN:SMP_CD);
    smpSetOptions(plsmpRate, plsmpOpt);
    void *buf;
    int len;
    if (!smpOpenSampler(buf, len, smpBufSize))
      return -1;
  }


//  cdLockTray(cdpDrive, 1);
  cdPlay(cdpDrive, basesec, length);

  return 0;
}


static int cdaInit()
{
  if (cdInit())
    printf("cd audio detected\n");

  cfCDAtLineIn=cfGetProfileBool2(cfSoundSec, "sound", "cdsamplelinein", 0, 0);

  return errOk;
}

static void cdaClose()
{
  cdClose();
}

static int cdaReadDir(modlist &ml, unsigned short dirref, const char *mask, unsigned long opt)
{
  char path[_MAX_PATH];
  dmGetPath(path, dirref);
  modlistentry m;

  if (opt&RD_PUTDSUBS)
  {
    int i;
    for (i=0; i<26; i++)
    {
      if (!cdIsCDDrive(i))
        continue;

      fsConvFileName12(m.name, "A:", "");
      *m.name+=i;
      m.fileref=0xFFFF;
      m.dirref=dmGetDriveDir(i+1);
      if (!mdbAppendNew(ml,m))
        return 0;
    }
  }

  if ((path[3]&&strcmp(path+3, "CDADISK\\"))||(path[0]=='@')||!cdIsCDDrive(path[0]-'A'))
    return 1;

  unsigned char first;
  unsigned long trks[33];
  short trknum=cdGetTracks(path[0]-'A', trks, first, 32);
  int i;

  unsigned long cdid=0;
  for (i=0; i<=trknum; i++)
    cdid=_lrotr(cdid^trks[i], i);

  if (!strcmp(path+3, "CDADISK\\"))
  {
    fsConvFileName12(m.name, "DISK", ".CDA");
    if (trknum&&fsMatchFileName12(m.name, mask))
    {
      m.dirref=dirref;
      m.fileref=mdbGetModuleReference(m.name, cdid);
      if (m.fileref==0xFFFF)
        return 0;
      if (mdbGetModuleType(m.fileref)!=mtCDA)
      {
        moduleinfostruct mi;
        mdbGetModuleInfo(mi, m.fileref);
        mi.flags1|=MDB_VIRTUAL;
        mi.playtime=(trks[trknum]-trks[0])/75;
        mi.channels=2;
        mi.modtype=mtCDA;
        mdbWriteModuleInfo(m.fileref, mi);
      }
      if (!mdbAppend(ml, m))
        return 0;
    }
  }

  if (!path[3])
  {
    if ((opt&RD_PUTSUBS)&&trknum)
    {
      dmGetPath(path, dirref);
      fsConvFileName12(m.name, "CDADISK", "");
      strcat(path, "CDADISK");
      m.dirref=dmGetPathReference(path);
      if (m.dirref==0xFFFF)
        return 0;
      m.fileref=0xFFFE;
      if (!mdbAppend(ml, m))
        return 0;
    }

    for (i=0; i<trknum; i++)
    {
      fsConvFileName12(m.name, "TRACK00", ".CDA");
      m.name[5]+=(i+first)/10;
      m.name[6]+=(i+first)%10;
      if (!fsMatchFileName12(m.name, mask))
        continue;

      m.dirref=dirref;
      m.fileref=mdbGetModuleReference(m.name, cdid);
      if (m.fileref==0xFFFF)
        return 0;
      if (mdbGetModuleType(m.fileref)!=mtCDA)
      {
        moduleinfostruct mi;
        mdbGetModuleInfo(mi, m.fileref);
        mi.flags1|=MDB_VIRTUAL;
        mi.playtime=(trks[i+1]-trks[i])/75;
        mi.channels=2;
        mi.modtype=mtCDA;
        mdbWriteModuleInfo(m.fileref, mi);
      }
      if (!mdbAppend(ml, m))
        return 0;
    }
  }
  return 1;
}

extern "C"
{
  initcloseregstruct cdaReg = {cdaInit, cdaClose};
  mdbreaddirregstruct cdaReadDirReg = {cdaReadDir};
  cpifaceplayerstruct cdaPlayer = {cdaOpenFile, cdaCloseFile};
  char *dllinfo = "initclose _cdaReg; readdirs _cdaReadDirReg; player _cdaPlayer";
};