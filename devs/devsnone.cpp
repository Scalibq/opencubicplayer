// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// NoSound sampler device (samples perfect noise-free 16bit silence)
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -added _dllinfo record

#include "imsdev.h"
#include "sampler.h"

extern "C" extern sounddevice smpNone;

static void ndSetOptions(int rate, int opt)
{
  if (rate<5000)
    rate=5000;

  if (rate>48000)
    rate=48000;

  smpRate=rate;
  smpOpt=opt;
}

static void ndSetSource(int)
{
}


static int getbufpos()
{
  return 0;
}


static int ndStart(void *, int &)
{
  smpGetBufPos=getbufpos;
  return 1;
}

static void ndStop()
{
}




static int ndInit(const deviceinfo &)
{
  smpSetOptions=ndSetOptions;
  smpSample=ndStart;
  smpStop=ndStop;
  smpSetSource=ndSetSource;

  smpSetOptions(65535, SMP_STEREO|SMP_16BIT);
  smpSetSource(SMP_LINEIN);

  return 1;
}

static void ndClose()
{
  smpSample=0;
}

static int ndDetect(deviceinfo &c)
{
  c.dev=&smpNone;
  c.port=-1;
  c.port2=-1;
  c.irq=-1;
  c.irq2=-1;
  c.dma=-1;
  c.dma2=-1;
  c.subtype=-1;
  c.mem=0;
  c.chan=2;
  return 1;
}

extern "C" {
  sounddevice smpNone={SS_SAMPLER, "Eternal Silence Recorder", ndDetect, ndInit, ndClose};
  char *dllinfo="driver _smpNone";
}

