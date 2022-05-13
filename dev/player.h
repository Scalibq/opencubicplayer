#ifndef __PLAYER_H
#define __PLAYER_H

#ifdef WIN32
#include "w32idata.h"
#endif

#define PLR_STEREO 1
#define PLR_16BIT 2
#define PLR_SIGNEDOUT 4
#define PLR_REVERSESTEREO 8
#define PLR_RESTRICTED 16

enum
{
  plrGetSampleStereo=1
};

#if defined(DOS32) || (defined(WIN32)&&defined(NO_PLRBASE_IMPORT))

extern int plrRate;
extern int plrOpt;
extern int (*plrPlay)(void *&buf, int &len);
extern void (*plrStop)();
extern void (*plrSetOptions)(int rate, int opt);
extern int (*plrGetBufPos)();
extern int (*plrGetPlayPos)();
extern void (*plrAdvanceTo)(int pos);
extern long (*plrGetTimer)();
extern void (*plrIdle)();
#else
extern_data int plrRate;
extern_data int plrOpt;
extern_data int (*plrPlay)(void *&buf, int &len);
extern_data void (*plrStop)();
extern_data void (*plrSetOptions)(int rate, int opt);
extern_data int (*plrGetBufPos)();
extern_data int (*plrGetPlayPos)();
extern_data void (*plrAdvanceTo)(int pos);
extern_data long (*plrGetTimer)();
extern_data void (*plrIdle)();
#endif

int plrOpenPlayer(void *&buf, int &len, int blen);
void plrClosePlayer();
void plrGetRealMasterVolume(int &l, int &r);
void plrGetMasterSample(short *s, int len, int rate, int opt);

extern "C" void plr16to8(unsigned char *, const unsigned short *, unsigned long);
#pragma aux plr16to8 parm [edi] [esi] [ecx] modify [eax]
extern "C" void plrClearBuf(void *buf, int len, int unsign);
#pragma aux plrClearBuf parm [edi] [ecx] [eax]

#endif
