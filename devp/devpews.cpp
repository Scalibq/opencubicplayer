// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Player device for Terratec AudioSystem EWS64L or XL CS4236 Codec
//
// revision history: (please note changes here)
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -added _dllinfo record
//    -first release

#include <conio.h>
#include <stdio.h>
#include "dma.h"
#include "player.h"
#include "imsrtns.h"
#include "imsdev.h"

extern "C" extern sounddevice plrEWS64;

static long ewsPort;
static long ewsPort2;
static short ewsDMA;
static unsigned char  ewsReverb;
static unsigned char  ewsSurround;
static unsigned char  oldmuteleft, oldmuteright, oldmutemono;
static unsigned char  ewsSpeed;
static long           ewsBPS;
static long           playpos;
static long           buflen;
static __segment      dmabufsel;

// define PNPDEBUG

static unsigned char ewsinp(unsigned short r)
{
  return inp(ewsPort+r);
}

static void ewsoutp(unsigned short r, unsigned char v)
{
  outp(ewsPort+r, v);
}

static unsigned char ewsin(unsigned char r)
{
  while (ewsinp(0)&0x80);
  ewsoutp(0, r);
  return ewsinp(1);
}

static void ewsout(unsigned char r, unsigned char v)
{
  while (ewsinp(0)&0x80);
  ewsoutp(0, r);
  ewsoutp(1, v);
}



static void waste_time(char ficken)
{
  for (int j=0;j<ficken;j++)
    for (int i=0;i<0x180;i++)
      inp(0x21);
}

static char inpnp(char reg, short rdp)
{
  outp(0x279,reg);
  return inp(rdp);
}

static void outpnp(char reg, char val)
{
  outp(0x279,reg);
  outp(0xa79,val);
}

static void outsamc(char val)
{
  if (ewsPort2!=-1)
    outp(ewsPort2+1,val);
}

static void outsamd(char val)
{
  if (ewsPort2!=-1)
    outp(ewsPort2,val);
}


static char detectews(short rdp)
{
#ifdef PNPDEBUG
  printf("testing rdp 0x%X... ", rdp);
#endif

  outpnp(2,4);


  char csn=0;
  char ok=0;
  char funden=0;

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

#ifdef PNPDEBUG
    printf("!");
#endif

    outpnp(6,++csn);
    outpnp(3,csn);
    if (vall==0x36a8630e)
    {

      outpnp(7,0);
      ewsPort=(inpnp(0x60,rdp)<<8)|inpnp(0x61,rdp);
      ewsDMA=inpnp(0x74,rdp)&0x07;
      outpnp(7,4);
      ewsPort2=(inpnp(0x60,rdp)<<8)|inpnp(0x61,rdp);
#ifdef PNPDEBUG
      printf("found ews");
#endif
      funden=1;

    }
  }

#ifdef PNPDEBUG
  printf("\n");
#endif

  return csn;

}



static char ewsGetCfg()
{
  // send pnp init key

  ewsPort=-1;
  ewsDMA=-1;
  ewsPort2=-1;

  unsigned short keyval=0x006A;
  int i;
#ifdef PNPDEBUG
  printf("\nsending pnp wakeup key\n");
#endif
  outp(0x279,0);
  outp(0x279,0);
  for (i=0; i<32; i++)
  {
    outp(0x279,keyval&0xff);
    keyval=(keyval&0xff)|((keyval&0xfe)<<7);
    keyval^=keyval<<8;
    keyval=keyval>>1;
  }


#ifdef PNPDEBUG
  printf("starting search\n");
#endif


  if (!detectews(0x20f) && !detectews(0x27f) && !detectews(0x213));

#ifdef PNPDEBUG
  printf("ending isolation sequence\n");
#endif
  outpnp(2,2);

#ifdef PNPDEBUG
  printf("the eagle has landed, return status 0x%i\n", ewsPort);
#endif

  return (ewsPort!=-1);
}





static void ewsSetOptions(int rate, int opt)
{
  char stereo=!!(opt&PLR_STEREO);
  char bit16=!!(opt&PLR_16BIT);
  if (bit16)
    opt|=PLR_SIGNEDOUT;
  else
    opt&=~PLR_SIGNEDOUT;

  if (rate>48000)
    rate=48000;

  static unsigned short rates[16]=
  { 8000, 5513,16000,11025,27429,18900,32000,22050,
   54857,37800,64000,44100,48000,33075, 9600, 6615};

  unsigned char spd=10;
  int i;
  for (i=0; i<16; i++)
    if ((rates[i]>=rate)&&(rates[i]<rates[spd]))
      spd=i;

  rate=rates[spd];

  ewsSpeed=spd|(bit16?0x40:0x00)|(stereo?0x10:0x00);

  ewsBPS=rate<<(stereo+bit16);
  plrRate=rate;
  plrOpt=opt;
}


static void advance(int pos)
{
  playpos+=(pos-(playpos%buflen)+buflen)%buflen;
}

static long gettimer()
{
  return imuldiv(playpos+(dmaGetBufPos()-playpos%buflen+buflen)%buflen, 65536, ewsBPS);
}

