// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// the File selector ][
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -changed some INI lookups to dllinfo lookups
//    -fixed Long Filename lookup code a bit
//    -no other changes, i won't touch this monster
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile
//    -no other changes, i won't touch this monster
//     (we REALLY have to split up theis file!)

#define NO_PFILESEL_IMPORT
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <conio.h>
#include <direct.h>
#include <dos.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "poutput.h"
#include "inflate.h"
#include "pfilesel.h"
#include "binfstd.h"
#include "binfdel.h"
#include "psetting.h"
#include "plinkman.h"
#include "pmain.h"
#include "dpmi.h"
#include "cphlpfs.h"

char fsListScramble=1;
char fsListRemove=1;
char fsLoopMods=1;
char fsScanNames=1;
char fsScanArcs=1;
char fsScanInArc=1;
char fsScanMIF=1;
char fsWriteModInfo=1;
char fsScrType=0;
char fsEditWin=1;
char fsColorTypes=1;
char fsInfoMode=0;
char fsPutArcs=1;

static const char *fsDefMovePath;
static char fsTypeCols[256];
static const char *(fsTypeNames[256]);

static unsigned char mdbReadModType(const char *str)
{
  int v=255;
  int i;
  for (i=0; i<256; i++)
    if (!stricmp(str, fsTypeNames[i]))
      v=i;
  return v;
}

static const char *mdbGetModTypeString(unsigned char type)
{
  return fsTypeNames[type&0xFF];
}

static unsigned char mdbScanBuf[1084];

static mdbreadnforegstruct *mdbReadInfos;

static void mdbRegisterReadInfo(mdbreadnforegstruct *r)
{
  r->next=mdbReadInfos;
  mdbReadInfos=r;
}

int mdbReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int len)
{
  mdbreadnforegstruct *rinfos;
  for (rinfos=mdbReadInfos; rinfos; rinfos=rinfos->next)
    if (rinfos->ReadMemInfo)
      if (rinfos->ReadMemInfo(m, buf, len))
        return 1;
  return 0;
}

int mdbReadInfo(moduleinfostruct &m, binfile &f)
{
  int maxl=f.length()-f.tell();
  if (maxl>1084)
    maxl=1084;

  memset(mdbScanBuf, 0, 1084);
  f.read(mdbScanBuf, maxl);

  if (mdbReadMemInfo(m, mdbScanBuf, maxl))
    return 1;

  mdbreadnforegstruct *rinfos;
  for (rinfos=mdbReadInfos; rinfos; rinfos=rinfos->next)
    if (rinfos->ReadInfo)
      if (rinfos->ReadInfo(m, f, mdbScanBuf, maxl))
        return 1;

  return 0;
}


// code for fullpath2 taken from bc3.1, converted and modified...

static void reducepath(char *buf)
{
  char c;
  char *src,*dst;
  src=dst=buf;
  while (1)
  {
    c=*src++;
    if (!c||(c=='\\'))
    {
      if ((dst[-1]=='.')&&(dst[-2]=='\\'))
        dst-=2;
      else
        if ((dst[-1]=='.')&&(dst[-2]=='.')&&(dst[-3]=='\\'))
        {
          if (dst[-4]!=':')
          {
            dst-=3;
            while (*--dst!='\\');
          }
        }
      if (!c)
      {
        if (dst[-1]=='\\')
          dst--;
        if (dst[-1]==':')
          *dst++='\\';
        *dst=0;
        break;
      }
      else
        *dst++='\\';
    }
    else
      *dst++=toupper(c);
  }
}


void forwardb(void *dst, void *src, unsigned long n);
#pragma aux forwardb parm [edi] [esi] [ecx] = "rep movsb"

void backwardb(void *dst, void *src, unsigned long n);
#pragma aux backwardb parm [edi] [esi] [ecx] modify [eax] = "add edi,ecx" "dec edi" "add esi,ecx" "dec esi" "std" "rep movsb" "cld"

static void mymemmove(void *dst, void *src, unsigned short n)
{
  if (dst==src)
    return;
  if (dst<src)
    forwardb(dst, src, n);
  else
    backwardb(dst, src, n);
}

void forwardw(void *dst, void *src, unsigned long n);
#pragma aux forwardw parm [edi] [esi] [ecx] = "rep movsw"

void backwardw(void *dst, void *src, unsigned long n);
#pragma aux backwardw parm [edi] [esi] [ecx] = "add edi,ecx" "add edi,ecx" "sub edi,2" "add esi,ecx" "add esi,ecx" "sub esi,2" "std" "rep movsw" "cld"

static void mymemmovew(void *dst, void *src, unsigned long n)
{
  if (dst==src)
    return;

  if (dst<src)
    forwardw(dst, src, n);
  else
    backwardw(dst, src, n);
}

void forwardp(void *dst, const void *src, unsigned long n);
#pragma aux forwardp parm [edi] [esi] [ecx] = "rep movsd"

void backwardp(void *dst, const void *src, unsigned long n);
#pragma aux backwardp parm [edi] [esi] [ecx] modify [eax] = "mov eax,ecx" "dec eax" "shl eax,2" "add edi,eax" "add esi,eax" "std" "rep movsd" "cld"

static void mymemmovep(void *dst, const void *src, unsigned long n)
{
  n<<=2;
  if (dst==src)
    return;
  if (dst<src)
    forwardp(dst, src, n);
  else
    backwardp(dst, src, n);
}

void fsConvFileName12(char *c, const char *f, const char *e)
{
  int i;
  for (i=0; i<8; i++)
    *c++=*f?*f++:' ';
  for (i=0; i<4; i++)
    *c++=*e?*e++:' ';
  for (i=0; i<12; i++)
    c[i-12]=toupper(c[i-12]);
}

static void convfilename12wc(char *c, const char *f, const char *e)
{
  int i;
  for (i=0; i<8; i++)
    *c++=(*f=='*')?'?':*f?*f++:' ';
  for (i=0; i<4; i++)
    *c++=(*e=='*')?'?':*e?*e++:' ';
  for (i=0; i<12; i++)
    c[i-12]=toupper(c[i-12]);
}

void fsConv12FileName(char *f, const char *c)
{
  int i;
  for (i=0; i<8; i++)
    if (c[i]==' ')
      break;
    else
      *f++=c[i];
  for (i=8; i<12; i++)
    if (c[i]==' ')
      break;
    else
      *f++=c[i];
  *f=0;
}

static void conv12filenamewc(char *f, const char *c)
{
  char *f0=f;
  short i;
  for (i=0; i<8; i++)
    if (c[i]==' ')
      break;
    else
      *f++=c[i];
  if (i==8)
  {
    for (i=7; i>=0; i--)
      if (c[i]!='?')
        break;
    if (++i<7)
    {
      f-=8-i;
      *f++='*';
    }
  }
  for (i=8; i<12; i++)
    if (c[i]==' ')
      break;
    else
      *f++=c[i];
  if (i==12)
  {
    for (i=11; i>=9; i--)
      if (c[i]!='?')
        break;
    if (++i<10)
    {
      f-=12-i;
      *f++='*';
    }
  }
  *f=0;
}

int fsMatchFileName12(const char *a, const char *b)
{
  int i;
  for (i=0; i<12; i++, a++, b++)
    if ((i!=8)&&(*b!='?')&&(*a!=*b))
      break;
  return i==12;
}

#ifdef DOS32

void setcur(unsigned char y, unsigned char x);
#pragma aux setcur parm [al] [dl] modify [ax bx] = "mov dh,al" "mov ah,2" "mov bh,0" "int 10h"

void setcurshape(unsigned short);
#pragma aux setcurshape parm [cx] modify [ax] = "mov ax,0103h" "int 10h"

#else
static void setcur(unsigned char y, unsigned char x){}
static void setcurshape(unsigned short){}
#endif


struct directorynode
{
  char name[_MAX_NAME+1];
  unsigned short parent;
};

static directorynode *dmTree;
static unsigned short *dmReloc;
static unsigned short dmMaxNodes;
static unsigned short dmNumNodes;
static unsigned char dmCurDrive;
static unsigned short dmDriveDirs[27];

static unsigned short dmGetPathReference(const char *p, const char *endp)
{
  if (endp[-1]=='\\')
    endp--;
  const char *stp=endp;
  while (((stp+_MAX_NAME-1)>endp)&&(stp>p)&&(stp[-1]!='\\'))
    stp--;

  unsigned short v;
  if (stp!=p)
  {
    v=dmGetPathReference(p, stp);
    if (v==0xFFFF)
      return 0xFFFF;
  }
  else
    v=0xFFFF;

  char subdir[_MAX_NAME+1];
  memcpy(subdir, stp, endp-stp);
  subdir[endp-stp]='\\';
  subdir[endp-stp+1]=0;
  strupr(subdir);

  unsigned short *min=dmReloc;
  unsigned short num=dmNumNodes;

  while (num)
  {
    int res=strcmp(subdir, dmTree[min[num>>1]].name);
    if (!res)
    {
      res=dmTree[min[num>>1]].parent-v;
      if (!res)
        return min[num>>1];
    }
    if (res<0)
      num>>=1;
    else
    {
      min+=(num>>1)+1;
      num=(num-1)>>1;
    }
  }
  unsigned short mn=min-dmReloc;

  if (dmNumNodes==dmMaxNodes)
  {
    dmMaxNodes+=256;
    void *n1=realloc(dmTree, sizeof (*dmTree)*dmMaxNodes);
    void *n2=realloc(dmReloc, sizeof (*dmReloc)*dmMaxNodes);
    if (!n1||!n2)
      return 0xFFFF;

    dmTree=(directorynode *)n1;
    dmReloc=(unsigned short *)n2;
  }

  mymemmovew(dmReloc+mn+1, dmReloc+mn, dmNumNodes-mn);

  dmReloc[mn]=dmNumNodes;

  strcpy(dmTree[dmNumNodes].name, subdir);
  dmTree[dmNumNodes].parent=v;

  return dmNumNodes++;
}

unsigned short dmGetPathReference(const char *p)
{
  return dmGetPathReference(p, p+strlen(p));
}

static unsigned short dmGetParent(unsigned short ref)
{
  return (dmTree[ref].parent==0xFFFF)?ref:dmTree[ref].parent;
}

static unsigned short dmGetRoot(unsigned short ref)
{
  while (dmTree[ref].parent!=0xFFFF)
    ref=dmTree[ref].parent;
  return ref;
}

char *dmGetPathRel(char *path, unsigned short ref, unsigned short base)
{
  if ((ref==0xFFFF)||(ref==base))
  {
    *path=0;
    return path;
  }
  if (ref>=0xFFE0)
  {
    path[0]='@'+ref-0xFFE0;
    path[1]=':';
    path[2]=0;
    return path;
  }
  dmGetPathRel(path, dmTree[ref].parent, base);
  strcat(path, dmTree[ref].name);
  return path;
}

char *dmGetPath(char *path, unsigned short ref)
{
  return dmGetPathRel(path, ref, 0xFFFF);
}

static unsigned short dmConvertReference(unsigned short ref)
{
  if (ref<0xFFE0)
    return ref;


  unsigned char drv=ref-0xFFE0;

  if (dmDriveDirs[drv]<0xFFE0)
    return dmDriveDirs[drv];

  char path[_MAX_PATH];
  if (drv)
  {
    unsigned int savedrive, dum;
    _dos_getdrive(&savedrive);
    _dos_setdrive(drv, &dum);
    if (!getcwd(path, _MAX_PATH))
    {
      strcpy(path, "@:\\");
      path[0]+=drv;
    }
    _dos_setdrive(savedrive, &dum);
  }
  else
    strcpy(path, "@:\\");

  dmDriveDirs[drv]=dmGetPathReference(path);
  return dmDriveDirs[drv];
}

static unsigned short dmChangeDir(unsigned short dir)
{
  dir=dmConvertReference(dir);
  if (dir==0xFFFF)
    return 0xFFFF;

  char path[_MAX_PATH];
  dmGetPath(path, dir);
  char drive[_MAX_DRIVE];
  _splitpath(path, drive, 0, 0, 0);
  dmCurDrive=*drive-'@';
  dmDriveDirs[dmCurDrive]=dir;

  return dir;
}

unsigned short dmGetDriveDir(int drv)
{
  return dmDriveDirs[drv];
}

static unsigned short dmGetCurDir()
{
  return dmGetDriveDir(dmCurDrive);
}

static char dmFullPath(char *path)
{
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char name[_MAX_FNAME];
  char ext[_MAX_EXT];

  strupr(path);
  _splitpath(path, drive, dir, name, ext);

  if (!*drive||(*drive<'@')||(*drive>'Z'))
    *drive='@'+dmCurDrive;
  drive[1]=':';
  drive[2]=0;

  if (dir[0]!='\\')
  {
    char dir2[_MAX_DIR*2];

    unsigned short dr=dmConvertReference(dmDriveDirs[drive[0]-'@']);
    if (dr==0xFFFF)
      return 0;

    dmGetPath(path, dr);
    _splitpath(path, 0, dir2, 0, 0);
    strcat(dir2, dir);
    strcpy(dir, dir2);
  }
  _makepath(path, drive, dir, name, ext);
  reducepath(path);

  return 1;
}

static unsigned char isarchive(const char *ext);
static char isarchivepath(const char *p);

static char **moduleextensions;

int fsIsModule(const char *ext)
{
  if (*ext++!='.')
    return 0;
  char **e;
  for (e=moduleextensions; *e; e++)
    if (!strcmp(ext, *e))
      return 1;
  return 0;
}

struct modinfoentry
{
  unsigned char flags;
  union
  {
    struct
    {
      unsigned char modtype;
      unsigned short comref;
      unsigned short compref;
      unsigned short futref;
      char name[12];
      unsigned long size;
      char modname[32];
      unsigned long date;
      unsigned short playtime;
      unsigned char channels;
      unsigned char moduleflags;
    } gen;

    char comment[63];

    struct
    {
      char composer[32];
      char style[31];
    } comp;
  };
};

static modinfoentry *mdbData;
static unsigned long mdbNum;
static char mdbDirty;
static unsigned short *mdbReloc;
static unsigned long mdbGenNum;
static unsigned long mdbGenMax;
static moduleinfostruct mdbEditBuf;

static int miecmp(const void *a, const void *b)
{
  modinfoentry &c=mdbData[*(unsigned short*)a];
  modinfoentry &d=mdbData[*(unsigned short*)b];
  if (c.gen.size==d.gen.size)
    return memcmp(c.gen.name, d.gen.name, 12);
  if (c.gen.size<d.gen.size)
    return -1;
  else
    return 1;
}

