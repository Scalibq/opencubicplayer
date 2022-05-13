// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Sound settings module
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#define NO_SETS_IMPORT
#include <stdio.h>
#include "pmain.h"
#include "psetting.h"
#include "sets.h"
#include "err.h"

settings set;

static int ssInit()
{
  int per;
  per=cfGetProfileInt2(cfSoundSec, "sound", "amplify", 100, 10);
  per=cfGetProfileInt("commandline_v", "a", per, 10);
  set.amp=(per>=800)?511:(per*64/100);
  per=cfGetProfileInt2(cfSoundSec, "sound", "volume", 100, 10);
  per=cfGetProfileInt("commandline_v", "v", per, 10);
  set.vol=(per>=100)?64:(per*64/100);
  per=cfGetProfileInt2(cfSoundSec, "sound", "balance", 0, 10);
  per=cfGetProfileInt("commandline_v", "b", per, 10);
  set.bal=(per>=100)?64:(per<=-100)?-64:(per*64/100);
  per=cfGetProfileInt2(cfSoundSec, "sound", "panning", 100, 10);
  per=cfGetProfileInt("commandline_v", "p", per, 10);
  set.pan=(per>=100)?64:(per<=-100)?-64:(per*64/100);
  set.srnd=cfGetProfileBool2(cfSoundSec, "sound", "surround", 0, 0);
  set.srnd=cfGetProfileBool("commandline_v", "s", set.srnd, 1);
  set.filter=cfGetProfileInt2(cfSoundSec, "sound", "filter", 1, 10)%3;
  set.filter=cfGetProfileInt("commandline_v", "f", set.filter, 10)%3;
  per=cfGetProfileInt2(cfSoundSec, "sound", "reverb", 0, 10);
  per=cfGetProfileInt("commandline_v", "r", per, 10);
  set.reverb=(per>=100)?64:(per<=-100)?-64:(per*64/100);
  per=cfGetProfileInt2(cfSoundSec, "sound", "chorus", 0, 10);
  per=cfGetProfileInt("commandline_v", "c", per, 10);
  set.chorus=(per>=100)?64:(per<=-100)?-64:(per*64/100);
  set.speed=256;
  set.pitch=256;

  return errOk;
}

extern "C"
{
  initcloseregstruct setsReg={ssInit, 0};
  char *dllinfo = "initclose _setsReg";
};
