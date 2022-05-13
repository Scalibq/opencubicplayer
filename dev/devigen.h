struct deviceinfo;
struct sounddevice;

struct devaddstruct
{
  unsigned long (*GetOpt)(const char *);
  void (*Init)(const char *);
  void (*Close)();
  int (*ProcessKey)(unsigned short);
};

struct devinfonode
{
  devinfonode *next;
  char handle[9];
  deviceinfo dev;
  devaddstruct *addprocs;
  char name[32];
  char ihandle;
  char keep;
  int linkhand;
};

int deviReadDevices(const char *list, devinfonode **devs);
