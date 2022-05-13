// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Wavetable Device: AudioSystem EWS64 SAM9407 synthesizer
//
// THIS SOURCE IS NOT FINISHED, IN FACT IT'S STILL FAR AWAY FROM WHAT
// IT SHALL BECOME

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "ss.h"
#include "mcp.h"
#include "mix.h"
#include "irq.h"
#include "timer.h"
#include "imsrtns.h"


#define MAXSAMPLES   256
#define FXBUFFERSIZE 65536

extern "C" extern sounddevice mcpEWS64;

struct ewsmemtableentry
{
  unsigned short type;
  unsigned long  addr;
};

struct ewsbanktableentry
{
  char           name[8];
  unsigned short prio;
};

struct ewsmemmap
{
  ewsmemtableentry  memblock[64];
  ewsbanktableentry midibank[8];
};

static short ewsPort;
static short ewsPort2;
static short ewsIRQ;
static short ewsIRQ2;

static char maxchan;
static long memsize;
static long mappos;
static ewsmemmap map;

static unsigned short linvol[513];

// ----------------------------------------- ISA PnP and detection routines

// waste a fair bit of time
static void waste_time(char ficken)
{
  for (int j=0;j<ficken;j++)
    for (int i=0;i<0x180;i++)
      inp(0x21);
}

// get pnp register
static char inpnp(char reg, short rdp)
{
  outp(0x279,reg);
  return inp(rdp);
}

// set pnp register
static void outpnp(char reg, char val)
{
  outp(0x279,reg);
  outp(0xa79,val);
}

static char detectews(short rdp)
{

  outpnp(2,4);

  char csn=0;
  char ok=0;

  while (1)
  {

    outpnp(3,0);
    outpnp(0,rdp>>2);
    waste_time(4);

    outp(0x279,1);
    waste_time(4);

    unsigned long vall=0,valh=0;
    unsigned short bitval;
    unsigned short kval1=0x006a;
    unsigned char  kval2=0;

    char f=_disableint();
    for (int i=0; i<64; i++)
    {
      kval1=(kval1&0xff)|((kval1&0xfe)<<7);
      kval1^=kval1<<8;
      kval1=kval1>>1;

      vall=(vall>>1)|((valh&1)?0x80000000:0);
      valh=(valh>>1);

      bitval=inp(rdp)<<8;
      waste_time(1);
      bitval|=inp(rdp);
      waste_time(1);
      if (bitval==0x55aa)
      {
        valh|=0x80000000;
        kval1^=0x80;
      }
    }
    kval1&=0xff;
    for (i=0; i<8; i++)
    {
      kval2=kval2>>1;
      bitval=inp(rdp)<<8;
      waste_time(1);
      bitval|=inp(rdp);
      waste_time(1);
      if (bitval==0x55aa)
      {
        kval2|=0x80;
      }
    }
    _restoreint(f);


    if (kval1!=kval2)
      break;

    outpnp(6,++csn);
    if (vall==0x36a8630e)
    {
      outpnp(3,csn);

      outpnp(7,4);
      ewsPort=(inpnp(0x60,rdp)<<8)|inpnp(0x61,rdp);
      ewsIRQ=inpnp(0x70,rdp);
      if (ewsIRQ<1 || ewsIRQ>15)
        ewsIRQ=-1;

      outpnp(7,0);
      ewsPort2=(inpnp(0x60,rdp)<<8)|inpnp(0x61,rdp);
      ewsIRQ2=inpnp(0x70,rdp);
      if (ewsIRQ2<1 || ewsIRQ2>15)
        ewsIRQ2=-1;

      outp(ewsPort+1,0xff);
      outp(ewsPort+1,0x3f);
      if (inp(ewsPort)==0xfe && !(inp(ewsPort+1)&0x30))
        return 1;

    }
  }

  return 0;

}


static char ewsGetCfg()
{
  // send pnp init key
  unsigned short keyval=0x006A;
  int i;
  outp(0x279,0);
  outp(0x279,0);
  for (i=0; i<32; i++)
  {
    outp(0x279,keyval&0xff);
    keyval=(keyval&0xff)|((keyval&0xfe)<<7);
    keyval^=keyval<<8;
    keyval=keyval>>1;
  }

  for (int rdp=0x207; rdp<0x280 && !detectews(rdp); rdp+=4);

  outpnp(2,2);

  return rdp<0x280;
}

