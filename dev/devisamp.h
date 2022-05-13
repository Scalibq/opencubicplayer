#ifndef __DEVISAMP_H
#define __DEVISAMP_H

#ifdef WIN32
#include "w32idata.h"
#endif

void smpSetDevice(const char *name, int def);

#if defined(DOS32) || (defined(WIN32)&&defined(NO_SMPBASE_IMPORT))

struct devinfonode;
extern devinfonode *plSamplerDevices;
extern int (*smpProcessKey)(unsigned short);

extern unsigned char plsmpOpt;
extern unsigned short plsmpRate;

#else

struct devinfonode;
extern_data devinfonode *plSamplerDevices;
extern_data int (*smpProcessKey)(unsigned short);

extern_data unsigned char plsmpOpt;
extern_data unsigned short plsmpRate;
#endif

#endif
