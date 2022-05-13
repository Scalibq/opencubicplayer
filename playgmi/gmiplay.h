#ifndef __MIDI_H
#define __MIDI_H

struct msample
{
  char name[32];
  unsigned char sampnum;
  signed short handle;
  unsigned short normnote;
  unsigned long volrte[6];
  unsigned short volpos[6];
  unsigned char end;
  signed char sustain;
  unsigned short tremswp;
  unsigned short tremrte;
  unsigned short tremdep;
  unsigned short vibswp;
  unsigned short vibrte;
  unsigned short vibdep;
  unsigned short sclfac;
  unsigned char sclbas;
};

struct minstrument
{
  char name[32];
  unsigned char prognum;
  unsigned short sampnum;
  msample *samples;
  unsigned char note[128];
};

struct miditrack
{
  unsigned char *trk;
  unsigned char *trkend;
};

struct midifile
{
  unsigned long opt;
  unsigned short tracknum;
  unsigned short tempo;
  miditrack *tracks;
  unsigned long ticknum;
  unsigned char instmap[129];
  unsigned short instnum;
  unsigned short sampnum;
  minstrument *instruments;
  sampleinfo *samples;

  void reset();
  void free();
  char loadsamples();
};

struct mchaninfo
{
  unsigned char ins;
  unsigned char pan;
  unsigned char gvol;
  signed short pitch;
  unsigned char reverb;
  unsigned char chorus;
  unsigned char notenum;
  unsigned char pedal;
  unsigned char note[32];
  unsigned char vol[32];
  unsigned char opt[32];
};

struct mchaninfo2
{
  unsigned char mute;
  unsigned char notenum;
  unsigned char opt[32];
  unsigned char ins[32];
  unsigned short note[32];
  unsigned char voll[32];
  unsigned char volr[32];
};

struct mglobinfo
{
  unsigned long curtick;
  unsigned long ticknum;
  unsigned long speed;
};

unsigned char midInit(const char *path);
void midClose();

#define MID_DRUMCH16 1

char midLoadMidi(midifile &, binfile &, unsigned long opt, const char *);
char midPlayMidi(const midifile &, unsigned char voices);
void midStopMidi();
unsigned long midGetPosition();
void midSetPosition(signed long pos);
void midGetChanInfo(unsigned char ch, mchaninfo &ci);
void midGetRealNoteVol(unsigned char ch, mchaninfo2 &ci);
void midGetGlobInfo(mglobinfo &gi);
void midSetMute(int ch, int p);
unsigned char midGetMute(unsigned char ch);
char midLooped();
void midSetLoop(unsigned char s);
int midGetChanSample(int ch, short *buf, int len, int rate, int opt);

#endif
