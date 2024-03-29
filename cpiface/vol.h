#ifndef __VOL_H
#define __VOL_H

struct ocpvolstruct
{
  int val,               // the current value
      min,               // the minimal value
      max,               // the maximal value
      step,              // stepping
      log;               // log scale? (not yet supported)
  char *name;
};

// min<=val<=max !!
// (max-min)%step==0 !!

struct ocpvolregstruct
{
  int (*GetVolumes)();                   // returns the number of setable volumes
  int (*GetVolume)(ocpvolstruct&, int);  // gets a volume (min, max, step and log should be same on every call!)
  int (*SetVolume)(ocpvolstruct&, int);  // sets a volume (only val should be changed!)
};

#endif