// ------------------------------------------------------ Register handlers

// SAM9407 command list
enum
{
  WRT_MEM=0x01, RD_MEM, GET_MMT, SET_MMT,

  MASTER_VOL=0x07, REC_MODE,

  EQ_LBL=0x10, EQ_MLBL, EQ_MHBL, EQ_HBL, EQ_LBR, EQ_MLBR, EQ_MHBR, EQ_HBR,
    EQF_LB, EQF_MLB, EQF_MHB, EQF_HB,

  AUD_SEL=0x20, AUD_GAINL, AUD_GAINR,

  GMREV_SEND=0x25, GMCHR_SEND, AUDREV_SEND, ECH_LEV, ECH_TIM, ECH_FEED,

  SUR_VOL=0x30, SUR_DEL, SUR_INP, SUR_24, AUDL_VOL, AUDR_VOL, AUDL_PAN,
    AUDR_PAN, GM_VOL, GM_PAN, REV_VOL, CHR_VOL,

  EN_MIDOUT=0x3d,
  UART_MOD=0x3f,

  W_OPEN=0x40, W_CLOSE, W_START, END_XFER, W_PITCH, W_VOLLEFT, W_VOLRIGHT,
    W_VOLAUXLEFT, GEN_INT, W_VOLAUXRIGHT, W_FILT_FC, W_FILT_Q,

  GET_VOI=0x51, VOI_OPEN, VOI_CLOSE, VOI_START, VOI_STOP, VOI_VOL, VOI_MAIN,
    VOI_PITCH, VOI_AUX, VOI_FILT, VOI_MEM, GET_POS, ADD_POS,

  WAVE_ASS=0x60, MOD_ASS, GM_POST, WAVE_POST, MOD_POST, AUDECH_POST,
    EFF_POST,

  ECH_ONOFF=0x68, REV_TYPE, CHR_TYPE, EQU_TYPE, REV_ONOFF, CHR_ONOFF,
    SUR_ONOFF, AUD_ONOFF,

  HOT_RES=0x70,
  POLY_64=0x72,

  CHR_DEL=0x74, CHR_FEED, CHR_RATE, CHR_DEPTH, REV_TIME, REV_FEED,
  EN_CONTROL=0xbe,

  RESET=0xff
};


static inline char ewsstat()
{
  return inp(ewsPort+1);
}


static inline void ewscmd(char cmd)
{
  while (ewsstat()&0x40) printf("!");
  outp(ewsPort+1,cmd);
}


static inline char ewsbwait()
{
  for (int wl=0; (wl<1000) && (ewsstat()&0x80); wl++);
  return wl<1000;
}


static inline void ewsout(char val)
{
  while (ewsstat()&0x40) printf("!");
  outp(ewsPort,val);
}


static inline void ewsoutw(unsigned short val)
{
  ewsout(val);
  ewsout(val>>8);
}


static inline void ewsoutt(unsigned long val)
{
  ewsout(val);
  ewsout(val>>8);
  ewsout(val>>16);
}


static inline void ewsoutd(unsigned long val)
{
  ewsout(val);
  ewsout(val>>8);
  ewsout(val>>16);
  ewsout(val>>24);
}


static inline char ewsin()
{
  return inp(ewsPort);
}


static inline unsigned short ewsinw()
{
  return ewsin()|(ewsin()<<8);
}


static inline unsigned long ewsint()
{
  return ewsin()|(ewsin()<<8)|(ewsin()<<16);
}


static inline unsigned long ewsind()
{
  return ewsin()|(ewsin()<<8)|(ewsin()<<16)|(ewsin()<<24);
}


static inline void ewsout16(unsigned short val)
{
  outpw(ewsPort+2,val);
}


static unsigned short ewsin16()
{
  return inpw(ewsPort+2);
}



// ------------------------------------------------ On-card memory handlers



// ---------------------------------------------------- Types and Variables

struct ewschan
{
  unsigned char  bank;
  unsigned long  startpos;
  unsigned long  endpos;
  unsigned long  loopstart;
  unsigned long  loopend;
  unsigned long  sloopstart;
  unsigned long  sloopend;
  unsigned long  samprate;
  unsigned long  curstart;
  unsigned long  curend;
  unsigned char  redlev;

