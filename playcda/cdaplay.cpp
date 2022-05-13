// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Audio CD player
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <i86.h>
#include <string.h>
#include "dpmi.h"

struct reqhdr
{
  unsigned char len;
  unsigned char subunit;
  unsigned char command;
  unsigned short status;
  unsigned char res[8];
};

struct iocontrol
{
  reqhdr rh;
  unsigned char mediadesc;
  void __far16 *transadr;
  unsigned short numbytes;
  unsigned short startsec;
  void *reqvol;
  unsigned char buf[7];
};

struct audiocontrol
{
  reqhdr rh;
  unsigned char addrmode;
  unsigned long start;
  unsigned long numsecs;
};

union cdtransfer
{
  reqhdr rh;
  iocontrol ic;
  audiocontrol ac;
};

static cdtransfer *cdtrns=0;
static cdtransfer __far16 *cdrmtrns;
static __segment pmdatasel;

unsigned char cdInit()
{
  cdtrns=0;

  callrmstruct regs;
  regs.w.ax=0x150C;
  regs.w.bx=0;
  intrrm(0x2F, regs);
  if (regs.w.bx<0x210)
    return 0;

  void __far16 *rm;
  cdtrns=(cdtransfer *)dosmalloc(sizeof(cdtransfer), rm, pmdatasel);
  if (!cdtrns)
    return 0;
  cdrmtrns=(cdtransfer __far16 *)rm;
  return 1;
}

void cdClose()
{
  if (cdtrns)
    dosfree(pmdatasel);
  cdtrns=0;
}

unsigned char cdIsCDDrive(unsigned short drive)
{
  if (!cdtrns)
    return 0;
  callrmstruct regs;
  regs.w.ax=0x150B;
  regs.w.bx=0;
  regs.w.cx=drive;
  intrrm(0x2F, regs);
  if (regs.w.bx!=0xADAD)
    return 0;
  return !!regs.w.ax;
}

inline unsigned long time2sec(unsigned long time)
{
  return (time&0xFF)+((time&0xFF00)>>8)*75+((time&0xFF0000)>>16)*75*60-150;
}

inline unsigned long sec2time(unsigned long sec)
{
  return (sec%75)|((((sec+150)/75)%60)<<8)|(((sec+150)/(75*60))<<16);
}

static unsigned short cddevreq(unsigned short drive, unsigned char command, unsigned char len)
{
  cdtrns->rh.subunit=0;
  cdtrns->rh.status=0x8000;
  cdtrns->rh.command=command;
  cdtrns->rh.len=len;
  callrmstruct regs;
  regs.w.ax=0x1510;
  regs.w.cx=drive;
  regs.w.bx=(unsigned short)cdrmtrns;
  regs.s.es=(unsigned long)cdrmtrns>>16;
  intrrm(0x2F, regs);
  return cdtrns->rh.status;
}

static unsigned short doiocontrol(unsigned short drive, unsigned char cmd, void *buf, unsigned char len)
{
  *cdtrns->ic.buf=cmd;
  memcpy(cdtrns->ic.buf, buf, len);
  cdtrns->ic.transadr=/*cdrmtrns->ic.buf;*/(void __far16*)(((unsigned long)cdrmtrns)+26);
  cdtrns->ic.numbytes=len;
  cddevreq(drive, cmd, 26);
  memcpy(buf, cdtrns->ic.buf, len);
  return cdtrns->ic.rh.status;
}

unsigned short cdGetTracks(unsigned short drive, unsigned long *starts, unsigned char &first, unsigned short maxtracks)
{
  unsigned char cmd[7];
  cmd[0]=10;
  if (doiocontrol(drive, 3, cmd, 7)&0x8000)
    return 0;
  first=cmd[1];
  char last=cmd[2];
  unsigned long lastsec=*(unsigned long*)(cmd+3);
  short i;
  for (i=first; i<=last; i++)
  {
    cmd[0]=11;
    cmd[1]=i;
    if (doiocontrol(drive, 3, cmd, 7)&0x8000)
      return 0;
    starts[i-first]=time2sec(*(unsigned long*)(cmd+2));
    if ((i-first)==maxtracks)
      return maxtracks;
    if (cmd[6]&0x40)
    {
      if (i!=first)
        return i-first;
      first++;
      continue;
    }
  }
  starts[i-first]=time2sec(lastsec);
  return i-first;
}

void cdLockTray(unsigned short drive, unsigned char lock)
{
  doiocontrol(drive, 12, lock?"\x01\x01":"\x01\x00", 2);
}

unsigned long cdGetHeadLocation(unsigned short drive, unsigned short &stat)
{
  unsigned char cmd[6];
  cmd[0]=1;
  cmd[1]=0;
  stat=doiocontrol(drive, 3, cmd, 6);
  return *(unsigned long*)(cmd+2);
}

void cdSetVolumes(unsigned short drive, unsigned char left, unsigned char right)
{
  unsigned char cmd[9];
  cmd[0]=3;
  cmd[1]=0;
  cmd[2]=left;
  cmd[3]=1;
  cmd[4]=right;
  cmd[5]=2;
  cmd[6]=0xFF;
  cmd[7]=3;
  cmd[8]=0xFF;
  doiocontrol(drive, 12, cmd, 9);
}

void cdGetVolumes(unsigned short drive, unsigned char &left, unsigned char &right)
{
  unsigned char cmd[9];
  cmd[0]=4;
  doiocontrol(drive, 3, cmd, 9);
  left=cmd[2];
  right=cmd[4];
}

void cdPlay(unsigned short drive, unsigned long start, unsigned long len)
{
  cdtrns->ac.start=start;
  cdtrns->ac.numsecs=len;
  cdtrns->ac.addrmode=0;
  cddevreq(drive, 0x84, 22);
}

void cdStop(unsigned short drive)
{
  cddevreq(drive, 0x85, 13);
}

void cdRestart(unsigned short drive)
{
  cddevreq(drive, 0x88, 13);
}
