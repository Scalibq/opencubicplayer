// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Player device for ESS AudioDrive 688
//
// revision history: (please note changes here)
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -got this file via e-mail ;)
//    -added _dllinfo record


//***************************************************************************
//
// this file is (c) '97 OleGPro
//
// this file is NOT the part of the cubic player development kit but
// you may only use/modify/spread this file under the terms stated
// in the cubic player development kit accompanying documentation.
//
//***************************************************************************


#include <stdlib.h>
#include <ctype.h>
#include <conio.h>
#include "dma.h"
#include "imsdev.h"
#include "player.h"

extern "C" extern sounddevice plrESSAudioDrive;

static signed   short  ess_port;
static signed   char   ess_irq;      // ESS688 IRQ. Though we will not use it.
static signed   char   ess_dma;      // 8 bit-dma channel
static unsigned char   Register_B1;
static unsigned char   Register_B2;

/*
    Define some important ESS688 i/o ports:
*/

#define MIXER_ADDRESS       (ess_port+0x4)
#define MIXER_DATA          (ess_port+0x5)
#define DSP_RESET           (ess_port+0x6)
#define DSP_READ_DATA       (ess_port+0xa)
#define DSP_WRITE_DATA      (ess_port+0xc)
#define DSP_WRITE_STATUS    (ess_port+0xc)
#define DSP_DATA_AVAIL      (ess_port+0xe)

static unsigned char getcfg()
{
  ess_port=-1;
  ess_irq=-1;
  ess_dma=-1;
  char *s=getenv("BLASTER");
  if (!s)
    return 0;
  while (1)
  {
    while (isspace(*s))
      s++;
    if (!*s)
      break;
    switch (*s++)
    {
    case 'a': case 'A':
      ess_port=strtoul(s, 0, 16);
      break;
    case 'i': case 'I':
      ess_irq=strtoul(s, 0, 10);
      break;
    case 'd': case 'D':
      ess_dma=strtoul(s, 0, 10);
    }
    while (!isspace(*s)&&*s)
      s++;
  }
  return 1;
}

static int ESS_WaitDSPWrite(void)
/*
	Waits until the DSP is ready to be written to.

	returns FALSE on timeout
*/
{
        unsigned short timeout=32767;

	while(timeout--){
        if(!(inp(DSP_WRITE_STATUS)&0x80)) return 1;
	}
	return 0;
}

static int ESS_WaitDSPRead(void)
/*
	Waits until the DSP is ready to read from.

	returns FALSE on timeout
*/
{
        unsigned short timeout=32767;

	while(timeout--){
        if((inp(DSP_WRITE_STATUS)&0x40)) return 1;
	}
	return 0;
}

static int ESSWrite(unsigned char Register, unsigned char Value)
/*
	Writes byte 'data' to the DSP.

	returns FALSE on timeout.
*/
{
    if(!ESS_WaitDSPWrite()) return 0;
    outp(DSP_WRITE_DATA,Register);

    if(!ESS_WaitDSPWrite()) return 0;
    outp(DSP_WRITE_DATA,Value);
    return 1;
}

static void ESSCommand(unsigned char Command)
{
    ESS_WaitDSPWrite();
    outp(DSP_WRITE_STATUS,Command);
}

static unsigned short ESSRead(unsigned char Register)
/*
	Reads a byte from the DSP.

	returns 0xffff on timeout.
*/
{
    ESSCommand(0xC0);                /* send Read ESS Register command */

    if(!ESS_WaitDSPWrite()) return 0xffff;
    outp(DSP_WRITE_DATA,Register);

    if(!ESS_WaitDSPRead()) return 0xffff;
        return(inp(DSP_READ_DATA));

}

static int ESS_Reset(void)
/*
	Resets the DSP.
*/
{
    outp(DSP_RESET,3);

    outp(DSP_RESET,0);
    if(!ESS_WaitDSPRead()) return 0;
        return(inp(DSP_READ_DATA)==0xAA);

}

static int testPort(void)
/*
    Checks if a ESS688 is present at the current baseport by
    resetting the DSP and checking the returned value of
    0xE7 command.

    returns: TRUE   => ESS688 is present
             FALSE  => No ESS688 detected
*/
{
   unsigned char high,low;
    if(!ESS_Reset()) return 0;
	else
      {
        if(!ESS_WaitDSPWrite()) return 0;
            outp(DSP_WRITE_DATA,0xE7);

        if(!ESS_WaitDSPRead()) return 0;
            high=inp(DSP_READ_DATA);

        if(!ESS_WaitDSPRead()) return (0);
            low=inp(DSP_READ_DATA);
     }
     return(high==0x68 && ((low & 0xF0)==0x80));
}

static long playpos;
static long buflen;

static void advance(int pos)
{
  playpos+=(pos-(playpos%buflen)+buflen)%buflen;
}

static int ESSRate;
static unsigned char ESSStereo;
static unsigned char ESS16Bit;
static unsigned char ESSSignedOut;
static __segment dmabufsel;

static void ESSSetOptions(int rate, int opt)
{
  ESSStereo=!!(opt&PLR_STEREO);
  ESS16Bit=!!(opt&PLR_16BIT);
  ESSSignedOut=!!(opt&PLR_SIGNEDOUT);

  unsigned long rt=rate;

  if (rt<4000)
    rt=4000;
  if (rt>64000)
    rt=64000;
  ESSRate=rate;
  plrRate=rate;
  plrOpt=opt;
}

