// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Archive handler for ACE archives
//
// revision history: (please note changes here)
//  -fd980623   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <kb@nwn.de>
//    -added _dllinfo record
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile

// move etc. are not yet supported, because i think they are totally useless
// for a player :).
// implementing them should be no problem.

// PLEASE NOTE: the UNACE routines used herein are NOW covered under
//              the GNU GPL (look into uac_dcpr.* for more info)

#include <stdlib.h>
#include <string.h>
#include "binfstd.h"
#include "pfilesel.h"
#include "psetting.h"

#include "arcace.h"
#include "uac_dcpr.h"

binfile *garchive;

unsigned long *buf_rd=0;
char *buf=0;
char *buf_wr=0;
unsigned char *readbuf=0;
int curread;

int solid;
int scaninsolid=0;              // scan even in solid-files (time-consuming :)

short rpos           =0,
      dcpr_do        =0,
      dcpr_do_max    =0,
      blocksize      =0,
      dcpr_dic       =0,
      dcpr_oldnum    =0,
      bits_rd        =0,
      dcpr_frst_file =0;
unsigned        short dcpr_code_mn[1 << maxwd_mn],
                dcpr_code_lg[1 << maxwd_lg];
unsigned char   dcpr_wd_mn[maxcode + 2],
                dcpr_wd_lg[maxcode + 2];
unsigned char   wd_svwd[svwd_cnt];

unsigned long   dcpr_dpos      =0,
      cpr_dpos2      =0,
      dcpr_dicsiz    =0,
      dcpr_dican     =0,
      dcpr_size      =0,
      code_rd        =0;

unsigned long dcpr_olddist[4]={0,0,0,0};

char *dcpr_text      =0;



unsigned short sort_org[maxcode+2];
unsigned char sort_freq[(maxcode+2)*2];

aceheaderstruct *aheader;

acexheaderstruct *axheader;

acefileheaderstruct *afheader;

char *amem;
long nexthpos;
int ReadNextHeader(binfile &);


static int adbACEScan(const char *path)   // adds the contents of
                                          // path (an archive) via adbAdd();
                                          // (including the archive itself)
                                          // returns 0 for ok, 1 for error.
{
 scaninsolid=cfGetProfileBool("arcACE", "scaninsolid", 0, 0);
 sbinfile archive;
 if(archive.open(path, sbinfile::openro))
  return(1);

 unsigned short arcref;                 // whatever..
 {
  char arcname[12];
  char ext[_MAX_EXT];
  char name[_MAX_FNAME];
  _splitpath(path, 0, 0, name, ext);
  fsConvFileName12(arcname, name, ext);
  arcentry a;
  memcpy(a.name, arcname, 12);
  a.size=archive.length();
  a.flags=ADB_ARC;
  if (!adbAdd(a))
  {
   archive.close();
   return 0;
  }
  arcref=adbFind(arcname);
 }
 amem=(char*)malloc(65536);
 aheader=0;
 axheader=0;
 nexthpos=0;

 if(!amem)
 {
  archive.close();
  return(0);
 }

 buf_rd=(unsigned long*)malloc(size_rdb*sizeof(long));
 buf=(char*)malloc(size_buf);
 buf_wr=(char*)malloc(size_wrb);
 readbuf=(unsigned char*)malloc(size_headrdb);

 if((!buf_rd)||(!buf)||(!buf_wr)||(!readbuf))
 {
  if(buf_rd) free(buf_rd);
  if(buf) free(buf);
  if(buf_wr) free(buf_wr);
  if(readbuf) free(readbuf);
  archive.close();
  return(0);
 }

 garchive=&archive;

 dcpr_init();

 // list
 solid=0;
 arcentry a;
 while(ReadNextHeader(archive))
 {
  char ext[_MAX_EXT];
  char name[_MAX_FNAME];
  if(!aheader->type)
   solid=!!(aheader->flags&ACE_SOLID);
  else if(aheader->type==1)
  {
   afheader=(acefileheaderstruct*)amem;
   curread=afheader->packsize;
   char filename[_MAX_PATH];
   memcpy(filename, afheader->filename, afheader->filenamesize);
   filename[afheader->filenamesize]=0;
   strupr(filename);
   _splitpath(filename, 0, 0, name, ext);
   if(fsIsModule(ext))
   {
    a.size=afheader->unpsize;
    a.parent=arcref;
    a.flags=0;
    fsConvFileName12(a.name, name, ext);
    if(!adbAdd(a))
    {
     archive.close();
     free(amem);
     if(buf_rd) free(buf_rd);
     if(buf) free(buf);
     if(buf_wr) free(buf_wr);
     if(readbuf) free(readbuf);
     if(dcpr_text) free(dcpr_text);
     archive.close();
     return 0;
    } else
    {
     if(fsScanInArc&&(scaninsolid||(!solid)))
     {
      dcpr_init_file();
      int rd=dcpr_adds_blk(buf_wr, size_wrb);
      unsigned short fileref;
      fileref=mdbGetModuleReference(a.name, a.size);
      if((fileref!=0xFFFF)&&(!mdbInfoRead(fileref)))
      {
       moduleinfostruct ms;
       if(mdbGetModuleInfo(ms, fileref))
       {
        mdbReadMemInfo(ms, (unsigned char*)buf_wr, rd);
        mdbWriteModuleInfo(fileref, ms);
       }
      }
     }
    }
   }
   if(fsScanInArc&&solid&&scaninsolid)
    while(dcpr_adds_blk(buf_wr, size_wrb));
  }
 }
 if(buf_rd) free(buf_rd);
 if(buf) free(buf);
 if(buf_wr) free(buf_wr);
 if(readbuf) free(readbuf);
 if(dcpr_text) free(dcpr_text);
 free(amem);
 archive.close();
 return(1);
}

static int adbACECall(int act,
                      const char *apath,
                      const char *file,
                      const char *dpath)
{
 switch (act)
 {
  case adbCallGet:
  {
   return !adbCallArc(cfGetProfileString("arcACE", "get", "ace e %a %n %d"), apath, file, dpath);
  }
  case adbCallPut:
   // not implemented
   break;
  case adbCallDelete:
   // not implemented
   break;
  case adbCallMoveTo:
   // not implemented
   break;
  case adbCallMoveFrom:
   // not implemented
   break;
  }
  return 0;
}

int ReadNextHeader(binfile &file)
{
 if(nexthpos==file.length()) { return(0); }
 file[nexthpos];
 aheader=(aceheaderstruct*)amem;
 axheader=(acexheaderstruct*)amem;
 if(!file.eread(aheader, sizeof(aceheaderstruct)))
  { return(0); }
 if(aheader->size<(sizeof(aceheaderstruct)-4)) { return(0); }
 if(aheader->size-(sizeof(aceheaderstruct)-4))
  if(!file.eread((char*)aheader+(sizeof(aceheaderstruct)), aheader->size-(sizeof(aceheaderstruct)-4)))
   { return(0); }

 nexthpos=file.tell()+((axheader->flags&1)?axheader->addsize:0);
 if(nexthpos>file.length()) { return(0); }
 return(1);
}

int read_adds_blk(char* buffer, int len)
{
 if(curread<len) len=curread;
 curread-=len;
 return(garchive->read(buffer, len));
}

extern "C"
{
  adbregstruct adbACEReg = {".ACE", adbACEScan, adbACECall};
  char *dllinfo = "arcs _adbACEReg";
};