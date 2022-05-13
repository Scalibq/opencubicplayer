// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Low-level loading/handling functions for OS/2 LinExe DLLs
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981118   Felix Domke <tmbinc@gmx.net>
//    -made the "silly memory debugger", with more hacks than real code,
//     but with this "tool" we do not have to SEARCH for memory leaks, we
//     can FIX them instead ;)
//  -kb981201   Tammo Hinrichs <opencp@gmx.net>
//    -added realloc_ to memory debugger
//    -added some verbosity
//    -turned NULL free warnings off (it's ok)
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile
//  -ryg981506  Fabian Giesen <fabian@jdcs.su.nw.schule.de>
//    -added DLL autoloader (DOS only)

#define NO_CPDLL_IMPORT
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "plinkman.h"
#include "usedll.h"
#include "err.h"

#define MAXDLLS 150

#define GRAN 32

unsigned int calldll(void *, unsigned int, unsigned int);
#pragma aux calldll parm [eax] [ebx] [ecx] value [eax] = "pushad" "push ds" "push es" "push ecx" "push ebx" "call eax" "add esp,8" "pop es" "pop ds" "mov [esp+28],eax" "popad" "cld"

struct debmodposstruct
{
  void *off;
  long len;
  short mod;
};


struct dllstruct
{
  int local;
  int refcount;
  int userrefcount;
  const char *name;
  const char *desc;
  unsigned long ver;
  unsigned long size;
  void *data;
  void *base;
  int nordentries;
  int nnameentries;
  dllordentry *ordentries;
  dllnameentry *nameentries;
  char *namebuf;
  void *entrypoint;
  int nrefs;
  int *refs;

  int debnsyms;
  int debnlines;
  int debnmodpos;
  char *debnamebuf;
  char **debmods;
  dllnameentry *debsyms;
  dllordentry *deblines;
  debmodposstruct *debmodpos;
};

struct lehdr
{
  unsigned short sig;
  unsigned char byteorder,wordorder;
  unsigned long formatlevel;
  unsigned short cputype;
  unsigned short target;
  unsigned long modver;
  unsigned long modtype;
  unsigned long npages;
  unsigned long ics,ieip;
  unsigned long iss,iesp;
  unsigned long pagesize;
  unsigned long lastpage;
  unsigned long fixsize,fixchk;
  unsigned long loadsize,loadchk;
  unsigned long objtaboff,nobjs,pagemapoff,itedataoff;
  unsigned long resoff,resentries;
  unsigned long resnamesoff;
  unsigned long entryoff;
  unsigned long moddiroff,moddirentries;
  unsigned long fixpageoff,fixrecoff;
  unsigned long impnameoff,impnmods,impprocoff;
  unsigned long pagechkoff;
  unsigned long pageoff;
  unsigned long prenpages;
  unsigned long nresnamesoff,nresnamessize,nresnamechk;
  unsigned long ids;
  unsigned long deboff;
  unsigned long deblen;
  unsigned long preinsnpages;
  unsigned long deminsnpages;
  unsigned long extraheap;
  unsigned long res[5];
  unsigned short devid,ddkver;
};

static dllstruct *(dlls[MAXDLLS]);

static int getdllhandle(const char *name)
{
  int i;
  for (i=0; i<MAXDLLS; i++)
    if (dlls[i])
      if (!stricmp(dlls[i]->name, name))
        return i;
  return -1;
}

static int getpdllhandle(const char *name)
{
  int i;
  for (i=0; i<MAXDLLS; i++)
    if (dlls[i])
      if (!strnicmp(dlls[i]->name, name+1, *name))
        return i;
  return -1;
}

void *getordentry(dllstruct &d, int ord)
{
  int i;
  for (i=0; i<d.nordentries; i++)
    if (d.ordentries[i].ord==ord)
      return d.ordentries[i].ptr;
  return 0;
}

#ifdef DEBUG

void *GetReturnAddress12();
#pragma aux GetReturnAddress12="mov eax, [esp+12]" value [eax];
void *GetReturnAddress8();
#pragma aux GetReturnAddress8="mov eax, [esp+8]" value [eax];
                        /* mir ist v”llig klar das diese beiden routinchen
                           v”lliger mist sind und das ganze damit v”llig
                           imkompatibel ist. ist mir egal, weil ich momentan
                           keine andere, einfach m”glichkeit weiss...

                           die my..-sachen in asm schreiben hatte ich kein
                           bock.
                           ich hoffe nurmal, das das auch mit watcom11
                           geht, habs jetzt nur mit 106 ausprobiert...
                        */

// 'W?$nan(uipnv)pnv'           these are unchecked atm, no mood, no time: no need....
// 'W?$nwn(uipnv)pnv'


