// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// DevIGen - devices system overhead
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -changed INI reading of driver symbols to _dllinfo lookup

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "imsdev.h"
#include "psetting.h"
#include "plinkman.h"
#include "devigen.h"

int deviReadDevices(const char *list, devinfonode **devs)
{
  int handlfn=1;

  while (1)
  {
    char drvhand[9];
    if (!cfGetSpaceListEntry(drvhand, list, 8))
      break;

    devinfonode *n=new devinfonode;
    if (!n)
      return 0;
    n->next=0;
    strcpy(n->handle, drvhand);

    printf(" %s", drvhand);
    int i;
    for (i=strlen(drvhand); i<8; i++)
      printf(" ");
    printf(": ");

    char dname[10];
    strncpy(dname,cfGetProfileString(drvhand, "link", ""),9);

    n->linkhand=lnkLink(dname);
    if (n->linkhand<0)
    {
      printf("link error\n");
      delete n;
      continue;
    }

    char *dsym;

    dsym=lnkReadInfoReg(dname,"driver");
    if (!*dsym)
    {
      printf("no driver found\n");
      lnkFree(n->linkhand);
      delete n;
      continue;
    }
    sounddevice *dev=(sounddevice*)lnkGetSymbol(dsym);
    if (!dev)
    {
      printf("sym error\n");
      lnkFree(n->linkhand);
      delete n;
      continue;
    }

    dsym=lnkReadInfoReg(dname,"addprocs");
    if (*dsym)
      n->addprocs=(devaddstruct*)lnkGetSymbol(dsym);
    else
      n->addprocs=NULL;

    int bypass=cfGetProfileBool(drvhand, "bypass", 0, 0);
    n->ihandle=handlfn++;
    n->keep=cfGetProfileBool(drvhand, "keep", 0, 0);
    n->dev.port=cfGetProfileInt(drvhand, "port", -1, 16);
    n->dev.port2=cfGetProfileInt(drvhand, "port2", -1, 16);
    n->dev.irq=cfGetProfileInt(drvhand, "irq", -1, 10);
    n->dev.irq2=cfGetProfileInt(drvhand, "irq2", -1, 10);
    n->dev.dma=cfGetProfileInt(drvhand, "dma", -1, 10);
    n->dev.dma2=cfGetProfileInt(drvhand, "dma2", -1, 10);
    n->dev.subtype=cfGetProfileInt(drvhand, "subtype", -1, 10);
    n->dev.chan=0;
    n->dev.mem=0;
    n->dev.opt=0;

    strcpy(n->name, dev->name);
    if (n->addprocs&&n->addprocs->GetOpt)
      n->dev.opt=n->addprocs->GetOpt(drvhand);
    n->dev.opt|=cfGetProfileInt(drvhand, "options", 0, 16);

    printf("%s", n->name);
    for (i=strlen(n->name); i<32; i++)
      printf(".");
    if (!bypass)
    {
      if (!dev->Detect(n->dev))
      {
        printf(" not found: optimize cp.ini!\n");
        lnkFree(n->linkhand);
        delete n;
        continue;
      }
    }
    else
      n->dev.dev=dev;

    if (!n->keep)
    {
      lnkFree(n->linkhand);
      n->linkhand=-1;
    }

    char str[100];
    strcpy(str, " (");

    strcat(str, "#");
    ltoa(n->ihandle, str+strlen(str), 10);

    if (n->dev.subtype!=-1)
    {
      strcat(str, " t");
      ltoa(n->dev.subtype, str+strlen(str), 10);
    }
    if (n->dev.port!=-1)
    {
      strcat(str, " p");
      ltoa(n->dev.port, str+strlen(str), 16);
    }
    if (n->dev.port2!=-1)
    {
      strcat(str, " q");
      ltoa(n->dev.port2, str+strlen(str), 16);
    }
    if (n->dev.irq!=-1)
    {
      strcat(str, " i");
      ltoa(n->dev.irq, str+strlen(str), 10);
    }
    if (n->dev.irq2!=-1)
    {
      strcat(str, " j");
      ltoa(n->dev.irq2, str+strlen(str), 10);
    }
    if (n->dev.dma!=-1)
    {
      strcat(str, " d");
      ltoa(n->dev.dma, str+strlen(str), 10);
    }
    if (n->dev.dma2!=-1)
    {
      strcat(str, " e");
      ltoa(n->dev.dma2, str+strlen(str), 10);
    }
    if (n->dev.mem)
    {
      strcat(str, " m");
      ltoa(n->dev.mem>>10, str+strlen(str), 10);
    }
    printf("%s)\n", str);

    *devs=n;
    devs=&(*devs)->next;
  }
  return 1;
}