  unsigned char  curloop;
  int            samptype;

  unsigned short cursamp;
  unsigned char  mode;

  unsigned short volume;
  unsigned short voll;
  unsigned short volr;
  unsigned short reverb;
  unsigned char  fxsend;

  unsigned char  inited;
  signed char    chstatus;
  signed short   nextsample;
  signed long    nextpos;
  unsigned char  orgloop;
  signed char    loopchange;
  signed char    dirchange;

  unsigned long  orgfreq;
  unsigned long  orgdiv;
  unsigned short orgvol;
  signed short   orgpan;
  unsigned char  orgrev;
  unsigned char  pause;
  unsigned char  wasplaying;

  void           *smpptr;
};


struct ewssample
{
  unsigned char bank;
  signed long pos;
  unsigned long length;
  unsigned long loopstart;
  unsigned long loopend;
  unsigned long sloopstart;
  unsigned long sloopend;
  unsigned long samprate;
  int type;
  unsigned char redlev;
  void *ptr;
};


static ewssample samples[MAXSAMPLES];
static unsigned short samplenum;

static unsigned char channelnum;
static void (*playerproc)();
static ewschan channels[64];

static unsigned long cmdtimerpos;
static unsigned long stimerlen;
static unsigned long stimerpos;

static unsigned short relspeed;
static unsigned long orgspeed;
static unsigned char mastervol;
static signed char masterpan;
static signed char masterbal;
static unsigned short masterfreq;
static signed short masterreverb;
static unsigned long amplify;

static unsigned char paused;



static void timerrout()
{
  if (paused)
    return;
  if (stimerpos<=65536)
    stimerpos=stimerlen;
  else
    stimerpos-=65536;
  tmSetNewRate((stimerpos<=65536)?stimerpos:65536);
  if (stimerpos==stimerlen)
  {
    //processtick();
    playerproc();
    cmdtimerpos+=stimerlen;
    stimerlen=umuldiv(256, 1193046*256, orgspeed*relspeed);
  }
}



static void GetMixChannel(int ch, mixchannel &chn, int rate)
{
  chn.status=0;
  chn.status|=MIX_INTERPOLATE;
  chn.status|=MIX_PLAYING;
}



static void SET(int ch, int opt, int val)
{
  switch (opt)
  {
  case mcpGSpeed: // set global speed
    break;
  case mcpCInstrument: // set new inst on channel
    break;
  case mcpCMute: // mute channel
    break;
  case mcpCStatus: // change channel status
    break;
  case mcpCLoop: // change loop type
    break;
  case mcpCDirect: // change direction
    break;
  case mcpCPosition: // change sample position
    break;
  case mcpCPitch: // change pitch (8363)
//    channels[ch].orgfreq=8363;
//    channels[ch].orgdiv=mcpGetFreq8363(-val);
    break;
  case mcpCPitchFix: // change pitch (fixed)
//    channels[ch].orgfreq=val;
//    channels[ch].orgdiv=0x10000;
    break;
  case mcpCPitch6848: // change pitch (6848)
//    channels[ch].orgfreq=6848;
//    channels[ch].orgdiv=val;
    break;
  case mcpCReset: // reset channel (preserves mute status)
    break;
  case mcpCVolume: // change channel volume (uc)
    break;
  case mcpCPanning: // change channel panning (sc)
    break;
  case mcpCReverb: // change channel reverb (uc)
    break;
  case mcpMasterAmplify: // change master amplification (ul)
    break;
  case mcpMasterPause: // pause
    break;
  case mcpMasterVolume: // change master volume (uc)
    break;
  case mcpMasterPanning: // change master panning (sc)
    break;
  case mcpMasterBalance: // change master balance (sc)
    break;
  case mcpMasterReverb:  // change master reverb (sc)
    break;
  case mcpMasterSpeed:   // change relative speed
    break;
  case mcpMasterPitch:   // change relative pitch
    break;
  case mcpMasterFilter:  // change filter type
    break;
  }
}