struct allocatedmemorystruct
{
  int used, free;
  struct
  {
    void *ptr;
    size_t size;
  } memory;
  struct
  {
    char *where;
  } caller;
} allocated[65536];             // much, much.. i know. maybe somone can write a... ;)

int allocations=0;

void memGetAddress(char *dst, void *ptr)
{
  char buf[100];
  linkaddressinfostruct a;
  lnkGetAddressInfo(a, ptr);
  strcpy(dst, a.module);
  strcat(dst, ".");
  strcat(dst, a.sym);
  strcat(dst, "+0x");
  ultoa(a.symoff, buf, 16);
  strcat(dst, buf);
  strcat(dst, " (");
  strcat(dst, a.source);
  strcat(dst, ".");
  ultoa(a.line, buf, 10);
  strcat(dst, buf);
  strcat(dst, "+0x");
  ultoa(a.lineoff, buf, 16);
  strcat(dst, buf);
  strcat(dst, ")");
}

void memAllocate(void *ptr, int size, void *caller)
{
  allocations++;
  if(!ptr)
    return;                     // maybe some error-correction/bla here ;)
  for(int i=0; i<65536; i++)
    if((!allocated[i].used)||((allocated[i].memory.ptr==ptr)&&allocated[i].free))
      break;
  if(i==65536)
  {
    printf("[mem: struct is full]\n");
    return;
  }
  if(allocated[i].used)
    delete[] allocated[i].caller.where;       // so at least this module shouldn't have a memory leak ;)
  allocated[i].used=!0;
  allocated[i].free=0;
  allocated[i].memory.ptr=ptr;
  allocated[i].memory.size=size;
  char who[1024];                       // this IS too much. but - wc?
  memGetAddress(who, caller);
  allocated[i].caller.where=new char[strlen(who)+1];
  strcpy(allocated[i].caller.where, who);
}

void memFree(void *ptr, void *caller)
{
  char who[0x100];
  // Change: freeing of NULL is ok
  if(!ptr)
    return;
  for(int i=0; i<65536; i++)
    if((allocated[i].used)&&(allocated[i].memory.ptr==ptr))
    {
      if(allocated[i].free)
      {
        char who[0x100];
        memGetAddress(who, caller);
        printf("[mem: (%s): memory already freed. (%x)]\n", who, ptr);
        printf("[allocated by %s]\n", allocated[i].caller.where);
      }
      allocated[i].free=!0;
      return;
    }
  memGetAddress(who, caller);
  printf("[mem: (%s): junk (%x) freed]\n", who, ptr);
}

void memShowResults()
{
  printf("memory leak report\n\n");

  int leaktotal=0, leaks=0;

  for(int i=0; i<65536; i++)
    if(allocated[i].used&&(!allocated[i].free))
    {
      printf("size:%06d where:%08x who:%s\n", allocated[i].memory.size, allocated[i].memory.ptr, allocated[i].caller.where);
      leaktotal+=allocated[i].memory.size;
      leaks++;
      delete[] allocated[i].caller.where;
      allocated[i].used=0;                      // jic
    }
  printf("    ... making a total of %d errors, resulting in %d lost bytes.\n", leaks, leaktotal);
  printf("    ... while %d allocations during the whole execution.\n", allocations);
}

void *mymalloc(size_t __size)
{
  void *res=malloc(__size);
  memAllocate(res, __size, (void*)GetReturnAddress12());
  return(res);
}

void myfree(void *__ptr)
{
  memFree(__ptr, (void*)GetReturnAddress8());
  free(__ptr);
}

void *mynew(size_t __size)
{
  char *res=new char[__size];
  memAllocate(res, __size, (void*)GetReturnAddress12());
  return(res);
}

void mydelete(void *__ptr)
{
  memFree(__ptr, (void*)GetReturnAddress8());
  delete (char*)__ptr;
}

void *myrealloc(void *__ptr, size_t __size)
{
  if (__ptr)
    memFree(__ptr, (void*)GetReturnAddress8());
  void *res=realloc(__ptr,__size);
  memAllocate(res, __size, (void*)GetReturnAddress12());
  return(res);
}

struct hookstruct
{
  char *name;
  void *fnc;
} hook[]={{"malloc_", mymalloc},
          {"free_",   myfree},
          {"realloc_",   myrealloc},
          {"W?$nwn(ui)pnv", mynew},             // void * operator new(unsigned int)
          {"W?$nan(ui)pnv", mynew},             // void * operator new[](unsigned int) (it's actually the same.)
          {"W?$dln(pnv)v", mydelete},           // void operator delete(void *)
          {"W?$dan(pnv)v", mydelete}            // void operator delete[](void *)
                                     };

const int hooks=sizeof(hook)/sizeof(*hook);

