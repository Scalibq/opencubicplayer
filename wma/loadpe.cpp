/*

                wma4cp, PE-Loader
                                                (c) 1999 by Felix Domke
                                                licensed in the same way
                                                as the OpenCP.
*/
#include <stdio.h>
#include <stdlib.h>
#include "binfile.h"

#include "pe.h"

symbol_c symbol;
int WideToByte(char *, short *, int max=-1);

symbol_c::symbol_c()
{
  symbol=0;
}

symbol_c::~symbol_c()
{
  while (symbol)
  {
    symbol_s o=*symbol;
    delete[] symbol->name;
    delete[] symbol->dll;
    delete symbol;
    symbol=o.next;
  }
}

void *symbol_c::Get(const char *dll, const char *name)
{
  symbol_s *t=symbol;
  while (t)
  {
    if ((!strcmp(t->name, name)) && ( (!dll) || (!stricmp(t->dll, dll))))
      return t->addr;
    t=t->next;
  }
  printf("symbol %s.%s not found!\n", dll?dll:"*", name);
  return 0;
}

void *symbol_c::Get(const char *dll, int ord)
{
  symbol_s *t=symbol;
  if (!dll)
  {
    printf("ord and no dll spec.\n");
    return 0;
  }
  while (t)
  {
    if ((ord==t->ord) && (!stricmp(t->dll, dll)))
      return t->addr;
    t=t->next;
  }
  printf("symbol %s.%d not found!\n", dll, ord);
  return 0;
}

void symbol_c::Add(const char *name, const char *dll, void *addr, int ord)
{
  symbol_s *n=new symbol_s;
  n->next=symbol;
  symbol=n;

  n->name=new char[strlen(name)+1];
  strcpy(n->name, name);
  n->dll=new char[strlen(dll)+1];
  strcpy(n->dll, dll);
  n->ord=ord;
  n->addr=addr;
}

void symbol_c::DeleteSymbols(const char *dll)
{
  symbol_s *s=symbol;
  symbol_s **ps=&symbol;
  while (s)
  {
    if (!stricmp(s->dll, dll))
    {
      *ps=s->next;
      delete[] s->name;
      delete[] s->dll;
      delete s;
      s=*ps;
    }
    ps=&s->next;
    s=s->next;
  }
}

void *pe_c::ConvertPtr(int rva)
{
  for (int s=0; s<sections; s++)
  {
    if ((rva<(secmem[s].rva+secmem[s].size)) && (rva>=secmem[s].rva))
      return (void*)((rva-secmem[s].rva)+(int)secmem[s].base);
  }
  printf("RVA %08xh not found!\n", rva);
  return 0;
}

int pe_c::IsLoaded()
{
  return ok;
}

pe_c::pe_c(binfile &image)
{
  ok=0;
  // find PE header
  uint2 dosh=image.gets();
  if ((dosh!='MZ') && (dosh!='ZM'))
  {
    printf("inval. sig.\n");
    return;
  }
  int peoff=image[60].getl();
  uint4 peh=image[peoff].getl();
  if (peh!=0x00004550)
  {
    printf("inval. pe sig.\n");
    return;
  }

  if (!image.eread(&peheader, sizeof(peheader)))
  {
    printf("read error.\n");
    return;
  }
  if ((peheader.Machine&0x140)!=0x140)
  {
    printf("image is not for x86. (%x)\n", peheader.Machine);
    return;
  }
  if (peheader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER))
  {
    printf("PE error. (header size mismatch)\n");
    return;
  }
