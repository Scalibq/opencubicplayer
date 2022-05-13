#ifndef __PSETTING_H
#define __PSETTING_H

#ifdef WIN32
#include "w32idata.h"
#endif

void cfGetConfig(const char *cppath, const char *cmdline);
void cfCloseConfig();
int cfCountSpaceList(const char *str, int maxlen);
int cfGetSpaceListEntry(char *buf, const char *&str, int maxlen);
const char *cfGetProfileString(const char *app, const char *key, const char *def);
const char *cfGetProfileString2(const char *app, const char *app2, const char *key, const char *def);
int cfGetProfileBool(const char *app, const char *key, int def, int err);
int cfGetProfileBool2(const char *app, const char *app2, const char *key, int def, int err);
int cfGetProfileInt(const char *app, const char *key, int def, int radix);
int cfGetProfileInt2(const char *app, const char *app2, const char *key, int def, int radix);

#if defined(DOS32) || (defined(WIN32)&&defined(NO_CPDLL_IMPORT))
extern char cfConfigDir[];
extern char cfDataDir[];
extern char cfTempDir[];
extern char cfCPPath[];
extern char cfProgramDir[];
extern const char *cfConfigSec;
extern const char *cfSoundSec;
extern const char *cfScreenSec;
extern const char *cfCommandLine;
#else
extern_data char cfConfigDir[];
extern_data char cfDataDir[];
extern_data char cfTempDir[];
extern_data char cfCPPath[];
extern_data char cfProgramDir[];
extern_data const char *cfConfigSec;
extern_data const char *cfSoundSec;
extern_data const char *cfScreenSec;
extern_data const char *cfCommandLine;
#endif

#endif