void *gethook(char const *name, int len)
{
  for(int i=0; i<hooks; i++)
    if(!strnicmp(hook[i].name, name, len))
      return hook[i].fnc;
  return 0;
}

#endif // DEBUG

void *getnameentry(dllstruct &d, const char *name)
{
#ifdef DEBUG
  void *hook=gethook(name, strlen(name));
  if(hook)
    return hook;
#endif

  int i;
  for (i=0; i<d.nnameentries; i++)
  {
    if (!strcmp(d.nameentries[i].name,name))
      return d.nameentries[i].ptr;
  }
  return 0;
}

void *getpnameentry(dllstruct &d, const char *name)
{
#ifdef DEBUG
  void *hook=gethook(name+1, *name);
  if(hook)
    return hook;
#endif

  int i;
  for (i=0; i<d.nnameentries; i++)
    if (!strncmp(d.nameentries[i].name,name+1,*name))
      return d.nameentries[i].ptr;
  return 0;
}


static int readentries(dllstruct *dll, char *entp, long (*objtab)[6], char *namep, char *nres)
{
  int curord=1;
  int nordentries=0;
  while (1)
  {
    if (!*entp)
      break;
    if (!entp[1])
    {
      curord+=*entp;
      entp+=2;
      continue;
    }
    int tl=*entp++;
    unsigned char fl=*entp++;
    unsigned short curobj=*(short*)entp;
    entp+=2;
    int i;
    for (i=0; i<tl; i++)
    {
      if (!objtab[curobj-1][5])
        return errFormMiss;
      entp++;
      dll->ordentries[nordentries].ptr=(void*)(*(long*)entp+objtab[curobj-1][5]);
      dll->ordentries[nordentries++].ord=curord++;
      entp+=4;
    }
  }

  int nnameentries=0;
  int nameslen=0;
  while (1)
  {
    if (!*namep)
      break;
    memcpy(dll->namebuf+nameslen, namep+1, *namep);
    if (*(short*)(namep+*namep+1))
    {
      dll->nameentries[nnameentries].name=dll->namebuf+nameslen;
      dll->nameentries[nnameentries++].ptr=getordentry(*dll, *(short*)(namep+*namep+1));
    }
    else
      dll->name=dll->namebuf+nameslen;
    nameslen+=1+*namep;
    dll->namebuf[nameslen-1]=0;
    namep+=*namep+3;
  }

  while (1)
  {
    if (!*nres)
      break;
    memcpy(dll->namebuf+nameslen, nres+1, *nres);
    if (*(short*)(nres+*nres+1))
    {
      dll->nameentries[nnameentries].name=dll->namebuf+nameslen;
      dll->nameentries[nnameentries++].ptr=getordentry(*dll, *(short*)(nres+*nres+1));
    }
    else
      dll->desc=dll->namebuf+nameslen;
    nameslen+=1+*nres;
    dll->namebuf[nameslen-1]=0;
    nres+=*nres+3;
  }

  return errOk;
}


