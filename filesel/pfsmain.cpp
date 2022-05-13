// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Main routine, calls fileselector and interface code
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs
//    -complete restructuring of fsMain_ etc. to enable non-playable
//     file types which are handled differently
//    -therefore, added "handler" line in CP.INI filetype entries
//    -added routines to read out PLS and M3U play lists
//    -as always, added dllinfo record ;)
//  -fd981014   Felix Domke <tmbinc@gmx.net>
//    -small bugfix (tf was closed, even if it was NULL)

#define NO_PFILESEL_IMPORT
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "binfile.h"
#include "dpmi.h"
#include "err.h"
#include "pfilesel.h"
#include "plinkman.h"
#include "pmain.h"
#include "poutput.h"
#include "psetting.h"

static preprocregstruct *plPreprocess = 0;

static void plRegisterPreprocess(preprocregstruct *r)
{
  r->next=plPreprocess;
  plPreprocess=r;
}


int callselector (char *path, moduleinfostruct &info, binfile *&fi,
                  char callfs, char forcecall, char forcenext,
                  interfacestruct *&iface)
{
  char ret;
  char result;
  char tpath[_MAX_PATH];
  interfacestruct *intr;
  filehandlerstruct *hdlr;
  moduleinfostruct tmodinfo;
  binfile *tf;
  iface=0;
  fi=0;

  do
  {
    ret=result=0;
    if ((callfs && !fsFilesLeft())||forcecall)
    {
      ret=result=fsFileSelect();
    }

    if (!fsFilesLeft())
      return 0;

    while (ret || forcenext)
    {
#ifdef DOS32
      conRestore();
#endif

      if (!fsFilesLeft())
      {
#ifdef DOS32
        conSave();
#endif
        break;
      }
      if (!fsGetNextFile(tpath,tmodinfo,tf))
      {
        if (tf)
        {
          tf->close();
          delete tf;
        }
#ifdef DOS32
        conSave();
#endif
        continue;
      }

      char secname[20];
      strcpy(secname, "filetype ");
      ultoa(tmodinfo.modtype&0xFF, secname+strlen(secname), 10);
      intr=(interfacestruct *)lnkGetSymbol(cfGetProfileString(secname, "interface", ""));
      hdlr=(filehandlerstruct *)lnkGetSymbol(cfGetProfileString(secname, "handler", ""));

      if (hdlr)
      {
        hdlr->Process(tpath,tmodinfo,tf);
      }

#ifdef DOS32
      conSave();
#endif

      if (intr)
      {
        ret=0;
        iface=intr;
        info=tmodinfo;
        fi=tf;
        strcpy(path,tpath);
        return result?-1:1;
      }
      else
      {
        if(tf)
        {
          tf->close();
          delete tf;
        }
      }
    }
#ifdef DOS32
    if (ret)
      conSave();
#endif
  } while (ret);

  return 0;

}


static interfacestruct *plintr;
static interfacestruct *nextintr;
static binfile *thisf;
static binfile *nextf;
static moduleinfostruct nextinfo;
static moduleinfostruct plModuleInfo;
static char thispath[_MAX_PATH];
static char nextpath[_MAX_PATH];
static char callfs;
static char firstfile;
static int  stop;

// stop: 0 cont
//       1 next song
//       2 quit
//       3 iface : next song or fs
//       4 iface : call fs
//       5 iface : dosshell

