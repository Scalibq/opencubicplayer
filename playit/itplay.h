class binfile;
struct sampleinfo;

class itplayerclass
{
public:
  struct envelope
  {
    int len;
    int loops, loope;
    int sloops, sloope;
    int type;
    enum
    {
      active=1, looped=2, slooped=4, carry=8, filter=128
    };
    unsigned short x[26];
    signed char y[28];
  };

  struct sample
  {
    char name[32];
    char packed;
    unsigned short handle;
    signed short normnote;
    unsigned char gvl;
    unsigned char vol;
    unsigned char vis;
    unsigned char vid;
    unsigned char vit;
    unsigned char vir;
    unsigned char dfp;
  };

  struct instrument
  {
    char name[32];
    char handle;
    unsigned char keytab[120][2];
    int fadeout;
    envelope envs[3];
    unsigned char nna;
    unsigned char dct;
    unsigned char dca;
    unsigned char pps;
    unsigned char ppc;
    unsigned char gbv;
    unsigned char dfp;
    unsigned char rv;
    unsigned char rp;
    unsigned char ifp;
    unsigned char ifr;
    unsigned char mch;
    unsigned char mpr;
    unsigned short midibnk;
  };

  struct chaninfo
  {
    unsigned char ins;
    int smp;
    unsigned char note;
    unsigned char vol;
    unsigned char pan;
    unsigned char notehit;
    unsigned char volslide;
    unsigned char pitchslide;
    unsigned char panslide;
    unsigned char volfx;
    unsigned char pitchfx;
    unsigned char notefx;
    unsigned char fx;
  };

  struct module
  {
  public:
    char name[32];
    int nchan;
    int ninst;
    int nsampi;
    int nsamp;
    int npat;
    int nord;
    int linearfreq;
    int endord;
    char **message;
    char **midicmds;
    unsigned short *orders;
    unsigned short *patlens;
    unsigned char **patterns;
    sample *samples;
    instrument *instruments;
    sampleinfo *sampleinfos;
    int deltapacked;
    int inispeed;
    int initempo;
    int inigvol;
    unsigned char inipan[64];
    unsigned char inivol[64];
    int chsep;
    int linear;
    int oldfx;
    int instmode;
    int geffect;

    int load(binfile &);
    void free();
    void optimizepatlens();
    int precalctime(int startpos, int (*calctimer)[2], int calcn, int ite);

  private:
    int decompress8 (binfile &module, void *dst, int len, char it215);
    int decompress16(binfile &module, void *dst, int len, char it215);
  
  };

  enum
  {
    cmdSpeed=1, cmdJump=2, cmdBreak=3, cmdVolSlide=4, cmdPortaD=5, cmdPortaU=6,
    cmdPortaNote=7, cmdVibrato=8, cmdTremor=9, cmdArpeggio=10, cmdVibVol=11,
    cmdPortaVol=12, cmdChanVol=13, cmdChanVolSlide=14, cmdOffset=15, cmdPanSlide=16,
    cmdRetrigger=17, cmdTremolo=18, cmdSpecial=19, cmdTempo=20, cmdFineVib=21,
    cmdGVolume=22, cmdGVolSlide=23, cmdPanning=24, cmdPanbrello=25, cmdMIDI=26,

    cmdSVibType=3, cmdSTremType=4, cmdSPanbrType=5, cmdSPatDelayTick=6,
    cmdSInstFX=7, cmdSPanning=8, cmdSSurround=9, cmdSOffsetHigh=10,
    cmdSPatLoop=11, cmdSNoteCut=12, cmdSNoteDelay=13, cmdSPatDelayRow=14,
    cmdSSetMIDIMacro=15,

    cmdSIPastCut=0, cmdSIPastOff=1, cmdSIPastFade=2, cmdSINNACut=3,
    cmdSINNACont=4, cmdSINNAOff=5, cmdSINNAFade=6, cmdSIVolEnvOff=7,
    cmdSIVolEnvOn=8, cmdSIPanEnvOff=9, cmdSIPanEnvOn=10, cmdSIPitchEnvOff=11,
    cmdSIPitchEnvOn=12,

