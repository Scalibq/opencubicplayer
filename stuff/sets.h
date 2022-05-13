#ifndef __SETS_H
#define __SETS_H

#ifdef WIN32
#include "w32idata.h"
#endif

struct settings
{
  signed short amp;
  signed short speed;
  signed short pitch;
  signed short pan;
  signed short bal;
  signed short vol;
  signed short srnd;
  signed short filter;
  short useecho;
  signed short reverb;
  signed short chorus;
};

#if defined(DOS32) || (defined(WIN32)&&defined(NO_SETS_IMPORT))
extern settings set;
#else
extern_data settings set;
#endif

#endif
