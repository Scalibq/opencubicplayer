// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
#ifdef CPWIN

//#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "binfstd.h"
#include "xmplay.h"
#include "gmdplay.h"
#include "mcp.h"
#include "ims.h"
#include "itplay.h"

void main(int argc, char **argv)
{
  imsinitstruct is;
  imsFillDefaults(is);
  is.bufsize=65536; // 1sec buffer
  is.pollmin=61440; // use polling only after 0.9375sec
  is.usersetup=0;
  int i;
  for (i=1; i<argc; i++)
    if (!stricmp(argv[i], "nosound"))
      is.usequiet=1;
  if (!imsInit(is))
  {
    printf("could not init\n");
    return;
  }


/*  gmdmodule mod;
  sbinfile fil;
  fil.open("h:\\sound\\mainmod.s3m", fil.openro);
  mpLoadS3M(mod, fil);
  fil.close();
  mpReduceSamples(mod);
  mpLoadSamples(mod);
  mpPlayModule(mod);

  while(!kbhit())
  {
   if(mcpIdle)
    mcpIdle();
  }

  mpStopModule();
  mpFree(mod); */

  xmodule mod;
  sbinfile fil;
  if (fil.open("303.xm", fil.openro))
  {
    printf("could not open\n");
    return;
  }
  if (xmpLoadModule(mod, fil))
  {
    printf("could not load\n");
    return;
  }
  fil.close();

  if (!xmpLoadSamples(mod))
  {
    printf("could not upload\n");
    return;
  }
  if (!xmpPlayModule(mod))
  {
    printf("could not play\n");
    return;
  }

  xmpSetEvPos(0, 0x20000, 2, 8);
  xmpSetEvPos(1, 0x20400, 2, 8);
  while (!kbhit())
  {
    int time1,time2,time3;
    if (mcpIdle)
      mcpIdle();
/*    while (inp(0x3da)&8);
    while (!(inp(0x3da)&8)); */
//    cerr << "\r" << hex << xmpGetRealPos();
    xmpGetEvPos(0, time1);
    xmpGetEvPos(1, time2);
/*    outp(0x3c8,0);
    outp(0x3c9,(time1<4096)?(63-(time1/64)):0);
    outp(0x3c9,0);
    outp(0x3c9,(time2<8192)?(63-(time2/128)):0); */
  }
  while (kbhit())
    getch();

  xmpStopModule();
  xmpFreeModule(mod);


  imsClose();
}
#else
#error Must compile under Win32.
#endif