#ifndef __MCP_H
#define __MCP_H

#ifdef WIN32
#include "w32idata.h"
#endif

struct sampleinfo
{
  unsigned long type;
  void *ptr;
  long length;
  long samprate;
  long loopstart;
  long loopend;
  long sloopstart;
  long sloopend;
};

enum
{
  mcpSampUnsigned=1,
  mcpSampDelta=2,
  mcpSamp16Bit=4,
  mcpSampBigEndian=8,
  mcpSampLoop=16,
  mcpSampBiDi=32,
  mcpSampSLoop=64,
  mcpSampSBiDi=128,
  mcpSampStereo=256,
  mcpSampFloat=512,
  mcpSampRedBits=0x80000000,
  mcpSampRedRate2=0x40000000,
  mcpSampRedRate4=0x20000000,
  mcpSampRedStereo=0x10000000,
};

enum
{
  mcpMasterVolume, mcpMasterPanning, mcpMasterBalance, mcpMasterSurround,
  mcpMasterSpeed, mcpMasterPitch, mcpMasterBass, mcpMasterTreble,
  mcpMasterReverb, mcpMasterChorus, mcpMasterPause, mcpMasterFilter,
  mcpMasterAmplify,
  mcpGSpeed,
  mcpCVolume, mcpCPanning, mcpCPanY, mcpCPanZ, mcpCSurround, mcpCPosition,
  mcpCPitch, mcpCPitchFix, mcpCPitch6848, mcpCStop, mcpCReset,
  mcpCBass, mcpCTreble, mcpCReverb, mcpCChorus, mcpCMute, mcpCStatus,
  mcpCInstrument, mcpCLoop, mcpCDirect, mcpCFilterFreq, mcpCFilterRez,
  mcpGTimer, mcpGCmdTimer,
  mcpGRestrict
};

int mcpReduceSamples(sampleinfo *s, int n, long m, int o);
enum
{
  mcpRedAlways16Bit=1,
  mcpRedNoPingPong=2,
  mcpRedGUS=4,
  mcpRedToMono=8,
  mcpRedTo8Bit=16,
  mcpRedToFloat=32,
};


enum
{
  mcpGetSampleStereo=1, mcpGetSampleHQ=2,
};

#if defined(DOS32) || (defined(WIN32)&&defined(NO_MCPBASE_IMPORT))
extern int mcpNChan;

extern int (*mcpLoadSamples)(sampleinfo* si, int n);
extern int (*mcpOpenPlayer)(int, void (*p)());
extern void (*mcpClosePlayer)();
extern void (*mcpSet)(int ch, int opt, int val);
extern int (*mcpGet)(int ch, int opt);
extern void (*mcpGetRealVolume)(int ch, int &l, int &r);
extern void (*mcpGetRealMasterVolume)(int &l, int &r);
extern void (*mcpGetMasterSample)(short *s, int len, int rate, int opt);
extern int (*mcpGetChanSample)(int ch, short *s, int len, int rate, int opt);
extern int (*mcpMixChanSamples)(int *ch, int n, short *s, int len, int rate, int opt);

extern int mcpMixMaxRate;
extern int mcpMixProcRate;
extern int mcpMixOpt;
extern int mcpMixBufSize;
extern int mcpMixMax;
extern int mcpMixPoll;

extern void (*mcpIdle)();

#else

extern_data int mcpNChan;

extern_data int (*mcpLoadSamples)(sampleinfo* si, int n);
extern_data int (*mcpOpenPlayer)(int, void (*p)());
extern_data void (*mcpClosePlayer)();
extern_data void (*mcpSet)(int ch, int opt, int val);
extern_data int (*mcpGet)(int ch, int opt);
extern_data void (*mcpGetRealVolume)(int ch, int &l, int &r);
extern_data void (*mcpGetRealMasterVolume)(int &l, int &r);
extern_data void (*mcpGetMasterSample)(short *s, int len, int rate, int opt);
extern_data int (*mcpGetChanSample)(int ch, short *s, int len, int rate, int opt);
extern_data int (*mcpMixChanSamples)(int *ch, int n, short *s, int len, int rate, int opt);

extern_data int mcpMixMaxRate;
extern_data int mcpMixProcRate;
extern_data int mcpMixOpt;
extern_data int mcpMixBufSize;
extern_data int mcpMixMax;
extern_data int mcpMixPoll;

extern_data void (*mcpIdle)();

#endif

int mcpGetFreq6848(int note);
int mcpGetFreq8363(int note);
int mcpGetNote6848(int freq);
int mcpGetNote8363(int freq);

#endif
