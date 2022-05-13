#ifndef __DEVIPLAY_H
#define __DEVIPLAY_H

#ifdef WIN32
#include "w32idata.h"
#endif

void plrSetDevice(const char *name, int def);

#if defined(DOS32) || (defined(WIN32)&&defined(NO_PLRBASE_IMPORT))

struct devinfonode;
extern devinfonode *plPlayerDevices;
extern int (*plrProcessKey)(unsigned short);
extern int plrBufSize;

#else

struct devinfonode;
extern_data devinfonode *plPlayerDevices;
extern_data int (*plrProcessKey)(unsigned short);
extern_data int plrBufSize;
#endif

#endif
