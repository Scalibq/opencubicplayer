#pragma pack (push, 4)

struct mixfpostprocregstruct
{
  void (*Process)(float *buffer, int len, int rate, int stereo);
  void (*Init)(int rate, int stereo);
  void (*Close)();
  mixfpostprocregstruct *next;
};

#pragma pack (pop)

void mixfRegisterPostProc(mixfpostprocregstruct *);

// #ifdef CPDOS

struct mixfpostprocaddregstruct
{
  int (*ProcessKey)(unsigned short key);
  mixfpostprocaddregstruct *next;
};

// #endif