/*  if (peheader.Characteristics&IMAGE_FILE_BYTES_REVERSED_HI)
  {
    printf("wrong endianess.\n");
    return;
  } */

  if (!image.eread(&opheader, sizeof(opheader)))
  {
    printf("read error.\n");
    return;
  }
  if (opheader.Magic!=0x010b)
  {
    printf("op header magic wrong.\n");
  }

  sechdr=new IMAGE_SECTION_HEADER[sections=peheader.NumberOfSections];
  if (!image.eread(sechdr, sizeof(IMAGE_SECTION_HEADER)*sections))
  {
    printf("read error\n");
    return;
  }
  secmem=new secmem_s[sections];
  for (int section=0; section<sections; section++)
  {
    char sname[IMAGE_SIZEOF_SHORT_NAME+1];
    strncpy(sname, (char*)sechdr[section].Name, IMAGE_SIZEOF_SHORT_NAME);
    sname[IMAGE_SIZEOF_SHORT_NAME]=0;
    secmem[section].rva=sechdr[section].VirtualAddress;
    secmem[section].size=sechdr[section].SizeOfRawData;
    if (secmem[section].size<sechdr[section].Misc.VirtualSize)
      secmem[section].size=sechdr[section].Misc.VirtualSize;         // some linkers put 0??
    secmem[section].base=malloc(secmem[section].size);
/*    printf("S%2d: %-8s RVA 0x%08x, %d bytes, %x\n", section, sname, sechdr[section].VirtualAddress,
            sechdr[section].SizeOfRawData, secmem[section].base); */
    if (!secmem[section].base)
    {
      printf("malloc error in section %d.\n", section);
      return;
    }
    if (sechdr[section].PointerToRawData)
    {
      image.seek(sechdr[section].PointerToRawData);
      if (!image.eread(secmem[section].base, sechdr[section].SizeOfRawData))
      {
        printf("read error in section %d\n", section);
        return;
      }
    } else              // BSS
      memset(secmem[section].base, 0, sechdr[section].SizeOfRawData);
  }

  // process relocations
  char *rel=(char*)ConvertPtr(opheader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
  int pos=0;

  while (rel && (pos<opheader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size))
  {
    IMAGE_BASE_RELOCATION *h=(IMAGE_BASE_RELOCATION *)rel;
    void *base=ConvertPtr(h->VirtualAddress);
    if (!base)
    {
      printf("illegal relocation base.\n");
      return;
    }
    int numrel=(h->SizeOfBlock-sizeof(IMAGE_BASE_RELOCATION))/2;
    uint2 *re=(uint2*)(((char*)h)+sizeof(IMAGE_BASE_RELOCATION));
    for (int r=0; r<numrel; r++)
    {
      void *tb=((char*)base)+(re[r]&0xFFF);
      int type=re[r]>>12;
      switch (type)
      {
        case IMAGE_REL_BASED_ABSOLUTE:
          break;
        case IMAGE_REL_BASED_HIGHLOW:
          *((void**)tb)=ConvertPtr((*((long*)tb))-opheader.ImageBase);
          break;
        default:
          printf("unsupported relocation type %d\n", type);
      }
    }
    rel+=h->SizeOfBlock;
    pos+=h->SizeOfBlock;
  }

  // process imports

  IMAGE_IMPORT_DESCRIPTOR *iid=(IMAGE_IMPORT_DESCRIPTOR *)ConvertPtr(opheader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
  if (!iid)
  {
    printf("no imports.\n");
  } else
    while (iid->Characteristics)
    {
      name=(char*)ConvertPtr(iid->Name);
//      printf("%s (TS: %d)\n", name?name:"<illegal>", iid->TimeDateStamp);
      IMAGE_THUNK_DATA *oitd=(IMAGE_THUNK_DATA *)ConvertPtr((int)iid->OrdinalFirstThunk);
      IMAGE_THUNK_DATA *itd=(IMAGE_THUNK_DATA *)ConvertPtr((int)iid->FirstThunk);
      while (oitd->u1.AddressOfData)
      {
        IMAGE_IMPORT_BY_NAME *iin=(IMAGE_IMPORT_BY_NAME *)ConvertPtr((int)oitd->u1.AddressOfData);
        void *sym;
        if (((uint4)oitd->u1.AddressOfData)&(1<<31))
        {
          int ord=((uint4)oitd->u1.AddressOfData)&~(1<<31);
          printf("IMPORT BY ORDINAL: %s.%d\n", name, ord);
          sym=symbol.Get(name, ord);
          if (!sym)
            printf("symbol %d not found!!\n", ord);
        }
        else
        {
          sym=iin->Name?symbol.Get(name, (const char*)iin->Name):0;
          if (!sym)
            printf("symbol %s not found!!\n", iin->Name);
        }

        if (!sym) return;

        itd->u1.AddressOfData=(DWORD)sym;
        itd++;
        oitd++;
      }
      iid++;
    }

  // processing exports

  IMAGE_EXPORT_DIRECTORY *ied=(IMAGE_EXPORT_DIRECTORY *)ConvertPtr(opheader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
  ied->Name=(int)ConvertPtr(ied->Name);
  if (ied)
  {
    long *aof=(long*)ConvertPtr((int)ied->AddressOfFunctions);
    if (!aof)
    {
      printf("no aof\n");
      return;
    }
    int fnc;
    for (fnc=0; fnc<ied->NumberOfFunctions; fnc++)
      aof[fnc]=(long)ConvertPtr(aof[fnc]);
    long *aon=(long*)ConvertPtr((int)ied->AddressOfNames);
    short *aono=(short*)ConvertPtr((int)ied->AddressOfNameOrdinals);
    for (fnc=0; fnc<ied->NumberOfNames; fnc++)
    {
      aon[fnc]=(long)ConvertPtr(aon[fnc]);
//      printf("%s@%p\n", aon[fnc], aof[aono[fnc]]);
      symbol.Add((char*)aon[fnc], (char*)ied->Name, (void*)aof[aono[fnc]], -1);
    }
  }
  ok=!0;
}

pe_c::~pe_c()
{
  for (int i=0; i<sections; i++)
    free (secmem[i].base);
  delete[] secmem;
  symbol.DeleteSymbols(name);
  delete[] sechdr;
}

IMAGE_RESOURCE_DATA_ENTRY *pe_c::GetResource(const char *id, const char *type, int lang)
{
  IMAGE_RESOURCE_DIRECTORY *ird=(IMAGE_RESOURCE_DIRECTORY*)ConvertPtr(opheader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
  if (!ird) return 0;
  // get type-dir
  IMAGE_RESOURCE_DIRECTORY_ENTRY *irde=GetResourceEntry(ird, type);
  if (!irde) return 0;
  if (!irde->DataIsDirectory) return 0;
  irde=GetResourceEntry((IMAGE_RESOURCE_DIRECTORY *)(((char*)ird)+irde->OffsetToDirectory), id);
  if (!irde) return 0;
  if (irde->DataIsDirectory)
  {
    irde=GetResourceEntry((IMAGE_RESOURCE_DIRECTORY *)(((char*)ird)+irde->OffsetToDirectory), (const char*)lang);
    if (!irde) return 0;
    if (irde->DataIsDirectory) return 0;
    return ((IMAGE_RESOURCE_DATA_ENTRY*)(((char*)ird)+irde->OffsetToData));
  } else
    return ((IMAGE_RESOURCE_DATA_ENTRY*)(((char*)ird)+irde->OffsetToData));     // untested ...
}

IMAGE_RESOURCE_DIRECTORY_ENTRY *pe_c::GetResourceEntry(IMAGE_RESOURCE_DIRECTORY *ird, const char *id)
{
  if ((int)id==-1) return ((IMAGE_RESOURCE_DIRECTORY_ENTRY*)(ird+1));
  char name[100];
  for (int i=0; i<ird->NumberOfNamedEntries+ird->NumberOfIdEntries; i++)
  {
    IMAGE_RESOURCE_DIRECTORY_ENTRY *irde=((IMAGE_RESOURCE_DIRECTORY_ENTRY*)(ird+1))+i;
    if (irde->NameIsString)
      WideToByte(name, (short*)(((char*)ird)+irde->NameOffset));

    if ((((int)name&0xFFFF0000) && !stricmp(name, id))||((int)id==irde->Id))
      return irde;
  }
  return 0;
}

void *pe_c::GetResourcePtr()
{
  return ConvertPtr(opheader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
}

int WideToByte(char *src, short *dst, int max)
{
  if (max==-1) max=strlen(src);
  int c=max;
  while (max--)
    *src++=*dst++;
  return c;
}

int pe_c::GetRVA(void *ptr)
{
  for (int s=0; s<sections; s++)
  {
    int reladr=((char*)ptr)-((char*)secmem[s].base);
    if ( (reladr>0) && (reladr<secmem[s].size))
      return reladr+secmem[s].rva;
  }
  return 0;
}
