#ifndef __WAVE_H
#define __WAVE_H

struct waveinfo
{
  unsigned long pos;
  unsigned long len;
  unsigned long rate;
  unsigned char stereo;
  unsigned char bit16;
};

unsigned char wpOpenPlayer(binfile &, int tostereo, int tolerance, int, int);
void wpClosePlayer();
void wpIdle();
void wpSetLoop(unsigned char s);
char wpLooped();
void wpPause(unsigned char p);
void wpSetAmplify(unsigned long amp);
void wpSetSpeed(unsigned short sp);
void wpSetVolume(unsigned char vol, signed char bal, signed char pan, unsigned char opt);
void wpGetInfo(waveinfo &);
unsigned long wpGetPos();
void wpSetPos(signed long pos);

#endif
