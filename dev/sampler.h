#ifndef __SAMPLER_H
#define __SAMPLER_H

#ifdef WIN32
#include "w32idata.h"
#endif

#define SMP_STEREO 1
#define SMP_16BIT 2
#define SMP_SIGNEDOUT 4
#define SMP_REVERSESTEREO 8

#define SMP_MIC 0
#define SMP_LINEIN 1
#define SMP_CD 2

enum
{
  smpGetSampleStereo=1
};


#if defined(DOS32) || (defined(WIN32)&&defined(NO_SMPBASE_IMPORT))
extern int smpRate;
extern int smpOpt;
extern int smpBufSize;
extern int (*smpSample)(void *buf, int &len);
extern void (*smpStop)();
extern void (*smpSetOptions)(int rate, int opt);
extern void (*smpSetSource)(int src);
extern int (*smpGetBufPos)();
#else
extern_data int smpRate;
extern_data int smpOpt;
extern_data int smpBufSize;
extern_data int (*smpSample)(void *buf, int &len);
extern_data void (*smpStop)();
extern_data void (*smpSetOptions)(int rate, int opt);
extern_data void (*smpSetSource)(int src);
extern_data int (*smpGetBufPos)();
#endif

int smpOpenSampler(void *&buf, int &len, int blen);
void smpCloseSampler();
void smpGetRealMasterVolume(int &l, int &r);
void smpGetMasterSample(short *s, int len, int rate, int opt);

#endif