static int GET(int ch, int opt)
{
  switch (opt)
  {
  case mcpCStatus:
//    selvoc(ch);
//    return !(getmode()&1)||(paused&&channels[ch].wasplaying);
  case mcpCMute:
//    return !!channels[ch].pause;
  case mcpGTimer:
    return tmGetTimer();
  case mcpGCmdTimer:
    return umulshr16(cmdtimerpos, 3600);
  }
  return 0;
}


static int LoadSamples(sampleinfo *sil, int n)
{
  if (n>MAXSAMPLES) return 0;

//  if (!mcpReduceSamples(sil, n, memsize-samplen[largestsample], mcpRedToMono))

  return 0;
}


static int OpenPlayer(int chan, void (*proc)())
{

  if (!mixInit(GetMixChannel, 1, chan, amplify))
    return 0;

  orgspeed=50*256;

  memset(channels, 0, sizeof(ewschan)*chan);

  playerproc=proc;

  channelnum=chan;

  cmdtimerpos=0;

  stimerlen=umuldiv(256, 1193046*256, orgspeed*relspeed);
  stimerpos=stimerlen;
  tmInit(timerrout, (stimerpos<=65536)?stimerpos:65536, 8192);

  mcpNChan=chan;

  return 1;
}



static void ClosePlayer()
{
  mcpNChan=0;

  tmClose();

  mixClose();
}



static int initu(const deviceinfo &c)
{
  channelnum=0;
  mastervol=64;
  masterpan=64;
  masterbal=0;
  masterfreq=256;
  amplify=65536;

  linvol[0]=0;
  linvol[512]=0x0FFF;
  for (int i=1; i<512; i++)
  {
    int k=i;
    int j;
    for (j=0x0600; k; j+=0x0100)
      k>>=1;
    linvol[i]=j|((i<<(8-((j-0x700)>>8)))&0xFF);
  }

  mcpLoadSamples=LoadSamples;
  mcpOpenPlayer=OpenPlayer;
  mcpClosePlayer=ClosePlayer;
  mcpSet=SET;
  mcpGet=GET;

  return 1;
}


static void closeu()
{
  mcpOpenPlayer=0;
}


static int detectu(deviceinfo &c)
{
  ewsPort=c.port;
  ewsPort2=c.port2;

  printf("\ndetecting...\n");

  if (ewsPort==-1 || ewsPort2==-1)
    if (!ewsGetCfg())
      return 0;

  printf("synth found at port 0x%03x\n",ewsPort);
  printf("CODEC found at port 0x%03x",ewsPort2);
  if (ewsIRQ2!=-1)
    printf(" (irq %d)",ewsIRQ2);
  printf("\n");

  // place chip in UART mode

  printf("enabling UART mode\n");
  int i;
  ewscmd(0x3f);
  if (!ewsbwait())
    return 0;
  if (ewsin()!=0xFE)
    return 0;

  printf("flushing\n");
  for (i=0; i<100; i++)
    ewsin();

  // test gen_int function (ohne irq, is klar, ehh)
  printf("testing GEN_INT\n");
  ewscmd(GEN_INT);
  ewsout(0);
  if (!ewsbwait())
    return 0;
  if (ewsin()!=0x88)
    return 0;

  // find out most probable number of available channels
  printf("reading voice number\n");
  ewscmd(GET_VOI);
  ewsout(0x00);
  ewsbwait();
  ewsin();    // skip strange 0xFF byte
  ewsbwait();
  maxchan=ewsin();
  printf("voices available: %d\n",maxchan);

  c.dev=&mcpEWS64;
  c.port=ewsPort;
  c.port2=ewsPort2;
  c.irq=ewsIRQ;
  c.irq2=ewsIRQ2;
  c.dma=-1;
  c.dma2=-1;
  c.subtype=-1;
  c.chan=maxchan;
  c.mem=memsize;

  printf("press a key\n");
  getch();

  return 1;

}


#include "devigen.h"
#include "psetting.h"

static unsigned long ewsGetOpt(const char *sec)
{
  printf("reading additional information... \n");
  return -1;
}


extern "C" {
  sounddevice mcpEWS64={SS_WAVETABLE, "Terratec AudioSystem EWS64", detectu, initu, closeu};
  devaddstruct mcpEWSAdd = {ewsGetOpt, 0, 0, 0};
  char *dllinfo = "driver _mcpEWS64; addprocs _mcpEWSAdd";
}