    cmdVVolume=1, cmdVFVolSlU=66, cmdVFVolSlD=76, cmdVVolSlU=86,
    cmdVVolSlD=96, cmdVPortaD=106, cmdVPortaU=116, cmdVPanning=129,
    cmdVPortaNote=194, cmdVVibrato=204,

    cmdNNote=1, cmdNNoteFade=121, cmdNNoteCut=254, cmdNNoteOff=255,
    cmdSync=30303,
  };

  enum
  {
    ifxGVSUp=1, ifxGVSDown,
    ifxVSUp=1, ifxVSDown,
    ifxPSUp=1, ifxPSDown, ifxPSToNote,
    ifxPnSRight=1, ifxPnSLeft,
    ifxVXVibrato=1, ifxVXTremor,
    ifxPXVibrato=1, ifxPXArpeggio,
    ifxPnXVibrato=1,
    ifxNXNoteCut=1, ifxNXRetrig, ifxNXDelay,

    ifxVolSlideUp=1, ifxVolSlideDown,
    ifxRowVolSlideUp, ifxRowVolSlideDown,
    ifxPitchSlideUp, ifxPitchSlideDown, ifxPitchSlideToNote,
    ifxRowPitchSlideUp, ifxRowPitchSlideDown,
    ifxPanSlideRight, ifxPanSlideLeft,
    ifxVolVibrato, ifxTremor,
    ifxPitchVibrato, ifxArpeggio,
    ifxNoteCut, ifxRetrig,
    ifxOffset, ifxEnvPos,
    ifxDelay,
    ifxChanVolSlideUp, ifxChanVolSlideDown,
    ifxRowChanVolSlideUp, ifxRowChanVolSlideDown,
    ifxPastCut, ifxPastOff, ifxPastFade,
    ifxVEnvOff, ifxVEnvOn, ifxPEnvOff, ifxPEnvOn, ifxFEnvOff, ifxFEnvOn,
    ifxPanBrello
  };


private:
  struct logchan;

  struct physchan
  {
    int no;
    int lch;
    logchan *lchp;
    const sample *smp;
    const instrument *inst;
    int note;
    int newsamp;
    int newpos;
    int vol;
    int fvol;
    int pan;
    int fpan;
    int cutoff;
    int fcutoff;
    int reso;
    int srnd;
    int pitch;
    int fpitch;
    int fadeval;
    int fadespd;
    int notefade;
    int notecut;
    int noteoff;
    int dead;
    int looptype;
    int volenv;
    int panenv;
    int pitchenv;
    int filterenv;
    int penvtype;
    int panenvpos;
    int volenvpos;
    int pitchenvpos;
    int filterenvpos;
    int noteoffset;
    int avibpos;
    int avibdep;
  };
  struct logchan
  {
    physchan *pch;
    physchan newchan;
    int lastins;
    int curnote;
    int lastnote;
    int cvol;
    int vol;
    int fvol;
    int cpan;
    int pan;
    int fpan;
    int srnd;
    int pitch;
    int fpitch;
    int dpitch;
    int cutoff;
    int fcutoff;
    int reso;
    int mute;
    int disabled;
    int vcmd;
    int command;
    int specialcmd;
    int specialdata;
    int volslide;
    int cvolslide;
    int panslide;
    int gvolslide;
    int vibspd;
    int vibdep;
    int vibtype;
    int vibpos;
    int tremspd;
    int tremdep;
    int tremtype;
    int trempos;
    int panbrspd;
    int panbrdep;
    int panbrtype;
    int panbrpos;
    int panbrrnd;
    int arpeggio1;
    int arpeggio2;
    int offset;
    int porta;
    int portanote;
    int vvolslide;
    int retrigpos;
    int retrigspd;
    int retrigvol;
    int tremoroff;
    int tremorlen;
    int tremorpos;
    int patloopstart;
    int patloopcount;
    int nna;
    int realnote;
    int basenote;
    int realsync;
    int realsynctime;
    unsigned char delayed[8];
    int tempo;
    int evpos0;
    int evmodtype;
    int evmod;
    int evmodpos;
    int evpos;
    int evtime;
    int sfznum;
    unsigned char fnotehit;
    unsigned char fvolslide;
    unsigned char fpitchslide;
    unsigned char fpanslide;
    unsigned char fvolfx;
    unsigned char fpitchfx;
    unsigned char fnotefx;
    unsigned char fx;
  };

