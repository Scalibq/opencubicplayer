#ifndef __DEVIWAVE_H
#define __DEVIWAVE_H

#ifdef WIN32
#include "w32idata.h"
#endif

void mcpSetDevice(const char *name, int def);

#if defined(DOS32) || (defined(WIN32)&&defined(NO_MCPBASE_IMPORT))
struct devinfonode;
extern devinfonode *plWaveTableDevices;
extern int (*mcpProcessKey)(unsigned short);
#else
struct devinfonode;
extern_data devinfonode *plWaveTableDevices;
extern_data int (*mcpProcessKey)(unsigned short);
#endif

#endif
