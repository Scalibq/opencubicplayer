// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// IMS player replacement routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include "player.h"
#include "imsrtns.h"

void (*plrIdle)();

int plrRate;
int plrOpt;
int (*plrPlay)(void *&buf, int &len);
void (*plrStop)();
void (*plrSetOptions)(int rate, int opt);
int (*plrGetBufPos)();
int (*plrGetPlayPos)();
void (*plrAdvanceTo)(int pos);
long (*plrGetTimer)();
int (*plrProcessKey)(unsigned short);

static unsigned char stereo;
static unsigned char bit16;
static unsigned char reversestereo;
static unsigned char signedout;
static unsigned long samprate;
static unsigned char *plrbuf;
static unsigned long buflen;


void plrGetRealMasterVolume(int &, int &)
{
}

void plrGetMasterSample(short *, int, int, int)
{
}


int plrOpenPlayer(void *&buf, int &len, int bufl)
{
  if (!plrPlay)
    return 0;

  stereo=!!(plrOpt&PLR_STEREO);
  bit16=!!(plrOpt&PLR_16BIT);
  reversestereo=!!(plrOpt&PLR_REVERSESTEREO);
  signedout=!!(plrOpt&PLR_SIGNEDOUT);
  samprate=plrRate;

  int dmalen=umuldiv(samprate<<(stereo+bit16), bufl, 65536)&~15;

  plrbuf=0;
  if (!plrPlay(*(void**)&plrbuf, dmalen))
    return 0;

  buflen=dmalen>>(stereo+bit16);
  buf=plrbuf;
  len=buflen;

  return 1;
}

void plrClosePlayer()
{
  plrStop();
}