static int readdebug(dllstruct *dll, binfile &f, long (*objtab)[6])
{
  if (f.gets()!=0x0043)
    return errOk;
  if (f.getl()!=0x00505043)
    return errOk;
  if (f.getl()!=0x00020001)
    return errOk;
  long len=f.length()-f.tell();
  char *buf=new char [len];
  if (!buf)
    return errAllocMem;
  f.read(buf, len);
  int nnames=0;
  int nameslen=0;
  int nmods=0;
  int nsyms=0;
  int nmodpos=0;
  int nlines=0;
  char *modtab=buf+(*(long*)(buf+0));
  char *symtab=buf+(*(long*)(buf+4));
  char *modpos=buf+(*(long*)(buf+8));
  char *modposend=buf+(*(long*)(buf+12));
  char *modp=modtab;
  while (modp<symtab)
  {
    if (*(short*)(modp+18))
    {
      long oo=*(long*)(modp+14);
      char *lines=buf+*(long*)(buf+oo);
      char *lineend=buf+*(long*)(buf+oo+4);
      while (lines<lineend)
      {
        nlines+=*(short*)(lines+4);
        lines+=6+6**(short*)(lines+4);
      }
    }
    nameslen+=modp[20]+1;
    modp+=21+modp[20];
    nmods++;
  }
  char *symp=symtab;
  while (symp<modpos)
  {
    nameslen+=symp[9]+1;
    symp+=10+symp[9];
    nsyms++;
  }
  char *modposp=modpos;
  while (modposp<modposend)
  {
    nmodpos+=*(short*)(modposp+6);
    modposp+=8+6**(short*)(modposp+6);
  }

  dll->debnsyms=nsyms;
  dll->debnlines=nlines;
  dll->debnmodpos=nmodpos;
  dll->debnamebuf=new char [nameslen];
  dll->debmods=new char *[nmods];
  dll->debsyms=new dllnameentry [nsyms];
  dll->deblines=new dllordentry [nlines];
  dll->debmodpos=new debmodposstruct [nmodpos];
  if (!dll->debnamebuf||!dll->debnamebuf||!dll->debsyms||!dll->deblines||!dll->debmodpos)
    return errAllocMem;

  nmodpos=0;
  modposp=modpos;
  int i;
  while (modposp<modposend)
  {
    int n=*(short*)(modposp+6);
    char *base=(char*)objtab[*(short*)(modposp+4)-1][5]+*(long*)modposp;
    modposp+=8;
    for (i=0; i<n; i++)
    {
      dll->debmodpos[nmodpos].off=base;
      dll->debmodpos[nmodpos].len=*(long*)modposp;
      dll->debmodpos[nmodpos].mod=*(short*)(modposp+4);
      *(void**)modposp=base;
      base+=dll->debmodpos[nmodpos].len;
      modposp+=6;
      nmodpos++;
    }
  }
  nameslen=0;
  nsyms=0;
  symp=symtab;
  while (symp<modpos)
  {
    memcpy(dll->debnamebuf+nameslen, symp+10, symp[9]);
    dll->debnamebuf[nameslen+symp[9]]=0;
    dll->debsyms[nsyms].name=dll->debnamebuf+nameslen;
    dll->debsyms[nsyms].ptr=(char*)(objtab[*(short*)(symp+4)-1][5])+*(long*)symp;
    nameslen+=symp[9]+1;
    symp+=10+symp[9];
    nsyms++;
  }
  nmods=0;
  nlines=0;
  modp=modtab;
  while (modp<symtab)
  {
    memcpy(dll->debnamebuf+nameslen, modp+21, modp[20]);
    dll->debnamebuf[nameslen+modp[20]]=0;
    dll->debmods[nmods]=dll->debnamebuf+nameslen;
    if (*(short*)(modp+18))
    {
      long oo=*(long*)(modp+14);
      char *lines=buf+*(long*)(buf+oo);
      char *lineend=buf+*(long*)(buf+oo+4);
      while (lines<lineend)
      {
        int n=*(short*)(lines+4);
        char *o=*(char**)(modpos+*(long*)lines);
        lines+=6;
        for (i=0; i<n; i++)
        {
          dll->deblines[nlines].ord=*(short*)lines;
          dll->deblines[nlines].ptr=o+*(long*)(lines+2);
          nlines++;
          lines+=6;
        }
      }
    }
    nameslen+=modp[20]+1;
    modp+=21+modp[20];
    nmods++;
  }

  delete buf;

  return errOk;
}



