#ifndef __PLAYSID_H
#define __PLAYSID_H

struct sidChanInfo {
  unsigned long freq;
  char ad;
  char sr;
  unsigned short pulse;
  unsigned short wave;
  char filtenabled;
  char filttype;
  long leftvol;
  long rightvol;
};


struct sidDigiInfo {
  char l;
  char r;
  sidDigiInfo() { l=r=0; };
};


struct sidTuneInfo;

unsigned char sidpOpenPlayer(binfile &);
void sidpClosePlayer();
void sidpIdle();
void sidpPause(unsigned char p);
void sidpSetAmplify(unsigned long amp);
void sidpSetVolume(unsigned char vol, signed char bal, signed char pan, unsigned char opt);
void sidpGetGlobInfo(sidTuneInfo &si);
void sidpStartSong(char sng);
void sidpToggleVideo();
char sidpGetVideo();
char sidpGetFilter();
void sidpToggleFilter();
char sidpGetSIDVersion();
void sidpToggleSIDVersion();
void sidpMute(int i, int m);
void sidpGetChanInfo(int i, sidChanInfo &ci);
void sidpGetDigiInfo(sidDigiInfo &di);

#endif