extern "C" int fsMain()
{
  callfs=0;
  stop=0;
  firstfile=1;
  plintr=0;
  thisf=0;
  nextf=0;

#ifdef DOS32   
 setwintitle("OpenCP");
#endif 

  while (1)
  {

    while (ekbhit())
    {
      unsigned short key=egetch();
      if (!(key&0xFF))
        continue;
      key&=0xFF;
      if ((key==0)||(key==3)||(key==27))
        stop=2;
    }
    if (stop==2)
      break;

    stop=0;

    if (!plintr)
    {
#ifdef DOS32
      conSave();
#endif
      int fsr=callselector(nextpath,nextinfo,nextf,(callfs||firstfile),0,1,nextintr);
      if (!fsr)
        break;
      else if (fsr==-1);
        callfs=1;
#ifdef DOS32
      conRestore();
#endif
    }

    if (callfs)
      firstfile=0;

    if (nextintr)
    {
#ifdef DOS32
      conRestore();
#endif

      if (plintr)
      {
        plintr->Close();
        plintr=0;
        _heapshrink();
      }

      if (thisf)
      {
        thisf->close();
        delete thisf;
        thisf=0;
      }

      strcpy(thispath,nextpath);
      thisf=nextf;
      nextf=0;
      plModuleInfo=nextinfo;
      plintr=nextintr;
      nextintr=0;

      stop=0;

      preprocregstruct *prep;
      for (prep=plPreprocess; prep; prep=prep->next)
        prep->Preprocess(thispath, plModuleInfo, thisf);


      if (!plintr->Init(thispath, plModuleInfo, thisf))
      {
        plintr=0;
      }
      else
      {
        char wt[256];
        memset(wt,0,256);
        char realname[13];
        fsConv12FileName(realname,plModuleInfo.name);
        strncpy(wt,realname,12);
        strcat(wt," - ");
        strncat(wt,plModuleInfo.modname,32);
#ifdef DOS32
        setwintitle(wt);
#endif
      }
#ifdef DOS32
      conSave();
#endif
    }

    if (plintr)
    {

      while (ekbhit())
        egetch();

      while (!stop)
      {
        stop=plintr->Run();
        switch (stop)
        {
          case 1: // next playlist file (auto)
            if (firstfile)
              stop=2;
            else
              stop=callselector(nextpath,nextinfo,nextf,callfs,0,1,nextintr)?1:2;
            break;
          case 3: // next playlist file (man)
            if (fsFilesLeft())
              stop=callselector(nextpath,nextinfo,nextf,1,0,1,nextintr);
            else
              stop=callselector(nextpath,nextinfo,nextf,1,0,0,nextintr);
            if (stop==-1)
              callfs=1;
            break;
          case 4: // call fs
            callfs=1;
            stop=callselector(nextpath,nextinfo,nextf,1,1,0,nextintr);
            break;
          case 5: // dos shell
#ifdef DOS32
            stop=0;
            conRestore();
            plDosShell();
            while (ekbhit())
              egetch();
            conSave();
#endif
            break;
        }
      }
      firstfile=0;
    }
  }

#ifdef DOS32
  conRestore();
#endif
  if (plintr)
    plintr->Close();
  if (thisf)
  {
    thisf->close();
    delete thisf;
  }

  return errOk;
}


static int fsint()
{
  const char *sec=cfGetProfileString(cfConfigSec, "fileselsec", "fileselector");

  char regname[50];
  const char *regs;
  regs=lnkReadInfoReg("preprocess");

  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
      plRegisterPreprocess((preprocregstruct*)reg);
  }

  printf("initializing fileselector...\n");
  if (!fsInit())
  {
    printf("fileselector init failed!\n");
    return errGen;
  }

  fsAddFiles(cfCommandLine);

  return errOk;
}


static void fsclose()
{
  fsClose();
}


void addtoplaylist(const char *source,const char *homepath, char *dest)
{
  char finalpath[_MAX_PATH];
  char longpath[_MAX_PATH];

  if (strchr(source,'\\'))
    strcpy(longpath,source);
  else
  {
    strcpy(longpath,homepath);
    strcat(longpath,source);
  }
  strcpy(finalpath,longpath);

#ifdef DOS32
  callrmstruct r;
  char *mem;
  void __far16 *rmptr;
  __segment pmsel;
  mem=(char*)dosmalloc(1024, rmptr, pmsel);
  if (!mem)
    return;
  strcpy(mem, longpath);
  strcpy(mem+512, longpath);
  clearcallrm(r);
  r.w.ax=0x7160;
  r.b.cl=1;
  r.b.ch=0x80;
  r.w.si=(unsigned short)rmptr;
  r.w.di=((unsigned short)rmptr)+512;
  r.s.ds=((unsigned long)rmptr)>>16;
  r.s.es=((unsigned long)rmptr)>>16;
  r.s.flags|=1;
  intrrm(0x21,r);

  if (!(r.s.flags&1))
    strcpy(finalpath, mem+512);
  else if (strchr(finalpath,' '))
    *finalpath=0;

  dosfree(pmsel);
#endif
  if (!memcmp(finalpath,"\\\\",2))
    *finalpath=0;

  if (*finalpath)
  {
    strcat(dest,finalpath);
    strcat(dest," ");
  }
}