int dllLoadLocals(binfile &f, dllnameentry *nameref, int nnameref, dllordentry *ordref, int nordref)
{
  int i,j;
  for (i=0; i<MAXDLLS; i++)
    if (!dlls[i])
      break;
  if (i==MAXDLLS)
    return errAllocMem;

  if (f[0].getus()!=0x5A4D)
  {
    printf("it's %x\n", f[0].getus());
    return errFormSig;
  }
  if (f[0x18].getus()<0x40)
    return errFormSig;
  unsigned long leoff=f[0x3C].getul();
  lehdr h;
  f[leoff].read(&h, sizeof(h));
  if (h.sig!=0x454C)
    return errFormSig;
  if ((h.modtype&0x38000)!=0x00000)
    return errFormSig;
  if (h.modtype&0x40000004)
    return errFormSupp;
  if (h.modtype&0x30)
    return errFormSupp;
  if (h.formatlevel||h.byteorder||h.wordorder||(h.pagesize!=0x1000))
    return errFormSupp;
  if (h.prenpages||h.itedataoff||h.preinsnpages||h.deminsnpages||h.extraheap)
    return errFormSupp;

  char *lsec=new char [h.loadsize+sizeof(h)];
  dllordentry *xordref=new dllordentry [nnameref+nordref];
  if (!lsec||!xordref)
    return errAllocMem;
  f[leoff].read(lsec, h.loadsize+sizeof(h));
  memcpy(xordref, ordref, nordref*sizeof(*ordref));
  int nxordref=nordref;

  long (*objtab)[6]=(long(*)[6])(lsec+h.objtaboff);

  unsigned long datasize1=0;
  for (i=0; i<h.nobjs; i++)
    datasize1+=objtab[i][0];

  for (i=0; i<h.nobjs; i++)
    objtab[i][5]=0;

  unsigned long nameslen=0;
  char *namep;
  int nnameentries=0;
  const char *modname=0;
  namep=lsec+h.resnamesoff;
  int refcode=0;
  int refdata=0;
  while (1)
  {
    if (!*namep)
      break;
    for (i=0; i<nnameref; i++)
      if (!strncmp(namep+1, nameref[i].name, *namep))
      {
        xordref[nxordref].ord=*(short*)(namep+*namep+1);
        xordref[nxordref].ptr=nameref[i].ptr;
        nxordref++;
      }
    nameslen+=1+*namep;
    if (*(short*)(namep+*namep+1))
      nnameentries++;
    else
      modname=namep;
    namep+=*namep+3;
  }

  if (!modname)
    return errFormMiss;
  if (getpdllhandle(modname)!=-1)
  {
    i=getpdllhandle(modname);
    dlls[i]->refcount++;
    dlls[i]->userrefcount++;
    delete lsec;
    return i;
  }

  char *nres=new char [h.nresnamessize+1];
  if (!nres)
    return errAllocMem;
  f[h.nresnamesoff].read(nres, h.nresnamessize);
  nres[h.nresnamessize]=0;

  namep=nres;
  while (1)
  {
    if (!*namep)
      break;
    for (i=0; i<nnameref; i++)
      if (!strncmp(namep+1, nameref[i].name, *namep))
      {
        xordref[nxordref].ord=*(short*)(namep+*namep+1);
        xordref[nxordref].ptr=nameref[i].ptr;
        nxordref++;
      }
    nameslen+=1+*namep;
    if (*(short*)(namep+*namep+1))
      nnameentries++;
    namep+=*namep+3;
  }
  int nordentries=0;
  char *entp=lsec+h.entryoff;
  while (1)
  {
    if (!*entp)
      break;
    if (!entp[1])
    {
      entp+=2;
      continue;
    }
    nordentries+=*entp;
    entp+=*entp*5+4;
  }
  dllstruct *dll=new dllstruct;
  char *names=new char [nameslen];
  dllnameentry *nameentries=new dllnameentry [nnameentries];
  dllordentry *ordentries=new dllordentry [nordentries];
  if (!dll||!names||!nameentries||!ordentries)
  {
    delete xordref;
    delete lsec;
    delete nres;
    delete dll;
    delete names;
    delete ordentries;
    delete nameentries;
    return errAllocMem;
  }

  dll->desc="";
  dll->size=datasize1;
  dll->ver=h.modver;
  dll->data=0;
  dll->base=(void*)0xFFFFFFFF;
  dll->refcount=1;
  dll->userrefcount=1;
  dll->nnameentries=nnameentries;
  dll->nordentries=nordentries;
  dll->ordentries=ordentries;
  dll->nameentries=nameentries;
  dll->namebuf=names;
  dll->nrefs=0;
  dll->refs=0;
  dll->entrypoint=0;
  dll->local=0;

  dll->debnsyms=0;
  dll->debnlines=0;
  dll->debnmodpos=0;
  dll->debnamebuf=0;
  dll->debmods=0;
  dll->debsyms=0;
  dll->deblines=0;
  dll->debmodpos=0;

  for (i=0; i<h.nobjs; i++)
  {
    int pageo=objtab[i][3]-1;
    int npages=objtab[i][4];
    int rlen=npages*0x1000;
    if (((pageo+npages)==h.npages)&&rlen)
      rlen-=4096-h.lastpage;
    if (rlen>objtab[i][0])
      rlen=objtab[i][0];
    char *buf=new char [rlen];
    if (!buf)
      return errAllocMem;
    f[h.pageoff+pageo*0x1000].read(buf, rlen);
    delete buf;
  }

  int curord=1;
  entp=lsec+h.entryoff;
  while (1)
  {
    if (!*entp)
      break;
    if (!entp[1])
    {
      curord+=*entp;
      entp+=2;
      continue;
    }
    int tl=*entp++;
    unsigned char fl=*entp++;
    unsigned short curobj=*(short*)entp;
    entp+=2;
    for (i=0; i<tl; i++)
    {
      entp++;
      for (j=0; j<nxordref; j++)
        if (curord==xordref[j].ord)
        {
          objtab[curobj-1][5]=(long)xordref[j].ptr-*(long*)entp;
          if ((unsigned long)objtab[curobj-1][5]<(unsigned long)dll->base)
            dll->base=(void*)objtab[curobj-1][5];
        }
      curord++;
      entp+=4;
    }
  }

  int ret=readentries(dll, lsec+h.entryoff, objtab, lsec+h.resnamesoff, nres);
  if (ret)
    return ret;

  delete xordref;
  delete nres;

  ret=readdebug(dll, f[h.nresnamessize?(h.nresnamesoff+h.nresnamessize):(h.pageoff+(h.npages-1)*4096+h.lastpage)], objtab);
  if (ret)
    return ret;

  delete lsec;

  for (i=0; i<MAXDLLS; i++)
    if (!dlls[i])
      break;
  dlls[i]=dll;

  return i;
}