long muldiv64(long, long, long);
#pragma aux muldiv64 parm [eax] [edx] [ecx] = "imul edx" "idiv ecx"

static long gettimer()
{
  return muldiv64(playpos+(dmaGetBufPos()-playpos%buflen+buflen)%buflen, 65536, ESSRate<<(ESSStereo+ESS16Bit));
}

void memsetd(void *, long, int);
#pragma aux memsetd parm [edi] [eax] [ecx] = "rep stosd"

static int ESSPlay(void *&buf, int &len)
{
  buf=(unsigned char*)dmaAlloc(len, dmabufsel);
  if (!buf)
    return 0;
  memsetd(buf, (plrOpt&PLR_SIGNEDOUT)?0:(plrOpt&PLR_16BIT)?0x80008000:0x80808080, len>>2);

  unsigned long t;

  ESS_Reset();
  ESSCommand(0xC6);                /* Enable Extended Mode commands  */

  Register_B1=ESSRead(0xB1);
  Register_B2=ESSRead(0xB2);

  ESSWrite(0xB8,4);                /* Set auto-initialize DAC transfer */

  t=ESSRead(0xA8) & 0xFC;
  if(ESSStereo) {
     t|=1;
  }
  else {
     t|=2;
  }
  ESSWrite(0xA8,t);

  if(ESSRate>22000) {
      t=-795500l/ESSRate;
  }
  else {
      t=(128-397700l/ESSRate);
  }
  ESSWrite(0xA1,t);                /* Set Sample Rate Clock Divider */

  t=-218293l/ESSRate;
  ESSWrite(0xA2,t);                /* Set Filter Clock Divider */
  if(ESSSignedOut) {
      ESSWrite(0xB6,0x0);
      ESSWrite(0xB7,0x71);
  }
  else {
      ESSWrite(0xB6,0x80);
      ESSWrite(0xB7,0x51);
  }

  if(ESSStereo) {
      t= 0x98;
  }
  else {
      t= 0xD0;
  }

  if(ESSSignedOut){
      t= t | 0x20;
  }

  if(ESS16Bit){
        t= t | 0x4;
  }

  ESSWrite(0xB7,t);
  ESSWrite(0xB1,0x50);             /* Set IRQ configuration register */

  t=ess_dma+(ess_dma<3);
  t=(5*t) | 0x50;
  ESSWrite(0xB2,t);                /* Set DRQ configuration register */

  dmaStart(ess_dma, buf, len, 0x58);

  ESSCommand(0xD1);                /* Enable voice to mixer */
  ESSWrite(0xB8, ESSRead(0xB8) | 1);  /* Start DMA */

  buflen=len;
  playpos=0;

  plrGetBufPos=dmaGetBufPos;
  plrGetPlayPos=dmaGetBufPos;
  plrAdvanceTo=advance;
  plrGetTimer=gettimer;

  return 1;
}

static void ESSStop()
{
  dmaStop();
  ESS_Reset();
  ESSCommand(0xC6);
  ESSWrite(0xB1,Register_B1);
  ESSWrite(0xB2,Register_B2);

  outp(0x0C,0);           /* Clear flip-flop. Otherwise
                               nobody will detect your ESS688 as
                               SB compatible card. The detection of
                               IRQ will fail.   */

  dmaFree(dmabufsel);
}


static int ESSInit(const deviceinfo &card)
{
  ess_port=card.port;
  if (!testPort()) {
    ess_port=-1;
    return 0;
  }
  ess_irq=card.irq;
  ess_dma=card.dma;

  plrSetOptions=ESSSetOptions;
  plrPlay=ESSPlay;
  plrStop=ESSStop;

  return 1;
}

static void ESSClose()
{
  plrPlay=0;
}

static int ESSDetect(deviceinfo &card)
{
  getcfg();
  if (card.port!=-1)
    ess_port=card.port;
  if (card.irq!=-1)
    ess_irq=card.irq;
  if (card.dma!=-1)
    ess_dma=card.dma;

  int i;
  if (ess_port==-1)
  {
    unsigned short ports[6]={0x220, 0x240, 0x260, 0x210, 0x230, 0x250};
    for (i=0; i<6; i++) {
      ess_port=ports[i];
      if (testPort())
        break;
    }
    if (i==6) {
      ess_port=-1;
      return 0;
    }
  }
  else
    if (!testPort())
      return 0;

  ESSCommand(0xC6);
  ess_irq=ESSRead(0xB1);

  switch (ess_irq&0xC) {
   case 0xC: ess_irq=10; break;
   case 0x0: ess_irq=9;  break;
   case 0x8: ess_irq=7;  break;
   case 0x4: ess_irq=5;
  }

  ess_dma=ESSRead(0xB2);
  switch (ess_dma&0xC) {
   case 0xC: ess_dma=3;  break;
   case 0x0: ess_dma=-1; break;
   case 0x8: ess_dma=1;  break;
   case 0x4: ess_dma=0;
  }

  if(ess_dma==-1) {
    return 0;
  }

  card.dev=&plrESSAudioDrive;
  card.port=ess_port;
  card.port2=-1;
  card.irq=ess_irq;
  card.irq2=-1;
  card.dma=ess_dma;
  card.dma2=-1;
  card.subtype=-1;
  card.mem=0;
  card.chan=2;
  return 1;
}

extern "C"
{
  sounddevice plrESSAudioDrive={SS_PLAYER, "ESS688 AudioDrive", ESSDetect, ESSInit, ESSClose};
  char *dllinfo = "driver _plrESSAudioDrive";
} 