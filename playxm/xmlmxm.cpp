// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// XMPlay .MXM module loader
//
// revision history: (please note changes here)
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -EXPERIMENTAL STAGE - i need MANY .MXMs to test! :)
//    -loops don't seem to be correct sometimes, don't know why

#include <string.h>
#include "mcp.h"
#include "binfile.h"
#include "xmplay.h"
#include "err.h"

int xmpLoadMXM(xmodule &m, binfile &file)
{

  char deltasamps, modpanning;

  m.envelopes=0;
  m.samples=0;
  m.instruments=0;
  m.sampleinfos=0;
  m.patlens=0;
  m.patterns=0;
  m.orders=0;
  m.ismod=0;

  // 1st: read headers

  struct
  {
    unsigned long sig;
    unsigned long ordnum;
    unsigned long restart;
    unsigned long channum;
    unsigned long patnum;
    unsigned long insnum;
    unsigned char tempo;
    unsigned char speed;
    unsigned short opt;
    unsigned long sampstart;
    unsigned long samples8;
    unsigned long samples16;
    signed long lowpitch;
    signed long highpitch;
    unsigned char panpos[32];
    unsigned char ord[256];
    unsigned long insofs[128];
    unsigned long patofs[256];
  } mxmhead;

  file.read(&mxmhead, sizeof(mxmhead));
  if (memcmp(&mxmhead.sig, "MXM\0", 4))
    return errFormStruc;

  memcpy(m.name, "MXMPlay module      ", 20);
  m.name[20]=0;

  modpanning = !!(mxmhead.opt&2);
  deltasamps = !!(mxmhead.opt&4);

  m.linearfreq=!!(mxmhead.opt&1);
  m.nchan=mxmhead.channum;

  m.ninst=mxmhead.insnum;
  m.nenv=2*mxmhead.insnum;

  m.npat=mxmhead.patnum+1;

  m.nord=mxmhead.ordnum;
  m.loopord=mxmhead.restart;

  m.inibpm=mxmhead.speed;
  m.initempo=mxmhead.tempo;

  m.orders=new unsigned short [m.nord];

  m.patterns=(unsigned char (**)[5])new void *[m.npat];
  m.patlens=new unsigned short [m.npat];

  m.instruments=new xmpinstrument [m.ninst];
  m.envelopes=new xmpenvelope [m.nenv];


  sampleinfo **smps = new sampleinfo *[m.ninst];
  xmpsample **msmps = new xmpsample *[m.ninst];
  int *instsmpnum = new int [m.ninst];

  if (!smps||!msmps||!instsmpnum||!m.instruments||!m.envelopes||!m.patterns||!m.orders||!m.patlens)
    return errAllocMem;

  memset(m.patterns, 0, m.npat*sizeof(void*));
  memset(m.instruments, 0, m.ninst*sizeof(xmpinstrument));
  memset(m.envelopes, 0, m.nenv*sizeof(xmpenvelope));

  int i,j;

  for (i=0; i<32; i++)
    m.panpos[i]=0x11*mxmhead.panpos[i];

  for (i=0; i<m.nord; i++)
    m.orders[i]=(mxmhead.ord[i]<mxmhead.patnum)?mxmhead.ord[i]:mxmhead.patnum;

  m.patlens[mxmhead.patnum]=64;
  m.patterns[mxmhead.patnum]= new unsigned char [64*mxmhead.channum][5];
  if (!m.patterns[mxmhead.patnum])
    return errAllocMem;
  memset(m.patterns[mxmhead.patnum], 0, 64*5*mxmhead.channum);

  // 2nd: read instruments

  long guspos[128*16];
  memset(guspos,0xff,4*128*16);

  m.nsampi=0;
  m.nsamp=0;

  for (i=0; i<m.ninst; i++)
  {
    file.seek(mxmhead.insofs[i]);

    xmpinstrument &ip=m.instruments[i];
    xmpenvelope *env=m.envelopes+2*i;

    smps[i]=0;
    msmps[i]=0;

    struct
    {
      unsigned long sampnum;
      unsigned char snum[96];
      unsigned short volfade;
      unsigned char vibtype, vibsweep, vibdepth, vibrate;
      unsigned char vnum, vsustain, vloops, vloope;
      unsigned short venv[12][2];
      unsigned char pnum, psustain, ploops, ploope;
      unsigned short penv[12][2];
      unsigned char res[46];
    } mxmins;

    file.read(&mxmins,sizeof(mxmins));

    memcpy(ip.name,"                      ",22);
    ip.name[22]=0;

    memset(ip.samples, 0xff, 2*128);
    instsmpnum[i]=mxmins.sampnum;

    smps[i]=new sampleinfo[mxmins.sampnum];
    msmps[i]=new xmpsample[mxmins.sampnum];

    if (!smps[i]||!msmps[i])
      return errAllocMem;
    memset(msmps[i], 0, sizeof(xmpsample)*mxmins.sampnum);
    memset(smps[i], 0, sizeof(sampleinfo)*mxmins.sampnum);

    for (j=0; j<96; j++)
      if (mxmins.snum[j]<mxmins.sampnum)
        ip.samples[j]=m.nsamp+mxmins.snum[j];

    unsigned short volfade = mxmins.volfade;

    env[0].speed=0;
    env[0].type=0;
    long el=0;
    for (j=0; j<mxmins.vnum; j++)
      el+=mxmins.venv[j][0];
    env[0].env=new unsigned char[el+1];
    if (!env[0].env)
      return errAllocMem;
    short k, p=0, h=mxmins.venv[0][1]*4;
    for (j=0; j<mxmins.vnum; j++)
    {
      short l=mxmins.venv[j][0];
      short dh=mxmins.venv[j+1][1]*4-h;
      for (k=0; k<l; k++)
      {
        short cv=h+dh*k/l;
        env[0].env[p++]=(cv>255)?255:cv;
      }
      h+=dh;
    }
    env[0].len=p;
    env[0].env[p]=(h>255)?255:h;
    if (mxmins.vsustain<0xff)
    {
      env[0].type|=xmpEnvSLoop;
      env[0].sustain=0;
      for (j=0; j<mxmins.vsustain; j++)
        env[0].sustain+=mxmins.venv[j][0];
    }
    if (mxmins.vloope<0xff)
    {
      env[0].type|=xmpEnvLoop;
      env[0].loops=env[0].loope=0;
      for (j=0; j<mxmins.vloops; j++)
        env[0].loops+=mxmins.venv[j][0];
      for (j=0; j<mxmins.vloope; j++)
        env[0].loope+=mxmins.venv[j][0];
    }

    env[1].speed=0;
    env[1].type=0;
    el=0;
    for (j=0; j<mxmins.pnum; j++)
      el+=mxmins.penv[j][0];
    env[1].env=new unsigned char[el+1];
    if (!env[1].env)
      return errAllocMem;
    p=0;
    h=mxmins.penv[0][1]*4;
    for (j=0; j<mxmins.pnum; j++)
    {
      short l=mxmins.penv[j][0];
      short dh=mxmins.penv[j+1][1]*4-h;
      for (k=0; k<l; k++)
      {
        short cv=h+dh*k/l;
        env[1].env[p++]=(cv>255)?255:cv;
      }
      h+=dh;
    }
    env[1].len=p;
    env[1].env[p]=(h>255)?255:h;
    if (mxmins.psustain<0xff)
    {
      env[1].type|=xmpEnvSLoop;
      env[1].sustain=0;
      for (j=0; j<mxmins.psustain; j++)
        env[1].sustain+=mxmins.penv[j][0];
    }
    if (mxmins.ploope<0xff)
    {
      env[1].type|=xmpEnvLoop;
      env[1].loops=env[1].loope=0;
      for (j=0; j<mxmins.ploops; j++)
        env[1].loops+=mxmins.penv[j][0];
      for (j=0; j<mxmins.ploope; j++)
        env[1].loope+=mxmins.penv[j][0];
    }


    for (j=0; j<mxmins.sampnum; j++)
    {

      struct
      {
        unsigned short gusstartl;
        unsigned char gusstarth;
        unsigned short gusloopstl;
        unsigned char gusloopsth;
        unsigned short gusloopendl;
        unsigned char gusloopendh;
        unsigned char gusmode;
        unsigned char vol;
        unsigned char pan;
        unsigned short relpitch;
        unsigned char res[2];
      } mxmsamp;

      file.read(&mxmsamp, sizeof (mxmsamp));

      char bit16=mxmsamp.gusmode&0x04;
      char sloop=mxmsamp.gusmode&0x08;
      char sbidi=mxmsamp.gusmode&0x18;

      xmpsample &sp=msmps[i][j];

      memcpy(sp.name, "                      ", 22);
      sp.name[22]=0;
      sp.handle=0xFFFF;

      sp.normnote=-mxmsamp.relpitch;
      signed char rpf=mxmsamp.relpitch&0xff;
      sp.normtrans=rpf-mxmsamp.relpitch;
      sp.stdvol=(mxmsamp.vol>0x3F)?0xFF:(mxmsamp.vol<<2);
      sp.stdpan=mxmsamp.pan;
      sp.opt=0;
      sp.volfade=volfade;
      sp.vibtype=(mxmins.vibtype==1)?3:(mxmins.vibtype==2)?1:(mxmins.vibtype==3)?2:0;
      sp.vibdepth=mxmins.vibdepth<<2;
      sp.vibspeed=0;
      sp.vibrate=mxmins.vibrate<<8;
      sp.vibsweep=0xFFFF/(mxmins.vibsweep+1);
      sp.volenv=env[0].env?(2*i+0):0xFFFF;
      sp.panenv=env[1].env?(2*i+1):0xFFFF;
      sp.pchenv=0xFFFF;

      sampleinfo &sip=smps[i][j];

      long sampstart=mxmsamp.gusstartl+(mxmsamp.gusstarth<<16);
      long loopstart=mxmsamp.gusloopstl+(mxmsamp.gusloopsth<<16);
      long loopend=mxmsamp.gusloopendl+(mxmsamp.gusloopendh<<16);
      guspos[m.nsampi]=sampstart;
      if (loopstart<sampstart)
        loopstart=sampstart;
      if (loopend<sampstart)
        loopend=sampstart;
      loopstart-=sampstart;
      loopend-=sampstart;

      if (bit16)
      {
        loopstart>>=1;
        loopend>>=1;
      }

      sip.length=loopend;
      sip.loopstart=loopstart;
      sip.loopend=loopend+1;
      sip.samprate=8363;
      sip.type=(bit16?mcpSamp16Bit:0)|(sloop?mcpSampLoop:0)|(sbidi?mcpSampBiDi:0);

      unsigned long l=sip.length<<(!!(sip.type&mcpSamp16Bit));
      if (!l)
        continue;
      sip.ptr=new char [l+528];
      if (!sip.ptr)
        return errAllocMem;
      sp.handle=m.nsampi++;

    }

    m.nsamp+=mxmins.sampnum;

  }

  m.samples=new xmpsample [m.nsamp];
  m.sampleinfos=new sampleinfo [m.nsampi];

  if (!m.samples||!m.sampleinfos)
    return errAllocMem;

  m.nsampi=0;
  m.nsamp=0;
  for (i=0; i<m.ninst; i++)
  {
    for (j=0; j<instsmpnum[i]; j++)
    {
      m.samples[m.nsamp++]=msmps[i][j];
      if (smps[i][j].ptr)
        m.sampleinfos[m.nsampi++]=smps[i][j];
    }
    delete smps[i];
    delete msmps[i];
  }
  delete smps;
  delete msmps;
  delete instsmpnum;


  for (i=0; i<mxmhead.patnum; i++)
  {
    file.seek(mxmhead.patofs[i]);

    long patrows;
    file.read(&patrows, 4);

    m.patlens[i]=patrows;
    m.patterns[i]=new unsigned char [patrows*mxmhead.channum][5];

    if (!m.patterns[i])
      return errAllocMem;

    memset(m.patterns[i], 0, patrows*mxmhead.channum*5);

    unsigned char  pack;
    unsigned short pd;
    unsigned char  *pd1=(unsigned char *)(&pd);
    unsigned char  *pd2=pd1+1;

    for (j=0; j<patrows; j++)
    {
      unsigned char *currow=(unsigned char*)(m.patterns[i])+j*mxmhead.channum*5;
      file.read(&pack, 1);
      while (pack)
      {
        unsigned char *cur=currow+5*(pack&0x1f);
        if (pack&0x20)
        {
          file.read(&pd,2);
          cur[0]=*pd1;
          cur[1]=*pd2;
        }
        if (pack&0x40)
        {
          file.read(&pd,1);
          cur[2]=*pd1;
        }
        if (pack&0x80)
        {
          file.read(&pd,2);
          cur[3]=*pd1;
          cur[4]=*pd2;
        }
        file.read(&pack,1);
      }
    }

  }

  long gsize=mxmhead.samples8+2*mxmhead.samples16;

  signed char *gusmem = new signed char[gsize];
  signed short *gus16 = (signed short *)(gusmem+mxmhead.samples8);
  file.seek(mxmhead.sampstart);
  file.read(gusmem,gsize);

  if (deltasamps)
  {
    signed char db=0;
    for (i=0; i<mxmhead.samples8; i++)
      gusmem[i]=db+=gusmem[i];
    signed short dw=0;
    for (i=0; i<mxmhead.samples16; i++)
      gus16[i]=dw+=gus16[i];
  }

  for (i=0; i<m.nsampi; i++)
  {
    long actpos=guspos[i];
    long len=m.sampleinfos[i].length;
    if (m.sampleinfos[i].type&mcpSamp16Bit)
    {
      actpos<<=1;
      len<<=1;
      long poslo=actpos&0x03FFFE;
      long poshi=actpos&0x180000;
      actpos=poslo|(poshi>>1);
    }
    memcpy(m.sampleinfos[i].ptr,gusmem+actpos,len);
  }

  delete gusmem;

  return errOk;
}