int dllLoad(binfile &f)
{
  int i,j;
  for (i=0; i<MAXDLLS; i++)
    if (!dlls[i])
      break;
  if (i==MAXDLLS)
    return errAllocMem;

  if (f[0].getus()!=0x5A4D)
    return errFormSig;
  if (f[0x18].getus()<0x40)
    return errFormSig;
  unsigned long leoff=f[0x3C].getul();
  lehdr h;
  f[leoff].read(&h, sizeof(h));
  if (h.sig!=0x454C)
    return errFormSig;
  if ((h.modtype&0x38000)!=0x08000)
    return errFormSig;
  if (h.modtype&0x40000004)
    return errFormSupp;
  if (h.modtype&0x30)
    return errFormSupp;
  if (h.formatlevel||h.byteorder||h.wordorder||(h.pagesize!=0x1000))
    return errFormSupp;
  if (h.prenpages||h.itedataoff||h.preinsnpages||h.deminsnpages||h.extraheap)
    return errFormSupp;

  char *lsec=new char [h.loadsize+sizeof(h)];
  if (!lsec)
    return errAllocMem;
  f[leoff].read(lsec, h.loadsize+sizeof(h));

  long (*objtab)[6]=(long(*)[6])(lsec+h.objtaboff);

  unsigned long datasize=0;
  unsigned long datasize1=0;
  for (i=0; i<h.nobjs; i++)
  {
    objtab[i][5]=datasize;
    datasize1+=objtab[i][0];
    datasize+=objtab[i][0];
    datasize=(datasize+GRAN-1)&~(GRAN-1);
  }

  unsigned long nameslen=0;
  char *namep;
  int nnameentries=0;
  const char *modname=0;
  namep=lsec+h.resnamesoff;
  while (1)
  {
    if (!*namep)
      break;
    nameslen+=1+*namep;
    if (*(short*)(namep+*namep+1))
      nnameentries++;
    else
      modname=namep;
    namep+=*namep+3;
  }

  if (!modname)
    return errFormMiss;
  if (getpdllhandle(modname)!=-1)
  {
    i=getpdllhandle(modname);
    dlls[i]->refcount++;
    dlls[i]->userrefcount++;
    delete lsec;
    return i;
  }

  char *nres=new char [h.nresnamessize+1];
  if (!nres)
    return errAllocMem;
  f[h.nresnamesoff].read(nres, h.nresnamessize);
  nres[h.nresnamessize]=0;

  namep=nres;
  while (1)
  {
    if (!*namep)
      break;
    nameslen+=1+*namep;
    if (*(short*)(namep+*namep+1))
      nnameentries++;
    namep+=*namep+3;
  }
  int nordentries=0;
  char *entp=lsec+h.entryoff;
  while (1)
  {
    if (!*entp)
      break;
    if (!entp[1])
    {
      entp+=2;
      continue;
    }
    nordentries+=*entp;
    entp+=*entp*5+4;
  }
  dllstruct *dll=new dllstruct;
  char *data=new char [datasize+GRAN-1];
  char *names=new char [nameslen];
  int *refs=new int [h.impnmods];
  dllnameentry *nameentries=new dllnameentry [nnameentries];
  dllordentry *ordentries=new dllordentry [nordentries];
  if (!dll||!data||!names||!nameentries||!ordentries)
  {
    delete lsec;
    delete nres;
    delete dll;
    delete data;
    delete names;
    delete ordentries;
    delete nameentries;
    delete refs;
    return errAllocMem;
  }

  namep=lsec+h.impnameoff;
  for (i=0; i<h.impnmods; i++)
  {
    refs[i]=getpdllhandle(namep);
    if (refs[i]==-1)
    {
      for (i=0; i<namep[0]; i++) modlinkname[i]=namep[i+1];
      modlinkname[i]=0;
      return errSymMod;
    }
    dlls[refs[i]]->refcount++;
    namep+=*namep+1;
  }

  for (i=0; i<h.nobjs; i++)
    objtab[i][5]+=(((long)data+GRAN-1)&~(GRAN-1));

  dll->desc="";
  dll->size=datasize1;
  dll->ver=h.modver;
  dll->data=data;
  memset(data, 0, datasize+GRAN-1);
  data=(char*)(((long)data+GRAN-1)&~(GRAN-1));
  dll->base=data;
  dll->refcount=1;
  dll->userrefcount=1;
  dll->nnameentries=nnameentries;
  dll->nordentries=nordentries;
  dll->ordentries=ordentries;
  dll->nameentries=nameentries;
  dll->namebuf=names;
  dll->nrefs=h.impnmods;
  dll->refs=refs;

  dll->debnsyms=0;
  dll->debnlines=0;
  dll->debnmodpos=0;
  dll->debnamebuf=0;
  dll->debmods=0;
  dll->debsyms=0;
  dll->deblines=0;
  dll->debmodpos=0;

  if (h.ics)
    dll->entrypoint=(char*)objtab[h.ics-1][5]+h.ieip;
  else
    dll->entrypoint=0;
  dll->local=0;

  int ret=readentries(dll, lsec+h.entryoff, objtab, lsec+h.resnamesoff, nres);
  if (ret)
    return ret;

  delete nres;

  for (i=0; i<h.nobjs; i++)
  {
    int pageo=objtab[i][3]-1;
    int npages=objtab[i][4];
    int rlen=npages*0x1000;
    if (((pageo+npages)==h.npages)&&rlen)
      rlen-=4096-h.lastpage;
    if (rlen>objtab[i][0])
      rlen=objtab[i][0];
    f[h.pageoff+pageo*0x1000].read((char*)objtab[i][5], rlen);
    for (j=0; j<npages; j++)
    {
      long *fixoffs=(long*)(lsec+h.fixpageoff);
      unsigned short rpo=*(unsigned short*)(lsec+h.pagemapoff+(pageo+j)*4+1);
      rpo=(rpo>>8)|(rpo<<8);
      if (!rpo)
        continue;
      char *fp=lsec+h.fixrecoff+fixoffs[rpo-1];
      char *fpe=lsec+h.fixrecoff+fixoffs[rpo];
      while (fp<fpe)
      {
        unsigned char t1=*fp++;
        if ((t1!=7)&&(t1!=8))
          return errFormSupp;
        unsigned char t2=*fp++;
        if (t2&0x8)
          return errFormSupp;
        if ((t2&3)==3)
          return errFormSupp;
        unsigned long *ptr=(unsigned long *)((char*)objtab[i][5]+j*0x1000+*(short*)fp);
        fp+=2;
        unsigned long dest;
        if ((t2&3)==0)
        {
          if (t2&0x40)
          {
            dest=objtab[*(unsigned short*)fp-1][5];
            fp+=2;
          }
          else
            dest=objtab[*fp++-1][5];
          if (t2&0x10)
          {
            dest+=*(unsigned long*)fp;
            fp+=4;
          }
          else
          {
            dest+=*(unsigned short*)fp;
            fp+=2;
          }
        }
        else
        if ((t2&3)==1)
        {
          unsigned char mod=*fp++;
          unsigned short ord;
          if (t2&0x80)
            ord=*fp++;
          else
          {
            ord=*(unsigned short*)fp;
            fp+=2;
          }
          dest=(unsigned long)(getordentry(*(dlls[refs[mod-1]]),ord));
          if (!dest)
            return errSymSym;
          if (t2&0x04)
            if (t2&0x20)
            {
              dest+=*(unsigned long*)fp;
              fp+=4;
            }
            else
            {
              dest+=*(unsigned short*)fp;
              fp+=2;
            }
        }
        else
        if ((t2&3)==2)
        {
          unsigned char mod=*fp++;
          unsigned short noff=*(unsigned short*)fp;
          fp+=2;
          dest=(unsigned long)(getpnameentry(*(dlls[refs[mod-1]]),lsec+h.impprocoff+noff));
          if (!dest)
          {
            char name[128];
            strncpy(name, lsec+h.impprocoff+noff+1, *(lsec+h.impprocoff+noff));
            name[*(lsec+h.impprocoff+noff)]=0;
            printf("symbol not found: %s\n",name);
            return errSymSym;
          }
          if (t2&0x04)
            if (t2&0x20)
            {
              dest+=*(unsigned long*)fp;
              fp+=4;
            }
            else
            {
              dest+=*(unsigned short*)fp;
              fp+=2;
            }
        }
        if (t1==8)
          *ptr=dest-(long)(ptr+1);
        else
          *ptr=dest;
      }
    }
  }

  ret=readdebug(dll, f[h.nresnamessize?(h.nresnamesoff+h.nresnamessize):(h.pageoff+(h.npages-1)*4096+h.lastpage)], objtab);
  if (ret)
    return ret;

  delete lsec;

  for (i=0; i<MAXDLLS; i++)
    if (!dlls[i])
      break;
  dlls[i]=dll;

  if (dlls[i]->entrypoint)
    calldll(dlls[i]->entrypoint, i, 0);

  return i;
}

