#ifndef __ASFREAD_HPP
#define __ASFREAD_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include "binfile.h"


class asfreader : public binfile
{
protected:
  binfile *src;
  WAVEFORMATEX *format;
  int ok;
  int bread, rbread;
  int data;
  int btg;
  int blen, packets, npackets;
  int error;

  virtual errstat rawclose();
  binfilepos rawseek(binfilepos);
  virtual binfilepos rawread(void *, binfilepos);
  virtual binfilepos rawioctl(intm, void *, binfilepos);

  void ReadChunk();

public:
  errstat open(binfile &);

  enum ioctlenum
  {
    ioctlgetformat=4096,
  };
};

#endif
