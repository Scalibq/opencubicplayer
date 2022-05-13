// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Win32-Timer-dummy-functions
//
// revision history: (please note changes here)
//  -fd981021   Felix Domke <tmbinc@gmx.net>
//    -first release

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int tmGetTimer()
{
  return GetTickCount()*65.520;
}
#else
#error please compile under win32
#endif