static int ewsPlay(void *&buf, int &len)
{
  buf=(unsigned char*)dmaAlloc(len, dmabufsel);
  if (!buf)
    return 0;
  memsetd(buf, (plrOpt&PLR_SIGNEDOUT)?0:(plrOpt&PLR_16BIT)?0x80008000:0x80808080, len>>2);

  ewsout(0x48,ewsSpeed);
  ewsin(0x08);

  ewsoutp(0,0x0b);
  while (ewsinp(1)&0x20)
    ewsoutp(0, 0x0B);

  plrGetBufPos=dmaGetBufPos;
  plrGetPlayPos=dmaGetBufPos;
  plrAdvanceTo=advance;
  plrGetTimer=gettimer;

  dmaStart(ewsDMA, buf, len, 0x58);

  ewsout(0x0F, 0xF0);
  ewsout(0x0E, 0xFF);

  oldmuteleft=ewsin(0x06);
  oldmuteright=ewsin(0x07);
  oldmutemono=ewsin(0x1A);
  ewsout(0x1A, oldmutemono&~0x40);
  ewsout(0x06, oldmuteleft&~0x80);
  ewsout(0x07, oldmuteright&~0x80);

  ewsout(0x09, 1);
  ewsin(9);

  buflen=len;
  playpos=-buflen;

  return 1;
}

static void ewsStop()
{
  ewsout(0x06, oldmuteleft);
  ewsout(0x07, oldmuteright);
  ewsout(0x1A, oldmutemono);
  ewsout(0x0A, 0);
  ewsout(0x09, 0);
  dmaStop();
  dmaFree(dmabufsel);
}


static int ewsInit(const deviceinfo &card)
{
  if (card.subtype)
    return 0;

  ewsPort=card.port;
  ewsPort2=card.port2;
  ewsDMA=card.dma;

  ewsReverb=card.opt&0xff;
  ewsSurround=card.opt>>8;

  // switch to mode 0
  ewsoutp(0, 0x4C);
  ewsoutp(1, 0);

  // test if card ready
  if (ewsinp(0)&0x80)
    return 0;

  // stop any playing
  ewsout(0x09, 0);
  ewsin(0x09);

  // set up synthesizer

  outsamc(0x3f);                              // switch to uart mode

  outsamc(0x65); outsamd(0x7f);               // enable post fx for audio in
  outsamc(0x27); outsamd(ewsReverb);          // set reverb level
  outsamc(0x30); outsamd(ewsSurround);        // set surround level

  plrSetOptions=ewsSetOptions;
  plrPlay=ewsPlay;
  plrStop=ewsStop;

  return 1;
}

static void ewsClose()
{
  plrPlay=0;
  ewsout(0x0a,0);
  ewsout(0x49, 0);
  ewsout(0x4C, 0);
  ewsin(0x0c);
}


static int ewsDetect(deviceinfo &card)
{

  if (card.port==-1 || card.dma==-1)
  {
     if (!(ewsGetCfg() && ewsPort!=-1 && ewsDMA!=-1))
     {
#ifdef PNPDEBUG
       printf("ok, Houston here, trying to shut down\n");
#endif
       return 0;
     }
#ifdef PNPDEBUG
       printf("ok, Houston here, we got your message - now we're starting the show.\n");
#endif
     if (card.port!=-1)
       ewsPort=card.port;
     if (card.dma!=-1)
       ewsDMA=card.dma;
     if (card.port2!=-1)
       ewsPort2=card.port2;
  }
  else
  {
#ifdef PNPDEBUG
    printf("This is ground control. Everything has been overridden manually.\n");
#endif
    ewsPort=card.port;
    ewsDMA=card.dma;
    ewsPort2=card.port2;
  }

  // switch to mode 3
  ewsoutp(0,0x4c); ewsoutp(1, 0xe0);
  ewsout(0x09,0);

  // test for cs4236 or above
  if ((ewsin(12)&0x0f)!=0x0a || (ewsin(25)&0x1f) != 0x03)
    return 0;

  // switch back to mode 0
  ewsout(0x4c,0);
  ewsin(0x0c);

  while (ewsinp(0)&0x80);

  card.dev=&plrEWS64;
  if (card.port==-1)
    card.port=ewsPort;
  if (card.dma==-1)
    card.dma=ewsDMA;
  if (card.port2==-1)
    card.port2=ewsPort2;

  card.subtype=0;
  card.irq=-1;
  card.irq2=-1;
  card.dma2=-1;
  card.mem=0;
  card.chan=2;

  return 1;
}



#include "devigen.h"
#include "psetting.h"

static unsigned long ewsGetOpt(const char *sec)
{
  unsigned long opt=0;
  opt  = (unsigned char)(cfGetProfileInt(sec, "reverb", 0, 10));
  opt |= (unsigned char)(cfGetProfileInt(sec, "surround", 0, 10))<<8;
  return opt;
}

extern "C" {
  sounddevice plrEWS64={SS_PLAYER, "AudioSystem EWS64L/XL Codec", ewsDetect, ewsInit, ewsClose};
  devaddstruct plrEWS64Add = {ewsGetOpt, 0, 0, 0};
  char *dllinfo = "driver _plrEWS64; addprocs _plrEWS64Add";
}
