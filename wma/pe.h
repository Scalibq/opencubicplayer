#ifndef __PE_H
#define __PE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "binfile.h"

class symbol_c
{
  struct symbol_s
  {
    char *name;
    char *dll;
    void *addr;
    int ord;
    symbol_s *next;
  } *symbol;
public:
  void Add(const char *name, const char *dll, void *addr, int ord=-1);
  void *Get(const char *dll, const char *name);
  void *Get(const char *dll, int ord);
  void DeleteSymbols(const char *dll);
  symbol_c();
  ~symbol_c();
};

class pe_c
{
  struct secmem_s
  {
    void *base;
    int rva;
    int size;
  } *secmem;
  int sections;
  IMAGE_FILE_HEADER peheader;
  IMAGE_OPTIONAL_HEADER opheader;
  IMAGE_SECTION_HEADER *sechdr;

  int ok;
  char *name;

  IMAGE_RESOURCE_DIRECTORY_ENTRY *GetResourceEntry(IMAGE_RESOURCE_DIRECTORY *, const char *);

public:
  void *GetResourcePtr();
  void *ConvertPtr(int rva);
  int GetRVA(void *);
  int IsLoaded();
  IMAGE_RESOURCE_DATA_ENTRY *GetResource(const char *, const char *, int lang=-1);
  pe_c(binfile &);
  ~pe_c();
};

#endif
