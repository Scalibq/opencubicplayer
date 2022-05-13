#ifndef __PLINKMAN_H
#define __PLINKMAN_H

struct linkinfostruct
{
  const char *name;
  const char *desc;
  unsigned long ver;
  unsigned long size;
  int handle;
};

struct linkaddressinfostruct
{
  const char *module;
  const char *sym;
  int symoff;
  const char *source;
  int line;
  int lineoff;
};

int lnkInit();
void lnkClose();
void lnkFree(int h);
int lnkLink(const char *files);
void *lnkGetSymbol(const char *name);
int lnkCountLinks();
int lnkGetLinkInfo(linkinfostruct &, int first);
void lnkGetAddressInfo(linkaddressinfostruct &a, void *ptr);
char *lnkReadInfoReg(const char *key);
char *lnkReadInfoReg(const char *name, const char *key);

#endif