static char mdbInit()
{
  mdbDirty=0;
  mdbData=0;
  mdbNum=0;
  mdbReloc=0;
  mdbGenNum=0;
  mdbGenMax=0;

  char path[_MAX_PATH];
  strcpy(path, cfConfigDir);
  strcat(path, "CPMODNFO.DAT");

  long f=open(path, O_RDONLY|O_BINARY);
  if (f<0)
    return 1;

  char sig[64];

  if (read(f, sig, 64)!=64)
  {
    close(f);
    return 1;
  }

  if (memcmp(sig, "Cubic Player Module Information Data Base\x1A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 60))
  {
    close(f);
    return 1;
  }

  mdbNum=*(unsigned long*)(sig+60);
  if (!mdbNum)
  {
    close(f);
    return 1;
  }

  mdbData=new modinfoentry[mdbNum];
  if (!mdbData)
    return 0;
  if (read(f, mdbData, mdbNum*sizeof(*mdbData))!=(mdbNum*sizeof(*mdbData)))
  {
    mdbNum=0;
    delete mdbData;
    mdbData=0;
    close(f);
    return 1;
  }
  close(f);

  long i;
  for (i=0; i<mdbNum; i++)
    if ((mdbData[i].flags&(MDB_BLOCKTYPE|MDB_USED))==(MDB_USED|MDB_GENERAL))
      mdbGenMax++;

  if (mdbGenMax)
  {
    mdbReloc=new unsigned short [mdbGenMax];
    if (!mdbReloc)
      return 0;
    for (i=0; i<mdbNum; i++)
      if ((mdbData[i].flags&(MDB_BLOCKTYPE|MDB_USED))==(MDB_USED|MDB_GENERAL))
        mdbReloc[mdbGenNum++]=i;

    qsort(mdbReloc, mdbGenNum, sizeof(*mdbReloc), miecmp);
  }

  return 1;
}

static void mdbUpdate()
{
  if (!mdbDirty||!fsWriteModInfo)
    return;
  mdbDirty=0;

  char path[_MAX_PATH];
  strcpy(path, cfConfigDir);
  strcat(path, "CPMODNFO.DAT");

  long f=open(path, O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
  if (f<0)
    return;

  lseek(f, 0, SEEK_SET);
  write(f, "Cubic Player Module Information Data Base\x1A", 42);
  write(f, "\x00\x00", 2);
  write(f, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
  write(f, &mdbNum, 4);

  long i=0,j;

  while (i<mdbNum)
  {
    if (!(mdbData[i].flags&MDB_DIRTY))
    {
      i++;
      continue;
    }
    for (j=i; j<mdbNum; j++)
      if (mdbData[j].flags&MDB_DIRTY)
        mdbData[j].flags&=~MDB_DIRTY;
      else
        break;
    lseek(f, 64+i*sizeof(*mdbData), SEEK_SET);
    write(f, mdbData+i, (j-i)*sizeof(*mdbData));

    i=j;
  }
  lseek(f, 0, SEEK_END);
  close(f);
}

static void mdbClose()
{
  mdbUpdate();
  delete mdbData;
  delete mdbReloc;
}

static unsigned short mdbGetNew()
{
  long i;
  for (i=0; i<mdbNum; i++)
    if (!(mdbData[i].flags&MDB_USED))
      break;

  if (i==mdbNum)
  {
    mdbNum+=64;
    void *t=realloc(mdbData, mdbNum*sizeof(*mdbData));
    if (!t)
      return 0xFFFF;
    mdbData=(modinfoentry*)t;
    memset(mdbData+i, 0, (mdbNum-i)*sizeof(*mdbData));
    long j;
    for (j=i; j<mdbNum; j++)
      mdbData[j].flags|=MDB_DIRTY;
  }
  mdbDirty=1;
  return (unsigned short)i;
}

unsigned short mdbGetModuleReference(const char *name, unsigned long size)
{
  long i;

  unsigned short *min=mdbReloc;
  unsigned short num=(unsigned short)mdbGenNum;

  while (num)
  {
    modinfoentry &m=mdbData[min[num>>1]];
    int ret;
    if (size==m.gen.size)
      ret=memcmp(name, m.gen.name, 12);
    else
      if (size<m.gen.size)
        ret=-1;
      else
        ret=1;
    if (!ret)
      return min[num>>1];
    if (ret<0)
      num>>=1;
    else
    {
      min+=(num>>1)+1;
      num=(num-1)>>1;
    }
  }
  unsigned short mn=min-mdbReloc;

  i=mdbGetNew();
  if (i==0xFFFF)
    return 0xFFFF;
  if (mdbGenNum==mdbGenMax)
  {
    mdbGenMax+=512;
    void *n=realloc(mdbReloc, sizeof (*mdbReloc)*mdbGenMax);
    if (!n)
      return 0xFFFF;
    mdbReloc=(unsigned short *)n;
  }

  mymemmovew(mdbReloc+mn+1, mdbReloc+mn, mdbGenNum-mn);
  mdbReloc[mn]=(unsigned short)i;
  mdbGenNum++;

  modinfoentry &m=mdbData[i];
  m.flags=MDB_DIRTY|MDB_USED|MDB_GENERAL;
  memcpy(m.gen.name, name, 12);
  m.gen.size=size;
  m.gen.modtype=0xFF;
  m.gen.comref=0xFFFF;
  m.gen.compref=0xFFFF;
  m.gen.futref=0xFFFF;
  memset(m.gen.modname, 0, 32);
  m.gen.date=0;
  m.gen.playtime=0;
  m.gen.channels=0;
  m.gen.moduleflags=0;
  mdbDirty=1;
  return (unsigned short)i;
}

int mdbInfoRead(unsigned short fileref)
{
  if ((fileref>=mdbNum)||((mdbData[fileref].flags&(MDB_USED|MDB_BLOCKTYPE))!=(MDB_USED|MDB_GENERAL)))
    return -1;
  return mdbData[fileref].gen.modtype!=0xFF;
}

int mdbGetModuleType(unsigned short fileref)
{
  if ((fileref>=mdbNum)||((mdbData[fileref].flags&(MDB_USED|MDB_BLOCKTYPE))!=(MDB_USED|MDB_GENERAL)))
    return -1;
  return mdbData[fileref].gen.modtype;
}

int mdbGetModuleInfo(moduleinfostruct &m, unsigned short fileref)
{
  memset(&m, 0, 256);
  if ((fileref>=mdbNum)||((mdbData[fileref].flags&(MDB_USED|MDB_BLOCKTYPE))!=(MDB_USED|MDB_GENERAL)))
  {
    m.modtype=0xFF;
    m.comref=0xFFFF;
    m.compref=0xFFFF;
    m.futref=0xFFFF;
    return 0;
  }

  memcpy(&m, mdbData+fileref, 64);
  if (m.compref!=0xFFFF)
    memcpy(&m.flags2, mdbData+m.compref, 64);
  if (m.comref!=0xFFFF)
    memcpy(&m.flags3, mdbData+m.comref, 64);
  if (m.futref!=0xFFFF)
    memcpy(&m.flags4, mdbData+m.futref, 64);

  return 1;
}

int mdbWriteModuleInfo(unsigned short fileref, moduleinfostruct &m)
{
  if ((fileref>=mdbNum)||((mdbData[fileref].flags&(MDB_USED|MDB_BLOCKTYPE))!=(MDB_USED|MDB_GENERAL)))
    return 0;

  m.flags1=MDB_USED|MDB_DIRTY|MDB_GENERAL|(m.flags1&(MDB_VIRTUAL|MDB_BIGMODULE));
  m.flags2=MDB_DIRTY|MDB_COMPOSER;
  m.flags3=MDB_DIRTY|MDB_COMMENT;
  m.flags4=MDB_DIRTY|MDB_FUTURE;

  if (*m.composer||*m.style)
    m.flags2|=MDB_USED;
  if (*m.comment)
    m.flags3|=MDB_USED;

  if (m.compref!=0xFFFF)
    mdbData[m.compref].flags=MDB_DIRTY;
  if (m.comref!=0xFFFF)
    mdbData[m.comref].flags=MDB_DIRTY;
  if (m.futref!=0xFFFF)
    mdbData[m.futref].flags=MDB_DIRTY;

  m.compref=0xFFFF;
  m.comref=0xFFFF;
  m.futref=0xFFFF;
  if (m.flags2&MDB_USED)
  {
    m.compref=mdbGetNew();
    if (m.compref!=0xFFFF)
      memcpy(mdbData+m.compref, &m.flags2, 64);
  }
  if (m.flags3&MDB_USED)
  {
    m.comref=mdbGetNew();
    if (m.comref!=0xFFFF)
      memcpy(mdbData+m.comref, &m.flags3, 64);
  }
  if (m.flags4&MDB_USED)
  {
    m.futref=mdbGetNew();
    if (m.futref!=0xFFFF)
      memcpy(mdbData+m.futref, &m.flags4, 64);
  }
  memcpy(mdbData+fileref, &m, 64);
  mdbDirty=1;

  return 1;
}


struct mifentry
{
#define MIF_USED 1
#define MIF_DIRTY 2
  unsigned short flags;
  unsigned short size;
  char name[12];
};

static mifentry *mifData;
static unsigned long mifNum;
static char mifDirty;

static char mifInit()
{
  mifDirty=0;
  mifData=0;
  mifNum=0;

  char path[_MAX_PATH];
  strcpy(path, cfConfigDir);
  strcat(path, "CPMDZTAG.DAT");

  long f=open(path, O_RDONLY|O_BINARY);
  if (f<0)
    return 1;

  char sig[16];

  if (read(f, sig, 16)!=16)
  {
    close(f);
    return 1;
  }

  if (memcmp(sig, "MDZTagList\x1A\x00", 12))
  {
    close(f);
    return 1;
  }

  mifNum=*(unsigned long*)(sig+12);
  if (!mifNum)
  {
    close(f);
    return 1;
  }
  mifData=new mifentry[mifNum];
  if (!mifData)
    return 0;
  if (read(f, mifData, mifNum*sizeof(*mifData))!=(mifNum*sizeof(*mifData)))
  {
    delete mifData;
    mifNum=0;
    mifData=0;
    close(f);
    return 1;
  }
  close(f);
  return 1;
}

static void mifUpdate()
{
  if (!mifDirty||!fsWriteModInfo)
    return;
  mifDirty=0;

  char path[_MAX_PATH];
  strcpy(path, cfConfigDir);
  strcat(path, "CPMDZTAG.DAT");

  long f=open(path, O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
  if (f<0)
    return;

  lseek(f, 0, SEEK_SET);
  write(f, "MDZTagList\x1A\x00", 12);
  write(f, &mifNum, 4);

  long i=0,j;

  while (i<mifNum)
  {
    if (!(mifData[i].flags&MIF_DIRTY))
    {
      i++;
      continue;
    }
    for (j=i; j<mifNum; j++)
      if (mifData[j].flags&MIF_DIRTY)
        mifData[j].flags&=~MIF_DIRTY;
      else
        break;
    lseek(f, 16+i*sizeof(*mifData), SEEK_SET);
    write(f, mifData+i, (j-i)*sizeof(*mifData));

    i=j;
  }
  lseek(f, 0, SEEK_END);
  close(f);
}

static void mifClose()
{
  mifUpdate();
  delete mifData;
}

static char mifTagged(const char *name, unsigned short size)
{
  long i;
  for (i=0; i<mifNum; i++)
    if ((mifData[i].flags&MIF_USED)&&(mifData[i].size==size))
      if (!memcmp(mifData[i].name, name, 12))
        return 1;
  return 0;
}

static char mifTag(const char *name, unsigned short size)
{
  long i;
  for (i=0; i<mifNum; i++)
    if (!(mifData[i].flags&MIF_USED))
      break;

  if (i==mifNum)
  {
    mifNum+=256;
    void *t=realloc(mifData, mifNum*sizeof(*mifData));
    if (!t)
      return 0;
    mifData=(mifentry*)t;
    memset(mifData+i, 0, (mifNum-i)*sizeof(*mifData));
    long j;
    for (j=i; j<mifNum; j++)
      mifData[j].flags|=MIF_DIRTY;
  }
  mifData[i].size=size;
  mifData[i].flags=MIF_USED|MIF_DIRTY;
  memcpy(mifData[i].name, name, 12);
  mifDirty=1;
  return 1;
}

char mifMemRead(const char *name, unsigned short size, char *ptr)
{
  if (!mifTag(name, size))
    return 0;

  char *endp=ptr+size;

  if ((ptr+7)>endp)
    return 1;
  if (memicmp(ptr, "MODINFO", 7))
    return 1;
  ptr+=7;
  short ver=0;
  while (ptr<endp)
  {
    if (!isdigit(*ptr))
      break;
    ver=ver*10+*ptr++-'0';
  }
  if (ver!=1)
    return 1;

  while (ptr<endp)
    if ((*ptr=='\r')||(*ptr=='\n'))
      break;
    else
      ptr++;
  while (ptr<endp)
    if ((*ptr=='\r')||(*ptr=='\n'))
      ptr++;
    else
      break;

  char close=0;
  char fname[12];
  unsigned long fsize;
  unsigned char flags=0;
  unsigned short fileref=0xFFFF;
  while (1)
  {
    if (ptr==endp)
      close=1;

    if (close&&(fileref!=0xFFFF))
    {
      if (!mdbWriteModuleInfo(fileref, mdbEditBuf))
        return 0;
      fileref=0xFFFF;
    }
    close=0;

    if (ptr==endp)
      return 1;

    if (flags==3)
    {
      fileref=mdbGetModuleReference(fname, fsize);
      if (fileref==0xFFFF)
        return 0;
      if (!mdbGetModuleInfo(mdbEditBuf, fileref))
        return 0;
      flags=0;
    }

    char cmd[16];
    char arg[64];
    char *cmdp=cmd;
    char *argp=arg;

    while (ptr<endp)
      if ((*ptr==' ')||(*ptr=='\t'))
        ptr++;
      else
        break;

    while (ptr<endp)
      if (isspace(*ptr))
        break;
      else
        if ((cmd+15)>cmdp)
          *cmdp++=*ptr++;
        else
          ptr++;
    *cmdp=0;

    while (ptr<endp)
      if ((*ptr==' ')||(*ptr=='\t'))
        ptr++;
      else
        break;

    while (ptr<endp)
      if ((*ptr=='\r')||(*ptr=='\n'))
        break;
      else
        if ((arg+63)>argp)
          *argp++=*ptr++;
        else
          ptr++;
    while (argp>arg)
      if (isspace(argp[-1]))
        argp--;
      else
	break;
    *argp=0;

    while (ptr<endp)
      if ((*ptr=='\r')||(*ptr=='\n'))
        ptr++;
      else
        break;

    if (!stricmp(cmd, "MODULE"))
    {
      close=1;
      flags|=1;
      char fn[_MAX_FNAME];
      char ext[_MAX_EXT];
      strupr(arg);
      _splitpath(arg, 0, 0, fn, ext);
      fsConvFileName12(fname, fn, ext);
    }
    else
    if (!stricmp(cmd, "SIZE"))
    {
      close=1;
      flags|=2;
      fsize=strtoul(arg, 0, 10);
    }
    else
    if (!stricmp(cmd, "TYPE"))
      mdbEditBuf.modtype=mdbReadModType(arg);
    else
    if (!stricmp(cmd, "COMMENT"))
      strncpy(mdbEditBuf.comment, arg, 63);
    else
    if (!stricmp(cmd, "STYLE"))
      strncpy(mdbEditBuf.style, arg, 31);
    else
    if (!stricmp(cmd, "COMPOSER"))
      strncpy(mdbEditBuf.composer, arg, 32);
    else
    if (!stricmp(cmd, "TITLE"))
      strncpy(mdbEditBuf.modname, arg, 32);
    else
    if (!stricmp(cmd, "CHANNELS"))
      mdbEditBuf.channels=strtoul(arg, 0, 10);
    else
    if (!stricmp(cmd, "PLAYTIME"))
    {
      unsigned short min=0;
      argp=arg;
      while (isdigit(*argp))
        min=min*10+*argp++-'0';
      if ((argp[0]==':')&&isdigit(argp[1])&&isdigit(argp[2]))
        mdbEditBuf.playtime=min*60+(argp[1]-'0')*10+argp[2]-'0';
    }
    else
    if (!stricmp(cmd, "CDATE"))
    {
      argp=arg;
      if (isdigit(*argp))
      {
        unsigned char day=*argp++-'0';
        if (isdigit(*argp))
          day=day*10+*argp++-'0';
        if ((day<=31)&&(argp[0]=='.')&&isdigit(argp[1]))
        {
          argp++;
          unsigned char month=*argp++-'0';
          if (isdigit(*argp))
            month=month*10+*argp++-'0';
          if ((month<=12)&&(argp[0]=='.')&&isdigit(argp[1]))
          {
            argp++;
            unsigned short year=*argp++-'0';
            if (isdigit(*argp))
            {
              year=year*10+*argp++-'0';
              if (isdigit(*argp))
              {
                year=year*10+*argp++-'0';
                if (isdigit(*argp))
                  year=year*10+*argp++-'0';
              }
            }
            mdbEditBuf.date=(year<<16)|(month<<8)|day;
          }
        }
      }
    }
  }
}

static char mifRead(const char *name, unsigned short size, const char *path)
{
  short f=open(path, O_RDONLY|O_BINARY);
  if (f<0)
    return 1;
  char *buf=new char[size];
  if (!buf)
  {
    close(f);
    return 0;
  }
  read(f, buf, size);
  char stat=mifMemRead(name, size, buf);
  close(f);
  delete buf;
  return stat;
}

static void mifAppendInfo(short f, unsigned short fileref)
{
  char buf[60];
  modinfoentry *m=&mdbData[fileref];
  strcpy(buf, "MODULE ");
  fsConv12FileName(buf+strlen(buf), m->gen.name);
  strcat(buf, "\r\nSIZE ");
  ultoa(m->gen.size, buf+strlen(buf), 10);
  strcat(buf, "\r\n");
  write(f, buf, strlen(buf));
  if (m->gen.modtype!=0xFF)
  {
    strcpy(buf, "  TYPE ");
    strcat(buf, mdbGetModTypeString(m->gen.modtype));
    strcat(buf, "\r\n");
    write(f, buf, strlen(buf));
  }
  if (*m->gen.modname)
  {
    strcpy(buf, "  TITLE ");
    strcat(buf, m->gen.modname);
    strcat(buf, "\r\n");
    write(f, buf, strlen(buf));
  }
  if (m->gen.channels)
  {
    strcpy(buf, "  CHANNELS ");
    ultoa(m->gen.channels, buf+strlen(buf), 10);
    strcat(buf, "\r\n");
    write(f, buf, strlen(buf));
  }
  if (m->gen.playtime)
  {
    strcpy(buf, "  PLAYTIME ");
    ultoa(m->gen.playtime/60, buf+strlen(buf), 10);
    strcat(buf, ":00");
    buf[strlen(buf)-2]+=(m->gen.playtime%60)/10;
    buf[strlen(buf)-1]+=m->gen.playtime%10;
    strcat(buf, "\r\n");
    write(f, buf, strlen(buf));
  }
  if (m->gen.date)
  {
    strcpy(buf, "  CDATE ");
    ultoa(m->gen.date&0xFF, buf+strlen(buf), 10);
    strcat(buf, ".");
    ultoa((m->gen.date>>8)&0xFF, buf+strlen(buf), 10);
    strcat(buf, ".");
    ultoa(m->gen.date>>16, buf+strlen(buf), 10);
    strcat(buf, "\r\n");
    write(f, buf, strlen(buf));
  }
  if (m->gen.compref!=0xFFFF)
  {
    modinfoentry *m2=&mdbData[m->gen.compref];
    if (*m2->comp.composer)
    {
      strcpy(buf, "  COMPOSER ");
      strcat(buf, m2->comp.composer);
      strcat(buf, "\r\n");
      write(f, buf, strlen(buf));
    }
    if (*m2->comp.style)
    {
      strcpy(buf, "  STYLE ");
      strcat(buf, m2->comp.style);
      strcat(buf, "\r\n");
      write(f, buf, strlen(buf));
    }
  }
  if ((m->gen.comref!=0xFFFF)&&*mdbData[m->gen.comref].comment)
  {
    strcpy(buf, "  COMMENT ");
    strcat(buf, mdbData[m->gen.comref].comment);
    strcat(buf, "\r\n");
    write(f, buf, strlen(buf));
  }
}

static arcentry *adbData;
static unsigned long adbNum;
static unsigned long adbFindArc;
static unsigned long adbFindPos;
static char adbDirty;

static char adbInit()
{
  adbDirty=0;
  adbData=0;
  adbNum=0;

  char path[_MAX_PATH];
  strcpy(path, cfConfigDir);
  strcat(path, "CPARCS.DAT");

  long f=open(path, O_RDONLY|O_BINARY);
  if (f<0)
    return 1;

  char sig[20];

  if (read(f, sig, 20)!=20)
  {
    close(f);
    return 1;
  }

  if (memcmp(sig, "CPArchiveCache\x1A\x00", 16))
  {
    close(f);
    return 1;
  }

  adbNum=*(unsigned long*)(sig+16);
  if (!adbNum)
  {
    close(f);
    return 1;
  }
  adbData=new arcentry[adbNum];
  if (!adbData)
    return 0;
  if (read(f, adbData, adbNum*sizeof(*adbData))!=(adbNum*sizeof(*adbData)))
  {
    delete adbData;
    adbData=0;
    adbNum=0;
    close(f);
    return 1;
  }
  close(f);
  return 1;
}

static void adbUpdate()
{
  if (!adbDirty)
    return;
  adbDirty=0;

  char path[_MAX_PATH];
  strcpy(path, cfConfigDir);
  strcat(path, "CPARCS.DAT");

  long f=open(path, O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
  if (f<0)
    return;

  lseek(f, 0, SEEK_SET);
  write(f, "CPArchiveCache\x1A\x00", 16);
  write(f, &adbNum, 4);

  long i=0,j;

  while (i<adbNum)
  {
    if (!(adbData[i].flags&ADB_DIRTY))
    {
      i++;
      continue;
    }
    for (j=i; j<adbNum; j++)
      if (adbData[j].flags&ADB_DIRTY)
        adbData[j].flags&=~ADB_DIRTY;
      else
        break;
    lseek(f, 20+i*sizeof(*adbData), SEEK_SET);
    write(f, adbData+i, (j-i)*sizeof(*adbData));

    i=j;
  }
  lseek(f, 0, SEEK_END);
  close(f);
}

static void adbClose()
{
  adbUpdate();
  delete adbData;
}

int adbAdd(const arcentry &a)
{
  long i;
  for (i=0; i<adbNum; i++)
    if (!(adbData[i].flags&ADB_USED))
      break;

  if (i==adbNum)
  {
    adbNum+=256;
    void *t=realloc(adbData, adbNum*sizeof(*adbData));
    if (!t)
      return 0;
    adbData=(arcentry*)t;
    memset(adbData+i, 0, (adbNum-i)*sizeof(*adbData));
    long j;
    for (j=i; j<adbNum; j++)
      adbData[j].flags|=ADB_DIRTY;
  }
  adbData[i]=a;
  adbData[i].flags|=ADB_USED|ADB_DIRTY;
  if (a.flags&ADB_ARC)
    adbData[i].parent=i;
  adbDirty=1;
  return 1;
}

unsigned short adbFind(const char *arcname)
{
  long i;
  for (i=0; i<adbNum; i++)
    if ((adbData[i].flags&(ADB_USED|ADB_ARC))==(ADB_USED|ADB_ARC))
      if (!memcmp(adbData[i].name, arcname, 12))
        return (unsigned short)i;
  return 0xFFFF;
}

static adbregstruct *adbPackers = 0;

static void adbRegister(adbregstruct *r)
{
  r->next=adbPackers;
  adbPackers=r;
}

int adbCallArc(const char *cmd, const char *arc, const char *name, const char *dir)
{
  char cmdline[200];
  char *cp=cmdline;
  while (*cmd)
  {
    if (*cmd=='%')
    {
      if ((cmd[1]=='a')||(cmd[1]=='A'))
      {
        strcpy(cp, arc);
        cp+=strlen(cp);
      }
      if ((cmd[1]=='n')||(cmd[1]=='N'))
      {
        strcpy(cp, name);
        cp+=strlen(cp);
      }
      if ((cmd[1]=='d')||(cmd[1]=='D'))
      {
        strcpy(cp, dir);
        cp+=strlen(cp);
      }
      if (cmd[1]=='%')
        *cp++='%';
      cmd+=cmd[1]?2:1;
    }
    else
      *cp++=*cmd++;
  }
  *cp=0;
#ifdef DOS32
  return plSystem(cmdline);
#else
  return system(cmdline);
#endif
}

static unsigned char isarchive(const char *ext)
{
  adbregstruct *packers;
  for (packers=adbPackers; packers; packers=packers->next)
    if (!stricmp(ext, packers->ext))
      return 1;
  return 0;
}

static signed char adbFindNext(char *findname, unsigned long &findlen)
{
  long i;
  for (i=adbFindPos; i<adbNum; i++)
    if ((adbData[i].flags&(ADB_USED|ADB_ARC))==ADB_USED)
      if (adbData[i].parent==adbFindArc)
      {
        memcpy(findname, adbData[i].name, 12);
        findlen=adbData[i].size;
        adbFindPos=i+1;
        return 0;
      }
  return 1;
}

static signed char adbFindFirst(const char *path, unsigned long arclen, char *findname, unsigned long &findlen)
{
  char ext[_MAX_EXT];
  char name[_MAX_FNAME];
  char arcname[12];

  _splitpath(path, 0, 0, name, ext);
  fsConvFileName12(arcname, name, ext);

  unsigned short ar=adbFind(arcname);
  long i;
  if ((ar==0xFFFF)||(arclen!=adbData[ar].size))
  {
    if (ar!=0xFFFF)
      for (i=0; i<adbNum; i++)
        if (adbData[i].parent==ar)
          adbData[i].flags=(adbData[i].flags|ADB_DIRTY)&~ADB_USED;
    adbDirty=1;
    adbregstruct *packers;
    for (packers=adbPackers; packers; packers=packers->next)
      if (!strcmp(ext, packers->ext))
      {
        if (!packers->Scan(path))
          return -1;
        else
          break;
      }
    if (!packers)
      return 1;
    ar=adbFind(arcname);
  }
  adbFindArc=ar;
  adbFindPos=0;
  return adbFindNext(findname, findlen);
}


static void mdbScan(const modlistentry &m)
{
  if (m.fileref>=0xFFFC)
    return;

  char path[_MAX_PATH];
  dmGetPath(path, m.dirref);
  fsConv12FileName(path+strlen(path), m.name);

  if (!mdbInfoRead(m.fileref))
  {
    sbinfile f;
    if (!f.open(path, sbinfile::openro))
    {
      mdbGetModuleInfo(mdbEditBuf, m.fileref);
      mdbReadInfo(mdbEditBuf, f);
      f.close();
      mdbWriteModuleInfo(m.fileref, mdbEditBuf);
    }
  }
}


struct modlist
{
  modlistentry *files;
  signed long num;
  signed long max;
  signed long pos;

  long fuzfirst;
  unsigned short fuzval;
  char fuzmask[12];

  modlist() { files=0; num=max=pos=0; }
  ~modlist() { delete files; }

  char insert(unsigned long before, const modlistentry *f, unsigned long n);
  char append(const modlistentry &f);
  void remove(unsigned long from, unsigned long n);
  void get(modlistentry *f, unsigned long from, unsigned long n) const;
  void getcur(modlistentry &f) const;
//  char copy(modlist &dest, unsigned long to, unsigned long from, unsigned long n);

  void sort();

  long find(const modlistentry &f);
  long fuzzyfind(const char *c);
  long fuzzyfindnext();
};

int mdbAppend(modlist &m, const modlistentry &f)
{
  return m.append(f);
}

int mdbAppendNew(modlist &m, const modlistentry &f)
{
  if (m.find(f)!=-1)
    return 1;
  return m.append(f);
}

int fsReadDir(modlist &ml, unsigned short dirref, const char *mask, unsigned long opt);

char modlist::insert(unsigned long before, const modlistentry *f, unsigned long n)
{
  if (before>num)
    before=num;

  if ((num+n)>10000)
    return 1;

  if (pos>=before)
    pos+=n;

  if ((num+n)>max)
  {
    max=(num+n+255)&~255;
    void *t=realloc(files, sizeof(*files)*max);
    if (!t)
      return 0;
    files=(modlistentry *)t;
  }

  mymemmovep(files+before+n, files+before, num-before);
  mymemmovep(files+before, f, n);
  num+=n;
  return 1;
}

char modlist::append(const modlistentry &f)
{
  return insert(num, &f, 1);
}

void modlist::remove(unsigned long from, unsigned long n)
{
  if (from>num)
    return;
  if ((from+n)>num)
    n=num-from;
  if (pos>from)
    if (pos>(from+n))
      pos-=n;
    else
      pos=from;
  mymemmovep(files+from, files+from+n, num-from-n);
  num-=n;

  // free?
}

void modlist::get(modlistentry *f, unsigned long from, unsigned long n) const
{
  if (from>num)
    return;
  if ((from+n)>num)
    n=num-from;
  mymemmovep(f, files+from, n);
}

void modlist::getcur(modlistentry &f) const
{
  get(&f, pos, 1);
}

long modlist::find(const modlistentry &f)
{
  long i;
  for (i=0; i<num; i++)
    if (!memcmp(&files[i], &f, sizeof(*files)))
      break;
  return (i==num)?-1:i;
}

long modlist::fuzzyfind(const char *c)
{
  memcpy(fuzmask, c, 12);
  long i;
  fuzval=0;
  fuzfirst=0;
  for (i=0; i<num; i++)
  {
    char *cn=files[i].name;
    unsigned short cur=0;
    short j;
    for (j=0; j<8; j++)
      if (fuzmask[j]==cn[j])
        cur+=(fuzmask[j]==' ')?1:20;
    if (fuzmask[8]=='.')
      for (j=8; j<12; j++)
        if (fuzmask[j]==cn[j])
          cur+=(fuzmask[j]==' ')?1:20;
    if (cur>fuzval)
    {
      fuzval=cur;
      fuzfirst=i;
    }
  }
  return fuzfirst;
}

long modlist::fuzzyfindnext()
{
  long i;
  for (i=fuzfirst+1; i<num; i++)
  {
    char *cn=files[i].name;
    unsigned short cur=0;
    short j;
    for (j=0; j<8; j++)
      if (fuzmask[j]==cn[j])
        cur+=(fuzmask[j]==' ')?1:20;
    if (fuzmask[8]=='.')
      for (j=8; j<12; j++)
        if (fuzmask[j]==cn[j])
          cur+=(fuzmask[j]==' ')?1:20;
    if (cur==fuzval)
    {
      fuzfirst=i;
      return fuzfirst;
    }
  }
  fuzfirst=-1;
  return fuzzyfindnext();
}

static int mlecmp(const void *a, const void *b)
{
  modlistentry &x=*(modlistentry *)a;
  modlistentry &y=*(modlistentry *)b;
  if (x.fileref>=0xFFFC)
  {
    if (y.fileref<0xFFFC)
      return 1;
    if (x.fileref==y.fileref)
      return memcmp(x.name, y.name, sizeof(x.name));
    else
      if (x.fileref<y.fileref)
        return -1;
      else
        return 1;
  }
  if (y.fileref>=0xFFFC)
    return -1;
  return memcmp(x.name, y.name, sizeof(x.name));
}

void modlist::sort()
{
  qsort(files, num, sizeof (*files), mlecmp);
}


static int arcReadDir(modlist &ml, unsigned short dirref, const char *mask, unsigned long)
{
  char path[_MAX_PATH];

  modlistentry m;

  dmGetPath(path, dirref);
  if (isarchivepath(path))
  {
    dmGetPath(path, dirref);
    path[strlen(path)-1]=0;

    find_t fi;
    int tf=open(path, O_RDONLY|O_BINARY);
    if (tf<0)
      return 1;
    fi.size=filelength(tf);
    close(tf);

    unsigned long size;

    signed char done;
    for (done=adbFindFirst(path, fi.size, m.name, size); !done; done=adbFindNext(m.name, size))
    {
      if (!fsMatchFileName12(m.name, mask))
        continue;

      m.dirref=dirref;
      m.fileref=mdbGetModuleReference(m.name, size);
      if (m.fileref==0xFFFF)
        return 0;
      if (!ml.append(m))
        return 0;
    }
    if (done==-1)
      return 0;
  }

  return 1;
}

static int stdReadDir(modlist &ml, unsigned short dirref, const char *, unsigned long opt)
{
  modlistentry m;

  if (opt&RD_PUTDSUBS)
  {
    fsConvFileName12(m.name, "..", "");
    m.dirref=dmGetParent(dirref);
    m.fileref=0xFFFD;
    if (!mdbAppendNew(ml, m))
      return 0;

    fsConvFileName12(m.name, "\\", "");
    m.dirref=dmGetRoot(dirref);
    m.fileref=0xFFFD;
    if (!mdbAppendNew(ml, m))
      return 0;
  }
  return 1;
}

static int dosReadDir(modlist &ml, unsigned short dirref, const char *mask, unsigned long opt)
{
  char path[_MAX_PATH];

  modlistentry m;


  if (opt&RD_PUTDSUBS)
  {
    unsigned int savedrive;
    _dos_getdrive(&savedrive);
#ifdef WIN32
    unsigned int disknum=26;
#else
    unsigned int disknum;
    _dos_setdrive(savedrive, &disknum);
#endif
    int i;
    unsigned int dummy;
//    for (i=2; i<disknum; i++)
    for (i=0; i<disknum; i++)
    {
      _dos_setdrive(i+1, &dummy);
      _dos_getdrive(&dummy);
      if (i!=(dummy-1))
        continue;

      fsConvFileName12(m.name, "A:", "");
      *m.name+=i;
      m.fileref=0xFFFF;
      m.dirref=dmGetDriveDir(i+1);
      if (!mdbAppendNew(ml,m))
        return 0;
    }
    _dos_setdrive(savedrive, &dummy);
  }

  dmGetPath(path, dirref);
  if (!isarchivepath(path))
  {
    dmGetPath(path, dirref);
    if (opt&RD_PUTSUBS)
      strcat(path, "*.*");
    else
    {
      char curfile[_MAX_NAME];
      fsConv12FileName(curfile, mask);
      strcat(path, curfile);
    }
    find_t fi;

    char done=(char)_dos_findfirst(path, _A_SUBDIR
#ifdef DOS32
                                            |_A_RDONLY
#endif
                                                       , &fi);
#ifdef WIN32
    char thispath[_MAX_PATH];
    strcpy(thispath, path);
#endif
    int dirsrc=!0, first=!0;    // sorry for this, but i have no time invent some funny for/while/...-loops now. so i used this flags.
    while(1)
    {
      if(!first) done=(char)_dos_findnext(&fi);
      first=0;

      if(done&&dirsrc)
      {
//        _dos_findclose(&fi);
#ifdef WIN32
        done=_dos_findfirst(thispath, _A_NORMAL, &fi);
#endif
        dirsrc=0;
      }
      if(done) break;

      if (!strcmp(fi.name, ".")||!strcmp(fi.name, ".."))
        continue;

      char curname[_MAX_FNAME];
      char curext[_MAX_EXT];
      _splitpath(fi.name, 0, 0, curname, curext);
      fsConvFileName12(m.name, curname, curext);
      char isdir=!!(fi.attrib&_A_SUBDIR);
      if (isdir||isarchive(curext))
      {
        dmGetPath(path, dirref);
        strcat(path, fi.name);
        m.dirref=dmGetPathReference(path);
        if (m.dirref==0xFFFF)
          return 0;
        if (isdir)
        {
          if (opt&RD_PUTSUBS)
          {
            m.fileref=0xFFFE;
            if (!ml.append(m))
              return 0;
          }
        }
        else
        {
          if ((opt&RD_PUTSUBS)&&(fsPutArcs||!(opt&RD_ARCSCAN)))
          {
            m.fileref=0xFFFC;
            if (!ml.append(m))
              return 0;
          }
          if (opt&RD_ARCSCAN)
          {
            if (!fsReadDir(ml, m.dirref, mask, opt&~RD_PUTDSUBS))
              return 0;
          }
        }
        continue;
      }

      if ((fi.size<65536)&&!strcmp(curext, MIF_EXT)&&!mifTagged(m.name, fi.size)&&fsScanMIF)
      {
        dmGetPath(path, dirref);
        strcat(path, fi.name);
        if (!mifRead(m.name, fi.size, path))
          return 0;
      }

      if (!fsMatchFileName12(m.name, mask)||!fsIsModule(curext))
        continue;

      m.dirref=dirref;
      m.fileref=mdbGetModuleReference(m.name, fi.size);
      if (m.fileref==0xFFFF)
        return 0;
      if (!ml.append(m))
        return 0;
    }
  }

  return 1;
}


static mdbreaddirregstruct *mdbReadDirs = 0;

static void mdbRegisterReadDir(mdbreaddirregstruct *r)
{
  r->next=mdbReadDirs;
  mdbReadDirs=r;
}


int fsReadDir(modlist &ml, unsigned short dirref, const char *mask, unsigned long opt)
{
  mdbreaddirregstruct *readdirs;
  for (readdirs=mdbReadDirs; readdirs; readdirs=readdirs->next)
    if (!readdirs->ReadDir(ml, dirref, mask, opt))
      return 0;
  return 1;
}


static modlistentry nextplay;
static unsigned char isnextplay;
static modlist playlist;
static modlist viewlist;
static char curmask[12];
static char curdirpath[_MAX_PATH];
static short dirwinheight;
static char quickfind[12];
static char quickfindpos;
static unsigned long scanpos;
static short editpos=0;
static short editmode=0;



static int getlongfile(char *dst, const char *src)
{
#ifdef DOS32
  callrmstruct r;
  char *mem;
  void __far16 *rmptr;
  __segment pmsel;
  mem=(char*)dosmalloc(256+260, rmptr, pmsel);
  if (!mem)
    return 0;
  strcpy(mem, src);
  clearcallrm(r);
  r.w.ax=0x7160;
  r.b.cl=2;
  r.b.ch=0x80;
  r.w.si=(unsigned short)rmptr;
  r.w.di=((unsigned short)rmptr)+256;
  r.s.ds=((unsigned long)rmptr)>>16;
  r.s.es=((unsigned long)rmptr)>>16;
  r.s.flags|=1;
  intrrm(0x21,r);
  if (r.s.flags&1)
  {
    dosfree(pmsel);
    return 0;
  }
  strcpy(dst, mem+256);
  dosfree(pmsel);
  return 1;
#else
  strcpy(dst, src);             // yeah, we are native win32 :))
  return 1;
#endif
}


static char isarchivepath(const char *p)
{
  char path[_MAX_PATH];
  char ext[_MAX_EXT];
  strcpy(path, p);
  if (path[strlen(path)-1]=='\\')
    path[strlen(path)-1]=0;
  _splitpath(path, 0, 0, 0, ext);
  if (!isarchive(ext))
    return 0;

  find_t fi;
#ifdef DOS32
  if (_dos_findfirst(path, _A_RDONLY, &fi)) // |_A_SUBDIR ?!?!?
#else
  if (_dos_findfirst(path, _A_NORMAL, &fi)) // |_A_SUBDIR ?!?!?
#endif
    return 0;
  if (fi.attrib&_A_SUBDIR)                  // ?!??
    return 0;
  return 1;
}

static void displayfile(unsigned short y, const modlistentry &m, unsigned char sel)
{
  unsigned char col=((m.fileref>=0xFFFC)?0x07:0x0F)|((sel==1)?0x80:0x00);
  short sbuf[117];
  writestring(sbuf, 0, col, "", plScrWidth-15);
  if (sel==2)
  {
    writestring(sbuf, 0, 0x07, "->", 2);
    writestring(sbuf, plScrWidth-17, 0x07, "<-", 2);
  }
  writestring(sbuf, 2, col, m.name, 12);
  if (m.fileref>=0xFFFC)
    switch (m.fileref)
    {
    case 0xFFFC:
      writestring(sbuf, 16, col, "<ARC>", 5);
      break;
    case 0xFFFD:
    case 0xFFFE:
      writestring(sbuf, 16, col, "<DIR>", 5);
      break;
    case 0xFFFF:
      writestring(sbuf, 16, col, "<DRV>", 5);
      break;
    }
  else
  {
    modinfoentry &m1=mdbData[m.fileref];
    if (m1.gen.modtype==0xFF)
      col&=~0x08;
    else
      if (fsColorTypes)
      {
        col&=0xF8;
        col|=fsTypeCols[m1.gen.modtype&0xFF];
      }
    if (plScrWidth==132)
      if (fsInfoMode&1)
      {
        if (m1.gen.comref!=0xFFFF)
          writestring(sbuf, 16, col, mdbData[m1.gen.comref].comment, 63);
        if (m1.gen.compref!=0xFFFF)
          writestring(sbuf, 84, col, mdbData[m1.gen.compref].comp.style, 31);
      }
      else
      {
        writestring(sbuf, 16, col, m1.gen.modname, 32);
        if (m1.gen.channels)
          writenum(sbuf, 50, col, m1.gen.channels, 10, 2, 1);
        if (m1.gen.playtime)
        {
          writenum(sbuf, 53, col, m1.gen.playtime/60, 10, 3, 1);
          writestring(sbuf, 56, col, ":", 1);
          writenum(sbuf, 57, col, m1.gen.playtime%60, 10, 2, 0);
        }
        if (m1.gen.compref!=0xFFFF)
          writestring(sbuf, 61, col, mdbData[m1.gen.compref].comp.composer, 32);

        if (m1.gen.date)
        {
          if (m1.gen.date&0xFF)
          {
            writestring(sbuf, 96, col, ".", 3);
            writenum(sbuf, 94, col, m1.gen.date&0xFF, 10, 2, 1);
          }
          if (m1.gen.date&0xFFFF)
          {
            writestring(sbuf, 99, col, ".", 3);
            writenum(sbuf, 97, col, (m1.gen.date>>8)&0xFF, 10, 2, 1);
          }
          if (m1.gen.date>>16)
          {
            writenum(sbuf, 100, col, m1.gen.date>>16, 10, 4, 1);
            if (!((m1.gen.date>>16)/100))
              writestring(sbuf, 101, col, "'", 1);
          }
        }
        if (m1.gen.size<1000000000)
          writenum(sbuf, 106, (m1.flags&MDB_BIGMODULE)?((col&0xF0)|0x0C):col, m1.gen.size, 10, 9, 1);
        else
          writenum(sbuf, 107, col, m1.gen.size, 16, 8, 0);
      }
    else
      switch (fsInfoMode)
      {
      case 0:
        writestring(sbuf, 16, col, m1.gen.modname, 32);
        if (m1.gen.channels)
          writenum(sbuf, 50, col, m1.gen.channels, 10, 2, 1);
        if (m1.gen.size<1000000000)
          writenum(sbuf, 54, (m1.flags&MDB_BIGMODULE)?((col&0xF0)|0x0C):col, m1.gen.size, 10, 9, 1);
        else
          writenum(sbuf, 55, col, m1.gen.size, 16, 8, 0);
        break;
      case 1:
        if (m1.gen.compref!=0xFFFF)
          writestring(sbuf, 16, col, mdbData[m1.gen.compref].comp.composer, 32);
        if (m1.gen.date)
        {
          if (m1.gen.date&0xFF)
          {
            writestring(sbuf, 55, col, ".", 3);
            writenum(sbuf, 53, col, m1.gen.date&0xFF, 10, 2, 1);
          }
          if (m1.gen.date&0xFFFF)
          {
            writestring(sbuf, 58, col, ".", 3);
            writenum(sbuf, 56, col, (m1.gen.date>>8)&0xFF, 10, 2, 1);
          }
          if (m1.gen.date>>16)
          {
            writenum(sbuf, 59, col, m1.gen.date>>16, 10, 4, 1);
            if (!((m1.gen.date>>16)/100))
              writestring(sbuf, 60, col, "'", 1);
          }
        }
        break;
      case 2:
        if (m1.gen.comref!=0xFFFF)
          writestring(sbuf, 16, col, mdbData[m1.gen.comref].comment, 47);
        break;
      case 3:
        if (m1.gen.compref!=0xFFFF)
          writestring(sbuf, 16, col, mdbData[m1.gen.compref].comp.style, 31);
        if (m1.gen.playtime)
        {
          writenum(sbuf, 57, col, m1.gen.playtime/60, 10, 3, 1);
          writestring(sbuf, 60, col, ":", 1);
          writenum(sbuf, 61, col, m1.gen.playtime%60, 10, 2, 0);
        }
        break;
      }
  }

  displaystrattr(y, 0, sbuf, plScrWidth-15);
}

static void fsShowDir(short firstv, short selectv, short firstp, short selectp, short selecte, const modlistentry &mle)
{
  int i;

  short vrelpos=-1;
  if (viewlist.num>dirwinheight)
    vrelpos=dirwinheight*viewlist.pos/viewlist.num;
  short prelpos=-1;
  if (playlist.num>dirwinheight)
    prelpos=dirwinheight*playlist.pos/playlist.num;

  if (plScrWidth==132)
    displaystr(0, 0, 0x30, "    opencp                                    file selector ][                                 (c) '94-'98 Niklas Beisert et al.    ", 132);
  else
    displaystr(0, 0, 0x30, "  opencp         file selector ][            (c) '94-'98 Niklas Beisert et al.  ", 80);
  displaystr(1, 0, 0x0F, curdirpath, plScrWidth);
  displaystr(2, 0, 0x07, "ִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִ", plScrWidth-15);
  displaystr(2, plScrWidth-15, 0x07, "ֲִִִִִִִִִִִִִִ", 15);

  short sbuf[132];

  if (fsEditWin||(selecte>=0))
  {
    int first=dirwinheight+3;
    displaystr(first, 0, 0x07, "ִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִִ", plScrWidth-15);
    displaystr(first, plScrWidth-15, 0x07, "ֱִִִִִִִִִִִִִִ", 15);
    char longfile[270];
    modinfoentry *m1=0, *m2=0, *m3=0;
    const char *modtype="";
    if (mle.fileref<0xFFFC)
    {
      m1=mdbData+mle.fileref;
      if (m1->gen.compref!=0xFFFF)
        m2=mdbData+m1->gen.compref;
      if (m1->gen.comref!=0xFFFF)
        m3=mdbData+m1->gen.comref;
      modtype=mdbGetModTypeString(m1->gen.modtype);
    }
    char *longptr;
    if ((mle.fileref==0xFFFF)||(mle.fileref==0xFFFD))
    {
      fsConv12FileName(longfile, mle.name);
      longptr=longfile;
    }
    else
    {
      dmGetPath(longfile, mle.dirref);
      if (mle.fileref<0xFFFC)
        fsConv12FileName(longfile+strlen(longfile), mle.name);
      getlongfile(longfile, longfile);
      if (longfile[strlen(longfile)-1]=='\\')
        longfile[strlen(longfile)-1]=0;
      longptr=strrchr(longfile, '\\');
      longptr=longptr?(longptr+1):longfile;
    }
    if (plScrWidth==132)
    {
      writestring(sbuf, 0, 0x07, "  תתתתתתתת.תתת    תתתתתתתת      title:    תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת       type: תתתת     channels: תת      playtime: תתת:תת   ", 132);
      if (m1)
      {
        writestring(sbuf, 2, 0x0F, m1->gen.name, 12);
        if (memcmp(m1->gen.name+8, ".CDA", 4))
        {
          writenum(sbuf, 16, 0x0F, m1->gen.size, 10, 10, 1);
          if (m1->flags&MDB_BIGMODULE)
            writestring(sbuf, 26, 0x0F, "!", 1);
        }
        else
          writenum(sbuf, 18, 0x0F, m1->gen.size, 16, 8, 0);
      }
      if (m1&&*m1->gen.modname)
        writestring(sbuf, 42, 0x0F, m1->gen.modname, 32);
      if (selecte==0)
        markstring(sbuf, 42, 32);
      if (*modtype)
        writestring(sbuf, 86, 0x0F, modtype, 4);
      if (selecte==1)
        markstring(sbuf, 86, 4);
      if (m1&&m1->gen.channels)
        writenum(sbuf, 105, 0x0F, m1->gen.channels, 10, 2, 1);
      if (selecte==2)
        markstring(sbuf, 105, 2);

      if (m1&&m1->gen.playtime)
      {
        writenum(sbuf, 123, 0x0F, m1->gen.playtime/60, 10, 3, 1);
        writestring(sbuf, 126, 0x0F, ":", 1);
        writenum(sbuf, 127, 0x0F, m1->gen.playtime%60, 10, 2, 0);
      }
      if (selecte==3)
        markstring(sbuf, 123, 6);
      displaystrattr(first+1, 0, sbuf, 132);

      writestring(sbuf, 0, 0x07, "                                composer: תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת     style: תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת               ", 132);
      if (m2&&*m2->comp.composer)
        writestring(sbuf, 42, 0x0F, m2->comp.composer, 32);
      if (selecte==4)
        markstring(sbuf, 42, 32);
      if (m2&&*m2->comp.style)
        writestring(sbuf, 86, 0x0F, m2->comp.style, 31);
      if (selecte==5)
        markstring(sbuf, 86, 31);
      displaystrattr(first+2, 0, sbuf, 132);

      writestring(sbuf, 0, 0x07, "                                date:     תת.תת.תתתת     comment: תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת   ", 132);
      if (m1&&m1->gen.date)
      {
        if (m1->gen.date&0xFF)
        {
          writestring(sbuf, 44, 0x0F, ".", 3);
          writenum(sbuf, 42, 0x0F, m1->gen.date&0xFF, 10, 2, 1);
        }
        if (m1->gen.date&0xFFFF)
        {
          writestring(sbuf, 47, 0x0F, ".", 3);
          writenum(sbuf, 45, 0x0F, (m1->gen.date>>8)&0xFF, 10, 2, 1);
        }
        if (m1->gen.date>>16)
        {
          writenum(sbuf, 48, 0x0F, m1->gen.date>>16, 10, 4, 1);
          if (!((m1->gen.date>>16)/100))
            writestring(sbuf, 49, 0x0F, "'", 1);
        }
      }
      if (selecte==6)
        markstring(sbuf, 42, 10);
      if (m3&&*m3->comment)
        writestring(sbuf, 66, 0x0F, m3->comment, 63);
      if (selecte==7)
        markstring(sbuf, 66, 63);
      displaystrattr(first+3, 0, sbuf, 132);

      writestring(sbuf, 0, 0x07, "    long: ", 132);
      writestring(sbuf, 10, 0x0F, longptr, 122);
      displaystrattr(first+4, 0, sbuf, 132);
    }
    else
    {
      writestring(sbuf, 0, 0x07, "  תתתתתתתת.תתת   תתתתתתתת   title: תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת  type: תתתת ", 80);
      if (m1)
      {
        writestring(sbuf, 2, 0x0F, m1->gen.name, 12);
        if (memcmp(m1->gen.name+8, ".CDA", 4))
        {
          writenum(sbuf, 15, 0x0F, m1->gen.size, 10, 10, 1);
          if (m1->flags&MDB_BIGMODULE)
            writestring(sbuf, 25, 0x0F, "!", 1);
        }
        else
          writenum(sbuf, 17, 0x0F, m1->gen.size, 16, 8, 0);
      }
      if (m1&&*m1->gen.modname)
        writestring(sbuf, 35, (selecte==0)?0x8F:0x0F, m1->gen.modname, 32);
      if (selecte==0)
        markstring(sbuf, 35, 32);
      if (*modtype)
        writestring(sbuf, 75, (selecte==1)?0x8F:0x0F, modtype, 4);
      if (selecte==1)
        markstring(sbuf, 75, 4);

      displaystrattr(first+1, 0, sbuf, 80);

      writestring(sbuf, 0, 0x07, "   composer: תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת   date:     תת.תת.תתתת            ", 80);
      if (m1&&m1->gen.date)
      {
        if (m1->gen.date&0xFF)
        {
          writestring(sbuf, 60, 0x0F, ".", 3);
          writenum(sbuf, 58, 0x0F, m1->gen.date&0xFF, 10, 2, 1);
        }
        if (m1->gen.date&0xFFFF)
        {
          writestring(sbuf, 63, 0x0F, ".", 3);
          writenum(sbuf, 61, 0x0F, (m1->gen.date>>8)&0xFF, 10, 2, 1);
        }
        if (m1->gen.date>>16)
        {
          writenum(sbuf, 64, 0x0F, m1->gen.date>>16, 10, 4, 1);
          if (!((m1->gen.date>>16)/100))
            writestring(sbuf, 65, 0x0F, "'", 1);
        }
      }
      if (selecte==6)
        markstring(sbuf, 58, 10);
      if (m2&&*m2->comp.composer)
        writestring(sbuf, 13, 0x0F, m2->comp.composer, 32);
      if (selecte==4)
        markstring(sbuf, 13, 32);
      displaystrattr(first+2, 0, sbuf, 80);

      writestring(sbuf, 0, 0x07, "   style:    תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת    playtime: תתת:תת   channels: תת ", 80);
      if (m1&&m1->gen.channels)
        writenum(sbuf, 77, 0x0F, m1->gen.channels, 10, 2, 1);
      if (selecte==2)
        markstring(sbuf, 77, 2);
      if (m1&&m1->gen.playtime)
      {
        writenum(sbuf, 58, 0x0F, m1->gen.playtime/60, 10, 3, 1);
        writestring(sbuf, 61, 0x0F, ":", 1);
        writenum(sbuf, 62, 0x0F, m1->gen.playtime%60, 10, 2, 0);
      }
      if (selecte==3)
        markstring(sbuf, 58, 6);
      if (m2&&*m2->comp.style)
        writestring(sbuf, 13, 0x0F, m2->comp.style, 31);
      if (selecte==5)
        markstring(sbuf, 13, 31);
      displaystrattr(first+3, 0, sbuf, 80);

      writestring(sbuf, 0, 0x07, "   comment:  תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת    ", 80);
      if (m3&&*m3->comment)
        writestring(sbuf, 13, (selecte==7)?0x8F:0x0F, m3->comment, 63);
      if (selecte==7)
        markstring(sbuf, 13, 63);
      displaystrattr(first+4, 0, sbuf, 80);

      writestring(sbuf, 0, 0x07, "   long: ", 80);
      writestring(sbuf, 9, 0x0F, longptr, 71);
      displaystrattr(first+5, 0, sbuf, 80);
    }
  }

  writestring(sbuf, 0, 0x17, " quickfind: [תתתתתתתתתתתת]    press F1 for help ", plScrWidth);
  writestring(sbuf, 13, 0x1F, quickfind, quickfindpos);

  displaystrattr(plScrHeight-1, 0, sbuf, plScrWidth);

  for (i=0; i<dirwinheight; i++)
  {
    displaystr(i+3, plScrWidth-15, 0x07, (i==vrelpos)?(i==prelpos)?"Û":"Ý":(i==prelpos)?"Þ":"³", 1);

    modlistentry m;
    if (((firstv+i)<0)||((firstv+i)>=viewlist.num))
      displayvoid(i+3, 0, plScrWidth-15);
    else
    {
      viewlist.get(&m, firstv+i, 1);
      displayfile(i+3, m, ((firstv+i)!=selectv)?0:(selecte<0)?1:2);
    }

    if (((firstp+i)<0)||((firstp+i)>=playlist.num))
      displayvoid(i+3, plScrWidth-14, 14);
    else
    {
      playlist.get(&m, firstp+i, 1);
      short sbuf[14];
      if (((firstp+i)==selectp)&&(selecte>=0))
        writestring(sbuf, 0, 0x07, "\x1A            \x1B", 14);
      else
        writestring(sbuf, 0, (((firstp+i)==selectp)&&(selecte<0))?0x8F:0x0F, "", 14);
      writestring(sbuf, 1, (((firstp+i)==selectp)&&(selecte<0))?0x8F:0x0F, m.name, 12);
      displaystrattr(i+3, plScrWidth-14, sbuf, 14);
    }
  }
}

static char fsExpandPath(char *dp, char *mask, const char *p)
{
  char path[_MAX_PATH];
  strcpy(path, p);
  if (!dmFullPath(path))
    return 0;
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char name[_MAX_FNAME];
  char ext[_MAX_EXT];
  _splitpath(path, drive, dir, name, ext);
  if (!(strchr(name, '*')||strchr(name, '?')||strchr(ext, '*')||strchr(ext, '?')))
  {
    find_t fi;
    if (!_dos_findfirst(path, _A_RDONLY|_A_SUBDIR, &fi)&&((fi.attrib&_A_SUBDIR)||isarchive(ext)))
    {
      _makepath(dp, drive, dir, name, ext);
      memcpy(mask, curmask, 12);
      return 1;
    }
    if (!*ext)
    {
      _makepath(path, drive, dir, name, ".*");
      if (!_dos_findfirst(path, _A_RDONLY, &fi)&&_dos_findnext(&fi))
      {
        _splitpath(fi.name, 0, 0, 0, ext);
        if (isarchive(ext))
        {
          _makepath(dp, drive, dir, name, ext);
          memcpy(mask, curmask, 12);
          return 1;
        }
      }
    }
  }

  char cmask[_MAX_NAME];
  conv12filenamewc(cmask, curmask);
  _makepath(dp, drive, dir, 0, 0);
  if (!*name)
    _splitpath(cmask, 0, 0, name, 0);
  if (!*ext)
    _splitpath(cmask, 0, 0, 0, ext);
  convfilename12wc(mask, name, ext);
  return 1;
}

static char fsScanDir(int pos)
{
  int op=0;
  switch (pos)
  {
  case 0: op=0; break;
  case 1: op=viewlist.pos; break;
  case 2: op=viewlist.pos?(viewlist.pos-1):0; break;
  }
  viewlist.remove(0, viewlist.num);

  if (!fsReadDir(viewlist, dmGetCurDir(), curmask, RD_PUTDSUBS|RD_PUTSUBS|(fsScanArcs?RD_ARCSCAN:0)))
    return 0;
  viewlist.sort();
  viewlist.pos=(op>=viewlist.num)?(viewlist.num-1):op;
  quickfindpos=0;
  scanpos=fsScanNames?0:0xFFFFFFFF;

  dmGetPath(curdirpath, dmGetCurDir());
  conv12filenamewc(curdirpath+strlen(curdirpath), curmask);

  adbUpdate();

  return 1;
}

static char fsEditPath(char *s)
{
  char path[_MAX_PATH];
  strcpy(path, s);
  char *p=path;

  unsigned char curpos=strlen(p);
  unsigned char cmdlen=strlen(p);

  char insmode=1;
  setcurshape(0xD0E);

  while (1)
  {
    displaystr(1, 0, 0x0F, p, plScrWidth);
    setcur(1, curpos);
    while (!ekbhit());
    while (ekbhit())
    {
      unsigned short key=egetch();
      if ((key&0xFF)==0xE0)
        key&=0xFF00;
      if (key&0xFF)
        key&=0x00FF;
      if ((key>=0x20)&&(key<=0xFF))
      {
        if (insmode)
        {
          if (cmdlen<79)
          {
            mymemmove(p+curpos+1, p+curpos, cmdlen-curpos+1);
            p[curpos]=key;
            curpos++;
            cmdlen++;
          }
        }
        else
          if (curpos==cmdlen)
          {
            if (cmdlen<79)
            {
              p[curpos++]=key;
              p[curpos]=0;
              cmdlen++;
            }
          }
          else
            p[curpos++]=key;
      }
      else
        switch (key)
        {
        case 0x4b00: //left
          if (curpos)
            curpos--;
          break;
        case 0x4d00: //right
          if (curpos<cmdlen)
            curpos++;
          break;
        case 0x4700: //home
          curpos=0;
          break;
        case 0x4F00: //end
          curpos=cmdlen;
          break;
        case 0x5200: //ins
          insmode=!insmode;
          setcurshape(insmode?0x0D0E:0x010E);
          break;
        case 0x5300: //del
          if (curpos!=cmdlen)
          {
            mymemmove(p+curpos, p+curpos+1, cmdlen-curpos);
            cmdlen--;
          }
          break;
        case 8:
          if (curpos)
          {
            mymemmove(p+curpos-1, p+curpos, cmdlen-curpos+1);
            curpos--;
            cmdlen--;
          }
          break;

        case 27:
          setcurshape(0x2000);
          return 0;
        case 13:
          setcurshape(0x2000);
          strcpy(s, path);
          return 1;
        }
    }
  }
}

static char fsEditViewPath()
{
  char path[_MAX_PATH];
  strcpy(path, curdirpath);
  char *p=path;
  if (fsEditPath(path))
  {
    unsigned short dref;
    char nmask[12];
    if (!fsExpandPath(curdirpath, nmask, path))
      return 0;
    dref=dmGetPathReference(curdirpath);
    if (dref==0xFFFF)
      return 0;
    dmChangeDir(dref);
    memcpy(curmask, nmask, 12);
    if (!fsScanDir(0))
      return 0;
  }
  return 1;
}


static void fsEditString(short y, short x, short l, char *s)
{
  char str[64];
  strcpy(str, s);
  str[l]=0;
  char *p=str;

  unsigned char curpos=strlen(p);
  unsigned char cmdlen=strlen(p);

  char insmode=1;
  setcurshape(0xD0E);

  while (1)
  {
    displaystr(y, x, 0x8F, p, l);
    setcur(y, x+curpos);
    while (!ekbhit());
    while (ekbhit())
    {
      unsigned short key=egetch();
      if ((key&0xFF)==0xE0)
        key&=0xFF00;
      if (key&0xFF)
        key&=0x00FF;
      if ((key>=0x20)&&(key<=0xFF))
      {
        if (insmode)
        {
          if (cmdlen<l)
          {
            mymemmove(p+curpos+1, p+curpos, cmdlen-curpos+1);
            p[curpos]=key;
            curpos++;
            cmdlen++;
          }
        }
        else
          if (curpos==cmdlen)
          {
            if (cmdlen<l)
            {
              p[curpos++]=key;
              p[curpos]=0;
              cmdlen++;
            }
          }
          else
            p[curpos++]=key;
      }
      else
        switch (key)
        {
        case 0x4b00: //left
          if (curpos)
            curpos--;
          break;
        case 0x4d00: //right
          if (curpos<cmdlen)
            curpos++;
          break;
        case 0x4700: //home
          curpos=0;
          break;
        case 0x4F00: //end
          curpos=cmdlen;
          break;
        case 0x5200: //ins
          insmode=!insmode;
          setcurshape(insmode?0x0D0E:0x010E);
          break;
        case 0x5300: //del
          if (curpos!=cmdlen)
          {
            mymemmove(p+curpos, p+curpos+1, cmdlen-curpos);
            cmdlen--;
          }
          break;
        case 8:
          if (curpos)
          {
            mymemmove(p+curpos-1, p+curpos, cmdlen-curpos+1);
            curpos--;
            cmdlen--;
          }
          break;

        case 27:
          setcurshape(0x2000);
          return;
        case 13:
          setcurshape(0x2000);
          strncpy(s, str, l);
          return;
        }
    }
  }
}

static void fsEditPlayTime(short y, short x, unsigned short &playtime)
{
  char str[7];
  convnum(playtime/60, str, 10, 3, 0);
  str[3]=':';
  convnum(playtime%60, str+4, 10, 2, 0);

  unsigned char curpos=(str[0]!='0')?0:(str[1]!='0')?1:2;

  setcurshape(0xD0E);

  while (1)
  {
    displaystr(y, x, 0x8F, str, 6);
    setcur(y, x+curpos);
    while (!ekbhit());
    while (ekbhit())
    {
      unsigned short key=egetch();
      if ((key&0xFF)==0xE0)
        key&=0xFF00;
      if (key&0xFF)
        key&=0x00FF;
      switch (key)
      {
      case ' ':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if (key==' ')
          key='0';
        if ((curpos==4)&&(key>'5'))
          break;
        if (curpos<6)
          str[curpos]=key;
      case 0x4d00: //right
        curpos="\x01\x02\x04\x05\x05\x06\x06"[curpos];
        break;
      case 8:
      case 0x4b00: //left
        curpos="\x00\x00\x01\x02\x02\x04\x05"[curpos];
        if (key==8)
          str[curpos]='0';
        break;
      case 27:
        setcurshape(0x2000);
        return;
      case 13:
        playtime=((((str[0]-'0')*10+str[1]-'0')*10+str[2]-'0')*6+str[4]-'0')*10+str[5]-'0';
        setcurshape(0x2000);
        return;
      }
    }
  }
}

static void fsEditDate(short y, short x, unsigned long &date)
{
  char str[11];
  convnum(date&0xFF, str, 10, 2, 0);
  str[2]='.';
  convnum((date>>8)&0xFF, str+3, 10, 2, 0);
  str[5]='.';
  convnum(date>>16, str+6, 10, 4, 0);

  unsigned char curpos=0;

  setcurshape(0xD0E);

  while (1)
  {
    displaystr(y, x, 0x8F, str, 10);
    setcur(y, x+curpos);
    while (!ekbhit());
    while (ekbhit())
    {
      unsigned short key=egetch();
      if ((key&0xFF)==0xE0)
        key&=0xFF00;
      if (key&0xFF)
        key&=0x00FF;
      switch (key)
      {
      case '\'':
        if (curpos==6)
        {
          str[6]=str[7]='0';
          curpos=8;
        }
        break;
      case ' ':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if (key==' ')
          key='0';
        if ((curpos==0)&&(key>='4'))
          break;
        if (curpos==0)
          str[1]='0';
        if ((curpos==1)&&(str[0]=='3')&&(key>'1'))
          break;
        if ((curpos==3)&&(key>'1'))
          break;
        if (curpos==3)
          str[4]='0';
        if ((curpos==4)&&(str[3]=='1')&&(key>'2'))
          break;
        if (curpos<10)
          str[curpos]=key;
      case 0x4d00: //right
        curpos="\x01\x03\x03\x04\x06\x06\x07\x08\x09\x0A\x0A"[curpos];
        break;
      case 8:
      case 0x4b00: //left
        curpos="\x00\x00\x01\x01\x03\x04\x04\x06\x07\x08\x09"[curpos];
        if (key==8)
          str[curpos]='0';
        break;
      case 27:
        setcurshape(0x2000);
        return;
      case 13:
        date=((str[0]-'0')*10+str[1]-'0')|(((str[3]-'0')*10+str[4]-'0')<<8)|(((((str[6]-'0')*10+str[7]-'0')*10+str[8]-'0')*10+str[9]-'0')<<16);
        setcurshape(0x2000);
        return;
      }
    }
  }
}

static void fsEditChan(short y, short x, unsigned char &chan)
{
  char str[3];
  convnum(chan, str, 10, 2, 0);

  unsigned char curpos=0;

  setcurshape(0xD0E);

  while (1)
  {
    displaystr(y, x, 0x8F, str, 2);
    setcur(y, x+curpos);
    while (!ekbhit());
    while (ekbhit())
    {
      unsigned short key=egetch();
      if ((key&0xFF)==0xE0)
        key&=0xFF00;
      if (key&0xFF)
        key&=0x00FF;
      switch (key)
      {
      case ' ':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if (key==' ')
          key='0';
        if ((curpos==0)&&(key>='4'))
          break;
        if (curpos==0)
          str[1]='0';
        if ((curpos==1)&&(str[0]=='3')&&(key>'2'))
          break;
        if (curpos<2)
          str[curpos]=key;
      case 0x4d00: //right
        curpos="\x01\x02\x02"[curpos];
        break;
      case 8:
      case 0x4b00: //left
        curpos="\x00\x00\x01"[curpos];
        if (key==8)
          str[curpos]='0';
        break;
      case 27:
        setcurshape(0x2000);
        return;
      case 13:
        chan=(str[0]-'0')*10+str[1]-'0';
        setcurshape(0x2000);
        return;
      }
    }
  }
}

static unsigned char fsEditFileInfo(unsigned short fileref)
{
  if (!mdbGetModuleInfo(mdbEditBuf, fileref))
    return 1;

  char typeidx[5];

  if (editpos==1)
    strcpy(typeidx, mdbGetModTypeString(mdbEditBuf.modtype));

  if (plScrWidth==132)
    switch (editpos)
    {
    case 0:
      fsEditString(plScrHeight-5, 42, 32, mdbEditBuf.modname);
      break;
    case 1:
      fsEditString(plScrHeight-5, 86, 4, typeidx);
      break;
    case 2:
      fsEditChan(plScrHeight-5, 105, mdbEditBuf.channels);
      break;
    case 3:
      fsEditPlayTime(plScrHeight-5, 123, mdbEditBuf.playtime);
      break;
    case 4:
      fsEditString(plScrHeight-4, 42, 32, mdbEditBuf.composer);
      break;
    case 5:
      fsEditString(plScrHeight-4, 86, 31, mdbEditBuf.style);
      break;
    case 6:
      fsEditDate(plScrHeight-3, 42, mdbEditBuf.date);
      break;
    case 7:
      fsEditString(plScrHeight-3, 66, 63, mdbEditBuf.comment);
      break;
    }
  else
    switch (editpos)
    {
    case 0:
      fsEditString(plScrHeight-6, 35, 32, mdbEditBuf.modname);
      break;
    case 1:
      fsEditString(plScrHeight-6, 75, 4, typeidx);
      break;
    case 2:
      fsEditChan(plScrHeight-4, 77, mdbEditBuf.channels);
      break;
    case 3:
      fsEditPlayTime(plScrHeight-4, 58, mdbEditBuf.playtime);
      break;
    case 4:
      fsEditString(plScrHeight-5, 13, 32, mdbEditBuf.composer);
      break;
    case 5:
      fsEditString(plScrHeight-4, 13, 31, mdbEditBuf.style);
      break;
    case 6:
      fsEditDate(plScrHeight-5, 58, mdbEditBuf.date);
      break;
    case 7:
      fsEditString(plScrHeight-3, 13, 63, mdbEditBuf.comment);
      break;
    }

  if (editpos==1)
  {
    typeidx[4]=0;
    mdbEditBuf.modtype=mdbReadModType(typeidx);
  }

  if (!mdbWriteModuleInfo(fileref, mdbEditBuf))
    return 0;
  return 1;
}

static unsigned char fsToggleModType(unsigned short)
{
  return 1;
}


static void fsSaveModInfo(const modlistentry &m)
{
  char path[_MAX_PATH];
  dmGetPath(path, m.dirref);
  if (isarchivepath(path))
    dmGetPath(path, dmGetParent(m.dirref));
  char n[_MAX_NAME];
  char fn[_MAX_FNAME];
  fsConv12FileName(n, m.name);
  _splitpath(n, 0, 0, fn, 0);
  _makepath(n, 0, 0, fn, MIF_EXT);
  strcat(path, n);
  short f=open(path, O_WRONLY|O_BINARY|O_TRUNC|O_CREAT, S_IREAD|S_IWRITE);
  if (f<0)
    return;
  write(f, "MODINFO1\r\n\r\n", 12);
  mifAppendInfo(f, m.fileref);
  close(f);
}

static void fsSaveModInfoML(const modlist &ml)
{
  char path[_MAX_PATH];
  dmGetPath(path, dmGetCurDir());
  if (!fsEditPath(path))
    return;
  char dr[_MAX_DRIVE];
  char di[_MAX_DIR];
  char fn[_MAX_FNAME];
  char ext[_MAX_EXT];
  _splitpath(path, dr, di, fn, ext);
  if (!*ext)
    strcpy(ext, MIF_EXT);
  _makepath(path, dr, di, fn, ext);
  short f=open(path, O_WRONLY|O_BINARY|O_TRUNC|O_CREAT, S_IREAD|S_IWRITE);
  if (f<0)
    return;
  write(f, "MODINFO1\r\n\r\n", 12);
  long i;
  modlistentry m;
  for (i=0; i<ml.num; i++)
  {
    ml.get(&m, i, 1);
    if (m.fileref<0xFFFC)
    {
      mifAppendInfo(f, m.fileref);
      write(f, "\r\n", 2);
    }
  }
  close(f);
}

static void fsSavePlayList(const modlist &ml)
{
  char path[_MAX_PATH];
  dmGetPath(path, dmGetCurDir());
  if (!fsEditPath(path))
    return;
  char dr[_MAX_DRIVE];
  char di[_MAX_DIR];
  char fn[_MAX_FNAME];
  char ext[_MAX_EXT];
  _splitpath(path, dr, di, fn, ext);
  if (!*ext)
    strcpy(ext, ".PLS");
  _makepath(path, dr, di, fn, ext);
  sbinfile f;
  if (f.open(path, sbinfile::opencr))
    return;
  f.write("[playlist]",10);
  f.write("\r\n",2);
  long i;
  _makepath(path, dr, di, 0, 0);
  unsigned short basepath=dmGetPathReference(path);
  if (basepath==0xFFFF)
    return;
  int relative=1;
  if (!relative)
    basepath=0xFFFF;
  modlistentry m;
  char numbuf[4];
  for (i=0; i<ml.num; i++)
  {
    utoa(i+1,numbuf,10);
    f.write("File",4);
    f.write(numbuf,strlen(numbuf));
    f.write("=",1);
    ml.get(&m, i, 1);
    if (m.fileref>=0xFFFC)
      continue;
    dmGetPathRel(path, m.dirref, basepath);
    fsConv12FileName(path+strlen(path), m.name);
    f.write(path, strlen(path));
    f.write("\r\n", 2);
  }
  utoa(ml.num,numbuf,10);
  f.write("NumberOfEntries=",16);
  f.write(numbuf,strlen(numbuf));
  f.write("\r\n", 2);
  f.close();
  fsScanDir(1);
}


#ifndef WIN32
static int movefile(const char *dest, const char *src)
{
  if (!rename(src, dest))
    return 1;
  sbinfile fi,fo;
  if (fi.open(src, sbinfile::openro)||fo.open(dest, sbinfile::opencn))
    return 0;
  int len=fi.length();
  int max=len;
  char *buf=0;
  while (!buf)
  {
    buf=new char [max];
    if (!buf)
      max>>=1;
    if (max<65536)
      break;
  }
  if (!buf)
    return 0;
  while (len)
  {
    int l=(len>max)?max:len;
    if (!fi.eread(buf, l))
      return 0;
    if (!fo.ewrite(buf, l))
      return 0;
    len-=l;
  }
  delete buf;
  fo.close();
  fi.close();
  unlink(src);

  return 1;
}

static int movetoarc(const char *dest, const char *src)
{
  char path[_MAX_PATH];
  char path2[_MAX_PATH];
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char name[_MAX_FNAME];
  char ext[_MAX_EXT];
  _splitpath(dest, drive, dir, name, ext);
  _makepath(path, drive, dir, 0, 0);
  _splitpath(src, drive, dir, 0, 0);
  _makepath(path2, drive, dir, name, ext);
  path[strlen(path)-1]=0;
  _splitpath(path, 0, 0, 0, ext);

  adbregstruct *packers;
  for (packers=adbPackers; packers; packers=packers->next)
    if (!stricmp(ext, packers->ext))
    {
      if (stricmp(src, path2))
        if (rename(src, path2))
          return 0;
      conRestore();
      int r=packers->Call(adbCallMoveTo, path, path2, "");
      conSave();
      plSetTextMode(fsScrType);
      return r;
    }

  return 0;
}

static int movefromarc(const char *dest, const char *src)
{
  char path[_MAX_PATH];
  char path2[_MAX_PATH];
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fname[_MAX_FNAME];
  char name[_MAX_NAME];
  char ext[_MAX_EXT];
  _splitpath(src, drive, dir, fname, ext);
  _makepath(name, 0, 0, fname, ext);
  _makepath(path, drive, dir, 0, 0);
  _splitpath(dest, drive, dir, 0, 0);
  _makepath(path2, drive, dir, 0, 0);
  path[strlen(path)-1]=0;
  _splitpath(path, 0, 0, 0, ext);

  adbregstruct *packers;
  for (packers=adbPackers; packers; packers=packers->next)
    if (!stricmp(ext, packers->ext))
    {
      conRestore();
      int r=packers->Call(adbCallMoveFrom, path, name, path2);
      conSave();
      plSetTextMode(fsScrType);
      if (!r)
        return 0;
      strcat(path2, name);
      if (stricmp(path2, dest))
        if (rename(path2, dest))
          return 0;
      return 1;
    }

  return 0;
}


static int movecrossarc(const char *dest, const char *src)
{
  char spath[_MAX_PATH];
  char dpath[_MAX_PATH];
  char path[_MAX_PATH];
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fname[_MAX_FNAME];
  char sname[_MAX_NAME];
  char dname[_MAX_NAME];
  char ext[_MAX_EXT];
  char sext[_MAX_EXT];
  char dext[_MAX_EXT];
  _splitpath(src, drive, dir, fname, ext);
  _makepath(sname, 0, 0, fname, ext);
  _makepath(spath, drive, dir, 0, 0);
  _splitpath(dest, drive, dir, fname, ext);
  _makepath(dname, 0, 0, fname, ext);
  _makepath(dpath, drive, dir, 0, 0);
  spath[strlen(spath)-1]=0;
  dpath[strlen(dpath)-1]=0;
  _splitpath(spath, 0, 0, 0, sext);
  _splitpath(dpath, 0, 0, 0, dext);

  adbregstruct *spackers;
  for (spackers=adbPackers; spackers; spackers=spackers->next)
    if (!stricmp(sext, spackers->ext))
      break;
  adbregstruct *dpackers;
  for (dpackers=adbPackers; dpackers; dpackers=dpackers->next)
    if (!stricmp(dext, dpackers->ext))
      break;
  if (!spackers||!dpackers)
    return 0;

  conRestore();
  if (!spackers->Call(adbCallMoveFrom, spath, sname, cfTempDir))
  {
    conSave();
    plSetTextMode(fsScrType);
    return 0;
  }
  strcpy(spath, cfTempDir);
  strcat(spath, sname);
  strcpy(path, cfTempDir);
  strcat(path, dname);
  if (stricmp(spath, path))
    rename(spath, path);
  int r=dpackers->Call(adbCallMoveTo, dpath, path, "");
  conSave();
  plSetTextMode(fsScrType);
  return r;
}

static int fsQueryMove(modlistentry &m)
{
  char path[_MAX_PATH];
  char path2[_MAX_PATH];
  int srctype=0;
  dmGetPath(path, m.dirref);
  if (isarchivepath(path))
    srctype=1;
  if (m.fileref==0xFFFC)
    srctype=2;

  char name[_MAX_NAME];
  fsConv12FileName(name, m.name);

  if (*fsDefMovePath)
    strcpy(path, fsDefMovePath);
  else
    dmGetPath(path, (m.fileref==0xFFFC)?dmGetParent(m.dirref):m.dirref);
  if (!fsEditPath(path))
    return 0;
  dmFullPath(path);
  if (!*path)
    return 0;

  char ext[_MAX_EXT];
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  find_t ft;
  if (path[strlen(path)-1]!='\\')
  {
    _splitpath(path, 0, 0, 0, ext);
    if (isarchive(ext)&&(srctype!=2))
      strcat(path, "\\");
    else
#ifdef DOS32
      if (!_dos_findfirst(path, _A_NORMAL|_A_SUBDIR, &ft))
        if (ft.attrib&_A_SUBDIR)
          strcat(path, "\\");
#else
      if (!_dos_findfirst(path, _A_SUBDIR, &ft))
        if (ft.attrib&_A_SUBDIR)
          strcat(path, "\\");
#endif
  }
  if (path[strlen(path)-1]=='\\')
    strcat(path, name);

  _splitpath(path, drive, dir, 0, 0);
  _makepath(path2, drive, dir, 0, 0);
  path2[strlen(path2)-1]=0;
  _splitpath(path2, 0, 0, 0, ext);
  int desttype=0;
  if (isarchive(ext))
#ifdef DOS32
    if (_dos_findfirst(path2, _A_NORMAL|_A_SUBDIR, &ft))
#else
    if (_dos_findfirst(path2, _A_NORMAL, &ft))
#endif
      desttype=1;
    else
      if (!(ft.attrib&_A_SUBDIR))
        desttype=1;
  if ((desttype==1)&&(srctype==2))
    return 0;

  dmGetPath(path2, (m.fileref==0xFFFC)?dmGetParent(m.dirref):m.dirref);
  strcat(path2, name);
  if ((desttype==0)&&(srctype!=1))
    return movefile(path, path2);
  if ((desttype==1)&&(srctype==0))
    return movetoarc(path, path2);
  if ((desttype==0)&&(srctype==1))
    return movefromarc(path, path2);
  if ((desttype==1)&&(srctype==1))
    return movecrossarc(path, path2);

  return 0;
}
#endif

static unsigned char fsQueryKill(modlistentry &m)
{
  displaystr(1, 0, 0xF0, "are you sure you want to delete this file?", 80);
  while (!ekbhit());
  if (toupper(egetch()&0xFF)!='Y')
    return 0;

  char path[_MAX_PATH];
  char name[_MAX_NAME];
  fsConv12FileName(name, m.name);
  dmGetPath(path, m.dirref);

  if ((m.fileref!=0xFFFC)&&isarchivepath(path))
  {
    char ext[_MAX_EXT];
    path[strlen(path)-1]=0;
    _splitpath(path, 0, 0, 0, ext);

    adbregstruct *packers;
    for (packers=adbPackers; packers; packers=packers->next)
      if (!strcmp(ext, packers->ext))
      {
        conRestore();
        packers->Call(adbCallDelete, path, name, "");
        conSave();
        plSetTextMode(fsScrType);
      }

    return 1;
  }
  else
  {
    if (m.fileref!=0xFFFC)
      strcat(path, name);
    else
      path[strlen(path)-1]=0;
    unlink(path);
    return 1;
  }
// remove from lists...
}

static void fsSetup()
{
  plSetTextMode(0);
  while (1)
  {
    displaystr(0, 0, 0x30, "  opencp      file selector setup            (c) '94-'98 Niklas Beisert et al.  ", 80);

    displaystr(1, 0, 0x07, "1:  screen mode: ", 17);
    displaystr(1, 17, 0x0F, (fsScrType&4)?"132x":" 80x", 4);
    displaystr(1, 21, 0x0F, ((fsScrType&3)==0)?"25":((fsScrType&3)==1)?"30":((fsScrType&3)==2)?"50":"60", 69);
    displaystr(2, 0, 0x07, "2:  scramble module list order: ", 32);
    displaystr(2, 32, 0x0F, fsListScramble?"on":"off", 48);
    displaystr(3, 0, 0x07, "3:  remove modules from playlist when played: ", 46);
    displaystr(3, 46, 0x0F, fsListRemove?"on":"off", 34);
    displaystr(4, 0, 0x07, "4:  loop modules: ", 18);
    displaystr(4, 18, 0x0F, fsLoopMods?"on":"off", 62);
    displaystr(5, 0, 0x07, "5:  scan module informatin: ", 28);
    displaystr(5, 28, 0x0F, fsScanNames?"on":"off", 52);
    displaystr(6, 0, 0x07, "6:  scan module information files: ", 35);
    displaystr(6, 35, 0x0F, fsScanMIF?"on":"off", 45);
    displaystr(7, 0, 0x07, "7:  scan archive contents: ", 27);
    displaystr(7, 27, 0x0F, fsScanArcs?"on":"off", 53);
    displaystr(8, 0, 0x07, "8:  scan module information in archives: ", 41);
    displaystr(8, 41, 0x0F, fsScanInArc?"on":"off", 39);
    displaystr(9, 0, 0x07, "9:  save module information to disk: ", 37);
    displaystr(9, 37, 0x0F, fsWriteModInfo?"on":"off", 42);
    displaystr(10, 0, 0x07, "A:  edit window: ", 17);
    displaystr(10, 17, 0x0F, fsEditWin?"on":"off", 63);
    displaystr(11, 0, 0x07, "B:  module type colors: ", 24);
    displaystr(11, 24, 0x0F, fsColorTypes?"on":"off", 56);
    displaystr(12, 0, 0x07, "C:  module information display mode: ", 37);
    displaystr(12, 37, 0x0F, (fsInfoMode==0)?"0":(fsInfoMode==1)?"1":(fsInfoMode==2)?"2":"3", 43);
    displaystr(13, 0, 0x07, "D:  put archives: ", 18);
    displaystr(13, 18, 0x0F, fsPutArcs?"on":"off", 43);
    displaystr(24, 0, 0x17, "  press the number of the item you wish to change and esc when done", 80);

    while (!kbhit());

    unsigned short c=egetch();
    if ((c&0xFF)==0xE0)
      c&=0xFF00;
    if (c&0xFF)
      c&=0x00FF;

    switch (c)
    {
    case '1': fsScrType=(fsScrType+1)&7; break;
    case '2': fsListScramble=!fsListScramble; break;
    case '3': fsListRemove=!fsListRemove; break;
    case '4': fsLoopMods=!fsLoopMods; break;
    case '5': fsScanNames=!fsScanNames; break;
    case '6': fsScanMIF=!fsScanMIF; break;
    case '7': fsScanArcs=!fsScanArcs; break;
    case '8': fsScanInArc=!fsScanInArc; break;
    case '9': fsWriteModInfo=!fsWriteModInfo; break;
    case 'a': case 'A': fsEditWin=!fsEditWin; break;
    case 'b': case 'B': fsColorTypes=!fsColorTypes; break;
    case 'c': case 'C': fsInfoMode=(fsInfoMode+1)&3; break;
    case 'd': case 'D': fsPutArcs=!fsPutArcs; break;
    case 27:
      return;
    }
  }
}


static void adbGetFile(char *path, binfile *&fi)
{
  if (fi)
    return;
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char name[_MAX_NAME];
  char ext[_MAX_EXT];
  _splitpath(path, drive, dir, name, ext);
  strcat(name, ext);
  _makepath(path, drive, dir, 0, 0);

  if (!isarchivepath(path))
    return;

  path[strlen(path)-1]=0;
  _splitpath(path, 0, 0, 0, ext);

  adbregstruct *packers;
  for (packers=adbPackers; packers; packers=packers->next)
    if (!strcmp(ext, packers->ext))
      packers->Call(adbCallGet, path, name, cfTempDir);

  strcpy(path, cfTempDir);
  strcat(path, name);

  delbinfile *dfi=new delbinfile;
  if (dfi&&dfi->open(path, delbinfile::openro))
  {
    delete dfi;
    dfi=0;
    return;
  }
  fi=dfi;
}

static void dosGetFile(char *path, binfile *&fi)
{
  if (fi)
    return;
  sbinfile *fis=new sbinfile;
  if (fis&&fis->open(path, sbinfile::openro))
  {
    delete fis;
    fi=0;
    return;
  }
  fi=fis;
}


static fsgetfileregstruct *fsGetFiles;

static void fsRegisterGetFile(fsgetfileregstruct *r)
{
  r->next=fsGetFiles;
  fsGetFiles=r;
}

char fsGetNextFile(char *path, moduleinfostruct &info, binfile *&fi)
{
  modlistentry m;
  if (isnextplay)
  {
    m=nextplay;
    isnextplay=0;
  }
  else
  {
    if (!playlist.num)
      return 0;
    unsigned long pick=0;
    if (fsListScramble)
      pick=rand()%playlist.num;
    playlist.get(&m, pick, 1);
    playlist.remove(pick, 1);
    if (!fsListRemove)
      playlist.insert(playlist.num, &m, 1);
  }

  mdbGetModuleInfo(info, m.fileref);

  char name[_MAX_NAME];
  fsConv12FileName(name, m.name);
  dmGetPath(path, m.dirref);
  strcat(path, name);

  fi=0;
  if (!(info.flags1&MDB_VIRTUAL))
  {
    fsgetfileregstruct *getfiles;
    for (getfiles=fsGetFiles; getfiles; getfiles=getfiles->next)
      getfiles->GetFile(path, fi);
  }

  if (!mdbInfoRead(m.fileref)&&fi)
  {
    mdbReadInfo(info, *fi);
    fi->seek(0);
    mdbWriteModuleInfo(m.fileref, info);
    mdbGetModuleInfo(info, m.fileref);
  }

  return 1;
}

char fsFilesLeft()
{
  return isnextplay||playlist.num;
}

signed char fsFileSelect()
{
  plSetTextMode(fsScrType);

  isnextplay=0;

  short win=0;

  quickfindpos=0;
  long i;
  char curscanned=0;

  modlistentry m;
  while (1)
  {
    dirwinheight=plScrHeight-4;
    if (fsEditWin||editmode)
      dirwinheight-=(plScrWidth==132)?5:6;

    if (!playlist.num)
      win=0;

    if (playlist.pos>=playlist.num)
      playlist.pos=playlist.num-1;
    if (playlist.pos<0)
      playlist.pos=0;

    if (viewlist.pos>=viewlist.num)
      viewlist.pos=viewlist.num-1;
    if (viewlist.pos<0)
      viewlist.pos=0;

    short firstv=viewlist.pos-dirwinheight/2;

    if ((firstv+dirwinheight)>viewlist.num)
      firstv=viewlist.num-dirwinheight;
    if (firstv<0)
      firstv=0;

    short firstp=playlist.pos-dirwinheight/2;

    if ((firstp+dirwinheight)>playlist.num)
      firstp=playlist.num-dirwinheight;
    if (firstp<0)
      firstp=0;

    if (!win)
      viewlist.getcur(m);
    else
      playlist.getcur(m);

    fsShowDir(firstv, win?-1:viewlist.pos, firstp, win?playlist.pos:-1, editmode?editpos:-1, m);

    if (!ekbhit()&&fsScanNames)
    {
      if (curscanned||(m.fileref>=0xFFFC)||mdbInfoRead(m.fileref))
      {
        while (scanpos<viewlist.num)
        {
          viewlist.get(&m, scanpos++, 1);
          if ((m.fileref<0xFFFC)&&!mdbInfoRead(m.fileref))
          {
            mdbScan(m);
            break;
          }
        }
      }
      else
      {
        curscanned=1;
        mdbScan(m);
      }
     continue;
    }
    unsigned short c=egetch();
    curscanned=0;
    if ((c&0xFF)==0xE0)
      c&=0xFF00;
    if (c&0xFF)
      c&=0x00FF;

    if ((c>32)&&(c<=255)&&(c!=0x7f)||(c==8))
    {
      if (c==8)
      {
        if (quickfindpos)
          quickfindpos--;
        if ((quickfindpos==8)&&(quickfind[8]=='.'))
          while (quickfindpos&&(quickfind[quickfindpos-1]==' '))
            quickfindpos--;
      }
      else
        if (quickfindpos<12)
          if ((c=='.')&&(quickfindpos&&(*quickfind!='.')))
          {
            while (quickfindpos<9)
              quickfind[quickfindpos++]=' ';
            quickfind[8]='.';
          }
          else
            if (quickfindpos!=8)
              quickfind[quickfindpos++]=toupper(c);
      memcpy(quickfind+quickfindpos, "        .   "+quickfindpos, 12-quickfindpos);
      if (!quickfindpos)
        continue;
      if (!win)
        viewlist.pos=viewlist.fuzzyfind(quickfind);
      else
        playlist.pos=playlist.fuzzyfind(quickfind);
      continue;
    }

    quickfindpos=0;

    modlist &curlist=(win?playlist:viewlist);
    curlist.getcur(m);

    switch (c)
    {
    case 27:
      return 0;
    case 0x7f:  //c-bs
    case 0x1f00: // alt-s
      scanpos=0xFFFFFFFF;
      break;
    case 9:
      win=!win;
      break;
    case 0xF00:
    case 0x1200:
      editmode=!editmode;
      break;
    case 0x1700:
    case 0xa500:
      fsInfoMode=(fsInfoMode+1)&3;
      break;
    case 0x2E00:
      fsSetup();
      plSetTextMode(fsScrType);
      break;
    case 0x3b00:
      if (!fsHelp2())
        return -1;
      plSetTextMode(fsScrType);
      break;
    case 0x2C00:
      fsScrType=(fsScrType==0)?7:0;
      plSetTextMode(fsScrType);
      break;
    case 10:
      if (!fsEditViewPath())
        return -1;
      break;
    case 32:
      if (editmode&&(editpos==1))
        if (m.fileref<0xFFFC)
        {
          if (!fsToggleModType(m.fileref))
            return -1;
          break;
        }
      break;
    case 13:
      if (editmode)
        if (m.fileref<0xFFFC)
        {
          if (!fsEditFileInfo(m.fileref))
            return -1;
          break;
        }
      if (win)
      {
        nextplay=m;
        isnextplay=1;
        return 1;
      }
      else
      {
        if (m.fileref<0xFFFC)
        {
          nextplay=m;
          isnextplay=1;
          return 1;
        }
        else
        {
          unsigned short parentdir=0xFFFF;
          if (!memcmp(m.name, "..", 2))
            parentdir=dmGetCurDir();
          if (dmChangeDir(m.dirref)==0xFFFF)
            return -1;
          if (!fsScanDir(0))
            return -1;
          if (parentdir!=0xFFFF)
          {
            for (i=viewlist.num-1; i>=0; i--)
            {
              viewlist.get(&m, i, 1);
              if ((m.fileref<0xFFFC)||(m.dirref!=parentdir))
                continue;
              if (memcmp(m.name, "..", 2)&&(m.name[1]!=':'))
              {
                viewlist.pos=i;
                break;
              }
            }
          }
        }
      }
      break;
    case 0x4800: // up
      if (editmode)
        if (plScrWidth==132)
          editpos="\x00\x01\x02\x03\x00\x01\x04\x05"[editpos];
        else
          editpos="\x00\x01\x06\x06\x00\x04\x00\x05"[editpos];
      else
        curlist.pos--;
      break;
    case 0x5000: // down
      if (editmode)
        if (plScrWidth==132)
          editpos="\x04\x05\x05\x05\x06\x07\x06\x07"[editpos];
        else
          editpos="\x04\x06\x07\x07\x05\x07\x03\x07"[editpos];
      else
        curlist.pos++;
      break;
    case 0x4900: //pgup
      curlist.pos-=editmode?1:dirwinheight;
      break;
    case 0x5100: //pgdn
      curlist.pos+=editmode?1:dirwinheight;
      break;
    case 0x4700: //home
      if (editmode)
        break;
      curlist.pos=0;
      break;
    case 0x4F00: //end
      if (editmode)
        break;
      curlist.pos=curlist.num-1;
      break;

    case 0x4D00: // right
      if (editmode)
      {
        if (plScrWidth==132)
          editpos="\x01\x02\x03\x03\x05\x05\x07\x07"[editpos];
        else
          editpos="\x01\x01\x02\x02\x06\x03\x06\x07"[editpos];
      }
    case 0x5200: // add
      if (editmode)
        break;
      if (win)
      {
        if (!playlist.append(m))
          return -1;
//        playlist.pos=playlist.num-1;
      }
      else
      {
        if (m.fileref==0xFFFC)
        {
          if (!fsReadDir(playlist, m.dirref, curmask, 0))
            return -1;
        }
        else
          if (m.fileref<0xFFFC)
            if (!playlist.append(m))
              return -1;
      }
      break;

    case 0x4B00: // left
      if (editmode)
      {
        if (plScrWidth==132)
          editpos="\x00\x00\x01\x02\x04\x04\x06\x06"[editpos];
        else
          editpos="\x00\x00\x03\x05\x04\x05\x04\x07"[editpos];
      }
    case 0x5300: // del
      if (editmode)
        break;
      if (win)
        playlist.remove(playlist.pos, 1);
      else
      {
        long f;
        if (m.fileref<0xFFFC)
        {
          f=playlist.find(m);
          if (f!=-1)
            playlist.remove(f, 1);
        }
        else
        if (m.fileref==0xFFFC)
        {
          modlist tl;
          if (!fsReadDir(tl, m.dirref, curmask, 0))
            return -1;
          for (i=0; i<tl.num; i++)
          {
            tl.get(&m, i, 1);
            f=playlist.find(m);
            if (f!=-1)
              playlist.remove(f, 1);
          }
        }
      }
      break;

    case 0x9200:
    case 0x7400:
      if (editmode)
        break;
      for (i=0; i<viewlist.num; i++)
      {
        viewlist.get(&m, i, 1);
        if (m.fileref<0xFFFC)
          if (!playlist.append(m))
            return -1;
      }
      break;

    case 0x9300:
    case 0x7300:
      if (editmode)
        break;
      playlist.remove(0, playlist.num);
      break;

    case 0x2500:  // alt-k
      if (editmode||win)
        break;
      if (m.fileref<=0xFFFC)
        if (fsQueryKill(m))
          if (!fsScanDir(2))
            return -1;
      break;


#ifndef WIN32
    case 0x3200:  // alt-m !!!!!!!! STRANGE THINGS HAPPENS IF YOU ENABLE HIS UNDER W32!!
      if (editmode||win)
        break;
      if (m.fileref<=0xFFFC)
        if (fsQueryMove(m))
          if (!fsScanDir(1))
            return -1;
      break;
#endif

    case 0x3000: // alt-b
      if (m.fileref<=0xFFFC)
      {
        mdbGetModuleInfo(mdbEditBuf, m.fileref);
        mdbEditBuf.flags1^=MDB_BIGMODULE;
        mdbWriteModuleInfo(m.fileref, mdbEditBuf);
      }
      break;

    case 0x1900: // alt-p
      if (editmode)
        break;
      fsSavePlayList(playlist);
      break;

    case 0x1100: // alt-w
      if (m.fileref<0xFFFC)
        fsSaveModInfo(m);
      break;
    case 0x1e00: // alt-a
      if (editmode)
        break;
      fsSaveModInfoML(curlist);
      break;

    case 0x8d00: //ctrl-up
      if (editmode||!win)
        break;
      if (!playlist.pos)
        break;
      playlist.remove(playlist.pos, 1);
      playlist.insert(playlist.pos-1, &m, 1);
      playlist.pos-=2;
      break;
    case 0x9100: //ctrl-down
      if (editmode||!win)
        break;
      if ((playlist.pos+1)>=playlist.num)
        break;
      playlist.remove(playlist.pos, 1);
      playlist.insert(playlist.pos+1, &m, 1);
      playlist.pos++;
      break;
    case 0x8400: //ctrl-pgup
      if (editmode||!win)
        break;
      i=(playlist.pos>dirwinheight)?dirwinheight:playlist.pos;
      playlist.remove(playlist.pos, 1);
      playlist.insert(playlist.pos-i, &m, 1);
      playlist.pos-=i+1;
      break;
    case 0x7600: //ctrl-pgdown
      if (editmode||!win)
        break;
      i=((playlist.num-1-playlist.pos)>dirwinheight)?dirwinheight:(playlist.num-1-playlist.pos);
      playlist.remove(playlist.pos, 1);
      playlist.insert(playlist.pos+i, &m, 1);
      playlist.pos+=i;
      break;
    case 0x7700: //ctrl-home
      if (editmode||!win)
        break;
      playlist.remove(playlist.pos, 1);
      playlist.insert(0, &m, 1);
      playlist.pos=0;
      break;
    case 0x7500: //ctrl-end
      if (editmode||!win)
        break;
      playlist.remove(playlist.pos, 1);
      playlist.insert(playlist.num, &m, 1);
      playlist.pos=playlist.num-1;
      break;
    }
  }
  //return 0; // the above while loop doesn't go to this point
}

char fsAddFiles(const char *p)
{
  while (*p)
  {
    while (isspace(*p))
      p++;

    int i;
    char path[_MAX_PATH];

    for (i=0; (i<(_MAX_PATH-1))&&!isspace(*p)&&*p; i++)
      path[i]=*p++;
    path[i]=0;
    if ((*path=='-')||(*path=='/')||!*path)
      continue;
    if (*path=='@')
    {
      unsigned short olddir=dmGetCurDir();

      char nam[_MAX_FNAME];
      char ext[_MAX_EXT];
      char dir[_MAX_DIR];
      char drv[_MAX_DRIVE];

      _splitpath(path+1, drv, dir, nam, ext);
      _makepath(path, drv, dir, 0, 0);
      unsigned short dref=dmGetPathReference(path);
      if (dref==0xFFFF)
        return 0;
      dmGetPath(path, dref);
      dmChangeDir(dref);
      if (!*ext)
        strcpy(ext, ".M3U");
      _makepath(path+strlen(path), 0, 0, nam, ext);

      sbinfile lf;
      if (!lf.open(path, sbinfile::openro))
      {
        int len=lf.length();
        char *fbuf=new char [len+1];
        if (!fbuf)
          return 0;
        lf.read(fbuf, len);
        fbuf[len]=0;
        lf.close();
        len=fsAddFiles(fbuf);
        if (!len)
          return 0;
        delete fbuf;
      }
      dmChangeDir(olddir);
    }
    else
    {
      char path2[_MAX_PATH];
      unsigned short dref;
      char nmask[12];
      if (!fsExpandPath(path2, nmask, (*path=='@')?(path+1):path))
        return 0;
      dref=dmGetPathReference(path2);
      if (dref==0xFFFF)
        return 0;

      if (!fsReadDir(playlist, dref, nmask, (fsScanArcs?RD_ARCSCAN:0)))
        return 0;
    }
  }
  return 1;
}




//*************************************************************************
//init


char fsInit()
{
  int i;

  const char *sec=cfGetProfileString(cfConfigSec, "fileselsec", "fileselector");

  char regname[50];
  const char *regs=lnkReadInfoReg("arcs");

  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
      adbRegister((adbregstruct*)reg);
  }

  regs=lnkReadInfoReg("readdirs");
  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
      mdbRegisterReadDir((mdbreaddirregstruct*)reg);
  }

  fsDefMovePath=cfGetProfileString2(sec, "fileselector", "movepath", "");

  char *glist=new char[1024];
  *glist=0;
  regs=lnkReadInfoReg("pregetfile");
  strcat(glist,regs);
  regs=lnkReadInfoReg("getfile");
  strcat(glist," ");
  strcat(glist,regs);
  regs=lnkReadInfoReg("postgetfile");
  strcat(glist," ");
  strcat(glist,regs);

  const char *gl=glist;

  while (cfGetSpaceListEntry(regname, gl, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
    {
      fsRegisterGetFile((fsgetfileregstruct*)reg);
    }
  }

  delete glist;

  regs=lnkReadInfoReg("readinfos");
  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
    {
      mdbRegisterReadInfo((mdbreadnforegstruct*)reg);
    }
  }


  for (i=0; i<256; i++)
  {
    char secname[20];
    strcpy(secname, "filetype ");
    ultoa(i, secname+strlen(secname), 10);

    fsTypeCols[i]=cfGetProfileInt(secname, "color", 7, 10);
    fsTypeNames[i]=cfGetProfileString(secname, "name", "");
  }


  dmTree=0;
  dmReloc=0;
  dmMaxNodes=0;
  dmNumNodes=0;

  const char *modexts=cfGetProfileString2(sec, "fileselector", "modextensions", "MOD XM S3M MID MTM DMF ULT 669 NST WOW OKT PTM AMS MDL");
  int extnum=cfCountSpaceList(modexts, 3);
  moduleextensions=new char *[extnum+1];
  if (!moduleextensions)
    return 0;
  if (extnum)
    *moduleextensions=new char [extnum*4];

  for (i=0; i<extnum; i++)
  {
    moduleextensions[i]=*moduleextensions+i*4;
    cfGetSpaceListEntry(moduleextensions[i], modexts, 3);
    strupr(moduleextensions[i]);
  }
  moduleextensions[i]=0;

  if (!adbInit())
    return 0;

  if (!mdbInit())
    return 0;

  if (!mifInit())
    return 0;

  for (i=0; i<27; i++)
    dmDriveDirs[i]=0xFFE0+i;

  unsigned int curdrive;
  _dos_getdrive(&curdrive);
  dmCurDrive=curdrive;
  memcpy(curmask, "????????.???", 12);

  char path[_MAX_PATH];
  char nmask[12];
  if (!fsExpandPath(path, nmask, cfGetProfileString2(sec, "fileselector", "path", "")))
    return 0;
  unsigned short pt=dmGetPathReference(path);
  if (pt==0xFFFF)
    return 0;
  dmChangeDir(pt);

  if (!fsScanDir(0))
    return 0;

  fsScrType=cfGetProfileInt2(sec, "fileselector", "screentype", 0, 10)&7;
  fsColorTypes=cfGetProfileBool2(sec, "fileselector", "typecolors", 1, 1);
  fsEditWin=cfGetProfileBool2(sec, "fileselector", "editwin", 1, 1);
  fsWriteModInfo=cfGetProfileBool2(sec, "fileselector", "writeinfo", 1, 1);
  fsScanMIF=cfGetProfileBool2(sec, "fileselector", "scanmdz", 1, 1);
  fsScanInArc=cfGetProfileBool2(sec, "fileselector", "scaninarcs", 1, 1);
  fsScanNames=cfGetProfileBool2(sec, "fileselector", "scanmodinfo", 1, 1);
  fsScanArcs=cfGetProfileBool2(sec, "fileselector", "scanarchives", 1, 1);
  fsListRemove=cfGetProfileBool2(sec, "fileselector", "playonce", 1, 1);
  fsListScramble=cfGetProfileBool2(sec, "fileselector", "randomplay", 1, 1);
  fsPutArcs=cfGetProfileBool2(sec, "fileselector", "putarchives", 1, 1);
  fsLoopMods=cfGetProfileBool2(sec, "fileselector", "loop", 1, 1);
  fsListRemove=cfGetProfileBool("commandline_f", "r", fsListRemove, 0);
  fsListScramble=!cfGetProfileBool("commandline_f", "o", !fsListScramble, 1);
  fsLoopMods=cfGetProfileBool("commandline_f", "l", fsLoopMods, 0);
  return 1;
}

void fsClose()
{
  adbPackers=0;
  adbClose();
  mdbClose();
  mifClose();
  delete dmTree;
  delete dmReloc;
  if (moduleextensions)
    delete *moduleextensions;
  delete moduleextensions;
  delete playlist.files;
  playlist.files=0;
  delete viewlist.files;
  viewlist.files=0;
}

extern "C"
{
  mdbreaddirregstruct fsReadDirReg = {stdReadDir};
  mdbreaddirregstruct adbReadDirReg = {arcReadDir};
  mdbreaddirregstruct dosReadDirReg = {dosReadDir};
  fsgetfileregstruct adbGetFileReg = {adbGetFile};
  fsgetfileregstruct dosGetFileReg = {dosGetFile};
}
