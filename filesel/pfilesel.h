#ifndef __PFILESEL_H
#define __PFILESEL_H

#ifdef WIN32
#include "w32idata.h"
#endif

class binfile;

struct moduleinfostruct
{
#define MDB_USED 1
#define MDB_DIRTY 2
#define MDB_BLOCKTYPE 12
#define MDB_VIRTUAL 16
#define MDB_BIGMODULE 32
  unsigned char flags1;
#define MDB_GENERAL 0
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
  unsigned char flags2;
#define MDB_COMPOSER 4
  char composer[32];
  char style[31];
  unsigned char flags3;
#define MDB_COMMENT 8
  char comment[63];
  unsigned char flags4;
#define MDB_FUTURE 12
  char dum[63];
};

enum
{
  mtMOD=0, mtMODd=1, mtMODt=2, mtM31=3, mtM15=6, mtM15t=7, mtWOW=8,
  mtS3M=9, mtXM=10, mtMTM=11, mt669=12, mtULT=13, mtDMF=14, mtOKT=15,
  mtMID=16, mtCDA=17, mtMIDd=18, mtPTM=19, mtMED=20, mtMDL=21, mtAMS=22,
  mtINP=23, mtDEVp=24, mtDEVs=25, mtDEVw=26, mtIT=27, mtWAV=28, mtVOC=29,
  mtMPx=30, mtSID=31, mtMXM=32, mtMODf=33, mtWMA=34,
  mtPLS=128, mtM3U=129, mtANI=130,
  mtUnRead=0xFF
};

char fsGetNextFile(char *, moduleinfostruct &info, binfile *&fi);
char fsFilesLeft();
signed char fsFileSelect();
char fsAddFiles(const char *);
char fsInit();
void fsClose();

struct arcentry
{
#define ADB_USED 1
#define ADB_DIRTY 2
#define ADB_ARC 4
  unsigned short flags;
  unsigned short parent;
  char name[12];
  unsigned long size;
};

enum
{
  adbCallGet, adbCallDelete, adbCallMoveTo, adbCallMoveFrom, adbCallPut
};

struct adbregstruct
{
  const char *ext;
  int (*Scan)(const char *path);
  int (*Call)(int act, const char *apath, const char *file, const char *dpath);
  adbregstruct *next;
};

struct modlistentry
{
  char name[12];
  unsigned short dirref;
  unsigned short fileref;
};

struct modlist;

struct mdbreaddirregstruct
{
  int (*ReadDir)(modlist &ml, unsigned short dirref, const char *mask, unsigned long opt);
  mdbreaddirregstruct *next;
};
#define RD_PUTSUBS 1
#define RD_ARCSCAN 2
#define RD_DIRRECURSE 4
#define RD_PUTDSUBS 16

char *dmGetPath(char *path, unsigned short ref);
unsigned short dmGetPathReference(const char *p);
unsigned short dmGetDriveDir(int drv);

int adbAdd(const arcentry &a);
unsigned short adbFind(const char *arcname);
int adbCallArc(const char *cmd, const char *arc, const char *name, const char *dir);
int fsIsModule(const char *ext);
void fsConvFileName12(char *c, const char *f, const char *e);
int fsMatchFileName12(const char *a, const char *b);
void fsConv12FileName(char *f, const char *c);

extern char fsScanInArc;
unsigned short mdbGetModuleReference(const char *name, unsigned long size);
int mdbGetModuleInfo(moduleinfostruct &m, unsigned short fileref);
int mdbWriteModuleInfo(unsigned short fileref, moduleinfostruct &m);
int mdbInfoRead(unsigned short fileref);
int mdbGetModuleType(unsigned short fileref);
int mdbReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int len);
int mdbAppend(modlist &m, const modlistentry &f);
int mdbAppendNew(modlist &m, const modlistentry &f);
char mifMemRead(const char *name, unsigned short size, char *ptr);

#define MIF_EXT ".MDZ"

struct mdbreadnforegstruct
{
  int (*ReadMemInfo)(moduleinfostruct &m, const unsigned char *buf, int len);
  int (*ReadInfo)(moduleinfostruct &m, binfile &f, const unsigned char *buf, int len);
  mdbreadnforegstruct *next;
};

struct interfacestruct
{
  int (*Init)(const char *path, moduleinfostruct &info, binfile *f);
  int (*Run)();
  void (*Close)();
};

struct preprocregstruct
{
  void (*Preprocess)(const char *path, moduleinfostruct &info, binfile *&f);
  preprocregstruct *next;
};

struct fsgetfileregstruct
{
  void (*GetFile)(char *path, binfile *&f);
  fsgetfileregstruct *next;
};

struct filehandlerstruct
{
  int (*Process)(const char *path, moduleinfostruct &m, binfile *&f);
};

#if defined(DOS32) || (defined(WIN32)&&defined(NO_PFILESEL_IMPORT))
extern char fsLoopMods;
#else
extern_data char fsLoopMods;
#endif

#endif
