// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// IMS mixer replacement routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include "mcp.h"
#include "mix.h"

void mixGetMasterSample(short *, int, int, int)
{
}

int mixMixChanSamples(int *, int, short *, int, int, int)
{
  return 0;
}

int mixGetChanSample(int, short *, int, int, int)
{
  return 0;
}

void mixGetRealVolume(int, int &, int &)
{
}

void mixGetRealMasterVolume(int &, int &)
{
}

void mixSetAmplify(int)
{
}

int mixInit(void (*)(int, mixchannel &, int), int masterchan, int, int)
{
  mcpGetRealVolume=mixGetRealVolume;
  mcpGetChanSample=mixGetChanSample;
  mcpMixChanSamples=mixMixChanSamples;
  if (masterchan)
  {
    mcpGetRealMasterVolume=mixGetRealMasterVolume;
    mcpGetMasterSample=mixGetMasterSample;
  }

  return 1;
}

void mixClose()
{
}