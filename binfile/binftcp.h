#ifndef __BINFTCP_H
#define __BINFTCP_H

#include "binfile.h"

class tcpbinfile;
class tcplistener;

class tcplistener
{
  friend tcpbinfile;

private:
  intm handle;
  unsigned long noblock;
public:
  tcplistener();
  ~tcplistener();
  errstat open(uint2 port, intm conbuf);
  errstat close();
  boolm setblock(boolm);
  boolm getblock();
};

class tcpbinfile : public binfile
{
  friend tcplistener;

private:
  static intm initcount;
  intm handle;
  boolm closed;
  boolm wclosed;
  unsigned long noblock;
  int lingertime;

  static boolm globalinit();
  static void globaldone();

protected:
  virtual errstat rawclose();
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawpeek(void *buf, binfilepos len);
  virtual binfilepos rawwrite(const void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  tcpbinfile();
  virtual ~tcpbinfile();

  errstat open(uint4 addr, uint2 port);
  errstat open(const char *addr, uint2 port);
  errstat open(tcplistener &l);

  uint4 getremoteaddr();
  uint4 getlocaladdr();
  uint2 getremoteport();
  uint2 getlocalport();

  static char *addrtoa(char *buf, uint4 addr);
  static char *addrtoa(char *buf, uint4 addr, uint2 port);
  static uint4 atoaddr(const char *buf);
  static uint2 atoport(const char *buf, uint2 port);
};

#endif
