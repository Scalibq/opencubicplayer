#ifndef __BINFCON_H
#define __BINFCON_H

#include "binfile.h"

class consolebinfile : public binfile
{
protected:
  virtual errstat rawclose();
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  consolebinfile();
  virtual ~consolebinfile();
};

class iconsolebinfile : public binfile
{
protected:
  virtual errstat rawclose();
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  iconsolebinfile();
  virtual ~iconsolebinfile();
};

class oconsolebinfile : public binfile
{
protected:
  virtual errstat rawclose();
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  oconsolebinfile();
  virtual ~oconsolebinfile();
};

class econsolebinfile : public binfile
{
protected:
  virtual errstat rawclose();
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  econsolebinfile();
  virtual ~econsolebinfile();
};

#endif