void dllFree(int h)
{
  if ((h<0)||(h>=MAXDLLS)||!dlls[h])
    return;
  if (!dlls[h]->userrefcount)
    return;
  dlls[h]->refcount--;
  dlls[h]->userrefcount--;
  if (dlls[h]->refcount)
    return;

  if (dlls[h]->entrypoint)
    calldll(dlls[h]->entrypoint, h, 1);

  int i;
  for (i=0; i<dlls[h]->nrefs; i++)
  {
    dlls[dlls[h]->refs[i]]->userrefcount++;
    dllFree(dlls[h]->refs[i]);
  }

  if (!dlls[h]->local)
  {
    delete dlls[h]->data;
    delete dlls[h]->ordentries;
    delete dlls[h]->nameentries;
    delete dlls[h]->namebuf;
    delete dlls[h]->refs;
    delete dlls[h]->debnamebuf;
    delete dlls[h]->debmods;
    delete dlls[h]->debsyms;
    delete dlls[h]->deblines;
    delete dlls[h]->debmodpos;
  }
  delete dlls[h];
  dlls[h]=0;
}


int dllInit()
{
  int i;
  for (i=0; i<MAXDLLS; i++)
    dlls[i]=0;
  return 1;
}

void dllClose()
{
  int i;
  for (i=0; i<MAXDLLS; i++)
    while (dlls[i]&&dlls[i]->userrefcount)      // free-or-die
      dllFree(i);
  for (i=0; i<MAXDLLS; i++)
  {
    if (dlls[i])
    {
      dlls[i]->refcount=1;
      dlls[i]->userrefcount=1;
      dllFree(i);
    }
  }
}

