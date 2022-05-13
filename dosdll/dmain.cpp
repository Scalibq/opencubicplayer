// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// LibMain function which is called upon DLL loading
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern "C" void __InitRtns(int);
#pragma aux __InitRtns "*"
extern "C" void __FiniRtns(int,int);
#pragma aux __FiniRtns "*"
extern "C" unsigned __dll_terminate();
extern "C" unsigned __dll_initialize();

extern "C"  unsigned __LibMain(unsigned hmod, unsigned termination, void *x)
{
  switch(termination)
  {
    case(DLL_PROCESS_ATTACH):
      __InitRtns(255);
      return __dll_initialize();
    case(DLL_PROCESS_DETACH):
      unsigned rc=__dll_terminate();
      __FiniRtns(0, 255);
      return rc;
  }
  return 0;
}
#pragma aux __LibMain "*" parm routine []

#else

extern "C" void __InitRtns(int);
#pragma aux __InitRtns "*"
extern "C" void __FiniRtns(int,int);
#pragma aux __FiniRtns "*"
extern "C" unsigned __dll_terminate();
extern "C" unsigned __dll_initialize();

extern "C" unsigned __LibMain(unsigned, unsigned termination)
{
  if (termination)
  {
    unsigned rc=__dll_terminate();
    __FiniRtns(0, 255);
    return rc;
  }
  __InitRtns(255);
  return __dll_initialize();
}
#pragma aux __LibMain "*" parm caller []
#endif
