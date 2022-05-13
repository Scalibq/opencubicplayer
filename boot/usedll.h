#ifndef __USEDLL_H
#define __USEDLL_H

#include "binfile.h"

#ifndef __PLINKMAN_H
#define __PLINKMAN_H

struct linkaddressinfostruct
{
  const char *module;
  const char *sym;
  int symoff;
  const char *source;
  int line;
  int lineoff;
};

struct linkinfostruct
{
  const char *name;
  const char *desc;
  unsigned long ver;
  unsigned long size;
  int handle;
};

#endif

struct dllordentry
{
  int ord;
  void *ptr;
};

struct dllnameentry
{
  const char *name;
  void *ptr;
};

#ifdef DOS32
char modlinkname[128];
int dllInit();
int dllLoad(binfile &);
int dllLoadLocals(binfile &f, dllnameentry *nameref, int nnameref, dllordentry *ordref, int nordref);
void dllFree(int);
void dllClose();
void *dllGetSymbol(int h, const char *sym);
void *dllGetSymbol(int h, int sym);
void *dllGetSymbol(const char *mod, const char *sym);
void *dllGetSymbol(const char *mod, int sym);
void *dllGetSymbol(const char *sym);
int dllGetLinkInfo(linkinfostruct &m, int first);
int dllCountLinks();
void dllGetAddressInfo(linkaddressinfostruct &a, void *ptr);
void memShowResults();
#endif

#endif