void *dllGetSymbol(const char *mod, const char *sym)
{
  return dllGetSymbol(getdllhandle(mod), sym);
}

void *dllGetSymbol(const char *mod, int sym)
{
  return dllGetSymbol(getdllhandle(mod), sym);
}

void *dllGetSymbol(int h, const char *sym)
{
  if ((h<0)||(h>=MAXDLLS)||!dlls[h])
    return 0;
  return getnameentry(*(dlls[h]), sym);
}

void *dllGetSymbol(int h, int sym)
{
  if ((h<0)||(h>=MAXDLLS)||!dlls[h])
    return 0;
  return getordentry(*(dlls[h]), sym);
}

void *dllGetSymbol(const char *sym)
{
  if (!*sym)
    return 0;
  int h;
  for (h=0; h<MAXDLLS; h++)
    if (dlls[h])
    {
      void *r=getnameentry(*(dlls[h]), sym);
      if (r)
        return r;
    }
  printf("could not resolve symbol: %s\n",(char *)sym);
  return 0;
}

int dllCountLinks()
{
  int h,c=0;
  for (h=0; h<MAXDLLS; h++)
    if (dlls[h])
      c++;
  return c;
}

int dllGetLinkInfo(linkinfostruct &m, int n)
{
  int h;
  for (h=0; h<MAXDLLS; h++)
    if (dlls[h])
    {
      if (n--)
        continue;
      m.name=dlls[h]->name;
      m.desc=dlls[h]->desc;
      m.size=dlls[h]->size;
      m.ver=dlls[h]->ver;
      m.handle=h;
      return 1;
    }
  return 0;
}

void dllGetAddressInfo(linkaddressinfostruct &a, void *ptr)
{
  int h;
  a.module="";
  a.sym="";
  a.source="";
  a.line=0;
  a.symoff=0;
  unsigned long off=(long)ptr;
  int best=-1;
  for (h=0; h<MAXDLLS; h++)
    if (dlls[h])
    {
      unsigned long off2=(long)ptr-(long)dlls[h]->base;
      if (off2<off)
      {
        off=off2;
        best=h;
      }
    }
  a.symoff=off;
  a.lineoff=off;
  if (best==-1)
    return;
  dllstruct &d=*dlls[best];
  a.module=d.name;
  const char *bn=0;
  for (h=0; h<d.nnameentries; h++)
  {
    unsigned long off2=(long)ptr-(long)d.nameentries[h].ptr;
    if (off2<=off)
    {
      off=off2;
      bn=d.nameentries[h].name;
    }
  }
  for (h=0; h<d.debnsyms; h++)
  {
    unsigned long off2=(long)ptr-(long)d.debsyms[h].ptr;
    if (off2<=off)
    {
      off=off2;
      bn=d.debsyms[h].name;
    }
  }
  a.symoff=off;
  if (bn)
    a.sym=bn;
  bn=0;
  off=(long)ptr;
  for (h=0; h<d.debnmodpos; h++)
  {
    unsigned long off2=(long)ptr-(long)d.debmodpos[h].off;
    if (off2<=off)
    {
      off=off2;
      bn=d.debmods[d.debmodpos[h].mod];
    }
  }
  if (bn)
    a.source=bn;
  best=-1;
  for (h=0; h<d.debnlines; h++)
  {
    unsigned long off2=(long)ptr-(long)d.deblines[h].ptr;
    if (off2<=off)
    {
      off=off2;
      best=d.deblines[h].ord;
    }
  }
  a.lineoff=off;
  if (best!=-1)
    a.line=best;
}
