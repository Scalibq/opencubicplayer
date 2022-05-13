// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// new realloc_ function (the Watcom CLib realloc_ destroys the heap)
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <malloc.h>
#include <string.h>

extern "C" void *realloc(void *p, size_t size)
{
  if (!size)
  {
    delete (char*)p;
    return 0;
  }

  if (!p)
    return new char[size];

  size_t oldsize=_msize(p);
  if (_expand(p, size))
    return p;
  _expand(p, oldsize);

  void *n=new char [size];
  if (!n)
    return 0;
  memcpy(n, p, size);
  delete (char*)p;
  return n;
}