  static signed char sintab[256];

  void playnote(logchan &c, const unsigned char *);
  void playvcmd(logchan &c, int cmd);
  void playcmd(logchan &c, int cmd, int data);
  void inittickchan(physchan &c);
  void inittick(logchan &c);
  void initrow(logchan &c);
  void updatechan(logchan &c);
  void processfx(logchan &c);
  void processchan(physchan &c);
  void allocatechan(logchan &c);
  void putchandata(physchan &c);
  void putglobdata();
  void getproctime();
  void checkchan(physchan &c);
  int range64(int);
  int range128(int);
  int rangepitch(int);
  int rowslide(int);
  int rowvolslide(int);
  int tickslide(int);
  int rowudslide(int);
  void dovibrato(logchan &c);
  void dotremolo(logchan &c);
  void dopanbrello(logchan &c);
  void doportanote(logchan &c);
  void doretrigger(logchan &c);
  void dotremor(logchan &c);
  void dodelay(logchan &c);
  int  ishex(char c);
  void parsemidicmd(logchan &c, char *cmd, int z);
  int random();
  int processenvelope(const envelope &e, int &pos, int noteoff, int active);

  int pitchhigh;
  int pitchlow;
  int randseed;
  int gotoord;
  int gotorow;
  int manualgoto;
  int patdelayrow;
  int patdelaytick;
  unsigned char *patptr;
  int linear;
  int oldfx;
  int instmode;
  int geffect;
  int chsep;
  int speed;
  int tempo;
  int gvol;
  int gvolslide;
  int curtick;
  int currow;
  int curord;
  int endord;
  int nord;
  int nchan;
  int npchan;
  int ninst;
  int nsamp;
  int nsampi;
  int noloop;
  int looped;
  logchan *channels;
  physchan *pchannels;
  const instrument *instruments;
  const sample *samples;
  const sampleinfo *sampleinfos;
  const unsigned short *orders;
  unsigned char **patterns;
  const unsigned short *patlens;
  char **midicmds;

  int (*que)[4];
  int querpos;
  int quewpos;
  int quelen;
  void readque();
  void putque(int,int,int);
  int proctime;
  int realpos;
  int realsync;
  int realsynctime;
  int realtempo;
  int realspeed;
  int realgvol;

  enum
  {
    quePos, queSync, queTempo, queSpeed, queGVol,
  };

  static void playtickstatic();
  static itplayerclass *staticthis;
  void playtick();

public:
  int loadsamples(module &);
  int play(const module &, int chan);
  void stop();

  int getsync(int ch, int &time);
  int gettime();
  int getpos();
  int getrealpos();
  int getticktime();
  int getrowtime();
  void setevpos(int ch, int pos, int modtype, int mod);
  int getevpos(int ch, int &time);
  int findevpos(int pos, int &time);
  void setpos(int ord, int row);
  void mutechan(int,int);
  void getglobinfo(int &speed, int &bpm, int &gvol, int &gs);
  int getchansample(int ch, short *buf, int len, int rate, int opt);
  int getdotsdata(int ch, int pch, int &smp, int &note, int &voll, int &volr, int &sus);
  void getrealvol(int ch, int &l, int &r);
  void setloop(int s);
  int getloop();
  int chanactive  (int ch, int &lc);
  int getchanins  (int ch);
  int getchansamp (int ch);
  int lchanactive  (int lc);
  void getchaninfo(unsigned char ch, chaninfo &ci);
  int getchanalloc(unsigned char ch);

};