static char bdest[32768];

int addpls(const char *path, moduleinfostruct &info, binfile *&fil)
{
  printf("adding %s to playlist ...\n", path);

  char dir[_MAX_DIR];
  char pspec[_MAX_PATH];
  _splitpath(path, pspec, dir, 0, 0);
  strcat(pspec,dir);

  *bdest=0;

  char *buffer = new char[fil->length()+128];
  char *bufend = buffer+fil->length();
  *bufend=0;
  fil->seek(0);
  fil->read(buffer,fil->length());

  char *bufptr=buffer;
  char *lineptr[1000];
  int linenum=0;

  while (bufptr<bufend)
  {
    while(bufptr<bufend && *bufptr<33)
      bufptr++;
    if (bufptr==bufend)
      break;
    lineptr[linenum]=bufptr;
    while (bufptr<bufend && *bufptr>=32)
      bufptr++;
    if (bufptr==bufend)
      break;
    *bufptr++=0;
    linenum++;
  }

  int snum=1, fnd;
  char lbuf[10];
  strcpy(lbuf,"File");
  if (linenum && !strcmp(lineptr[0],"[playlist]"))
  {
    do
    {
      ltoa(snum,lbuf+4,10);
      for (fnd=1; fnd<linenum; fnd++)
        if (!memcmp(lineptr[fnd],lbuf,strlen(lbuf)) && lineptr[fnd][strlen(lbuf)]=='=')
          break;
      if (fnd<linenum)
      {
        addtoplaylist(lineptr[fnd]+strlen(lbuf)+1,pspec,bdest);
        snum++;
      }
    } while (fnd<linenum);
  }
  delete buffer;

  strcpy(info.modname,"PLS play list (");
  ltoa(snum-1,info.modname+21,10);
  strcat(info.modname," entries)");

  fsAddFiles(bdest);

  return 1;
}




int addm3u(const char *path, moduleinfostruct &info, binfile *&fil)
{
  printf("adding %s to playlist ...\n", path);

  char dir[_MAX_DIR];
  char pspec[_MAX_PATH];
  _splitpath(path, pspec, dir, 0, 0);
  strcat(pspec,dir);

  *bdest=0;

  char *buffer = new char[fil->length()+128];
  char *bufend = buffer+fil->length();
  *bufend=0;
  fil->seek(0);
  fil->read(buffer,fil->length());

  char *bufptr=buffer;
  char *lineptr[1000];
  int linenum=0;

  while (bufptr<bufend)
  {
    while(bufptr<bufend && *bufptr<33)
      bufptr++;
    if (bufptr==bufend)
      break;
    lineptr[linenum]=bufptr;
    while (bufptr<bufend && *bufptr>=32)
      bufptr++;
    if (bufptr==bufend)
      break;
    *bufptr++=0;
    if (strchr(lineptr[linenum],';'))
      *strchr(lineptr[linenum],';')=0;
    if (lineptr[linenum][0])
      linenum++;
  }

  int fnd;
  for (fnd=0; fnd<linenum; fnd++)
    addtoplaylist(lineptr[fnd],pspec,bdest);

  delete buffer;

  strcpy(info.modname,"M3U play list (");
  ltoa(linenum,info.modname+21,10);
  strcat(info.modname," entries)");

  fsAddFiles(bdest);

  return 1;
}

extern "C"
{
  initcloseregstruct fsReg={fsint, fsclose};
  filehandlerstruct fsAddPLS={addpls};
  filehandlerstruct fsAddM3U={addm3u};
  char *dllinfo = "initcloseafter _fsReg; main fsMain_; readdirs _adbReadDirReg _dosReadDirReg _fsReadDirReg; getfile _adbGetFileReg _dosGetFileReg; readinfos _fsReadInfoReg";
};
